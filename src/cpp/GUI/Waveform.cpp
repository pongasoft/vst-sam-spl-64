#include "Waveform.h"

#include <pongasoft/Utils/Lerp.h>
#include <pongasoft/Utils/Constants.h>
#include <pongasoft/VST/GUI/GUIUtils.h>

#include "../SampleBuffers.hpp"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

constexpr auto MIN_MAX_COMPUTATION_THRESHOLD = 2;

namespace internal {
inline bool isZeroCrossing(Sample32 iSample1, Sample32 iSample2)
{
  if(iSample1 == 0 || iSample2 == 0)
    return true;

  return iSample1 > 0 ? iSample2 < 0 : iSample2 > 0;
}
}

//------------------------------------------------------------------------
// Waveform::createBitmap
//------------------------------------------------------------------------
BitmapPtr Waveform::createBitmap(COffscreenContext *iContext,
                                 SampleBuffers32 const *iSamples,
                                 const Waveform::LAF &iLAF,
                                 double iOffsetPercent,
                                 double iZoomPercent,
                                 int32 *oStartOffset,
                                 int32 *oEndOffset)
{
  if(!iContext || !iSamples || !iSamples->hasSamples())
    return nullptr;

  auto w = iContext->getWidth() - iLAF.fMargin.fLeft - iLAF.fMargin.fRight;
  auto h = iContext->getHeight() - iLAF.fMargin.fTop - iLAF.fMargin.fBottom;

  if(h <= 0 || w <= 0)
    return nullptr;

  double numSamplesPerBucket;
  int32 startOffset;

  if(iSamples->getNumSamples() <= w)
  {
    // handling the case when there is less samples to display
    numSamplesPerBucket = 1;
    startOffset = 0;
  }
  else
  {
    // when zoom is 1.0 we display 1 sample per pixel => we display w samples
    // when zoom is 0.0 we display all samples
    auto numSamples = Utils::DPLerp::mapValue(iZoomPercent, 1.0, 0.0, w, iSamples->getNumSamples());

    // since we display numSamples in w pixels (and 1 pixel = 1 bucket)
    numSamplesPerBucket = numSamples / w;

    // the total number of buckets
    auto totalNumBuckets = iSamples->getNumSamples() / numSamplesPerBucket;

    // when offset is 0.0 we display the first bucket (0)
    // when offset is 1.0 we are at the right edge so we start at totalNumBuckets - w
    auto bucketStartOffset = Utils::DPLerp::mapValue(iOffsetPercent, 0.0, 1.0, 0, totalNumBuckets - w);

    startOffset = static_cast<int32>(std::round(bucketStartOffset * numSamplesPerBucket));
  }


  const auto numChannels = iSamples->getNumChannels();
  auto channelHeight = (h - iLAF.fVerticalSpacing * (numChannels - 1)) / numChannels;

  iContext->beginDraw();
  iContext->setFrameColor(iLAF.fColor);


  CCoord top = iLAF.fMargin.fTop;

  auto numBuckets = static_cast<int32>(w);

  std::vector<Sample32> avgs;
  std::vector<Sample32> mins;
  std::vector<Sample32> maxs;

  if(numSamplesPerBucket < MIN_MAX_COMPUTATION_THRESHOLD)
    avgs.reserve(static_cast<unsigned long>(numBuckets));
  else
  {
    mins.reserve(static_cast<unsigned long>(numBuckets));
    maxs.reserve(static_cast<unsigned long>(numBuckets));
  }

  bool drawAxis = !CColorUtils::isTransparent(iLAF.fAxisColor);
  bool showZeroCrossing = !CColorUtils::isTransparent(iLAF.fZeroCrossingColor) && iLAF.fAxisColor != iLAF.fZeroCrossingColor;

  CPoint p1,p2;

  for(int32 c = 0; c < numChannels; c++)
  {
    if(c > 0)
    {
      top += channelHeight + iLAF.fVerticalSpacing;
    }

    // shall we draw an axis?
    if(drawAxis)
    {
      p1.x = iLAF.fMargin.fLeft - 0.5;
      p1.y = top + (channelHeight / 2.0);
      p2.x = w + 0.5;
      p2.y = p1.y;

      iContext->setFrameColor(iLAF.fAxisColor);
      iContext->setDrawMode(kAliasing);
      iContext->drawLine(p1, p2);
    }

    // using antialising and non integral mode to smooth out rendering
    iContext->setDrawMode(kAntiAliasing | kNonIntegralMode);

    // mapping [1,-1] to [0, height] for display
    auto lerp = Utils::mapRangeDP(1.0, -1.0, top, top + channelHeight);

    // use average algorithm
    if(numSamplesPerBucket < MIN_MAX_COMPUTATION_THRESHOLD)
    {
      avgs.clear();
      auto size = iSamples->computeAvg(c,
                                       avgs,
                                       startOffset,
                                       numSamplesPerBucket,
                                       numBuckets,
                                       oEndOffset);

      if(size < 1)
        continue;

      // we actually draw the waveform connecting each sample to the next with a line
      auto previousSample = avgs[0];
      p1.x = iLAF.fMargin.fLeft;
      p1.y = lerp.computeY(previousSample);

      iContext->setFrameColor(iLAF.fColor);

      for(int i = 1; i < size; i++)
      {
        auto currentSample = avgs[i];

        p2.x = p1.x + 1;
        p2.y = lerp.computeY(currentSample);

        if(showZeroCrossing)
          iContext->setFrameColor(internal::isZeroCrossing(previousSample, currentSample) ?
                                  iLAF.fZeroCrossingColor :
                                  iLAF.fColor);

        iContext->drawLine(p1, p2);
        p1 = p2;
        previousSample = currentSample;
      }

    }
    else
    {
      // use min max algorithm
      mins.clear();
      maxs.clear();
      auto size = iSamples->computeMinMax(c,
                                          mins, maxs,
                                          startOffset,
                                          numSamplesPerBucket,
                                          numBuckets,
                                          oEndOffset);

      if(size < 1)
        continue;

      iContext->setFrameColor(iLAF.fColor);
      iContext->setFillColor(iLAF.fColor);

      // we draw a polygon connecting min/max of sample[n] to min/max of sample [n+1]
      std::vector<CPoint> polygon(4);

      polygon[0].x = iLAF.fMargin.fLeft;
      polygon[0].y = lerp.computeY(mins[0]);
      polygon[1].x = polygon[0].x;
      polygon[1].y = lerp.computeY(maxs[0]);

      for(int32 x = 1; x < size; x++)
      {
        polygon[2].x = polygon[0].x + 1;
        polygon[2].y = lerp.computeY(maxs[x]);
        polygon[3].x = polygon[2].x;
        polygon[3].y = lerp.computeY(mins[x]);

        iContext->drawPolygon(polygon, kDrawFilledAndStroked);

        polygon[0] = polygon[3];
        polygon[1] = polygon[2];
      }

    }
  }

  iContext->endDraw();

  if(oStartOffset)
    *oStartOffset = startOffset;

  return iContext->getBitmap();
}

