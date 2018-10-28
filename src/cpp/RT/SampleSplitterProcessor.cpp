#include <pongasoft/VST/AudioBuffer.h>
#include <pongasoft/VST/Debug/ParamTable.h>
#include <pongasoft/VST/Debug/ParamLine.h>


#include "SampleSplitterProcessor.h"
#include "../SampleSlice.hpp"

#include "version.h"
#include "jamba_version.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace RT {

//------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------
SampleSplitterProcessor::SampleSplitterProcessor() : RTProcessor(SampleSplitterControllerUID), fParameters{}, fState{fParameters}
{
  DLOG_F(INFO, "SampleSplitterProcessor() - jamba: %s - plugin: v%s", JAMBA_GIT_VERSION_STR, FULL_VERSION_STR);

  // in Debug mode we display the parameters in a table
#ifndef NDEBUG
  DLOG_F(INFO, "Parameters ---> \n%s", Debug::ParamTable::from(fParameters).full().toString().c_str());
#endif
}

//------------------------------------------------------------------------
// Destructor - purely for debugging purposes
//------------------------------------------------------------------------
SampleSplitterProcessor::~SampleSplitterProcessor()
{
  DLOG_F(INFO, "~SampleSplitterProcessor()");
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::initialize - define input/outputs
//------------------------------------------------------------------------
tresult SampleSplitterProcessor::initialize(FUnknown *context)
{
  DLOG_F(INFO, "SampleSplitterProcessor::initialize()");

  tresult result = RTProcessor::initialize(context);
  if(result != kResultOk)
  {
    return result;
  }

  //------------------------------------------------------------------------
  // This is where you define inputs and outputs
  //------------------------------------------------------------------------
  addAudioOutput(STR16 ("Stereo Out"), SpeakerArr::kStereo);

  //------------------------------------------------------------------------
  // Displays the order in which the RT parameters will be saved (debug only)
  //------------------------------------------------------------------------
#ifndef NDEBUG
  using Key = Debug::ParamDisplay::Key;
  DLOG_F(INFO, "RT Save State - Version=%d --->\n%s",
         fParameters.getRTSaveStateOrder().fVersion,
         Debug::ParamTable::from(getRTState(), true).keys({Key::kID, Key::kTitle}).full().toString().c_str());
#endif

  return result;
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::terminate - purely for debugging purposes
//------------------------------------------------------------------------
tresult SampleSplitterProcessor::terminate()
{
  DLOG_F(INFO, "SampleSplitterProcessor::terminate()");

  return RTProcessor::terminate();
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::setupProcessing
//------------------------------------------------------------------------
tresult SampleSplitterProcessor::setupProcessing(ProcessSetup &setup)
{
  tresult result = RTProcessor::setupProcessing(setup);

  if(result != kResultOk)
    return result;

  DLOG_F(INFO,
         "SampleSplitterProcessor::setupProcessing(%s, %s, maxSamples=%d, sampleRate=%f)",
         setup.processMode == kRealtime ? "Realtime" : (setup.processMode == kPrefetch ? "Prefetch" : "Offline"),
         setup.symbolicSampleSize == kSample32 ? "32bits" : "64bits",
         setup.maxSamplesPerBlock,
         setup.sampleRate);

  // sending the sample rate to the UI
  fState.fSampleRate.broadcast(setup.sampleRate);

  return result;
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::genericProcessInputs
// Implementation of the generic (32 and 64 bits) logic.
//------------------------------------------------------------------------
template<typename SampleType>
tresult SampleSplitterProcessor::genericProcessInputs(ProcessData &data)
{
  if(data.numOutputs == 0)
  {
    // nothing to do
    return kResultOk;
  }

  AudioBuffers<SampleType> out(data.outputs[0], data.numSamples);

  if(fState.fPads[0]->hasChanged())
    fState.fPad1Slice.resetCurrent();

  if(fState.fPads[0]->getValue())
  {
    if(fState.fFileSample.getNumSamples() > 0)
    {
      fState.fPad1Slice.play(fState.fFileSample, out);
    }
  }
  else
    out.clear();

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::processInputs
//------------------------------------------------------------------------
tresult SampleSplitterProcessor::processInputs(ProcessData &data)
{
  // Detect the fact that the GUI has sent a message to the RT.
  auto fileSample = fState.fFileSampleMessage.pop();
  if(fileSample)
  {
    // Implementation note: this code moves the sample from the queue into the RT... this may trigger a
    // memory delete (in RT code) but we are ok with it, because this happens ONLY if the user selects
    // a new file/sample so not in normal processing
    fState.fFileSample = std::move(*fileSample);

    DLOG_F(INFO, "Received fileSample from UI %f/%d/%d",
           fState.fFileSample.getSampleRate(),
           fState.fFileSample.getNumChannels(),
           fState.fFileSample.getNumSamples());

    fState.fPad1Slice.reset(0, fState.fFileSample.getNumSamples() / NUM_PADS);
  }

  return RTProcessor::processInputs(data);
}

}
}
}
}
