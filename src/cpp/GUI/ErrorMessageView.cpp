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

#include "ErrorMessageView.h"

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// ErrorMessageView::registerParameters
//------------------------------------------------------------------------
void ErrorMessageView::registerParameters()
{
  DebugParamDisplayView::registerParameters();
  fErrorMessage = registerParam(fParams->fErrorMessage);
}

//------------------------------------------------------------------------
// ErrorMessageView::onParameterChange
//------------------------------------------------------------------------
void ErrorMessageView::onParameterChange(ParamID iParamID)
{
  if(*fErrorMessage != std::nullopt)
    DebugParamDisplayView::onParameterChange(iParamID);
  else
    ParamDisplayView::onParameterChange(iParamID);
}

//------------------------------------------------------------------------
// ErrorMessageView::onMouseDown
//------------------------------------------------------------------------
CMouseEventResult ErrorMessageView::onMouseDown(CPoint &where, const CButtonState &buttons)
{
  if(!(buttons & kLButton))
    return kMouseEventNotHandled;

  fState->clearError();
  return kMouseEventHandled;
}

ErrorMessageView::Creator __gSampleSplitterErrorMessageCreator("SampleSplitter::ErrorMessage", "SampleSplitter - Error Message");

}