/*
 * Copyright (c) 2020 pongasoft
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

#ifndef SAMSPL64_LARGEFILEDIALOGCONTROLLER_H
#define SAMSPL64_LARGEFILEDIALOGCONTROLLER_H

#include <pongasoft/VST/GUI/Views/CustomController.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI;

/**
 * Handles Ok/Cancel action after trying to load a large file
 */
class LargeFileDialogController : public Views::StateAwareCustomController<SampleSplitterGUIState>
{
public:
  // Constructor
  explicit LargeFileDialogController(IController *iBaseController) : Views::StateAwareCustomController<SampleSplitterGUIState>(iBaseController) {}
  ~LargeFileDialogController() override = default;

  CView *verifyView(CView *view, const UIAttributes &attributes, const IUIDescription *description) override;
};

}

#endif //SAMSPL64_LARGEFILEDIALOGCONTROLLER_H