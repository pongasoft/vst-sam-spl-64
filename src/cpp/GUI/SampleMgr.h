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

#ifndef VST_SAM_SPL_64_SAMPLEMGR_H
#define VST_SAM_SPL_64_SAMPLEMGR_H


#include <pongasoft/VST/GUI/Params/ParamAware.hpp>
#include <pongasoft/VST/GUI/Views/StateAware.h>

#include "../SharedSampleBuffersMgr.h"
#include "../Plugin.h"

#include "UndoHistory.h"
#include "SampleFile.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI::Views;
using namespace pongasoft::VST::GUI::Params;

/**
 * Sample manager */
class SampleMgr : public ParamAware, public StateAware<SampleSplitterGUIState>
{
private:
public:
  void initState(VST::GUI::GUIState *iGUIState) override;

  void registerParameters() override;

  tresult loadSampleFromUser(UTF8Path const &iFilePath);

  tresult loadSampleFromState();

  /**
   * Save this sample to another file
   *
   * @param iFilePath
   * @return `true` if successful, `false` otherwise
   */
  bool save(UTF8Path const &iFilePath,
            SampleFile::ESampleMajorFormat iMajorFormat,
            SampleFile::ESampleMinorFormat iMinorFormat) const;

  /**
   * Executes the provided action on the current sample. Add current sample to undo history:
   *
   * - iAction + fCurrent stored as last UndoEntry
   * - iAction applied on fCurrent -> new fCurrent
   *
   * @return `true` if successful, `false` otherwise
   */
  bool executeAction(SampleAction const &iAction, bool clearRedoHistory = true);

  /**
   * Reverts the last action and add it to the redo history
   *
   * @return `true` if successful, `false` otherwise (or if nothing to undo)
   */
  bool undoLastAction();

  /**
   * Redo the last undone action.
   *
   * @return `true` if successful, `false` otherwise (or if nothing to redo)
   */
  bool redoLastUndo();

protected:
  SharedSampleBuffersMgr32 *getSharedMgr() const;

  tresult loadSampleFromSampling(SharedSampleBuffersVersion iVersion);

  tresult onMgrReceived(SharedSampleBuffersMgr32 *iMgr);

  tresult onSampleRateChanged(SampleRate iSampleRate);

  CurrentSample executeBufferAction(SampleAction const &iAction);

  void resetSettings();

private:
  GUIJmbParam<SharedSampleBuffersVersion> fGUINewSampleMessage;

  // this is for the case when we have not received the mgr from the RT which could be due to
  // 1. using only the editor (so RT will never send it)
  // 2. order in which events happen (since messaging is asynchronous, I am not sure there is a guarantee to when
  //    the message is actually delivered!
  mutable std::unique_ptr<SharedSampleBuffersMgr32> fGUIOnlyMgr{};
};

}

#endif //VST_SAM_SPL_64_SAMPLEMGR_H