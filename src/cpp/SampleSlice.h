#pragma once

#include "SampleBuffers.h"
#include "Slicer.hpp"
#include <pongasoft/VST/AudioBuffer.h>

namespace pongasoft::VST::SampleSplitter {

enum class EPlayingState
{
  kNotPlaying,
  kPlaying,
  kDonePlaying
};


class SampleSlice
{
public:
  inline void reset(int32 iStart, int32 iEnd) { fSlicer.reset(iStart, iEnd); }

  inline bool isSelected() const { return fPadSelected || fNoteSelected; }
  inline void setPadSelected(bool iSelected) { fPadSelected = iSelected; }
  inline void setNoteSelected(bool iSelected) { fNoteSelected = iSelected; }
  inline uint32 getStartFrame() const { return fStartFrame; }
  float getPercentPlayed() const;

  inline void setLoop(bool iLoop) { fLoop = iLoop; }
  inline void setReverse(bool iReverse) { fSlicer.reverse(iReverse); }

  /**
   * Specifies whether cross fading should happen or not (in order to avoid pops and clicks when starting/stopping or
   * looping.
   */
  inline void setCrossFade(bool iEnabled) { fSlicer.crossFade(iEnabled); }

  void start(uint32 iStartFrame = 0);

  //
  EPlayingState requestStop();

  inline bool isPlaying() const { return fState == EPlayingState::kPlaying; }
  inline bool isDonePlaying() const { return fState == EPlayingState::kDonePlaying; }

  EPlayingState getState() const { return fState; }

  /**
   * Play the sample according to the settings.
   * @param iSample the input sample (slice of sample comes from fStart/fEnd)
   * @param oAudioBuffers output buffer
   * @param iOverride whether to override what is already in oAudioBuffers (true) or add to it (false)
   * @return the state the slice is in
   */
  template<typename SampleType>
  EPlayingState play(SampleBuffers32 const &iSample, AudioBuffers<SampleType> &oAudioBuffers, bool iOverride);

private:
  Slicer<Sample32, 64> fSlicer{};

  uint32 fStartFrame{};
  bool fPadSelected{false};
  bool fNoteSelected{false};

  bool fLoop{false};

  EPlayingState fState{EPlayingState::kNotPlaying};
};

}

