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

}
}
}