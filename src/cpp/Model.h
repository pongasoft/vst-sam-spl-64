#pragma once

#include <pluginterfaces/base/ftypes.h>

#include <pongasoft/VST/ParamConverters.h>
#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/VST/AudioUtils.h>
#include <pongasoft/Utils/Lerp.h>

namespace pongasoft::VST::SampleSplitter {

using namespace Steinberg;

constexpr int NUM_SLICES = 64;

constexpr int NUM_PADS = 16;
constexpr int NUM_PAD_BANKS = 4;

constexpr long UI_FRAME_RATE_MS = 40; // 40ms => 25 frames per seconds
//constexpr long UI_FRAME_RATE_MS = 250; // 4 per seconds for dev

// number of samples to use for cross fading
constexpr int32 NUM_XFADE_SAMPLES = 65;

// the maximum number of channels supported for the input (sample): support stereo max at this time
constexpr int32 MAX_NUM_INPUT_CHANNELS = 2;

// the size of the sample file which triggers a confirmation screen
constexpr int64 LARGE_SAMPLE_FILE_SIZE = 20 * 1024 * 1024; // 20Mb

// Although samples are obviously integers, keeping the range as double due to interpolation computations to
// avoid converting back and forth between int and double and loosing precision
using SampleRange = Utils::Range<double>;

//------------------------------------------------------------------------
// SampleRangeParamSerializer
//------------------------------------------------------------------------
class SampleRangeParamSerializer : public IParamSerializer<SampleRange>
{
public:
  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override
  {
    tresult res = kResultOk;

    res |= IBStreamHelper::readDouble(iStreamer, oValue.fFrom);
    res |= IBStreamHelper::readDouble(iStreamer, oValue.fTo);

    return res;
  }

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override
  {
    oStreamer.writeDouble(iValue.fFrom);
    oStreamer.writeDouble(iValue.fTo);
    return kResultOk;
  }
};

// Type used for the number of slices
struct NumSlice
{
  using real_type = double;
  using int_type = int32;

  NumSlice() = default;
  explicit NumSlice(real_type iValue) : fRealValue{iValue}, fIntValue{static_cast<int_type>(std::ceil(iValue))} {}
  constexpr explicit NumSlice(int_type iValue) : fRealValue{static_cast<real_type>(iValue)}, fIntValue{iValue} {}

  constexpr real_type realValue() const { return fRealValue; }
  constexpr int_type intValue() const { return fIntValue; }

private:
  real_type fRealValue{};
  int_type fIntValue{};
};

constexpr auto DEFAULT_NUM_SLICES = NumSlice{static_cast<NumSlice::int_type>(16)};

//------------------------------------------------------------------------
// NumSlicesParamConverter: Number of slices is between 1 and NUM_SLICES
// mapping [0.0, 1.0] to [1.0, NUM_SLICES]
//------------------------------------------------------------------------
class NumSlicesParamConverter : public IParamConverter<NumSlice>
{
public:
  inline ParamValue normalize(ParamType const &iValue) const override
  {
    return Utils::mapValueDP(iValue.realValue(), 1, NUM_SLICES, 0.0, 1.0);
  }

  inline ParamType denormalize(ParamValue iNormalizedValue) const override
  {
    return NumSlice{Utils::mapValueDP(iNormalizedValue, 0.0, 1.0, 1.0, NUM_SLICES)};
  }

  inline void toString(ParamType const &iValue, String128 oString, int32 /* iPrecision */) const override
  {
    Steinberg::UString wrapper(oString, str16BufferSize (String128));
    wrapper.printFloat(iValue.realValue(), 2);
  }
};

constexpr float PERCENT_PLAYED_NOT_PLAYING = 10.0f;

//------------------------------------------------------------------------
// PlayingState
//------------------------------------------------------------------------
struct PlayingState
{
  PlayingState() { for(float &p : fPercentPlayed) p = PERCENT_PLAYED_NOT_PLAYING; }

