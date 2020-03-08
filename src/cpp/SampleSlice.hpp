#pragma once

#include <pongasoft/Utils/StringUtils.h>
#include <pongasoft/VST/AudioBuffer.h>
#include "SampleBuffers.h"
#include "Slicer.hpp"
#include "Model.h"

namespace pongasoft::VST::SampleSplitter {

/**
 * This class represents a "slice" of a sample to play defined by a buffer, a start and an end.
 *
 * @tparam numChannels the number of input channels supported (coming from the sample itself, currently only support
 *                     mono or stereo)
 * @tparam numXFadeSamples number of samples used in the cross fading algorithm
 */
template<int32 numChannels = MAX_NUM_INPUT_CHANNELS, int32 numXFadeSamples = NUM_XFADE_SAMPLES>
class SampleSlice
{
public:
  // Keep track of the state of the slice
  enum class EState
  {
    kNotPlaying,
    kPlaying,
    kStopping
  };

  /**
   * @return `true` if the slice is "selected" either via pad or midi/note
   */
  inline bool isSelected() const { return fPadSelected || fNoteSelected; }

  /**
   * Implementation note: before playing a slice, this method may be called multiple times (for example if in the
   * sequencer there is a note off/note on event) and internally it uses the concept of "transition" to figure out
   * what needs to happen when `play` is eventually called.
   *
   * For example:
   * - calling note on / note off cancels each other out => nothing happens
   * - calling note off / note on triggers a restart which needs to cross fade what is currently playing to 0 and
   *   cross fade with the beginning of the slice
   *
   * @param iPad `true` for pad, `false` for note */
  void setSelected(bool iPad, bool iSelected)
  {
    bool wasSelected = isSelected();

    if(iPad)
      fPadSelected = iSelected;
    else
      fNoteSelected = iSelected;

    bool selected = isSelected();

    if(wasSelected != selected)
      fTransition = adjustTransition(fTransition, selected);
  }

  // getState
  inline EState getState() const { return fState; }

  /**
   * \note `EState::kStopping` is considered playing because it happens during cross fading.. the slice has still
   *        some samples to play until stop.
   * @return `true` if the slice is currently playing
   */
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

  // Set/get whether the slice is looping
  inline void loop(bool iLoop) { fLoop = iLoop; }
  inline bool loop() const { return fLoop; }

  //! Sets the direction in which the slice is playing
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
   * "Plays" the slice. Plays each channels from the input sample into each channel of the output buffers (up to
   * the number of samples coming from the output buffers)
   *
   * @return `true` if anything was played at all or `false` if `out` was left untouched */
  template<typename SampleType>
  bool play(EPlayMode iPlayMode, AudioBuffers<SampleType> &oAudioBuffers, bool iOverride)
  {
    if(!prepareSliceForPlaying(iPlayMode))
      return false;

    int32 n = std::min(fNumActiveSlicers, oAudioBuffers.getNumChannels());

    bool donePlaying = false;

    bool played = false;
    auto loopAtEnd = iPlayMode == EPlayMode::kHold && loop();

    for(int32 c = 0; c < n; c++)
    {
      auto channel = oAudioBuffers.getAudioChannel(c);

      if(channel.isActive())
      {
        donePlaying |= playChannel<SampleType>(fSlicers[c], channel, iOverride, loopAtEnd);
        played = true;
      }
    }

    if(donePlaying)
      fState = EState::kNotPlaying;

    return played;
  }

  /**
   * Sets the position of the slice to the beginning, doing any cross fading if necessary */
  void start()
  {
    for(int32 i = 0; i < fNumActiveSlicers; i++)
      fSlicers[i].start();

    fState = EState::kPlaying;
    fTransition = ETransition::kNone;
  }

  /**
   * Requests the slice to stop (if it is currently playing).
   *
   * \note Due to cross fading, the slice may still play a few more samples after this call! */
  void requestStop()
  {
    if(fState == EState::kPlaying)
    {
      bool ended = false;

      for(int32 i = 0; i < fNumActiveSlicers; i++)
        ended = fSlicers[i].requestStop();

      fState = ended ? EState::kNotPlaying : EState::kStopping;
    }

    fTransition = ETransition::kNone;
  }

  /**
   * Forces stop no matter what. No cross fading will happen. After this call, the slice is guaranteed to not be
   * playing. */
  inline void hardStop()
  {
    for(auto &slicer: fSlicers)
      slicer.hardStop();

    fState = EState::kNotPlaying;
    fTransition = ETransition::kNone;
  }

protected:
  using SlicerImpl = Slicer<Sample32, numXFadeSamples>;

  /**
   * Issues the right combination of `start`/`requestStop` calls based on transition and play mode
   *
   * @return `true` if the slice is playing after this call */
  bool prepareSliceForPlaying(EPlayMode iPlayMode)
  {
    switch(fTransition)
    {
      case ETransition::kStarting:
        if(isPlaying())
          requestStop();
        start();
        break;

      case ETransition::kStopping:
        if(iPlayMode == EPlayMode::kHold)
          requestStop();
        else
          fTransition = ETransition::kNone;
        break;

      case ETransition::kRestarting:
        requestStop();
        start();
        break;

      default:
        // nothing to do
        break;
    }

    return isPlaying();
  }

  /**
   * Plays an individual channel. Implementation note: this call does **not** changes `fState`.
   *
   * @return `true` if after this call, the slice is done playing
   */
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

protected:
  // Keep track of transitions
  enum class ETransition
  {
    kNone, kStarting, kStopping, kRestarting
  };

  //------------------------------------------------------------------------
  // adjustTransition
  //------------------------------------------------------------------------
  static ETransition adjustTransition(ETransition iCurrentTransition, bool iStarting)
  {
    switch(iCurrentTransition)
    {
      case ETransition::kNone:
        return iStarting ? ETransition::kStarting : ETransition::kStopping;

      case ETransition::kStarting:
        return iStarting ? ETransition::kStarting : ETransition::kNone;

      case ETransition::kStopping:
      case ETransition::kRestarting:
        return iStarting ? ETransition::kRestarting : ETransition::kStopping;
    }
  }

private:
  bool fPadSelected{false};
  bool fNoteSelected{false};

  bool fLoop{false};

  EState fState{EState::kNotPlaying};
  ETransition fTransition{ETransition::kNone};

  int32 fNumActiveSlicers{numChannels};
  SlicerImpl fSlicers[numChannels];
};

}
