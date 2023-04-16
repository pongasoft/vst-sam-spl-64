#include "SampleLoaderView.h"
#include <vstgui4/vstgui/lib/cfileselector.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {


//------------------------------------------------------------------------
// SampleLoaderView::onClick
//------------------------------------------------------------------------
void SampleLoaderView::onClick()
{
  auto selector = VSTGUI::owned(CNewFileSelector::create(getFrame(), CNewFileSelector::kSelectFile));
  if(selector)
  {
    //selector->addFileExtension(CFileExtension("AIFF", "aif", "audio/x-aiff"));
    //selector->addFileExtension(CFileExtension("WAVE", "wav", "audio/x-wav"));
    selector->setTitle("Choose A Sample");
    if(selector->runModal())
    {
      if(selector->getNumSelectedFiles() > 0)
      {
        auto filename = selector->getSelectedFile(0);
        fState->maybeLoadSample(filename);
      }
    }
  }
  TextButtonView::onClick();
}

// the creator
SampleLoaderView::Creator __gSampleSplitterSampleLoaderCreator("SampleSplitter::SampleLoader", "SampleSplitter - SampleLoader");

}
}
}
}