  // For each slice a percentage [0.0 - 1.0] if played forward,
  // [-1.0 - 0.0] if played backward,
  // PERCENT_PLAYED_NOT_PLAYING if not playing
  float fPercentPlayed[NUM_SLICES]{};

  // percentage for the selection
  float fWESelectionPercentPlayer{PERCENT_PLAYED_NOT_PLAYING};
};

//------------------------------------------------------------------------
// PlayingStateParamSerializer
//------------------------------------------------------------------------
class PlayingStateParamSerializer : public IParamSerializer<PlayingState>
{
public:
  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override
  {
    tresult res = kResultOk;

    for(float &slice : oValue.fPercentPlayed)
    {
      res |= IBStreamHelper::readFloat(iStreamer, slice);
    }

    res |= IBStreamHelper::readFloat(iStreamer, oValue.fWESelectionPercentPlayer);

    return res;
  }

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override
  {
    tresult res = kResultOk;
    for(float slice : iValue.fPercentPlayed)
    {
      if(!oStreamer.writeFloat(slice))
        res = kResultFalse;
    }
    if(!oStreamer.writeFloat(iValue.fWESelectionPercentPlayer))
      res = kResultFalse;

    return res;
  }
};

constexpr float PERCENT_SAMPLED_WAITING = 10.0f;

//------------------------------------------------------------------------
// SamplingState (for now using only a float but will likely expand...)
//------------------------------------------------------------------------
struct SamplingState
{
  float fPercentSampled{};
};

//------------------------------------------------------------------------
// SamplingStateParamSerializer
//------------------------------------------------------------------------
class SamplingStateParamSerializer : public IParamSerializer<SamplingState>
{
public:
  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override
  {
    return IBStreamHelper::readFloat(iStreamer, oValue.fPercentSampled);
  }

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override
  {
    return oStreamer.writeFloat(iValue.fPercentSampled) ? kResultOk : kResultFalse;
  }
};

//------------------------------------------------------------------------
// ESamplingDuration
//------------------------------------------------------------------------
enum ESamplingDuration
{
  kSamplingDuration1Bar,
  kSamplingDuration2Bar,
  kSamplingDuration4Bar,
  kSamplingDuration8Bar,
  kSamplingDuration16Bar
};

//------------------------------------------------------------------------
// SamplingDurationParamConverter
//------------------------------------------------------------------------
class SamplingDurationParamConverter : public IParamConverter<int>
{
public:
  int32 getStepCount() const override { return fEnumConverter.getStepCount(); }

  inline ParamValue normalize(int const &iValue) const override
  {
    ESamplingDuration duration = ESamplingDuration::kSamplingDuration1Bar;
    switch(iValue)
    {
      case 1: duration = ESamplingDuration::kSamplingDuration1Bar; break;
      case 2: duration = ESamplingDuration::kSamplingDuration2Bar; break;
      case 4: duration = ESamplingDuration::kSamplingDuration4Bar; break;
      case 8: duration = ESamplingDuration::kSamplingDuration8Bar; break;
      case 16: duration = ESamplingDuration::kSamplingDuration16Bar; break;
      default: ESamplingDuration::kSamplingDuration1Bar; break;
    }
    return fEnumConverter.normalize(duration);
  }

  inline int denormalize(ParamValue iNormalizedValue) const override
  {
    ESamplingDuration duration = fEnumConverter.denormalize(iNormalizedValue);
    switch(duration)
    {
      case ESamplingDuration::kSamplingDuration1Bar: return 1;
      case ESamplingDuration::kSamplingDuration2Bar: return 2;
      case ESamplingDuration::kSamplingDuration4Bar: return 4;
      case ESamplingDuration::kSamplingDuration8Bar: return 8;
      case ESamplingDuration::kSamplingDuration16Bar: return 16;
      default: return 1;
    }
  }

  inline void toString(ParamType const &iValue, String128 oString, int32 /* iPrecision */) const override
  {
    Steinberg::UString wrapper(oString, str16BufferSize (String128));
    wrapper.printInt(iValue);
  }

