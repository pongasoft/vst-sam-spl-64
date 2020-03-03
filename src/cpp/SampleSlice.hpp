#pragma once

#include "SampleSlice.h"

namespace pongasoft::VST::SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::play
//------------------------------------------------------------------------
template<typename SampleType>
EPlayingState SampleSlice::play(AudioBuffers<SampleType> &oAudioBuffers, bool iOverride)
{
  if(fState != EPlayingState::kPlaying || oAudioBuffers.getNumChannels() == 0)
    return fState;

  auto leftState = playChannel<SampleType>(fLeftSlicer, oAudioBuffers.getLeftChannel(), iOverride);

  if(oAudioBuffers.getNumChannels() > 1)
  {
    auto rightState = playChannel<SampleType>(fRightSlicer, oAudioBuffers.getRightChannel(), iOverride);
    if(leftState != rightState)
      DLOG_F(ERROR, "leftState != rightState | %d != %d", leftState, rightState);
  }

  fState = leftState;

  return fState;
}

//------------------------------------------------------------------------
// SampleSlice::playChannel
//------------------------------------------------------------------------
template<typename SampleType>
EPlayingState
SampleSlice::playChannel(SampleSlice::SlicerImpl &iSlicer, typename AudioBuffers<SampleType>::Channel oChannel, bool iOverride)
{
  auto state = fState;

  auto audioBuffer = oChannel.getBuffer(); // we know it is not null here

  bool silent = true;

  for(int32 i = 0; i < oChannel.getNumSamples(); i++)
  {
    if(state != EPlayingState::kPlaying)
    {
      if(iOverride)
        audioBuffer[i] = 0;
      else
        silent = silent && isSilent(audioBuffer[i]);
      continue;
    }

    if(!iSlicer.hasNext())
    {
      if(!fLoop || !isSelected())
      {
        if(iOverride)
          audioBuffer[i] = 0;
        else
          silent = silent && isSilent(audioBuffer[i]);
        state = EPlayingState::kDonePlaying;
        continue;
      }
      iSlicer.start();
    }

    auto sample = static_cast<SampleType>(iSlicer.next());

    if(iOverride)
      audioBuffer[i] = static_cast<SampleType>(sample);
    else
      audioBuffer[i] += static_cast<SampleType>(sample);

    silent = silent && isSilent(audioBuffer[i]);
  }

  oChannel.setSilenceFlag(silent);

  return state;
}


}
