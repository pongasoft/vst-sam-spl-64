#pragma once

#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <algorithm>

#include <pongasoft/VST/ParamSerializers.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

//------------------------------------------------------------------------
// class SampleBuffers
//------------------------------------------------------------------------
template<typename SampleType>
class SampleBuffers
{
public:
  SampleBuffers(SampleRate iSampleRate, int32 iNumChannels, int32 iNumSamples);

  explicit SampleBuffers(SampleRate iSampleRate = 0);

  SampleBuffers(SampleBuffers const &other);
  SampleBuffers(SampleBuffers &&other) noexcept;

  ~SampleBuffers();

  SampleBuffers &operator=(SampleBuffers &&other) noexcept;

  inline SampleRate getSampleRate() const { return fSampleRate; }
  inline int32 getNumChannels() const {return fNumChannels; };
  inline int32 getNumSamples() const { return fNumSamples; }
  inline bool hasSamples() const { return fNumChannels > 0 && fNumSamples > 0; }

  // returns the underlying buffer
  inline SampleType **getBuffer() const { return fSamples; }

  /**
   * Generate a new sample with a different sample rate
   * @return a new instance (caller takes ownership)
   */
  std::unique_ptr<SampleBuffers> resample(SampleRate iSampleRate) const;

  /**
   * Convert an interleaved buffer of samples into a SampleBuffers
   * @return a new instance (caller takes ownership)
   */
  static std::unique_ptr<SampleBuffers<SampleType>> fromInterleaved(SampleRate iSampleRate,
                                                                    int32 iNumChannels,
                                                                    int32 iTotalNumSamples,
                                                                    SampleType *iInterleavedSamples);
  friend class SampleBuffersSerializer32;

private:
  void deleteBuffers();
  void resize(int32 iNumChannels, int32 iNumSamples);

private:
  SampleRate fSampleRate;
  int32 fNumChannels;
  int32 fNumSamples;
  SampleType **fSamples;
};

////------------------------------------------------------------------------
//// class StaticSampleBuffers
////------------------------------------------------------------------------
//template<typename SampleType, int32 MAX_NUM_CHANNELS, int32 MAX_NUM_SAMPLES>
//class StaticSampleBuffers
//{
//public:
//  StaticSampleBuffers(int32 iSampleRate, int32 iNumChannels, int32 iNumSamples);
//  inline StaticSampleBuffers(int32 iSampleRate) : fSampleRate{iSampleRate}, fNumChannels{0}, fNumSamples{0}, fSamples{nullptr} {}
//
////  inline int32 getSampleRate() const { return fSampleRate; }
////  inline int32 getNumChannels() const {return fNumChannels; };
////  inline int32 getNumSamples() const { return fNumSamples; }
////
////  // returns the underlying buffer
////  inline SampleType **getBuffer() const { return fSamples; }
////
////  /**
////   * Convert an interleaved buffer of samples into a SampleBuffers
////   * @return a new instance (caller takes ownership)
////   */
////  static std::unique_ptr<SampleBuffers<SampleType>> fromInterleaved(int32 iSampleRate,
////                                                                    int32 iNumChannels,
////                                                                    int32 iTotalNumSamples,
////                                                                    SampleType *iInterleavedSamples);
////  friend class SampleBuffersSerializer32;
////
////private:
////  void deleteBuffers();
////  void resize(int32 iNumChannels, int32 iNumSamples);
//
//private:
//  int32 fSampleRate;
//  int32 fNumChannels;
//  int32 fNumSamples;
//  SampleType fSamples[MAX_NUM_CHANNELS][MAX_NUM_SAMPLES];
//};


// implementation in SampleBuffers.hpp

using SampleBuffers32 = SampleBuffers<Vst::Sample32>;
using SampleBuffers64 = SampleBuffers<Vst::Sample64>;

class SampleBuffersSerializer32 : public IParamSerializer<SampleBuffers32>
{
public:
  using ParamType = SampleBuffers32;

  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override;

  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override;
};

}
}
}