  EnumParamConverter<ESamplingDuration, ESamplingDuration::kSamplingDuration16Bar> fEnumConverter;
};

//------------------------------------------------------------------------
// ESamplingTrigger
//------------------------------------------------------------------------
enum ESamplingTrigger
{
  kSamplingTriggerImmediate,
  kSamplingTriggerOnPlayFree,
  kSamplingTriggerOnPlaySync1Bar,
  kSamplingTriggerOnSound
};

//------------------------------------------------------------------------
// SlicesSettings
// Each bit represent a boolean flag per slice
//------------------------------------------------------------------------
struct SlicesSettings
{
  uint64 fReverse{0}; // play slice in reverse (1) or forward(0)
  uint64 fLoop{0}; // loop slice (1) or one shot (0)

  bool isReverse(int iSlice) const { return BIT_TEST(fReverse, iSlice); }
  bool isLoop(int iSlice) const { return BIT_TEST(fLoop, iSlice); }

  SlicesSettings reverse(int iSlice, bool iFlag) const
  {
    SlicesSettings newSettings{*this};
    if(iFlag)
      BIT_SET(newSettings.fReverse, iSlice);
    else
      BIT_CLEAR(newSettings.fReverse, iSlice);
    return newSettings;
  }

  SlicesSettings loop(int iSlice, bool iFlag) const
  {
    SlicesSettings newSettings{*this};
    if(iFlag)
      BIT_SET(newSettings.fLoop, iSlice);
    else
      BIT_CLEAR(newSettings.fLoop, iSlice);
    return newSettings;
  }

  SlicesSettings toggleLoopAll() const
  {
    constexpr uint64 allLoop = 0xffffffffffffffff;

    SlicesSettings newSettings{*this};

    if(newSettings.fLoop != allLoop)
      newSettings.fLoop = allLoop;
    else
      newSettings.fLoop = 0;

    return newSettings;
  }

  bool operator==(const SlicesSettings &rhs) const
  {
    return fReverse == rhs.fReverse && fLoop == rhs.fLoop;
  }

  bool operator!=(const SlicesSettings &rhs) const
  {
    return !(rhs == *this);
  }
};

//------------------------------------------------------------------------
// SlicesSettings
//------------------------------------------------------------------------
class SlicesSettingsParamSerializer : public IParamSerializer<SlicesSettings>
{
public:
  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override
  {
    tresult res = kResultOk;

    res |= IBStreamHelper::readInt64u(iStreamer, oValue.fReverse);
    res |= IBStreamHelper::readInt64u(iStreamer, oValue.fLoop);

    return res;
  }

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override
  {
    oStreamer.writeInt64u(iValue.fReverse);
    oStreamer.writeInt64u(iValue.fLoop);
    return kResultOk;
  }
};

//------------------------------------------------------------------------
// HostInfo
//------------------------------------------------------------------------
struct HostInfo
{
  double fTempo{120};					    // tempo in BPM (Beats Per Minute)
  int32 fTimeSigNumerator{4};			// time signature numerator (e.g. 3 for 3/4)
  int32 fTimeSigDenominator{4};		// time signature denominator (e.g. 4 for 3/4)

  bool operator==(const HostInfo &rhs) const
  {
    return fTempo == rhs.fTempo &&
           fTimeSigNumerator == rhs.fTimeSigNumerator &&
           fTimeSigDenominator == rhs.fTimeSigDenominator;
  }

