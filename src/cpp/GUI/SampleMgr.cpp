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

#include "SampleMgr.h"
#include "SampleFile.h"

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// SampleMgr::initState
//------------------------------------------------------------------------
void SampleMgr::initState(VST::GUI::GUIState *iGUIState)
{
  DLOG_F(INFO, "SampleMgr::initState()");
  ParamAware::initState(iGUIState);
  StateAware<SampleSplitterGUIState>::initState(iGUIState);
}

//------------------------------------------------------------------------
// SampleMgr::registerParameters
//------------------------------------------------------------------------
void SampleMgr::registerParameters()
{
  DLOG_F(INFO, "SampleMgr::registerParameters()");
  registerCallback<SharedSampleBuffersVersion>(fParams->fRTNewSampleMessage,
                                               [this] (GUIJmbParam<SharedSampleBuffersVersion> &iParam) {
                                                 loadSampleFromSampling(*iParam);
                                               });

  registerCallback<SharedSampleBuffersMgr32 *>(fParams->fSharedSampleBuffersMgrPtr,
                                               [this] (GUIJmbParam<SharedSampleBuffersMgr32 *> &iParam) {
                                                 onMgrReceived(*iParam);
                                               });

  // we need to be notified when:
  // there is a new sample rate (GUI does not have access to it otherwise)
  registerCallback<SampleRate>(fState->fSampleRate, [this](GUIJmbParam<SampleRate> &iParam) {
    DLOG_F(INFO, "Detected sample rate change... %f", iParam.getValue());
    onSampleRateChanged(iParam.getValue());
  });

  fOffsetPercent = registerParam(fParams->fWEOffsetPercent, false);
  fZoomPercent = registerParam(fParams->fWEZoomPercent, false);
  fNumSlices = registerParam(fParams->fNumSlices, false);
  fGUINewSampleMessage = registerParam(fParams->fGUINewSampleMessage, false);
}

