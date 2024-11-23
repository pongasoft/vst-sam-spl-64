#include <pongasoft/VST/GUI/Views/CustomView.h>
#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include <vstgui4/vstgui/lib/cfileselector.h>
#include "../Plugin.h"

#include "SampleMgr.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * Handles saving/exporting the file
 */
class SampleSaverView : public Views::StateAwareView<Views::TextButtonView, SampleSplitterGUIState>
{
public:
  using super_type = Views::StateAwareView<Views::TextButtonView, SampleSplitterGUIState>;

public:
  //------------------------------------------------------------------------
  // Constructor
  //------------------------------------------------------------------------
  explicit SampleSaverView(const CRect &iSize) : super_type(iSize) {};

  //------------------------------------------------------------------------
  // registerParameters
  //------------------------------------------------------------------------
  void registerParameters() override
  {
    TextButtonView::registerParameters();

    registerCallback<CurrentSample>(fState->fCurrentSample, [this] (GUIJmbParam<CurrentSample> &iParam) {
      setMouseEnabled(iParam->hasSamples());
    }, true);

    // this view is not interested to be notified on these param changes, but it uses them (hence false)
    fExportSampleMajorFormat = registerParam(fParams->fExportSampleMajorFormat, false);
    fExportSampleMinorFormat = registerParam(fParams->fExportSampleMinorFormat, false);
  }

  //------------------------------------------------------------------------
  // onClick
  //------------------------------------------------------------------------
  void onClick() override
  {
    auto selector = VSTGUI::owned(CNewFileSelector::create(getFrame(), CNewFileSelector::kSelectSaveFile));
    if(selector)
    {
      selector->setTitle("Save Sample To");
      selector->setDefaultSaveName(UTF8String(Steinberg::String().printf("sample.%s",
                                                                         fExportSampleMajorFormat == SampleFile::ESampleMajorFormat::kSampleFormatWAV ? "wav" : "aif")));
      if(selector->runModal())
      {
        if(selector->getNumSelectedFiles() > 0)
        {
          auto filename = selector->getSelectedFile(0);
          if(!fState->fSampleMgr->save(filename, *fExportSampleMajorFormat, *fExportSampleMinorFormat))
            DLOG_F(WARNING, "Could not save %s", filename);
          else
            DLOG_F(INFO, "Successfully saved %s", filename);
        }
      }
    }
    TextButtonView::onClick();
  }

protected:
  GUIVstParam<SampleFile::ESampleMajorFormat> fExportSampleMajorFormat{};
  GUIVstParam<SampleFile::ESampleMinorFormat> fExportSampleMinorFormat{};

public:
  // Creator
  using Creator = Views::CustomViewCreator<SampleSaverView, super_type>;
};

// the creator
SampleSaverView::Creator __gSampleSplitterSampleSaverCreator("SampleSplitter::SampleSaver", "SampleSplitter - SampleSaver");

}

