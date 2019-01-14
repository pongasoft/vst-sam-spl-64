#pragma once

#include <pluginterfaces/base/ftypes.h>

#include <pongasoft/VST/ParamConverters.h>
#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/VST/AudioUtils.h>
#include <pongasoft/Utils/Lerp.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

constexpr int NUM_SLICES = 64;
constexpr int DEFAULT_NUM_SLICES = 16;

constexpr int NUM_PADS = 16;
constexpr int NUM_PAD_BANKS = 4;

constexpr long UI_FRAME_RATE_MS = 40; // 40ms => 25 frames per seconds
//constexpr long UI_FRAME_RATE_MS = 250; // 4 per seconds for dev

constexpr uint32 MAX_SAMPLER_BUFFER_SIZE_BAR = 4; // 4 bars of sampling

// Although samples are obviously integers, keeping the range as double due to interpolation computations to
// avoid converting back and forth between int and double and loosing precision
using SampleRange = Utils::Range<double>;

//------------------------------------------------------------------------
// ENumSlices
//------------------------------------------------------------------------
enum ENumSlices
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
// NumSlicesParamConverter
//------------------------------------------------------------------------
class NumSlicesParamConverter : public IParamConverter<int>
{
public:
  int getStepCount() const override { return fEnumConverter.getStepCount(); }

  inline ParamValue normalize(int const &iValue) const override
  {
    ENumSlices numSlices = ENumSlices::kNumSlice16;
    switch(iValue)
    {
      case 1: numSlices = ENumSlices::kNumSlice1; break;
      case 2: numSlices = ENumSlices::kNumSlice2; break;
      case 3: numSlices = ENumSlices::kNumSlice4; break;
      case 8: numSlices = ENumSlices::kNumSlice8; break;
      case 16: numSlices = ENumSlices::kNumSlice16; break;
      case 32: numSlices = ENumSlices::kNumSlice32; break;
      case 48: numSlices = ENumSlices::kNumSlice48; break;
      case 64: numSlices = ENumSlices::kNumSlice64; break;
      default: ENumSlices::kNumSlice16; break;
    }
    return fEnumConverter.normalize(numSlices);
  }

  inline int denormalize(ParamValue iNormalizedValue) const override
  {
    ENumSlices numSlices = fEnumConverter.denormalize(iNormalizedValue);
    switch(numSlices)
    {
      case ENumSlices::kNumSlice1: return 1;
      case ENumSlices::kNumSlice2: return 2;
      case ENumSlices::kNumSlice4: return 4;
      case ENumSlices::kNumSlice8: return 8;
      case ENumSlices::kNumSlice16: return 16;
      case ENumSlices::kNumSlice32: return 32;
      case ENumSlices::kNumSlice48: return 48;
      case ENumSlices::kNumSlice64: return 64;
      default: return 16;
    }
  }

  inline void toString(ParamType const &iValue, String128 oString, int32 /* iPrecision */) const override
  {
    Steinberg::UString wrapper(oString, str16BufferSize (String128));
    wrapper.printInt(iValue);
  }

  EnumParamConverter<ENumSlices, ENumSlices::kNumSlice64> fEnumConverter;
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
    return res;
  }
};

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
  kSamplingViewType
};

}
}
}