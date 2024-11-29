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
      auto const extension = fExportSampleMajorFormat == SampleFile::ESampleMajorFormat::kSampleFormatWAV ?
                             CFileExtension("WAVE", "wav", "audio/x-wav") :
                             CFileExtension("AIFF", "aif", "audio/x-aiff");
      selector->setDefaultExtension(extension);
      selector->setTitle("Save Sample To");
      selector->setDefaultSaveName(UTF8String(Steinberg::String().printf("sample.%s", extension.getExtension().data())));
      if(selector->runModal())
      {
        if(selector->getNumSelectedFiles() > 0)
        {
          auto filename = std::string(selector->getSelectedFile(0));
          auto expectedExtension = std::string(".") + extension.getExtension().getString();
          if(filename.size() < expectedExtension.size() ||
             filename.substr(filename.size() - expectedExtension.size()) != expectedExtension)
          {
            filename += expectedExtension;
          }
          if(!fState->fSampleMgr->save(filename, *fExportSampleMajorFormat, *fExportSampleMinorFormat))
            DLOG_F(WARNING, "Could not save %s", filename.data());
          else
            DLOG_F(INFO, "Successfully saved %s", filename.data());
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

