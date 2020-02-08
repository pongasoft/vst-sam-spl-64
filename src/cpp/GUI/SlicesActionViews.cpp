#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI::Views;

/**
 * Button to toggle looping all slices */
class ToggleSliceLoopAllView : public StateAwareView<Views::TextButtonView, SampleSplitterGUIState>
{
public:
  explicit ToggleSliceLoopAllView(const CRect &iSize) : StateAwareView(iSize) {}

  void onClick() override
  {
    auto &settings = fState->fSlicesSettings;
    if(settings.update(settings->toggleLoopAll()))
      settings.broadcast();
  }

public:
  using Creator = CustomViewCreator<ToggleSliceLoopAllView, Views::TextButtonView>;
};

ToggleSliceLoopAllView::Creator __gSampleSplitterToggleSliceLoopAllViewCreator("SampleSplitter::ToggleSliceLoopAll", "SampleSplitter - Toggle Slice Loop All");

/**
 * Button to reset the settings of all slices */
class ResetSliceSettingsView : public StateAwareView<Views::TextButtonView, SampleSplitterGUIState>
{
public:
  explicit ResetSliceSettingsView(const CRect &iSize) : StateAwareView(iSize) {}

  void onClick() override
  {
    auto &settings = fState->fSlicesSettings;
    if(settings.update(SlicesSettings{}))
      settings.broadcast();
  }

public:
  using Creator = CustomViewCreator<ResetSliceSettingsView, Views::TextButtonView>;
};

ResetSliceSettingsView::Creator __gSampleSplitterResetSliceSettingsViewCreator("SampleSplitter::ResetSliceSettings", "SampleSplitter - Reset Slice Settings");
}