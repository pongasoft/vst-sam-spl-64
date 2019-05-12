#include "SampleDataMgr.h"
#include "SampleFile.h"
#include "SampleBuffers.hpp"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleDataMgr::SampleDataMgr
//------------------------------------------------------------------------
SampleDataMgr::SampleDataMgr(SampleDataMgr const &iOther) : fCurrent{iOther.fCurrent}
{
}

//------------------------------------------------------------------------
// SampleDataMgr::executeAction
//------------------------------------------------------------------------
bool SampleDataMgr::executeAction(const SampleDataAction &iAction,
                                  bool clearRedoHistory)
{
  SampleData sampleData{};

  switch(iAction.fType)
  {
    case SampleDataAction::Type::kLoad:
      if(sampleData.init(iAction.fFilePath) != kResultOk)
        return false;
      break;

    case SampleDataAction::Type::kSample:
      if(sampleData.init(iAction.fFilePath, iAction.fSamplingStorage) != kResultOk)
        return false;
      break;

    default:
      sampleData = executeBufferAction(iAction);
      break;
  }

  if(sampleData.exists())
  {
    if(fCurrent->exists())
    {
      UndoEntry undoEntry{iAction, fCurrent};
      fUndoHistory.push_front(undoEntry);
      if(clearRedoHistory)
        fRedoHistory.clear();
    }

    fCurrent.setValue(std::move(sampleData));

    return true;
  }

  return false;
}

constexpr Sample32 NORMALIZE_3DB = static_cast<const Sample32>(0.707945784384138); // 10 ^ (-3/20)
constexpr Sample32 NORMALIZE_6DB = static_cast<const Sample32>(0.501187233627272); // 10 ^ (-6/20)

//------------------------------------------------------------------------
// SampleDataMgr::executeAction
//------------------------------------------------------------------------
SampleData SampleDataMgr::executeBufferAction(SampleDataAction const &iAction)
{
  SampleData sampleData{};

  auto buffers = fCurrent->load();

  // no buffers
  if(!buffers)
    return sampleData;

  switch(iAction.fType)
  {
    case SampleDataAction::Type::kCut:
      buffers = buffers->cut(static_cast<int32>(iAction.fSelectedSampleRange.fFrom),
                             static_cast<int32>(iAction.fSelectedSampleRange.fTo));
      break;

    case SampleDataAction::Type::kCrop:
      buffers = buffers->crop(static_cast<int32>(iAction.fSelectedSampleRange.fFrom),
                              static_cast<int32>(iAction.fSelectedSampleRange.fTo));
      break;

    case SampleDataAction::Type::kTrim:
      buffers = buffers->trim();
      break;

    case SampleDataAction::Type::kNormalize0:
      buffers = buffers->normalize();
      break;

    case SampleDataAction::Type::kNormalize3:
      buffers = buffers->normalize(NORMALIZE_3DB);
      break;

    case SampleDataAction::Type::kNormalize6:
      buffers = buffers->normalize(NORMALIZE_6DB);
      break;

    case SampleDataAction::Type::kResample:
      buffers = buffers->resample(iAction.fSampleRate);
      break;

    default:
      DLOG_F(ERROR, "should not be here");
      break;
  }

  // action not completed
  if(!buffers)
    return sampleData;

  sampleData.init(*buffers,
                  fCurrent->getFilePath(),
                  fCurrent->getSource(),
                  SampleData::UpdateType::kAction);

  return sampleData;
}

//------------------------------------------------------------------------
// SampleDataMgr::undoLastAction
//------------------------------------------------------------------------
bool SampleDataMgr::undoLastAction()
{
  if(fUndoHistory.empty())
    return false;

  auto lastExecutedAction = fUndoHistory.front();
  fUndoHistory.pop_front();
  fRedoHistory.push_front(lastExecutedAction.fAction);
  // swap current with new one
  fCurrent.setValue(std::move(lastExecutedAction.fSampleData));
  return true;
}

//------------------------------------------------------------------------
// SampleDataMgr::redoLastUndo
//------------------------------------------------------------------------
bool SampleDataMgr::redoLastUndo()
{
  if(fRedoHistory.empty())
    return false;

  auto lastAction = fRedoHistory.front();
  fRedoHistory.pop_front();
  return executeAction(lastAction, false);
}

//------------------------------------------------------------------------
// SampleDataMgr::load
//------------------------------------------------------------------------
bool SampleDataMgr::load(SampleBuffers32 const &iSampleBuffers)
{
  std::ostringstream filePath;
  filePath << "samspl64://sampling@" << iSampleBuffers.getSampleRate() << "/sampling.wav";

  auto action = SampleDataAction{SampleDataAction::Type::kSample};
  action.fSamplingStorage = SampleFile::create(filePath.str(),
                                               iSampleBuffers,
                                               true,
                                               SampleFile::kSampleFormatWAV,
                                               SampleFile::kSampleFormatPCM24);
  action.fFilePath = filePath.str();

  return executeAction(action);
}

}
}
}