#include "SampleLoaderView.h"
#include <vstgui4/vstgui/lib/cfileselector.h>
#include "../mackron/dr_libs/dr_wav.h"
#include "../Model.h"
#include "../SampleBuffers.hpp"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {


//------------------------------------------------------------------------
// SampleLoaderView::setControlValue
//------------------------------------------------------------------------
void SampleLoaderView::setControlValue(bool const &iControlValue)
{
  if(iControlValue && getControlValue() != iControlValue)
  {
    DLOG_F(INFO, "opening file selector");
    CNewFileSelector *selector = CNewFileSelector::create(getFrame(), CNewFileSelector::kSelectFile);
    if(selector)
    {
      //selector->addFileExtension(CFileExtension("AIFF", "aif", "audio/x-aiff"));
      selector->addFileExtension(CFileExtension("WAVE", "wav", "audio/x-wav"));
      selector->setTitle("Choose A Sample");
      selector->run(this);
      selector->forget();
    }
  }

  ToggleButtonView::setControlValue(iControlValue);
}

//------------------------------------------------------------------------
// SampleLoaderView::notify
//------------------------------------------------------------------------
CMessageResult SampleLoaderView::notify(CBaseObject *sender, IdStringPtr message)
{
  if(message == CNewFileSelector::kSelectEndMessage)
  {
    auto selector = dynamic_cast<CNewFileSelector *>(sender);
    if(selector)
    {
      // do anything with the selected files here
      DLOG_F(INFO, "detected %d", selector->getNumSelectedFiles());
      if(selector->getNumSelectedFiles() > 0)
      {
        auto filename = selector->getSelectedFile(0);
        DLOG_F(INFO, "detected %s", filename);
        unsigned int channels;
        unsigned int sampleRate;
        drwav_uint64 totalSampleCount;
        float *pSampleData = drwav_open_and_read_file_f32(filename, &channels, &sampleRate, &totalSampleCount);
        if(pSampleData == nullptr)
        {
          DLOG_F(ERROR, "error opening file %s", filename);
        }
        else
        {
          DLOG_F(INFO, "read %d/%d/%llu", channels, sampleRate, totalSampleCount);
          auto sampleBuffers = SampleBuffers32::fromInterleaved(sampleRate,
                                                                channels,
                                                                static_cast<int32>(totalSampleCount),
                                                                pSampleData);
          fState->fFileSample.broadcast(std::move(*sampleBuffers));
        }

        drwav_free(pSampleData);
      }

      ToggleButtonView::setControlValue(false);
      return kMessageNotified;
    }
  }
  return ToggleButtonView::notify(sender, message);
}

void SampleLoaderView::initState(GUIState *iGUIState)
{
  PluginAccessor::initState(iGUIState);
  ToggleButtonView::initState(iGUIState);
}

// the creator
SampleLoaderView::Creator __gSampleSplitterSampleLoaderCreator("SampleSplitter::SampleLoader", "SampleSplitter - SampleLoader");

}
}
}
}