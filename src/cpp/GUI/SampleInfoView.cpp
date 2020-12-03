#include <pongasoft/VST/SampleRateBasedClock.h>
#include "SampleInfoView.h"

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
  registerParam(fState->fCurrentSample);
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
Steinberg::String formatDuration(SampleRate iSampleRate, int32 iNumSamples)
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

  auto const &currentSample = *fState->fCurrentSample;
  auto const &currentFile = *fState->fSampleFile;

  if(currentSample.hasSamples() && !currentFile.empty())
  {
    s.printf("%s @ %d | %llu bytes - %s - %d [%s]",
             SampleFile::extractFilename(currentFile.getOriginalFilePath()).c_str(),
             static_cast<int32>(currentSample.getOriginalSampleRate()),
             currentFile.getFileSize(),
             currentSample.getNumChannels() == 2 ? "stereo" : "mono",
             currentSample.getNumSamples(),
             internal::formatDuration(currentSample.getSampleRate(), currentSample.getNumSamples()).text8());
  }

  setText(UTF8String(s));
}

// the creator
SampleInfoView::Creator __gSampleSplitterSampleInfoCreator("SampleSplitter::SampleInfoView", "SampleSplitter - SampleInfoView");

}
}
}
}