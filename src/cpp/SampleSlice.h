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
  void reset(SampleBuffers32 const *iSample, int32 iStart, int32 iEnd);

  inline bool isSelected() const { return fPadSelected || fNoteSelected; }
  inline void setPadSelected(bool iSelected) { fPadSelected = iSelected; }
  inline void setNoteSelected(bool iSelected) { fNoteSelected = iSelected; }
  inline uint32 getStartFrame() const { return fStartFrame; }
  float getPercentPlayed() const;

  inline void setLoop(bool iLoop) { fLoop = iLoop; }
  inline void setReverse(bool iReverse) { fLeftSlicer.reverse(iReverse); fRightSlicer.reverse(iReverse); }

  /**
   * Specifies whether cross fading should happen or not (in order to avoid pops and clicks when starting/stopping or
   * looping.
   */
  inline void setCrossFade(bool iEnabled) { fLeftSlicer.crossFade(iEnabled); fRightSlicer.crossFade(iEnabled); }

  void start(uint32 iStartFrame = 0);

  //
  EPlayingState requestStop();

  inline bool isPlaying() const { return fState == EPlayingState::kPlaying; }
  inline bool isDonePlaying() const { return fState == EPlayingState::kDonePlaying; }

  EPlayingState getState() const { return fState; }

  /**
   * Play the sample according to the settings.
   * @param oAudioBuffers output buffer
   * @param iOverride whether to override what is already in oAudioBuffers (true) or add to it (false)
   * @return the state the slice is in
   */
  template<typename SampleType>
  EPlayingState play(AudioBuffers<SampleType> &oAudioBuffers, bool iOverride);

protected:
  using SlicerImpl = Slicer<Sample32, 65>;

  template<typename SampleType>
  EPlayingState playChannel(SlicerImpl &iSlicer, typename AudioBuffers<SampleType>::Channel oChannel, bool iOverride);

private:
  SlicerImpl fLeftSlicer{};
  SlicerImpl fRightSlicer{};

  uint32 fStartFrame{};
  bool fPadSelected{false};
  bool fNoteSelected{false};

  bool fLoop{false};

  EPlayingState fState{EPlayingState::kNotPlaying};
};

}

