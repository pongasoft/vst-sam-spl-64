#include <fstream>
#include <algorithm>
#include "SampleData.h"
#include <sstream>
#include <vstgui4/vstgui/lib/cstring.h>
#include "SampleFile.h"
#include "SampleBuffers.hpp"
#include "Model.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleData::SampleData (copy)
//------------------------------------------------------------------------
SampleData::SampleData(SampleData const &iOther)
{
  fFilePath = iOther.fFilePath;
  fSampleStorage = iOther.fSampleStorage ? iOther.fSampleStorage->clone() : nullptr;
  fSource = iOther.fSource;
  fUpdateType = iOther.fUpdateType;
}

//------------------------------------------------------------------------
// SampleData::SampleData (move)
//------------------------------------------------------------------------
SampleData::SampleData(SampleData &&iOther) noexcept
{
  fFilePath = std::move(iOther.fFilePath);
  fSampleStorage = std::move(iOther.fSampleStorage);
  fUndoHistory = std::move(iOther.fUndoHistory);
  fSource = iOther.fSource;
  fUpdateType = iOther.fUpdateType;
}

//------------------------------------------------------------------------
// SampleData::init (from a file)
//------------------------------------------------------------------------
tresult SampleData::init(UTF8Path const &iFilePath)
{
  DLOG_F(INFO, "SampleData::init(%s) - from file", iFilePath.c_str());

  fFilePath = iFilePath;
  fSampleStorage = SampleFile::create(iFilePath);
  fSource = Source::kFile;
  fUpdateType = UpdateType ::kNone;

  if(!fSampleStorage)
  {
    LOG_F(WARNING, "Could not save the data in a temporary file");
  }

  return exists() ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::init (from sampling)
//------------------------------------------------------------------------
tresult SampleData::init(SampleBuffers32 const &iSampleBuffers,
                         UTF8Path const *iFilePath,
                         Source iSource,
                         UpdateType iUpdateType)
{
  DLOG_F(INFO, "SampleData::init() - from sample buffers");

  if(iFilePath)
    fFilePath = *iFilePath;
  else
  {
    std::ostringstream filePath;
    filePath << "samspl64://sampling@" << iSampleBuffers.getSampleRate() << "/sampling.wav";
    fFilePath = filePath.str();
  }
  fSampleStorage = SampleFile::create(fFilePath,
                                      iSampleBuffers,
                                      true,
                                      SampleFile::kSampleFormatWAV,
                                      SampleFile::kSampleFormatPCM24);
  fSource = iSource;
  fUpdateType = iUpdateType;

  if(!fSampleStorage)
  {
    LOG_F(WARNING, "Could not save the sampling data in a temporary file");
  }

  return exists() ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::init (from saved state)
//------------------------------------------------------------------------
tresult SampleData::init(std::string iFilename, IBStreamer &iStreamer)
{
  DLOG_F(INFO, "SampleData::init(%s) - from state", iFilename.c_str());

  fFilePath = std::move(iFilename);
  fSampleStorage = nullptr;

  uint64 size = 0;
  tresult res = IBStreamHelper::readInt64u(iStreamer, size);

  if(res == kResultOk)
  {
    auto pos = iStreamer.tell();

    fSampleStorage = SampleFile::create(iStreamer, fFilePath, size);

    if(!fSampleStorage)
    {
      iStreamer.seek(pos, kSeekSet);

      LOG_F(WARNING, "Could not save the data in a temporary file");
    }
  }

  return exists() ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::load
//------------------------------------------------------------------------
std::unique_ptr<SampleBuffers32> SampleData::load(SampleRate iSampleRate) const
{
  DLOG_F(INFO, "SampleData::load(%f)", iSampleRate);

  if(fSampleStorage)
  {
    return fSampleStorage->toBuffers(iSampleRate);
  }

  return nullptr;
}

//------------------------------------------------------------------------
// SampleData::load
//------------------------------------------------------------------------
std::unique_ptr<SampleBuffers32> SampleData::load() const
{
  if(fSampleStorage)
  {
    return fSampleStorage->toBuffers();
  }

  return nullptr;
}


//------------------------------------------------------------------------
// SampleData::operator=
//------------------------------------------------------------------------
SampleData &SampleData::operator=(SampleData &&other) noexcept
{
  fFilePath = std::move(other.fFilePath);
  fSampleStorage = std::move(other.fSampleStorage);
  fSource = other.fSource;
  fUpdateType = other.fUpdateType;
  fUndoHistory = std::move(other.fUndoHistory);
  return *this;
}

//------------------------------------------------------------------------
// SampleData::copyData
//------------------------------------------------------------------------
tresult SampleData::copyData(IBStreamer &oStreamer) const
{
  oStreamer.writeInt64u(getSize());

  if(fSampleStorage)
    return fSampleStorage->copyTo(oStreamer);

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::getSize
//------------------------------------------------------------------------
uint64 SampleData::getSize() const
{
  if(fSampleStorage)
    return fSampleStorage->getSize();

  return 0;
}

constexpr Sample32 NORMALIZE_3DB = static_cast<const Sample32>(0.707945784384138); // 10 ^ (-3/20)
constexpr Sample32 NORMALIZE_6DB = static_cast<const Sample32>(0.501187233627272); // 10 ^ (-6/20)

//------------------------------------------------------------------------
// SampleData::execute
//------------------------------------------------------------------------
tresult SampleData::execute(SampleData::Action const &iAction)
{
  auto buffers = load();

  if(buffers)
  {
    auto result = std::make_unique<Action>(iAction.fType);

    switch(iAction.fType)
    {
      case Action::Type::kCut:
        buffers = buffers->cut(static_cast<int32>(iAction.fSelectedSampleRange.fFrom),
                               static_cast<int32>(iAction.fSelectedSampleRange.fTo));
        break;

      case Action::Type::kCrop:
        buffers = buffers->crop(static_cast<int32>(iAction.fSelectedSampleRange.fFrom),
                                static_cast<int32>(iAction.fSelectedSampleRange.fTo));
        break;

      case Action::Type::kTrim:
        buffers = buffers->trim();
        break;

      case Action::Type::kNormalize0:
        buffers = buffers->normalize();
        break;

      case Action::Type::kNormalize3:
        buffers = buffers->normalize(NORMALIZE_3DB);
        break;

      case Action::Type::kNormalize6:
        buffers = buffers->normalize(NORMALIZE_6DB);
        break;

      case Action::Type::kResample:
        buffers = buffers->resample(iAction.fSampleRate);
        break;

      default:
        // nothing to do
        break;
    }

    return addExecutedAction(iAction, std::move(buffers));
  }

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::addExecutedAction
//------------------------------------------------------------------------
tresult SampleData::addExecutedAction(Action const &iAction, std::unique_ptr<SampleBuffers32> iSampleBuffers)
{
  if(!iSampleBuffers)
    return kResultFalse;

  SampleData sampleData;
  auto res = sampleData.init(*iSampleBuffers, &fFilePath, fSource, UpdateType::kAction);

  if(res == kResultOk)
  {
    res = addToUndoHistory(iAction, std::move(sampleData));
  }

  return res;
}

//------------------------------------------------------------------------
// SampleData::addToUndoHistory
//------------------------------------------------------------------------
tresult SampleData::addToUndoHistory(const SampleData::Action &iAction, SampleData &&iSampleData)
{
  auto previous = std::make_unique<SampleData>(std::move(*this));
  *this = std::move(iSampleData);
  fUndoHistory = std::make_unique<ExecutedAction>(iAction, std::move(previous));
  return kResultOk;
}

//------------------------------------------------------------------------
// SampleData::undo
//------------------------------------------------------------------------
std::unique_ptr<SampleData::Action> SampleData::undo()
{
  auto executedAction = std::move(fUndoHistory);

  if(executedAction)
  {
    auto res = std::make_unique<Action>(executedAction->fAction);
    *this = std::move(*executedAction->fSampleData);
    fUpdateType = UpdateType::kUndo;
    return res;
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------
// SampleData::getSampleInfo
//------------------------------------------------------------------------
tresult SampleData::getSampleInfo(SampleInfo &oSampleInfo) const
{
  return fSampleStorage ? fSampleStorage->getSampleInfo(oSampleInfo) : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::getSampleInfo
//------------------------------------------------------------------------
std::unique_ptr<SampleInfo> SampleData::getSampleInfo() const
{
  return fSampleStorage ? fSampleStorage->getSampleInfo() : nullptr;
}

//------------------------------------------------------------------------
// SampleData::save
//------------------------------------------------------------------------
std::unique_ptr<SampleData> SampleData::save(UTF8Path const &iFilePath,
                                             SampleStorage::ESampleMajorFormat iMajorFormat,
                                             SampleStorage::ESampleMinorFormat iMinorFormat) const
{
  auto buffers = load();

  if(!buffers)
    return nullptr;

  auto sampleFile = SampleFile::create(iFilePath,
                                       *buffers,
                                       false,
                                       iMajorFormat,
                                       iMinorFormat);

  if(!sampleFile)
    return nullptr;

  auto res = std::make_unique<SampleData>();

  res->fFilePath = iFilePath;
  res->fSampleStorage = std::move(sampleFile);
  res->fSource = fSource;
  res->fUpdateType = fUpdateType;

  return res;
}

//------------------------------------------------------------------------
// SampleDataSerializer::readFromStream
//------------------------------------------------------------------------
tresult SampleDataSerializer::readFromStream(IBStreamer &iStreamer, SampleData &oValue) const
{
  VSTGUI::UTF8String filename;
  fStringSerializer.readFromStream(iStreamer, filename);
  return oValue.init(filename.getString(), iStreamer);
}

//------------------------------------------------------------------------
// SampleDataSerializer::writeToStream
//------------------------------------------------------------------------
tresult SampleDataSerializer::writeToStream(const SampleData &iValue, IBStreamer &oStreamer) const
{
  fStringSerializer.writeToStream(iValue.getFilePath().utf8_str(), oStreamer);
  return iValue.copyData(oStreamer);
}

}
}
}