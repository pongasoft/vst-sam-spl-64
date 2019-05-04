#include <pongasoft/VST/AudioBuffer.h>
#include <pongasoft/VST/Debug/ParamTable.h>
#include <pongasoft/VST/Debug/ParamLine.h>

#include <algorithm>

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
  fSamplingRateLimiter{},
  fSampler{2},
  fWaitingForSampling{false}
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
  fSamplingRateLimiter = fClock.getRateLimiter(UI_FRAME_RATE_MS);

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
// ::getNonSilentOffset
//------------------------------------------------------------------------
template<typename SampleType>
int32 getNonSilentOffset(AudioBuffers<SampleType> &iBuffers)
{
  int32 res = iBuffers.getNumSamples();

  for(int c = 0; c < iBuffers.getNumChannels(); c++)
  {
    auto buffer = iBuffers.getAudioChannel(c).getBuffer();
    if(buffer)
    {
      auto found = std::find_if(buffer,
                                buffer + iBuffers.getNumSamples(),
                                [](auto sample) {return !pongasoft::VST::isSilent(sample);});
      res = std::min<int32>(res, static_cast<int32>(found - buffer));
    }
  }

  if(res == iBuffers.getNumSamples())
    res = -1;

  return res;
}

//------------------------------------------------------------------------
// SampleSplitterProcessor::getStartSamplingOffset
//------------------------------------------------------------------------
template<typename SampleType>
int32 SampleSplitterProcessor::getStartSamplingOffset(ProcessData &iData, AudioBuffers<SampleType> &iBuffers) const
{
  int32 offset = 0;

  switch(fState.fSamplingTrigger)
  {
    case ESamplingTrigger::kSamplingTriggerImmediate:
      offset = fState.getParamUpdateSampleOffset(iData, fState.fSampling.getParamID());
      if(offset == -1)
        offset = 0;
      break;

    case ESamplingTrigger::kSamplingTriggerOnPlayFree:
    case ESamplingTrigger::kSamplingTriggerOnPlaySync1Bar:
      offset = getHostPlayingOffset(iData);
      break;

    case ESamplingTrigger::kSamplingTriggerOnSound:
      offset = getNonSilentOffset(iBuffers);
      break;
  }

  return offset;
}

//------------------------------------------------------------------------
// GainMaxSilentOp
//------------------------------------------------------------------------
template<typename SampleType>
struct GainMaxSilentOp
{
  SampleType operator()(SampleType const iSample)
  {
    auto res = static_cast<SampleType>(iSample * fGain.getValue());

    // keep track of silent flag
    if(fSilent)
      fSilent = pongasoft::VST::isSilent(res);

    // keep track of max
    if(iSample < 0)
      fAbsoluteMax = std::max(fAbsoluteMax, -res);
    else
      fAbsoluteMax = std::max(fAbsoluteMax, res);

    return res;
  }

