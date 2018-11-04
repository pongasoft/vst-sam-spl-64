#include "SampleSlice.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::reset
//------------------------------------------------------------------------
void SampleSlice::reset(int32 iStart, int32 iEnd)
{
  fStart = iStart;
  fEnd = iEnd;
  fCurrent = fStart;
}

//------------------------------------------------------------------------
// SampleSlice::getPercentPlayed
//------------------------------------------------------------------------
float SampleSlice::getPercentPlayed() const
{
  if(isSelected())
  {
    float numSlices = fEnd - fStart;
    if(numSlices > 0)
      return (fCurrent - fStart) / numSlices;
  }
  return -1.0f;
}

}
}
}