//------------------------------------------------------------------------
// Waveform::computeFromOffset
//------------------------------------------------------------------------
bool Waveform::computeFromOffset(int32 iNumSamples,
                                 CCoord iWidth,
                                 LAF const &iLAF,
                                 int32 iStartOffset,
                                 int32 iEndOffset,
                                 double &oOffsetPercent,
                                 double &oZoomPercent)
{
  auto w = iWidth - iLAF.fMargin.fLeft - iLAF.fMargin.fRight;

  if(w <= 0)
    return false;

  if(iNumSamples < w)
  {
    oOffsetPercent = 0;
    oZoomPercent = 0;
    return false;
  }

  iStartOffset = Utils::clamp(iStartOffset, Utils::ZERO_INT32, iNumSamples);
  iEndOffset = Utils::clamp(iEndOffset, iStartOffset, iNumSamples);

  // implementation note: reversing the formulas from createBitmap

  auto numSamples = static_cast<double>(iEndOffset - iStartOffset);

  // reversing auto numSamples = Utils::DPLerp::mapValue(iZoomPercent, 1.0, 0.0, w, iSamples->getNumSamples());
  oZoomPercent = Utils::DPLerp::mapValue(numSamples, w, iNumSamples, 1.0, 0.0);

  auto numSamplesPerBucket = numSamples / w;
  auto totalNumBuckets = iNumSamples / numSamplesPerBucket;

  // reversing auto startOffset = static_cast<int32>(std::round(bucketStartOffset * numSamplesPerBucket));
  auto bucketStartOffset = iStartOffset / numSamplesPerBucket;

  // reversing  auto bucketStartOffset = Utils::DPLerp::mapValue(iOffsetPercent, 0.0, 1.0, 0, totalNumBuckets - w);
  oOffsetPercent = Utils::DPLerp::mapValue(bucketStartOffset, 0, totalNumBuckets - w, 0.0, 1.0);

  return true;
}

}
}
}
}