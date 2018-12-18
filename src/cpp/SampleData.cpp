#include <fstream>
#include <algorithm>
#include "SampleData.h"
#include <sstream>
#include <vstgui4/vstgui/lib/cstring.h>
#include "SampleFile.h"
#include "SampleBuffers.hpp"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

constexpr int64 BUFFER_SIZE = 1024;

//------------------------------------------------------------------------
// SampleData::SampleData
//------------------------------------------------------------------------
SampleData::SampleData(SampleData const &iOther)
{
  fFilePath = iOther.fFilePath;
  fSampleStorage = iOther.fSampleStorage ? iOther.fSampleStorage->clone() : nullptr;
}

//------------------------------------------------------------------------
// SampleData::init (from a file)
//------------------------------------------------------------------------
tresult SampleData::init(std::string const &iFilePath)
{
  DLOG_F(INFO, "SampleData::init(%s) - from file", iFilePath.c_str());

  fFilePath = iFilePath;
  fSampleStorage = SampleFile::create(iFilePath);

  if(!fSampleStorage)
  {
    LOG_F(WARNING, "Could not save the data in a temporary file");
  }

  return exists() ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::init (from sampling)
//------------------------------------------------------------------------
tresult SampleData::init(SampleBuffers32 const &iSampleBuffers)
{
  DLOG_F(INFO, "SampleData::init() - from sample buffers");

  std::ostringstream filePath;
  filePath << "samspl64://sampling@" << iSampleBuffers.getSampleRate() << "/sampling.wav";
  fFilePath = filePath.str();
  fSampleStorage = SampleFile::create(fFilePath, iSampleBuffers);

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

//------------------------------------------------------------------------
// SampleData::trim
//------------------------------------------------------------------------
tresult SampleData::trim(bool iAddToUndoHistory)
{
  DLOG_F(INFO, "SampleData::trim");

  auto buffers = load();

  if(buffers)
  {
    return replace(buffers->trim(), iAddToUndoHistory);
  }

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::cut
//------------------------------------------------------------------------
tresult SampleData::cut(int32 iFromIndex, int32 iToIndex, bool iAddToUndoHistory)
{
  DLOG_F(INFO, "SampleData::cut(%d,%d)", iFromIndex, iToIndex);

  if(iFromIndex == iToIndex)
    return kResultFalse;

  auto buffers = load();

  if(buffers)
  {
    return replace(buffers->cut(iFromIndex, iToIndex), iAddToUndoHistory);
  }

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::crop
//------------------------------------------------------------------------
tresult SampleData::crop(int32 iFromIndex, int32 iToIndex, bool iAddToUndoHistory)
{
  DLOG_F(INFO, "SampleData::crop(%d,%d)", iFromIndex, iToIndex);

  if(iFromIndex == iToIndex)
    return kResultFalse;

  auto buffers = load();

  if(buffers)
  {
    return replace(buffers->crop(iFromIndex, iToIndex), iAddToUndoHistory);
  }

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::normalize
//------------------------------------------------------------------------
tresult SampleData::normalize(bool iAddToUndoHistory)
{
  DLOG_F(INFO, "SampleData::normalize");

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::replace
//------------------------------------------------------------------------
void SampleData::replace(SampleData &&iNext, bool iAddToUndoHistory)
{
  if(iAddToUndoHistory)
  {
    auto previous = std::make_unique<SampleData>();
    previous->fFilePath = std::move(fFilePath);
    previous->fSampleStorage = std::move(fSampleStorage);
    previous->fUndoHistory = std::move(fUndoHistory);

    fFilePath = std::move(iNext.fFilePath);
    fSampleStorage = std::move(iNext.fSampleStorage);
    fUndoHistory = std::move(previous);
  }
  else
  {
    fFilePath = std::move(iNext.fFilePath);
    fSampleStorage = std::move(iNext.fSampleStorage);
  }
}

//------------------------------------------------------------------------
// SampleData::replace
//------------------------------------------------------------------------
tresult SampleData::replace(std::unique_ptr<SampleBuffers32> iSampleBuffers,
                            bool iAddToUndoHistory)
{
  if(!iSampleBuffers)
    return kResultFalse;

  SampleData sampleData;
  auto res = sampleData.init(*iSampleBuffers);

  if(res == kResultOk)
  {
    replace(std::move(sampleData), iAddToUndoHistory);
  }

  return res;
}

//------------------------------------------------------------------------
// SampleData::undo
//------------------------------------------------------------------------
tresult SampleData::undo()
{
  if(!hasUndoHistory())
    return kResultFalse;

  auto previous = std::move(fUndoHistory);

  fFilePath = std::move(previous->fFilePath);
  fSampleStorage = std::move(previous->fSampleStorage);
  fUndoHistory = std::move(previous->fUndoHistory);

  return kResultOk;
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
  fStringSerializer.writeToStream(VSTGUI::UTF8String(iValue.getFilePath()), oStreamer);
  return iValue.copyData(oStreamer);
}

}
}
}