  bool operator!=(const HostInfo &rhs) const
  {
    return !(rhs == *this);
  }
};

//------------------------------------------------------------------------
// HostInfoParamSerializer
//------------------------------------------------------------------------
class HostInfoParamSerializer : public IParamSerializer<HostInfo>
{
public:
  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override
  {
    tresult res = kResultOk;

    res |= IBStreamHelper::readDouble(iStreamer, oValue.fTempo);
    res |= IBStreamHelper::readInt32(iStreamer, oValue.fTimeSigNumerator);
    res |= IBStreamHelper::readInt32(iStreamer, oValue.fTimeSigDenominator);

    return res;
  }

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override
  {
    oStreamer.writeDouble(iValue.fTempo);
    oStreamer.writeInt32(iValue.fTimeSigNumerator);
    oStreamer.writeInt32(iValue.fTimeSigDenominator);
    return kResultOk;
  }
};

//------------------------------------------------------------------------
// ESamplingInput
//------------------------------------------------------------------------
enum ESamplingInput
{
  kSamplingOff,
  kSamplingInput1,
  kSamplingInput2
};

//------------------------------------------------------------------------
// EViewType
//------------------------------------------------------------------------
enum EViewType
{
  kMainViewType,
  kEditSampleViewType,
  kGlobalSettingsViewType
};

//------------------------------------------------------------------------
// EEditingMode
//------------------------------------------------------------------------
enum EEditingMode
{
  kEditingEdit,
  kEditingSampling,
  kEditingIO
};

//------------------------------------------------------------------------
// EPlayMode
//------------------------------------------------------------------------
enum class EPlayMode
{
  kTrigger, // Pad is played until end even if released (one shot) => no looping
  kHold     // Pad is played only when held => looping allowed
};

//------------------------------------------------------------------------
// EInputRouting
//------------------------------------------------------------------------
enum EInputRouting
{
  kMonoInMonoOut, // mono in => mono out
  kMonoInStereoOut // mono in => stereo out
};

//------------------------------------------------------------------------
// toDbString
//------------------------------------------------------------------------
inline std::string toDbString(Sample64 iSample, int iPrecision = 2)
{
  if(iSample < 0)
    iSample = -iSample;

  std::ostringstream s;

  if(iSample >= VST::Sample64SilentThreshold)
  {
    s.precision(iPrecision);
    s.setf(std::ios::fixed);
    s << std::showpos << sampleToDb(iSample) << "dB";
  }
  else
    s << "-oo";
  return s.str();
}

//------------------------------------------------------------------------
// Gain
//------------------------------------------------------------------------
class Gain
{
public:
  static constexpr double Unity = 1.0;
  static constexpr double Factor = 0.7;

  constexpr explicit Gain(double iValue = Unity) noexcept : fValue{iValue} {}

  inline double getValue() const { return fValue; }
  inline double getValueInDb() const { return sampleToDb(fValue); }
  inline ParamValue getNormalizedValue() const
  {
    // value = (gain ^ 1/3) * 0.7
    return std::pow(fValue, 1.0/3) * Factor;
  }

private:
  double fValue;
};

constexpr Gain DEFAULT_GAIN = Gain{};

//------------------------------------------------------------------------
// GainParamConverter
//------------------------------------------------------------------------
class GainParamConverter : public IParamConverter<Gain>
{
public:
  /**
   * Gain uses an x^3 curve with 0.7 (Param Value) being unity gain
   */
  Gain denormalize(ParamValue value) const override
  {
    if(std::fabs(value - Gain::Factor) < 1e-5)
      return Gain{};

    // gain = (value / 0.7) ^ 3
    double correctedGain = value / Gain::Factor;
    return Gain{correctedGain * correctedGain * correctedGain};
  }

  // normalize
  ParamValue normalize(Gain const &iGain) const override
  {
    return iGain.getNormalizedValue();
  }

  // toString
  inline void toString(ParamType const &iValue, String128 iString, int32 iPrecision) const override
  {
    auto s = toDbString(iValue.getValue(), iPrecision);
    Steinberg::UString wrapper(iString, str16BufferSize (String128));
    wrapper.fromAscii(s.c_str());
  }
};

//------------------------------------------------------------------------
// RootKey - which key on keyboard triggers first slice
//------------------------------------------------------------------------
using RootKey = uint16;
constexpr RootKey DEFAULT_ROOT_KEY = 48; // C2 (48 + 64 = 112 < C8 [120] on a 88 keys piano)
constexpr size_t NUM_ROOT_KEYS = 128;
extern std::array<VstString16, NUM_ROOT_KEYS> KEYS;

//------------------------------------------------------------------------
// RootKeyParamConverter
//------------------------------------------------------------------------
class RootKeyParamConverter : public DiscreteValueParamConverter<NUM_ROOT_KEYS - 1, RootKey>
{
public:
  RootKeyParamConverter() : DiscreteValueParamConverter(KEYS) {}
};

template<typename T>
class PointerSerializer : public IParamSerializer<T *>
{
public:
  using ParamType = T *;

