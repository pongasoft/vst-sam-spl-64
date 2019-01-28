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
  fParams{},
  fState{fParams},
  fClock{44100},
  fRateLimiter{},
  fSampler{2}
{
  DLOG_F(INFO, "SampleSplitterProcessor() - jamba: %s - plugin: v%s", JAMBA_GIT_VERSION_STR, FULL_VERSION_STR);

  // in Debug mode we display the parameters in a table
#ifndef NDEBUG
  DLOG_F(INFO, "Parameters ---> \n%s", Debug::ParamTable::from(fParams).full().toString().c_str());
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

  // For sampling purposes (Stereo In 2 is used as side chain in some environments)
  addAudioInput(STR16 ("Stereo In 1"), SpeakerArr::kStereo);
  addAudioInput(STR16 ("Stereo In 2"), SpeakerArr::kStereo);

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
         fParams.getRTSaveStateOrder().fVersion,
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
  if(fState.fSamplingInput != ESamplingInput::kSamplingOff)
    return processSampling<SampleType>(data);

  if(data.numOutputs == 0)
  {
    // nothing to do
    return kResultOk;
  }

  AudioBuffers<SampleType> out(data.outputs[0], data.numSamples);

  bool clearOut = true;

  if(fState.fSampleBuffers.getNumSamples() > 0)
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
            if(s.play(fState.fSampleBuffers, out, clearOut) == EPlayingState::kDonePlaying)
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
      uint32 sliceFrame = 0;

      // we find which slice to play (the one that was selected the most recently)
      for(int slice = 0; slice < numSlices; slice++)
      {
        auto &s = fState.fSampleSlices[slice];

        if(s.isPlaying())
        {
          if(sliceToPlay == -1 || s.getStartFrame() >= sliceFrame)
          {
            sliceToPlay = slice;
            sliceFrame = s.getStartFrame();
          }
        }
      }

      if(sliceToPlay != -1)
      {
        for(int slice = 0; slice < numSlices; slice++)
        {
          auto &s = fState.fSampleSlices[slice];

          if(sliceToPlay == slice)
          {
            if(!fState.fPlayModeHold || s.isSelected())
            {
              if(s.play(fState.fSampleBuffers, out, true) == EPlayingState::kDonePlaying)
                s.stop();
              clearOut = false;
            }
            else
              s.stop();
          }
          else
          {
            if(s.isPlaying())
            {
              s.stop();
            }
          }
        }
      }
    }

    if(fState.fWESelectionSlice.isPlaying())
    {
      if(fState.fWESelectionSlice.play(fState.fSampleBuffers, out, clearOut) == EPlayingState::kDonePlaying)
        fState.fWESelectionSlice.stop();
      clearOut = false;
    }
  }

  if(clearOut)
    out.clear();

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::processSampling
//------------------------------------------------------------------------
template<typename SampleType>
tresult SampleSplitterProcessor::processSampling(ProcessData &data)
{
  if(data.numInputs == 0 || data.numOutputs == 0)
  {
    // nothing to do
    return kResultOk;
  }

  AudioBuffers<SampleType> out(data.outputs[0], data.numSamples);

  int input = fState.fSamplingInput == ESamplingInput::kSamplingInput1 ? 0 : 1;

  // not enough inputs => bailing
  if(data.numInputs < input + 1)
    return out.clear();

  AudioBuffers<SampleType> in(data.inputs[input], data.numSamples);

  bool broadcastSample = false;

  if(fState.fSampling.hasChanged())
  {
    int32 offset = fState.getParamUpdateSampleOffset(data, fState.fSampling.getParamID());
    if(fState.fSampling)
    {
      DLOG_F(INFO, "start sampling... offset=%d", offset);
      fSampler.start();
      fSampler.sample(in, offset, -1);
      fState.fSamplingState.broadcast(SamplingState{fSampler.getPercentSampled()});
    }
    else
    {
      if(fSampler.isSampling())
      {
        DLOG_F(INFO, "stop sampling... offset=%d", offset);
        fSampler.sample(in, -1, offset);
        fSampler.stop();
        fState.fSamplingState.broadcast(SamplingState{0});
        broadcastSample = true;
      }
    }
  }
  else
  {
    if(fState.fSampling && fSampler.isSampling())
    {
      if(fSampler.sample(in) == ESamplerState::kDoneSampling)
      {
        DLOG_F(INFO, "done sampling...");
        fSampler.stop();
        broadcastSample = true;
        // we turn off the toggle because we reached the end of the buffer
        fState.fSampling.update(false, data);
        fState.fSamplingState.broadcast(SamplingState{0});
      }
    }
  }

  if(broadcastSample)
  {
    fState.fRTSampleMessage.broadcast([this](SampleBuffers32 *oSampleBuffers) {
      // Implementation note: this call (which happens in RT) does allocate memory but it is OK because
      // it happens once when sampling is complete, not on every frame
      fSampler.copyTo(oSampleBuffers);
    });

    // also need to replace RT copy
    fSampler.copyTo(&fState.fSampleBuffers);
    splitSample();
  }

  if(fState.fSamplingMonitor)
    return out.copyFrom(in);
  else
    return out.clear();
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::processHostInfo
//------------------------------------------------------------------------
tresult SampleSplitterProcessor::processHostInfo(ProcessData &data)
{
  auto processContext = data.processContext;
  if(processContext)
  {
    HostInfo newHostInfo{processContext->tempo,
                         processContext->timeSigNumerator,
                         processContext->timeSigDenominator};

    if(fState.fHostInfo != newHostInfo)
    {
      fState.fHostInfo = newHostInfo;
      fState.fHostInfoMessage.broadcast(fState.fHostInfo);
    }

    return kResultOk;
  }

  return kResultFalse;
}


//------------------------------------------------------------------------
// SampleSplitterProcessor::processInputs
//------------------------------------------------------------------------
tresult SampleSplitterProcessor::processInputs(ProcessData &data)
{
  // increment the number of frames
  fFrameCount++;

  // update host info (if changed)
  processHostInfo(data);

  // Detect the fact that the GUI has sent a message to the RT.
  auto fileSample = fState.fGUISampleMessage.pop();
  if(fileSample)
  {
    // Implementation note: this code moves the sample from the queue into the RT... this may trigger a
    // memory delete (in RT code) but we are ok with it, because this happens ONLY if the user selects
    // a new file/sample so not in normal processing
    fState.fSampleBuffers = std::move(*fileSample);

    DLOG_F(INFO, "Received fileSample from UI %f/%d/%d",
           fState.fSampleBuffers.getSampleRate(),
           fState.fSampleBuffers.getNumChannels(),
           fState.fSampleBuffers.getNumSamples());

    splitSample();
  }

  // Detect a slice settings change
  if(auto slicesSettings = fState.fSlicesSettings.pop())
  {
    DLOG_F(INFO, "detected new slices settings");
    for(int i = 0; i < NUM_SLICES; i++)
    {
      fState.fSampleSlices[i].setLoop(slicesSettings->isLoop(i));
      fState.fSampleSlices[i].setReverse(slicesSettings->isReverse(i));
    }
  }

  // handle playing the selection
  if(fState.fWEPlaySelection.hasChanged() && fState.fSampleBuffers.getNumSamples() > 0)
  {
    if(fState.fWEPlaySelection)
    {
      auto selectedRange = fState.fWESelectedSampleRange.popOrLast();

      if(selectedRange->isSingleValue())
        fState.fWESelectionSlice.reset(0, fState.fSampleBuffers.getNumSamples() - 1);
      else
        fState.fWESelectionSlice.reset(Utils::clamp<int32>(selectedRange->fFrom, 0, fState.fSampleBuffers.getNumSamples() - 1),
                                       Utils::clamp<int32>(selectedRange->fTo, 0, fState.fSampleBuffers.getNumSamples() - 1));

      fState.fWESelectionSlice.setLoop(true);
      fState.fWESelectionSlice.setPadSelected(true);
      fState.fWESelectionSlice.start();
    }
    else
      fState.fWESelectionSlice.stop();
  }

  // detect num slices change
  if(fState.fNumSlices.hasChanged())
  {
    splitSample();
  }

  // detect sampling (allocate/free memory)
  if(fState.fSamplingInput.hasChanged())
  {
    if(fState.fSamplingInput == ESamplingInput::kSamplingOff)
    {
      // we make sure that sampling is not on anymore
      fState.fSampling.update(false, data);
      // Implementation note: this call frees memory but it is ok as this happens only when switching between
      // sampling and not sampling... as a user request
      fSampler.dispose();
    }
    else
    {
      if(fState.fSamplingInput.previous() == ESamplingInput::kSamplingOff)
      {
        auto sampleCount = fClock.getSampleCountFor1Bar(fState.fHostInfo.fTempo,
                                                        fState.fHostInfo.fTimeSigNumerator,
                                                        fState.fHostInfo.fTimeSigDenominator);

        // Implementation note: this call allocates memory but it is ok as this happens only when switching between
        // sampling and not sampling... as a user request
        fSampler.init(fClock.getSampleRate(), sampleCount * MAX_SAMPLER_BUFFER_SIZE_BAR);
      }
    }
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
        oPlayingState->fWESelectionPercentPlayer = fState.fWESelectionSlice.getPercentPlayed();
      });

      if(fState.fSampling)
        fState.fSamplingState.broadcast(SamplingState{fSampler.getPercentSampled()});
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
      s.start(fFrameCount);
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
//          DLOG_F(INFO, "Note on %d, %d, %f, %d", slice, e.sampleOffset, e.ppqPosition, e.flags);
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
          s.start(fFrameCount);
      }
    }
  }
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::splitSample
//------------------------------------------------------------------------
void SampleSplitterProcessor::splitSample()
{
  if(fState.fSampleBuffers.hasSamples())
  {
    int numSlices = fState.fNumSlices;
    int32 numSamplesPerSlice = fState.fSampleBuffers.getNumSamples() / numSlices;

    DLOG_F(INFO, "SampleSplitterProcessor::splitSample(%d)", numSlices);

    int32 start = 0;
    for(int i = 0; i < numSlices; i++, start += numSamplesPerSlice)
      fState.fSampleSlices[i].reset(start, start + numSamplesPerSlice - 1);

    for(int i = numSlices + 1; i < NUM_SLICES; i++)
      fState.fSampleSlices[i].stop();
  }
}

}
}
}
}
