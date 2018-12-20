#pragma once

#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <pongasoft/VST/GUI/Types.h>
#include <pongasoft/VST/GUI/LookAndFeel.h>

#include "../SampleBuffers.h"


namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * Represents the waveform
 */
class Waveform
{
public:
  /**
   * Look and feel of the waveform */
  struct LAF
  {
    CColor fColor{};
    CColor fAxisColor{};
    CCoord fVerticalSpacing{};
    Margin fMargin{};
    CColor fZeroCrossingColor{kTransparentCColor};
  };

public:
  /**
   * Generates a bitmap (waveform graphics representation) for the samples
   */
  static BitmapPtr createBitmap(COffscreenContext *iContext,
                                SampleBuffers32 const *iSamples,
                                LAF const &iLAF,
                                double iOffsetPercent = 0,
                                double iZoomPercent = 0,
                                int32 *oStartOffset = nullptr,
                                int32 *oEndOffset = nullptr);

  /**
   * Compute oOffsetPercent and oZoomPercent from start/end offset
   */
  static bool computeFromOffset(int32 iNumSamples,
                                CCoord iWidth,
                                LAF const &iLAF,
                                int32 iStartOffset,
                                int32 iEndOffset,
                                double &oOffsetPercent,
                                double &oZoomPercent);
};


}
}
}
}