//------------------------------------------------------------------------
// SampleMgr::loadSampleFromUser (user provided sample)
//------------------------------------------------------------------------
tresult SampleMgr::loadSampleFromUser(UTF8Path const &iFilePath)
{
  DLOG_F(INFO, "SampleMgr::loadSampleFromUser");

  auto action = SampleAction{SampleAction::Type::kLoad};
  action.fFilePath = iFilePath;

  if(executeAction(action))
  {
    resetSettings();
    return kResultOk;
  }

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleMgr::loadSampleFromSampling (from RT sampling)
//------------------------------------------------------------------------
tresult SampleMgr::loadSampleFromSampling(SharedSampleBuffersVersion iVersion)
{
  DLOG_F(INFO, "SampleMgr::loadSampleFromSampling");

  auto action = SampleAction{SampleAction::Type::kSample};
  action.fRTVersion = iVersion;

  if(executeAction(action))
  {
    resetSettings();
    return kResultOk;
  }

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleMgr::loadSampleFromState
//------------------------------------------------------------------------
tresult SampleMgr::loadSampleFromState()
{
  DLOG_F(INFO, "SampleMgr::loadSampleFromState");

  auto const &sampleFile = *fState->fSampleFile;

  if(!sampleFile.empty())
  {
    auto [buffers, originalSampleRate] = sampleFile.load(*fState->fSampleRate);

    if(buffers)
    {
      auto version = getSharedMgr()->uiSetObject(buffers);

      // we tell RT
      fGUINewSampleMessage.broadcast(version);

      // we set the current sample for views to use
      fState->fCurrentSample.setValue(CurrentSample(buffers,
                                                    originalSampleRate,
                                                    CurrentSample::Source::kFile,
                                                    CurrentSample::UpdateType::kNone));

      // notifying RT of slices settings right after loading
      fState->fSlicesSettings.broadcast();

      return kResultOk;
    }

  }

  return kResultFalse;
}



//------------------------------------------------------------------------
// SampleMgr::resetSettings
//------------------------------------------------------------------------
void SampleMgr::resetSettings()
{
  // reset number of slices
  fState->getGUIVstParameter(fParams->fNumSlices)->resetToDefault();

  // reset slice settings
  fState->fSlicesSettings.resetToDefault();
  fState->fSlicesSettings.broadcast();

  fState->fWESelectedSampleRange.resetToDefault();
}

//------------------------------------------------------------------------
// SampleMgr::getSharedMgr
//------------------------------------------------------------------------
SharedSampleBuffersMgr32 *SampleMgr::getSharedMgr() const
{
  auto sharedMgr = *fState->fSharedSampleBuffersMgrPtr;

  if(sharedMgr)
    return sharedMgr;

  // case when we have not received the manager yet
  if(!fGUIOnlyMgr)
  {
    DLOG_F(INFO, "SampleMgr::getSharedMgr - no shared mgr from RT => creating UI only one");
    fGUIOnlyMgr = std::make_unique<SharedSampleBuffersMgr32>();
  }

  return fGUIOnlyMgr.get();
}

//------------------------------------------------------------------------
// SampleMgr::onMgrReceived
//------------------------------------------------------------------------
tresult SampleMgr::onMgrReceived(SharedSampleBuffersMgr32 *iMgr)
{
  DLOG_F(INFO, "SampleMgr::onMgrReceived(%p)", iMgr);

  if(fGUIOnlyMgr)
  {
    auto uiBuffers = fGUIOnlyMgr->uiGetObject();

    if(uiBuffers)
    {
      // there was a buffer that we need to transfer to the real time
      auto version = iMgr->uiSetObject(uiBuffers);

      // we tell RT
      fGUINewSampleMessage.broadcast(version);
    }

    DLOG_F(INFO, "SampleMgr::onMgrReceived - discarding UI only mgr");

    fGUIOnlyMgr = nullptr;
  }

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleMgr::onSampleRateChanged
//------------------------------------------------------------------------
tresult SampleMgr::onSampleRateChanged(SampleRate iSampleRate)
{
  DLOG_F(INFO, "SampleMgr::onSampleRateChanged(%f)", iSampleRate);

  auto currentSample = fState->fCurrentSample;

  if(currentSample->hasSamples() && currentSample->getSampleRate() != iSampleRate)
  {
    auto [buffers, originalSampleRate] = fState->fSampleFile->load(*fState->fSampleRate);
    if(buffers)
    {
      currentSample.setValue({ buffers, originalSampleRate, currentSample->getSource(), currentSample->getUpdateType() });
      auto version = getSharedMgr()->uiSetObject(currentSample->getSharedBuffers());
      fGUINewSampleMessage.broadcast(version);
    }
  }

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleMgr::executeAction
//------------------------------------------------------------------------
bool SampleMgr::executeAction(SampleAction const &iAction)
{
  auto action = iAction;

  // we capture the current state so that we can restore on undo
  action.fNumSlices = *fNumSlices;
  action.fSelectedSampleRange = *fState->fWESelectedSampleRange;
  action.fOffsetPercent = *fOffsetPercent;
  action.fZoomPercent = *fZoomPercent;

  return doExecuteAction(action, true);
}

//------------------------------------------------------------------------
// SampleMgr::doExecuteAction
//------------------------------------------------------------------------
bool SampleMgr::doExecuteAction(SampleAction const &iAction, bool clearRedoHistory)
{
  CurrentSample currentSample{};
  SampleFile currentFile{};
  bool notifyRT{true};

  switch(iAction.fType)
  {
    case SampleAction::Type::kLoad:
    {
      auto sampleFile = SampleFile::create(iAction.fFilePath);
      if(sampleFile)
      {
        auto [buffers, originalSampleRate] = sampleFile->load(*fState->fSampleRate);
        if(buffers)
        {
          currentSample = { buffers, originalSampleRate, CurrentSample::Source::kFile, CurrentSample::UpdateType::kNone };
          currentFile = *sampleFile;
        }
      }
    }
      break;

    case SampleAction::Type::kSample:
    {
      notifyRT = false;

      auto buffers = getSharedMgr()->uiAdjustObjectFromRT(iAction.fRTVersion);

      if(buffers)
      {
        std::ostringstream filePath;
        filePath << "samspl64://sampling@" << buffers->getSampleRate() << "/sam_spl64_sampling.wav";

        auto sampleFile = SampleFile::create(filePath.str(), *buffers);

        if(sampleFile)
        {
          currentSample = { buffers, buffers->getSampleRate(), CurrentSample::Source::kSampling, CurrentSample::UpdateType::kNone };
          currentFile = *sampleFile;
        }
      }
    }
      break;

    default:
    {
      currentSample = executeBufferAction(iAction);
      if(currentSample.hasSamples())
      {
        auto sampleFile = SampleFile::create(fState->fSampleFile->empty() ? "" : fState->fSampleFile->getOriginalFilePath(),
                                             *currentSample.getBuffers());
        if(sampleFile)
          currentFile = *sampleFile;
      }
    }
      break;
  }

  if(currentSample.hasSamples() && !currentFile.empty())
  {
    if(!fState->fCurrentSample->empty())
    {
      fState->fUndoHistory.updateIf([this, &iAction, clearRedoHistory] (UndoHistory *iUndoHistory) {
        iUndoHistory->addEntry(iAction, *fState->fCurrentSample, *fState->fSampleFile);
        if(clearRedoHistory)
          iUndoHistory->clearRedoHistory();
        return true;
      });
    }

    auto version = getSharedMgr()->uiSetObject(currentSample.getSharedBuffers());

    if(notifyRT)
      fGUINewSampleMessage.broadcast(version);

    fState->fCurrentSample.setValue(currentSample);
    fState->fSampleFile.setValue(currentFile);

    return true;
  }

  return false;
}

constexpr Sample32 NORMALIZE_3DB = static_cast<const Sample32>(0.707945784384138); // 10 ^ (-3/20)
constexpr Sample32 NORMALIZE_6DB = static_cast<const Sample32>(0.501187233627272); // 10 ^ (-6/20)

//------------------------------------------------------------------------
// SampleDataMgr::executeBufferAction
//------------------------------------------------------------------------
CurrentSample SampleMgr::executeBufferAction(SampleAction const &iAction)
{
  auto buffers = iAction.fType == SampleAction::Type::kCut || iAction.fType == SampleAction::Type::kCrop ?
                 fState->fCurrentSample->getSharedBuffers() : // for cut and crop we work with the resampled buffer
                 fState->fSampleFile->loadOriginal(); // for the other actions we work with the original buffer

  // no buffers
  if(!buffers)
    return {};

  switch(iAction.fType)
  {
    case SampleAction::Type::kCut:
      buffers = buffers->cut(static_cast<int32>(iAction.fSelectedSampleRange.fFrom),
                             static_cast<int32>(iAction.fSelectedSampleRange.fTo));
      break;

    case SampleAction::Type::kCrop:
      buffers = buffers->crop(static_cast<int32>(iAction.fSelectedSampleRange.fFrom),
                              static_cast<int32>(iAction.fSelectedSampleRange.fTo));
      break;

    case SampleAction::Type::kTrim:
      buffers = buffers->trim();
      break;

    case SampleAction::Type::kNormalize0:
      buffers = buffers->normalize();
      break;

    case SampleAction::Type::kNormalize3:
      buffers = buffers->normalize(NORMALIZE_3DB);
      break;

    case SampleAction::Type::kNormalize6:
      buffers = buffers->normalize(NORMALIZE_6DB);
      break;

    case SampleAction::Type::kResample:
      buffers = buffers->resample(*fState->fSampleRate);
      break;

    default:
      DLOG_F(ERROR, "should not be here");
      break;
  }

  // action not completed
  if(!buffers)
    return {};

  // we make sure that the buffers match the sample rate
  if(buffers->getSampleRate() != *fState->fSampleRate)
    buffers = buffers->resample(*fState->fSampleRate);

  return CurrentSample(buffers, buffers->getSampleRate(), fState->fCurrentSample->getSource(), CurrentSample::UpdateType::kAction);
}

//------------------------------------------------------------------------
// SampleMgr::undoLastAction
//------------------------------------------------------------------------
bool SampleMgr::undoLastAction()
{
  return fState->fUndoHistory.updateIf([this] (UndoHistory *iUndoHistory) {

    if(!iUndoHistory->hasUndoHistory())
      return false;

    auto lastExecutedAction = iUndoHistory->undo();

    auto [buffers, originalSampleRate] = lastExecutedAction.fFile.load(*fState->fSampleRate);

    if(buffers)
    {
      auto version = getSharedMgr()->uiSetObject(buffers);

      // we tell RT
      fGUINewSampleMessage.broadcast(version);

      // we set the current sample for views to use
      fState->fCurrentSample.setValue(CurrentSample(buffers,
                                                    originalSampleRate,
                                                    lastExecutedAction.fSource,
                                                    lastExecutedAction.fUpdateType));

      // we set the sample file (for the plugin state)
      fState->fSampleFile.setValue(lastExecutedAction.fFile);

      if(fState->fWESelectedSampleRange.update(lastExecutedAction.fAction.fSelectedSampleRange))
        fState->fWESelectedSampleRange.broadcast();
      fNumSlices.setValue(lastExecutedAction.fAction.fNumSlices);
      fOffsetPercent.setValue(lastExecutedAction.fAction.fOffsetPercent);
      fZoomPercent.setValue(lastExecutedAction.fAction.fZoomPercent);

      return true;
    }

    return false;
  });
}

//------------------------------------------------------------------------
// SampleMgr::redoLastUndo
//------------------------------------------------------------------------
bool SampleMgr::redoLastUndo()
{
  return fState->fUndoHistory.updateIf([this] (UndoHistory *iUndoHistory) {
    if(!iUndoHistory->hasRedoHistory())
      return false;

    doExecuteAction(iUndoHistory->redo(), false);

    return true;
  });
}

//------------------------------------------------------------------------
// SampleMgr::save
//------------------------------------------------------------------------
bool SampleMgr::save(UTF8Path const &iFilePath,
                     SampleFile::ESampleMajorFormat iMajorFormat,
                     SampleFile::ESampleMinorFormat iMinorFormat) const
{
  if(fState->fCurrentSample->hasSamples())
    return SampleFile::save(iFilePath, *fState->fCurrentSample->getBuffers(), iMajorFormat, iMinorFormat) == kResultOk;
  else
    return false;
}

}