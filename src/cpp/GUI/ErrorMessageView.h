/*
 * Copyright (c) 2023 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#ifndef VST_SAM_SPL_64_ERROR_MESSAGE_VIEW_H
#define VST_SAM_SPL_64_ERROR_MESSAGE_VIEW_H

#include <pongasoft/VST/GUI/Views/DebugParamDisplayView.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI::Views;

class ErrorMessageView : public StateAwareView<DebugParamDisplayView, SampleSplitterGUIState>
{
public:
  explicit ErrorMessageView(CRect const &iSize) : StateAwareView(iSize) {}

  void registerParameters() override;

  void onParameterChange(ParamID iParamID) override;

  CMouseEventResult onMouseDown(CPoint &where, const CButtonState &buttons) override;

protected:
  GUIJmbParam<error_message_t> fErrorMessage{};

public:
  CLASS_METHODS_NOCOPY(ErrorMessageView, DebugParamDisplayView)

  // Creator
  class Creator : public CustomViewCreator<ErrorMessageView, DebugParamDisplayView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
    }
  };
};

}

#endif //VST_SAM_SPL_64_ERROR_MESSAGE_VIEW_H