#include <pongasoft/VST/GUI/Views/CustomView.h>
#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include <vstgui4/vstgui/lib/cfileselector.h>
#include "../Plugin.h"

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

    registerCallback<SampleData>(fState->fSampleData, [this] (GUIJmbParam<SampleData> &iParam) {
      setMouseEnabled(iParam->exists());
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
    CNewFileSelector *selector = CNewFileSelector::create(getFrame(), CNewFileSelector::kSelectSaveFile);
    if(selector)
    {
      selector->setTitle("Save Sample To");
      selector->setDefaultSaveName(UTF8String(String().printf("sample.%s",
                                                              fExportSampleMajorFormat == SampleStorage::kSampleFormatWAV ? "wav" : "aif")));
      selector->run(this);
      selector->forget();
    }
    TextButtonView::onClick();
  }

  //------------------------------------------------------------------------
  // notify
  //------------------------------------------------------------------------
  CMessageResult notify(CBaseObject *sender, IdStringPtr message) override
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
          if(!fState->fSampleData->save(filename, fExportSampleMajorFormat, fExportSampleMinorFormat))
            DLOG_F(WARNING, "Could not save %s", filename);
          else
            DLOG_F(INFO, "Successfully saved %s", filename);
        }

        return kMessageNotified;
      }
    }
    return TextButtonView::notify(sender, message);
  }

protected:
  GUIVstParam<SampleStorage::ESampleMajorFormat> fExportSampleMajorFormat{};
  GUIVstParam<SampleStorage::ESampleMinorFormat> fExportSampleMinorFormat{};

public:
  // Creator
  using Creator = Views::CustomViewCreator<SampleSaverView, super_type>;
};

// the creator
SampleSaverView::Creator __gSampleSplitterSampleSaverCreator("SampleSplitter::SampleSaver", "SampleSplitter - SampleSaver");

}

