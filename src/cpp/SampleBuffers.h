#pragma once

#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <algorithm>

#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/Utils/Disposable.h>

class SndfileHandle;

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

/**
 * Helper class which maintains buffers (one per channel) of samples (in a given SampleType type).
 */
template<typename SampleType>
class SampleBuffers : public Utils::Disposable
{
public:
  // Constructor => will allocate memory to contain iNumSamples per iNumChannels
  SampleBuffers(SampleRate iSampleRate, int32 iNumChannels, int32 iNumSamples);

  // Constructor / no memory allocation
  explicit SampleBuffers(SampleRate iSampleRate = 0);

  // Copy Constructor
  SampleBuffers(SampleBuffers const &other);

  // Move Constructor
  SampleBuffers(SampleBuffers &&other) noexcept;

  // Destructor
  ~SampleBuffers();

  // Move assignment => buf2 = buf1
  SampleBuffers &operator=(SampleBuffers &&other) noexcept;

  // Copy the other buffer into this one up to iNumSamples
  void copyFrom(SampleBuffers const &other, int32 iNumSamples);

  // getSampleRate
  inline SampleRate getSampleRate() const { return fSampleRate; }

  // getNumChannels
  inline int32 getNumChannels() const {return fNumChannels; };

  // getNumSamples
  inline int32 getNumSamples() const { return fNumSamples; }

  // hasSamples
  inline bool hasSamples() const { return fNumChannels > 0 && fNumSamples > 0; }

  // returns the underlying buffer
  inline SampleType **getBuffer() const { return fSamples; }

  // returns the underlying buffer
  inline SampleType *getChannelBuffer(int32 iChannel) const {
    return (iChannel >= 0 && iChannel < fNumChannels) ? fSamples[iChannel] : nullptr;
  }

  /**
   * Save this buffer to the file and return the file size */
  tresult save(SndfileHandle &iFileHandle) const;

  /**
   * Generate a new sample with a different sample rate
   * @return a new instance (caller takes ownership)
   */
  std::unique_ptr<SampleBuffers> resample(SampleRate iSampleRate) const;

  /**
   * Generate a mono version of this buffer. The result will have only 1 channel with the average of all channels.
   * @return a new instance (caller takes ownership)
   */
  std::unique_ptr<SampleBuffers> toMono() const;

  /**
   * Removes silence from beginning and end of the sample. When there are multiple channels, silence must be
   * present in all channels to be removed.
   *
   * @return a new instance (caller takes ownership) or nullptr if there is no silence
   */
  std::unique_ptr<SampleBuffers> trim() const;

  /**
   * Removes silence from beginning and end of the sample. When there are multiple channels, silence must be
   * present in all channels to be removed.
   *
   * @param iSilentThreshold use this value for the threshold level
   * @return a new instance (caller takes ownership) or nullptr if there is no silence
   */
  std::unique_ptr<SampleBuffers> trim(SampleType iSilentThreshold) const;

  /**
   * For a given channel, bucketize the samples starting at offset iStartOffset in buckets of size
   * iNumSamplesPerBucket and compute the min and max of each bucket.
   * The result is written (appended) in the oMin and oMax output vectors.
   *
   * @return the number of elements written to oMin and oMax or -1 if no processing due to invalid arguments
   * */
  int32 computeMinMax(int32 iChannel,
                      std::vector<SampleType> &oMin,
                      std::vector<SampleType> &oMax,
                      int32 iStartOffset,
                      double iNumSamplesPerBucket,
                      int32 iNumBuckets,
                      int32 *oEndOffset = nullptr) const;

  /**
   * For a given channel, bucketize the samples starting at offset iStartOffset in buckets of size
   * iNumSamplesPerBucket and compute the average for each bucket.
   * The result is written (appended) in the oAvg output vector
   *
   * @return the number of elements written to oAvg or -1 if no processing due to invalid arguments
   * */
  int32 computeAvg(int32 iChannel,
                   std::vector<SampleType> &oAvg,
                   int32 iStartOffset,
                   double iNumSamplesPerBucket,
                   int32 iNumBuckets,
                   int32 *oEndOffset = nullptr) const;

  /**
   * Convert from one sample type to another.
   *
   * @return a new instance (caller takes ownership)
   */
  template<typename ToSampleType>
  std::unique_ptr<SampleBuffers<ToSampleType>> convert() const;

  /**
   * Loads buffers from the file handle */
  static std::unique_ptr<SampleBuffers<SampleType>> load(SndfileHandle &iFileHandle);

  friend class SampleBuffersSerializer32;

  // dispose => free resources
  void dispose() override;

private:
  // release memory
  void deleteBuffers();

  // make sure there is enough memory for iNumSamples per iNumChannels
  void resize(int32 iNumChannels, int32 iNumSamples);

private:
  SampleRate fSampleRate;
  int32 fNumChannels;
  int32 fNumSamples; // an int32 can contain over 3h worth of samples at 192000
  SampleType **fSamples;
};

//------------------------------------------------------------------------
// template implementation in SampleBuffers.hpp
//------------------------------------------------------------------------

// shortcuts for common types
using SampleBuffers32 = SampleBuffers<Vst::Sample32>;
using SampleBuffers64 = SampleBuffers<Vst::Sample64>;

/**
 * Serializer to exchange RT <-> GUI via messaging
 */
class SampleBuffersSerializer32 : public IParamSerializer<SampleBuffers32>
{
public:
  using ParamType = SampleBuffers32;

  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override;

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override;
};

}
}
}