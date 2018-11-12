#include <pongasoft/VST/AudioBuffer.h>
#include <pongasoft/VST/Debug/ParamTable.h>
#include <pongasoft/VST/Debug/ParamLine.h>

#include <pluginterfaces/vst/ivstevents.h>

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
SampleSplitterProcessor::SampleSplitterProcessor() :
  RTProcessor(SampleSplitterControllerUID),
  fParameters{},
  fState{fParameters},
  fClock{44100},
  fRateLimiter{}
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

  // Handle stereo output
  addAudioOutput(STR16 ("Stereo Out"), SpeakerArr::kStereo);

  // Handle Midi keyboard events
  addEventInput(STR16 ("Event Input"), 1);

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

  fClock.setSampleRate(setup.sampleRate);
  fRateLimiter = fClock.getRateLimiter(UI_FRAME_RATE_MS);

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

  bool clearOut = true;

  if(fState.fFileSample.getNumSamples() > 0)
  {
    handlePadSelection();
    handleNoteSelection(data);

    int numSlices = fState.fNumSlices;

    if(fState.fPolyphonic)
    {
      // any number of slice can be playing at the same time
      for(int slice = 0; slice < numSlices; slice++)
      {
        auto &s = fState.fSampleSlices[slice];

        if(s.isPlaying())
        {
          if(!fState.fPlayModeHold || s.isSelected())
          {
            if(s.play(fState.fFileSample, out, clearOut))
              s.stop();
            clearOut = false;
          }
          else
            s.stop();
        }
      }
    }
    else
    {
      // only one slice can be playing at the same time
      int sliceToPlay = -1;

      for(int slice = 0; slice < numSlices; slice++)
      {
        auto &s = fState.fSampleSlices[slice];

        if(s.isPlaying())
        {
          if(sliceToPlay != -1)
            fState.fSampleSlices[sliceToPlay].stop();
          sliceToPlay = slice;
        }
      }

      if(sliceToPlay != -1)
      {
        auto &s = fState.fSampleSlices[sliceToPlay];
        if(!fState.fPlayModeHold || s.isSelected())
        {
          if(s.play(fState.fFileSample, out, true))
            s.stop();
          clearOut = false;
        }
        else
          s.stop();
      }
    }
  }

  if(clearOut)
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

    splitSample();
  }

  if(auto slicesSettings = fState.fSlicesSettings.pop())
  {
    DLOG_F(INFO, "detected new slices settings");
    for(int i = 0; i < NUM_SLICES; i++)
    {
      fState.fSampleSlices[i].setLoop(slicesSettings->isLoop(i));
      fState.fSampleSlices[i].setReverse(slicesSettings->isReverse(i));
    }
  }

  // detect num slices change
  if(fState.fNumSlices.hasChanged())
  {
    splitSample();
  }

  tresult res = RTProcessor::processInputs(data);

  if(res == kResultOk)
  {
    if(fRateLimiter.shouldUpdate(static_cast<uint32>(data.numSamples)))
    {
      fState.fPlayingState.broadcast([this](PlayingState *oPlayingState) {
        for(int slice = 0; slice < NUM_SLICES; slice++)
        {
          auto &s = fState.fSampleSlices[slice];
          oPlayingState->fPercentPlayed[slice] = s.getPercentPlayed();
        }
      });
    }
  }

  return res;
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::handlePadSelection
//------------------------------------------------------------------------
void SampleSplitterProcessor::handlePadSelection()
{
  int numSlices = fState.fNumSlices;
  int padBank = fState.fPadBank;

  int start = padBank * NUM_PADS;
  int end = std::min(start + NUM_PADS, numSlices);

  if(fState.fPadBank.hasChanged() || fState.fNumSlices.hasChanged())
  {
    for(int i = 0; i < numSlices; i++)
    {
      if(i < start || i >= end)
        fState.fSampleSlices[i].setPadSelected(false);
    }
  }

  for(int pad = 0, slice = start; pad < NUM_PADS && slice < end; pad++, slice++)
  {
    auto &s = fState.fSampleSlices[slice];

    bool selected = fState.fPads[pad]->getValue();
    s.setPadSelected(selected);

    if(fState.fPads[pad]->hasChanged() && selected)
      s.start();
  }
}

constexpr int16 ROOT_KEY = 48; // C2 (48 + 64 = 112 < C8 [120] on a 88 keys piano)

//------------------------------------------------------------------------
// SampleSplitterProcessor::handlePadSelection
//------------------------------------------------------------------------
void SampleSplitterProcessor::handleNoteSelection(ProcessData &data)
{
  auto events = data.inputEvents;

  if(events && events->getEventCount() > 0)
  {
    int numSlices = fState.fNumSlices;

    for(int32 i = 0; i < events->getEventCount(); i++)
    {
      int32 slice = -1;
      bool selected = false;

      Event e{};
      events->getEvent(i, e);

      switch(e.type)
      {
        case Event::kNoteOnEvent:
          slice = e.noteOn.pitch - ROOT_KEY;
          selected = true;
          break;

        case Event::kNoteOffEvent:
          slice = e.noteOn.pitch - ROOT_KEY;
          selected = false;
          break;

        default:
          break;
      }

      if(slice >= 0 && slice < numSlices)
      {
        auto &s = fState.fSampleSlices[slice];
        s.setNoteSelected(selected);
        if(selected)
          s.start();
      }
    }
  }
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::splitSample
//------------------------------------------------------------------------
void SampleSplitterProcessor::splitSample()
{
  if(fState.fFileSample.hasSamples())
  {
    int numSlices = fState.fNumSlices;
    int numSamplesPerSlice = fState.fFileSample.getNumSamples() / numSlices;

    DLOG_F(INFO, "SampleSplitterProcessor::splitSample(%d)", numSlices);

    for(int i = 0, start = 0; i < numSlices; i++, start += numSamplesPerSlice)
      fState.fSampleSlices[i].reset(start, start + numSamplesPerSlice - 1);

    for(int i = numSlices + 1; i < NUM_SLICES; i++)
      fState.fSampleSlices[i].stop();
  }
}

}
}
}
}
