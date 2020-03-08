#pragma once

#include <pongasoft/Utils/StringUtils.h>
#include <pongasoft/VST/AudioBuffer.h>
#include "SampleBuffers.h"
#include "Slicer.hpp"
#include "Model.h"

namespace pongasoft::VST::SampleSplitter {

template<int32 numChannels = 2, int32 numXFadeSamples = 65>
class SampleSlice
{
public:
  enum class EState
  {
    kNotPlaying,
    kPlaying,
    kStopping
  };

  inline bool isSelected() const { return fPadSelected || fNoteSelected; }

  /**
   * @param iPad `true` for pad, `false` for note */
  void setSelected(bool iPad, bool iSelected) { if(iPad) fPadSelected = iSelected; else fNoteSelected = iSelected;}

  inline EState getState() const { return fState; }
  inline bool isPlaying() const { return fState != EState::kNotPlaying; }

  /**
   * @return how much of a slice was played
   */
  float getPercentPlayed() const
  {
    if(isPlaying() && fNumActiveSlicers > 0)
    {
      auto &slicer = fSlicers[0];
      auto numSlices = slicer.numSlices();
      if(numSlices > 0)
      {
        return static_cast<float>(slicer.numSlicesPlayed()) / numSlices;
      }
    }

    return PERCENT_PLAYED_NOT_PLAYING;
  }

  inline void loop(bool iLoop) { fLoop = iLoop; }
  inline bool loop() const { return fLoop; }

  void reverse(bool iReverse)
  {
    for(auto &slicer : fSlicers)
      slicer.reverse(iReverse);
  }

  /**
   * Specifies whether cross fading should happen or not (in order to avoid pops and clicks when starting/stopping or
   * looping.
   */
  inline void crossFade(bool iEnabled)
  {
    for(auto &slicer : fSlicers)
      slicer.crossFade(iEnabled);
  }

  /**
   * Resets the slice to the new sample buffers and/or start/end */
  void reset(SampleBuffers32 const *iSample, int32 iStart, int32 iEnd)
  {
    DCHECK_F(iSample->getNumChannels() > 0);
    DCHECK_F(iStart >= 0 && iStart < iSample->getNumSamples());
    DCHECK_F(iEnd >= iStart && iEnd <= iSample->getNumSamples());

    fNumActiveSlicers = std::min(iSample->getNumChannels(), numChannels);

    for(int32 c = 0; c < fNumActiveSlicers; c++)
    {
      fSlicers[c].reset(iSample->getChannelBuffer(c), iStart, iEnd);
    }
  }

  /**
   * "Plays" the slice.
   * @return `true` if the slice is done playing at the end */
  template<typename SampleType>
  void play(AudioBuffers<SampleType> &oAudioBuffers, bool iOverride, bool iLoopAtEnd)
  {
    if(!isPlaying())
      return;

    int32 n = std::min(fNumActiveSlicers, oAudioBuffers.getNumChannels());

    bool donePlaying = false;

    for(int32 c = 0; c < n; c++)
    {
      auto channel = oAudioBuffers.getAudioChannel(c);

      if(channel.isActive())
        donePlaying |= playChannel<SampleType>(fSlicers[c], channel, iOverride, iLoopAtEnd);
    }

    if(donePlaying)
      fState = EState::kNotPlaying;
  }

  void start()
  {
    for(int32 i = 0; i < fNumActiveSlicers; i++)
      fSlicers[i].start();

    fState = EState::kPlaying;
  }

  void requestStop()
  {
    if(fState == EState::kPlaying)
    {
      bool ended = false;

      for(int32 i = 0; i < fNumActiveSlicers; i++)
        ended = fSlicers[i].requestStop();

      fState = ended ? EState::kNotPlaying : EState::kStopping;
    }
  }

  /**
   * Forces stop no matter what */
  inline void hardStop()
  {
    for(auto &slicer: fSlicers)
      slicer.hardStop();

    fState = EState::kNotPlaying;
  }

protected:
  using SlicerImpl = Slicer<Sample32, numXFadeSamples>;

  //------------------------------------------------------------------------
  // playChannel
  //------------------------------------------------------------------------
  template<typename SampleType>
  bool playChannel(SlicerImpl &iSlicer,
                   typename AudioBuffers<SampleType>::Channel oChannel,
                   bool iOverride,
                   bool iLoopAtEnd)
  {
    bool donePlaying = false;
    bool silent = true;

    auto audioBuffer = oChannel.getBuffer(); // we know it is not null here

    int32 silentSamples = 0;

    for(int32 i = 0; i < oChannel.getNumSamples(); i++)
    {
      if(donePlaying)
      {
        if(iOverride)
        {
          audioBuffer[i] = 0;
          silentSamples++;
        }
        else
          silent = silent && isSilent(audioBuffer[i]);
        continue;
      }

      if(!iSlicer.hasNext())
      {
        if(fState == EState::kStopping || !iLoopAtEnd)
        {
          if(iOverride)
          {
            audioBuffer[i] = 0;
            silentSamples++;
          }
          else
            silent = silent && isSilent(audioBuffer[i]);
          donePlaying = true;
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

    // need to account that we may have played the last sample and there may be no more in the "next" call
    donePlaying = donePlaying || (!iSlicer.hasNext() && (fState == EState::kStopping || !iLoopAtEnd));

    return donePlaying;
  }


private:
  bool fPadSelected{false};
  bool fNoteSelected{false};

  bool fLoop{false};

  EState fState{EState::kNotPlaying};

  int32 fNumActiveSlicers{numChannels};
  SlicerImpl fSlicers[numChannels];
};

}
