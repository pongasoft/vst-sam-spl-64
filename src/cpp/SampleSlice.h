#pragma once

#include "SampleBuffers.h"
#include <pongasoft/VST/AudioBuffer.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

enum class EPlayingState
{
  kNotPlaying,
  kPlaying,
  kDonePlaying
};

class SampleSlice
{
public:
  void reset(int32 iStart, int32 iEnd);
  void resetCurrent();

  inline bool isSelected() const { return fPadSelected || fNoteSelected; }
  inline void setPadSelected(bool iSelected) { fPadSelected = iSelected; }
  inline void setNoteSelected(bool iSelected) { fNoteSelected = iSelected; }
  inline uint32 getStartFrame() const { return fStartFrame; }
  float getPercentPlayed() const;

  int32 getPlayStart() const { return fReverse ? fEnd - 1 : fStart; };
  int32 getPlayEnd() const { return fReverse ? fStart - 1: fEnd; } ;

  inline void setLoop(bool iLoop) { fLoop = iLoop; }
  inline void setReverse(bool iReverse) { fReverse = iReverse; }

  inline void start(uint32 iStartFrame = 0) { resetCurrent(); fState = EPlayingState::kPlaying; fStartFrame = iStartFrame; }
  inline void stop() { fState = EPlayingState::kNotPlaying; }

  inline bool isPlaying() const { return fState == EPlayingState::kPlaying; }
  inline bool isDonePlaying() const { return fState == EPlayingState::kDonePlaying; }

  EPlayingState getState() const { return fState; }

  /**
   * Play the sample according to the settings.
   * @param iSample the input sample (slice of sample comes from fStart/fEnd)
   * @param oAudioBuffers output buffer
   * @param iOverride whether to override what is already in oAudioBuffers (true) or add to it (false)
   * @return true if done playing
   */
  template<typename SampleType>
  EPlayingState play(SampleBuffers32 &iSample, AudioBuffers<SampleType> &oAudioBuffers, bool iOverride);

private:
  int32 fStart{-1};
  int32 fEnd{-1};
  int32 fCurrent{-1};

  uint32 fStartFrame{};
  bool fPadSelected{false};
  bool fNoteSelected{false};

  bool fReverse{false};
  bool fLoop{false};

  EPlayingState fState{EPlayingState::kNotPlaying};
};

}
}
}