  static_assert(sizeof(ParamType) <= sizeof(uint64), "Making sure that a pointer will fit");

  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override
  {
    uint64 ptr;

    auto res = IBStreamHelper::readInt64u(iStreamer, ptr);
    if(res != kResultOk)
      return res;

    oValue = reinterpret_cast<ParamType>(ptr);

    return res;
  }

  tresult writeToStream(ParamType const &iValue, IBStreamer &oStreamer) const override
  {
    if(oStreamer.writeInt64u(reinterpret_cast<uint64>(iValue)))
      return kResultOk;
    else
      return kResultFalse;
  }
};

//------------------------------------------------------------------------
// __deprecated_ENumSlices
//------------------------------------------------------------------------
enum __deprecated_ENumSlices
{
  kNumSlice1,
  kNumSlice2,
  kNumSlice4,
  kNumSlice8,
  kNumSlice16,
  kNumSlice32,
  kNumSlice48,
  kNumSlice64,
};

//------------------------------------------------------------------------
// __deprecated_NumSlicesParamConverter
//------------------------------------------------------------------------
class __deprecated_NumSlicesParamConverter : public IParamConverter<int>
{
public:
  int32 getStepCount() const override { return fEnumConverter.getStepCount(); }

  inline ParamValue normalize(int const &iValue) const override
  {
    __deprecated_ENumSlices numSlices = __deprecated_ENumSlices::kNumSlice16;
    switch(iValue)
    {
      case 1: numSlices = __deprecated_ENumSlices::kNumSlice1; break;
      case 2: numSlices = __deprecated_ENumSlices::kNumSlice2; break;
      case 4: numSlices = __deprecated_ENumSlices::kNumSlice4; break;
      case 8: numSlices = __deprecated_ENumSlices::kNumSlice8; break;
      case 16: numSlices = __deprecated_ENumSlices::kNumSlice16; break;
      case 32: numSlices = __deprecated_ENumSlices::kNumSlice32; break;
      case 48: numSlices = __deprecated_ENumSlices::kNumSlice48; break;
      case 64: numSlices = __deprecated_ENumSlices::kNumSlice64; break;
      default: __deprecated_ENumSlices::kNumSlice16; break;
    }
    return fEnumConverter.normalize(numSlices);
  }

  inline int denormalize(ParamValue iNormalizedValue) const override
  {
    __deprecated_ENumSlices numSlices = fEnumConverter.denormalize(iNormalizedValue);
    switch(numSlices)
    {
      case __deprecated_ENumSlices::kNumSlice1: return 1;
      case __deprecated_ENumSlices::kNumSlice2: return 2;
      case __deprecated_ENumSlices::kNumSlice4: return 4;
      case __deprecated_ENumSlices::kNumSlice8: return 8;
      case __deprecated_ENumSlices::kNumSlice16: return 16;
      case __deprecated_ENumSlices::kNumSlice32: return 32;
      case __deprecated_ENumSlices::kNumSlice48: return 48;
      case __deprecated_ENumSlices::kNumSlice64: return 64;
      default: return 16;
    }
  }

  inline void toString(ParamType const &iValue, String128 oString, int32 /* iPrecision */) const override
  {
    Steinberg::UString wrapper(oString, str16BufferSize (String128));
    wrapper.printInt(iValue);
  }

  EnumParamConverter<__deprecated_ENumSlices, __deprecated_ENumSlices::kNumSlice64> fEnumConverter;
};

}