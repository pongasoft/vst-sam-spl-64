#include <pongasoft/VST/SampleRateBasedClock.h>
#include "SampleInfoView.h"
#include "../SampleFile.h"

#include <chrono>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// SampleInfoView::registerParameters
//------------------------------------------------------------------------
void SampleInfoView::registerParameters()
{
  registerParam(fState->fSampleData);
  registerParam(fState->fWESelectedSampleRange);
  computeInfo();
}

//------------------------------------------------------------------------
// SampleInfoView::onParameterChange
//------------------------------------------------------------------------
void SampleInfoView::onParameterChange(ParamID iParamID)
{
  computeInfo();
  CustomViewAdapter::onParameterChange(iParamID);
}

namespace internal {
//------------------------------------------------------------------------
// formatDuration
//------------------------------------------------------------------------
String formatDuration(SampleRate iSampleRate, int32 iNumSamples)
{
  using namespace std::chrono;

  SampleRateBasedClock clock(iSampleRate);
  milliseconds ms(clock.getTimeForSampleCount(static_cast<uint32>(iNumSamples)));

  auto secs = duration_cast<seconds>(ms);
  ms -= duration_cast<milliseconds>(secs);
  auto mins = duration_cast<minutes>(secs);
  secs -= duration_cast<seconds>(mins);

  if(mins.count() > 0)
  {
    return Steinberg::String().printf("%lim %lli.%llis",
                                      mins.count(),
                                      secs.count(),
                                      ms.count());
  }
  else
  {
    return Steinberg::String().printf("%lli.%llis",
                                      secs.count(),
                                      ms.count());
  }
}
}

//------------------------------------------------------------------------
// SampleInfoView::computeInfo
//------------------------------------------------------------------------
void SampleInfoView::computeInfo()
{
  Steinberg::String s("");

  if(fState->fSampleData->exists())
  {
    auto const &sampleData = fState->fSampleData;
    SampleInfo info;

    if(sampleData->getSampleInfo(info) == kResultOk)
    {
      s.printf("%s @ %d | %llu bytes - %s - %d [%s]",
               SampleFile::extractFilename(sampleData->getFilePath()).data(),
               static_cast<int32>(info.fSampleRate),
               sampleData->getSize(),
               info.fNumChannels == 2 ? "stereo" : "mono",
               info.fNumSamples,
               internal::formatDuration(info.fSampleRate, info.fNumSamples).text8());
    }
  }

  setText(UTF8String(s));
}

// the creator
SampleInfoView::Creator __gSampleSplitterSampleInfoCreator("SampleSplitter::SampleInfoView", "SampleSplitter - SampleInfoView");

}
}
}
}