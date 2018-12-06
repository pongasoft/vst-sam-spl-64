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
    CCoord fVerticalSpacing{};
    Margin fMargin{};
  };

public:
  /**
   * Generates a bitmap (waveform graphics representation) for the samples
   */
  static BitmapPtr createBitmap(COffscreenContext *iContext,
                                SampleBuffers32 const *iSamples,
                                LAF const &iLAF);
};


}
}
}
}