#pragma once

#include <memory>
#include <pongasoft/VST/AudioBuffer.h>
#include "SampleBuffers.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

/**
 * Keeps track of the state the sampler is in.
 */
enum class ESamplerState
{
  kNotSampling,
  kSampling,
  kDoneSampling
};

/**
 * Generic Sampler. Typical usage pattern:
 *
 * Sampler32 sampler(2); // stereo
 *
 * sampler.init(48000, 100000); // this allocates memory
 * sampler.start();
 * sampler.sample(in, startOffset); // may not start at the beginning of the buffer
 * for ... {
 *   if <user press stop> { sampler.sample(in, -1, stopOffset); sampler.stop(); }
 *   else { sample.sample(in); }
 *
 *   if(!sampler.isSampling()) {
 *     sampler.stop();
 *     sampler.copyTo(otherBuffer);
 *     break;
 *   }
 * }
 * sampler.dispose(); // this releases memory
 *
 * @tparam SampleType the type of the samples this sampler store
 */
template<typename SampleType>
class Sampler : public Utils::Disposable
{
public:
  using SampleBuffersT = SampleBuffers<SampleType>;

public:
  // Constructor
  explicit Sampler(int32 iNumChannels) : fNumChannels{iNumChannels}, fCurrent{0}, fState{ESamplerState::kNotSampling} {};

  // initializes this sampler (allocate enough memory for iMaxSamples)
  void init(SampleRate iSampleRate, int32 iMaxSamples);

  // isInitialized
  bool isInitialized() const { return fBuffers != nullptr; }

  // start
  void start(bool iResetCurrent = true);

  // stop
  inline void stop() { fState = ESamplerState::kNotSampling; }

  // getPercentSampled
  float getPercentSampled() const;

  // returns true if the sampler is currently sampling (start has been called and it has not reached the maximum
  // number of samples)
  inline bool isSampling() const { return fState == ESamplerState::kSampling; }

  // free resources
  void dispose() override;

  /**
   * Sample the in buffer (which can be a different type) and return the end state. If the sampler is not sampling,
   * then doesn't do anything.
   *
   * @param iStartOffset optional start offset in the in buffer to start sampling from
   * @param iEndOffset optional end offset in the in buffer to stop sampling at
   * @return state
   */
  template<typename InputSampleType>
  ESamplerState sample(AudioBuffers<InputSampleType> &iIn, int32 iStartOffset = -1, int32 iEndOffset = -1);

  // copyTo
  void copyTo(SampleBuffersT *oSampleBuffers);

private:
  int32 fNumChannels;
  int32 fCurrent;
  ESamplerState fState;
  std::unique_ptr<SampleBuffersT> fBuffers{};
};

// shortcut types
using Sampler32 = Sampler<Sample32>;
using Sampler64 = Sampler<Sample64>;

}
}
}


