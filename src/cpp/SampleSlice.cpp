#include "SampleSlice.h"
#include "Model.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::resetCurrent
//------------------------------------------------------------------------
void SampleSlice::resetCurrent()
{
  fCurrent = getPlayStart();
}

//------------------------------------------------------------------------
// SampleSlice::reset
//------------------------------------------------------------------------
void SampleSlice::reset(int32 iStart, int32 iEnd)
{
  fStart = iStart;
  fEnd = iEnd;
  resetCurrent();
}

//------------------------------------------------------------------------
// SampleSlice::getPercentPlayed
//------------------------------------------------------------------------
float SampleSlice::getPercentPlayed() const
{
  switch(fState)
  {
    case EPlayingState::kPlaying:
    {
      float numSlices = fEnd - fStart;
      if(numSlices > 0)
      {
        return (fCurrent - getPlayStart()) / numSlices;
      }
      break;
    }

    case EPlayingState::kDonePlaying:
      return fReverse ? -1.0f : 1.0f;

    default:
      break;
  }

  return PERCENT_PLAYED_NOT_PLAYING;
}

}
}
}