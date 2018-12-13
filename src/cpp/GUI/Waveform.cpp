#include "Waveform.h"

#include <pongasoft/Utils/Lerp.h>

#include "../SampleBuffers.hpp"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// Waveform::createBitmap
//------------------------------------------------------------------------
BitmapPtr Waveform::createBitmap(COffscreenContext *iContext,
                                 SampleBuffers32 const *iSamples,
                                 const Waveform::LAF &iLAF,
                                 double iOffsetPercent,
                                 double iZoomPercent)
{
  if(!iContext || !iSamples || !iSamples->hasSamples())
    return nullptr;

  auto w = iContext->getWidth() - iLAF.fMargin.fLeft - iLAF.fMargin.fRight;
  auto h = iContext->getHeight() - iLAF.fMargin.fTop - iLAF.fMargin.fBottom;

  if(h <= 0 || w <= 0)
    return nullptr;

  auto maxPercent = 1.0 - w / iSamples->getNumSamples();

  if(iZoomPercent > maxPercent)
    iZoomPercent = maxPercent;

  auto totalNumBuckets = w / (1.0 - iZoomPercent);

  auto zoomedStartOffset = Utils::Lerp<CCoord>::mapValue(iOffsetPercent, 0, 1.0, 0, totalNumBuckets - w);

  auto numSamplesPerBucket = static_cast<int32>(std::round(iSamples->getNumSamples() / totalNumBuckets));
  auto startOffset = static_cast<int32>(std::round(zoomedStartOffset * numSamplesPerBucket));

  const auto numChannels = iSamples->getNumChannels();
  auto channelHeight = (h - iLAF.fVerticalSpacing * (numChannels - 1)) / numChannels;

  iContext->beginDraw();
  iContext->setFrameColor(iLAF.fColor);

  // using antialising and non integral mode to smooth out rendering

  CCoord top = iLAF.fMargin.fTop;

  auto numBuckets = static_cast<int32>(w);

  for(int32 c = 0; c < numChannels; c++)
  {
    iContext->setDrawMode(kAntiAliasing | kNonIntegralMode);

    std::vector<Sample32> mins;
    std::vector<Sample32> maxs;

    auto size = iSamples->computeMinMax(c, mins, maxs, startOffset, numSamplesPerBucket, numBuckets);
    if(size == -1)
      continue;

    // mapping [1,-1] to [0, height] for display
    auto lerp = Utils::Lerp<CCoord>::mapRange(1, -1, top, top + channelHeight);

    // for each x, draw a line connecting min to max
    CPoint p1, p2;
    for(int32 x = 0; x < size; x++)
    {
      p1.x = x + iLAF.fMargin.fLeft;
      p1.y = lerp.computeY(mins[x]);
      p2.x = p1.x;
      p2.y = lerp.computeY(maxs[x]);

      iContext->drawLine(p1, p2);
    }

    // draw the main axis to make sure there is no holes
    p1.x = iLAF.fMargin.fLeft - 0.5;
    p1.y = top + (channelHeight / 2.0);
    p2.x = iLAF.fMargin.fLeft + size - 1 + 0.5;
    p2.y = p1.y;

    iContext->setDrawMode(kAliasing);
    iContext->drawLine(p1, p2);

    top += channelHeight + iLAF.fVerticalSpacing;
  }

  iContext->endDraw();

  return iContext->getBitmap();
}
}
}
}
}