  Gain fGain;
  SampleType fAbsoluteMax{0};
  bool fSilent{true};
};

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

  const AudioBuffers<SampleType> in(data.inputs[input], data.numSamples);

  // we start by applying gain and computing max + silent flag
  auto left =
    out.getLeftChannel().copyFrom(in.getLeftChannel(), GainMaxSilentOp<SampleType>{fState.fSamplingInputGain});
  auto right =
    out.getRightChannel().copyFrom(in.getRightChannel(), GainMaxSilentOp<SampleType>{fState.fSamplingInputGain});

  bool broadcastSample = false;

  if(fState.fSampling.hasChanged())
  {
    if(fState.fSampling)
    {
      int32 offset = getStartSamplingOffset(data, out);

      fWaitingForSampling = offset == -1;

      if(fWaitingForSampling)
      {
        DLOG_F(INFO, "waiting for sampling...");
        fState.fSamplingState.broadcast(SamplingState{PERCENT_SAMPLED_WAITING});
      }
      else
      {
        DLOG_F(INFO, "start sampling... offset=%d", offset);
        fSampler.start();
        fSampler.sample(out, offset, -1);
        fState.fSamplingState.broadcast(SamplingState{fSampler.getPercentSampled()});
      }
    }
    else
    {
      if(fSampler.isSampling())
      {
        int32 offset = fState.getParamUpdateSampleOffset(data, fState.fSampling.getParamID());
        DLOG_F(INFO, "stop sampling... offset=%d", offset);
        fSampler.sample(out, -1, offset);
        fSampler.stop();
        fState.fSamplingState.broadcast(SamplingState{0});
        broadcastSample = true;
      }
    }
  }
  else
  {
    if(fState.fSampling)
    {
      if(fWaitingForSampling)
      {
        int32 offset = getStartSamplingOffset(data, out);

        fWaitingForSampling = offset == -1;

        if(!fWaitingForSampling)
        {
          DLOG_F(INFO, "start sampling... offset=%d", offset);
          fSampler.start();
          fSampler.sample(out, offset, -1);
          fState.fSamplingState.broadcast(SamplingState{fSampler.getPercentSampled()});
        }
      }
      else
      {
        if(fSampler.isSampling())
        {
          if(fSampler.sample(out) == ESamplerState::kDoneSampling)
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

  if(fSamplingRateLimiter.shouldUpdate(static_cast<uint32>(data.numSamples)))
  {
    // update vu meter
    fState.fSamplingLeftVuPPM.update(left.fAbsoluteMax, data);
    fState.fSamplingRightVuPPM.update(right.fAbsoluteMax, data);

    if(fState.fSampling && !fWaitingForSampling)
      fState.fSamplingState.broadcast(SamplingState{fSampler.getPercentSampled()});
  }

  if(fState.fSamplingMonitor)
  {
    out.getLeftChannel().setSilenceFlag(left.fSilent);
    out.getRightChannel().setSilenceFlag(right.fSilent);
    return kResultOk;
  }
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
// SampleSplitterProcessor::getHostPlayingOffset
//------------------------------------------------------------------------
int32 SampleSplitterProcessor::getHostPlayingOffset(ProcessData &iData) const
{
  auto processContext = iData.processContext;
  if(processContext)
  {
    auto isPlaying = (processContext->state & ProcessContext::StatesAndFlags::kPlaying) != 0;

    if(isPlaying)
    {
      if(fState.fSamplingTrigger == ESamplingTrigger::kSamplingTriggerOnPlaySync1Bar)
      {
        auto currentFrameStartSample = processContext->projectTimeSamples;

        auto nextBarSampleCount =
          fClock.getNextBarSampleCount(currentFrameStartSample,
                                       processContext->tempo,
                                       processContext->timeSigNumerator,
                                       processContext->timeSigDenominator);

        auto offset = static_cast<int32>(nextBarSampleCount - currentFrameStartSample);

        if(offset < iData.numSamples)
          return offset;
      }
      else
        return 0;
    }
  }

  return -1;
}


//------------------------------------------------------------------------
// SampleSplitterProcessor::processInputs
//------------------------------------------------------------------------
tresult SampleSplitterProcessor::processInputs(ProcessData &data)
{
  // increment the number of frames
  fFrameCount++;

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

      // we reset the vu ppms
      fState.fSamplingLeftVuPPM.update(0, data);
      fState.fSamplingRightVuPPM.update(0, data);
    }
    else
    {
      if(fState.fSamplingInput.previous() == ESamplingInput::kSamplingOff)
      {
        maybeInitSampler(data);
      }
    }
  }

  // the sampling duration has changed
  if(fState.fSamplingDurationInBars.hasChanged())
    maybeInitSampler(data);

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

      // update host info (if changed)
      processHostInfo(data);
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
          slice = e.noteOn.pitch - fState.fRootKey;
          selected = true;
//          DLOG_F(INFO, "Note on %d, %d, %f, %d", slice, e.sampleOffset, e.ppqPosition, e.flags);
          break;

        case Event::kNoteOffEvent:
          slice = e.noteOn.pitch - fState.fRootKey;
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

//------------------------------------------------------------------------
// SampleSplitterProcessor::maybeInitSampler
//------------------------------------------------------------------------
bool SampleSplitterProcessor::maybeInitSampler(ProcessData &iData)
{
  // when sampling is off we don't initialize the sampler
  if(fState.fSamplingInput == ESamplingInput::kSamplingOff)
    return false;

  // if we are in the middle of sampling, we do not reinitialize the sampler
  if(fState.fSampling)
    return false;

  // we make sure we have the most up to date info about the host
  processHostInfo(iData);

  auto sampleCount = fClock.getSampleCountFor1Bar(fState.fHostInfo.fTempo,
                                                  fState.fHostInfo.fTimeSigNumerator,
                                                  fState.fHostInfo.fTimeSigDenominator)
                     * fState.fSamplingDurationInBars;

  // Implementation note: this call allocates memory but it is ok as this happens only when switching between
  // sampling and not sampling... as a user request
  fSampler.init(fClock.getSampleRate(), sampleCount);

  return true;
}

}
}
}
}
