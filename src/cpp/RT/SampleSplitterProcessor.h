#pragma once

#include <pongasoft/VST/RT/RTProcessor.h>
#include <pongasoft/VST/SampleRateBasedClock.h>
#include "../Plugin.h"
#include "../Sampler.hpp"

namespace pongasoft::VST::SampleSplitter::RT {

using namespace pongasoft::VST::RT;

//------------------------------------------------------------------------
// SampleSplitterProcessor - Real Time Processor
//------------------------------------------------------------------------
class SampleSplitterProcessor : public RTProcessor
{
public:
  //------------------------------------------------------------------------
  // UUID() method used to create the processor
  //------------------------------------------------------------------------
  static inline ::Steinberg::FUID UUID() { return SampleSplitterProcessorUID; };

  //------------------------------------------------------------------------
  // Factory method used to create the processor
  //------------------------------------------------------------------------
  static FUnknown *createInstance(void * /*context*/) { return (IAudioProcessor *) new SampleSplitterProcessor(); }

public:
  // Constructor
  SampleSplitterProcessor();

  // Destructor
  ~SampleSplitterProcessor() override;

  // getRTState
  RTState *getRTState() override { return &fState; }

  /** Called at first after constructor (setup input/output) */
  tresult PLUGIN_API initialize(FUnknown *context) override;

  // Called at the end before destructor
  tresult PLUGIN_API terminate() override;

  // This is where the setup happens which depends on sample rate, etc..
  tresult PLUGIN_API setupProcessing(ProcessSetup &setup) override;

protected:
  // processInputs
  tresult processInputs(ProcessData &data) override;

  // processHostInfo
  tresult processHostInfo(ProcessData &data);

  // genericProcessInputs<SampleType>
  template<typename SampleType>
  tresult genericProcessInputs(ProcessData &data);

  // processSampling<SampleType>
  template<typename SampleType>
  tresult processSampling(ProcessData &data);

  // processInputs32Bits
  tresult processInputs32Bits(ProcessData &data) override { return genericProcessInputs<Sample32>(data); }

  // processInputs64Bits
  tresult processInputs64Bits(ProcessData &data) override { return genericProcessInputs<Sample64>(data); }

  // processMonoInput
  template<typename SampleType>
  void processMonoInput(AudioBuffers<SampleType> &out) const;

  /**
   * will determine which pad is selected
   */
  void handlePadSelection();

  /**
   * will determine which note is selected
   */
  void handleNoteSelection(ProcessData &data);

  /**
   * @return `-1` if the host is not playing, otherwise the playing offset */
  int32 getHostPlayingOffset(ProcessData &iData) const;

  /**
   * Determines the offset at which sampling should start.
   * @return `-1` if should not start
   */
  template<typename SampleType>
  int32 getStartSamplingOffset(ProcessData &iData, AudioBuffers<SampleType> &iBuffers) const;

  /**
   * Initializes the sampler if it is possible (for example, cannot initialize the sampler while sampling...)
   *
   * @return `true` if the sampler was initialized, `false` otherwise
   */
  bool maybeInitSampler(ProcessData &iData);

private:
  // The processor gets its own copy of the parameters (defined in Plugin.h)
  SampleSplitterParameters fParams;

  // The state
  SampleSplitterRTState fState;

  // The clock (based on frame rate)
  SampleRateBasedClock fClock;

  // Limit how often the data is sent to the UI
  SampleRateBasedClock::RateLimiter fRateLimiter;
  SampleRateBasedClock::RateLimiter fSamplingRateLimiter;

  // The sampler
  Sampler32 fSampler;
  bool fWaitingForSampling;

  // Counter to keep track of frames (used in pad selection)
  uint32 fFrameCount{};
};

}

