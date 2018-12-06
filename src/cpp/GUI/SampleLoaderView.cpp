#include "SampleLoaderView.h"
#include <vstgui4/vstgui/lib/cfileselector.h>
#include "../Model.h"
#include "../SampleBuffers.hpp"
#include "../SampleFile.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {


//------------------------------------------------------------------------
// SampleLoaderView::onClick
//------------------------------------------------------------------------
void SampleLoaderView::onClick()
{
  DLOG_F(INFO, "opening file selector");
  CNewFileSelector *selector = CNewFileSelector::create(getFrame(), CNewFileSelector::kSelectFile);
  if(selector)
  {
    //selector->addFileExtension(CFileExtension("AIFF", "aif", "audio/x-aiff"));
    //selector->addFileExtension(CFileExtension("WAVE", "wav", "audio/x-wav"));
    selector->setTitle("Choose A Sample");
    selector->run(this);
    selector->forget();
  }
  TextButtonView::onClick();
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
        SampleData sampleData;
        if(sampleData.init(filename) == kResultOk)
        {
          fState->fSampleData.setValue(std::move(sampleData));
          fState->broadcastSample();
        }
      }

      return kMessageNotified;
    }
  }
  return TextButtonView::notify(sender, message);
}

//------------------------------------------------------------------------
// SampleLoaderView::initState
//------------------------------------------------------------------------
void SampleLoaderView::initState(GUIState *iGUIState)
{
  PluginAccessor::initState(iGUIState);
  TextButtonView::initState(iGUIState);
}

// the creator
SampleLoaderView::Creator __gSampleSplitterSampleLoaderCreator("SampleSplitter::SampleLoader", "SampleSplitter - SampleLoader");

}
}
}
}