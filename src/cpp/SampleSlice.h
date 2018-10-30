#pragma once

#include "SampleBuffers.h"
#include <pongasoft/VST/AudioBuffer.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

class SampleSlice
{
public:
  void reset(int32 iStart, int32 iEnd);
  void resetCurrent() { fCurrent = fStart; }

  inline bool isSelected() const { return fPadSelected || fNoteSelected; }
  inline void setPadSelected(bool iSelected) { fPadSelected = iSelected; }
  inline void setNoteSelected(bool iSelected) { fNoteSelected = iSelected; }

  template<typename SampleType>
  void play(SampleBuffers32 &iSample, AudioBuffers<SampleType> &oAudioBuffers);

private:
  int32 fStart{-1};
  int32 fEnd{-1};
  int32 fCurrent{-1};

  bool fPadSelected{false};
  bool fNoteSelected{false};
};

}
}
}

