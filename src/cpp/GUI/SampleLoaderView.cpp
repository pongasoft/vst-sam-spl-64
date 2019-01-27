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
      if(selector->getNumSelectedFiles() > 0)
      {
        auto filename = selector->getSelectedFile(0);
        fState->loadSample(filename);
      }

      return kMessageNotified;
    }
  }
  return TextButtonView::notify(sender, message);
}

// the creator
SampleLoaderView::Creator __gSampleSplitterSampleLoaderCreator("SampleSplitter::SampleLoader", "SampleSplitter - SampleLoader");

}
}
}
}