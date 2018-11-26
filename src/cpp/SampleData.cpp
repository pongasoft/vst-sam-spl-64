#include <fstream>
#include <algorithm>
#include "SampleData.h"
#include "SampleBuffers.hpp"
#include <sstream>
#include <vstgui4/vstgui/lib/cstring.h>
#include "mackron/dr_libs/dr_wav.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

constexpr int64 BUFFER_SIZE = 1024;

//------------------------------------------------------------------------
// SampleData::SampleData
//------------------------------------------------------------------------
SampleData::SampleData(SampleData const &iOther)
{
  fUseFilesystem = iOther.fUseFilesystem;
  fFilePath = iOther.fFilePath;
  fTemporaryFileSample = iOther.fTemporaryFileSample ? TemporarySampleFile::create(iOther.fTemporaryFileSample->getFilePath()) : nullptr;
  fSampleMemory = iOther.fSampleMemory ? std::make_unique<SampleMemory>(*iOther.fSampleMemory) : nullptr;
}

//------------------------------------------------------------------------
// SampleData::init (from a file)
//------------------------------------------------------------------------
tresult SampleData::init(std::string const &iFilePath)
{
  DLOG_F(INFO, "SampleData::init(%s) - from file", iFilePath.c_str());

  fFilePath = iFilePath;
  fTemporaryFileSample = nullptr;
  fSampleMemory = nullptr;

  if(fUseFilesystem)
    fTemporaryFileSample = TemporarySampleFile::create(iFilePath);

  if(!fTemporaryFileSample)
  {
    DLOG_F(WARNING, "Could not save the data in a temporary file, using memory instead");
    fSampleMemory = SampleMemory::create(iFilePath);
  }

  return exists() ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::init (from sampling)
//------------------------------------------------------------------------
tresult SampleData::init(SampleBuffers32 const &iSampleBuffers)
{
  DLOG_F(INFO, "SampleData::init() - from sample buffers (NOT IMPLEMENTED YET!)");

  // TODO implement
  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::init (from saved state)
//------------------------------------------------------------------------
tresult SampleData::init(std::string iFilename, IBStreamer &iStreamer)
{
  DLOG_F(INFO, "SampleData::init(%s) - from state", iFilename.c_str());

  fFilePath = std::move(iFilename);
  fTemporaryFileSample = nullptr;
  fSampleMemory = nullptr;

  uint64 size = 0;
  tresult res = IBStreamHelper::readInt64u(iStreamer, size);

  if(res == kResultOk)
  {
    auto pos = iStreamer.tell();

    if(fUseFilesystem)
      fTemporaryFileSample = TemporarySampleFile::create(iStreamer, fFilePath, size);

    if(!fTemporaryFileSample)
    {
      iStreamer.seek(pos, kSeekSet);

      DLOG_F(WARNING, "Could not save the data in a temporary file, using memory instead");
      fSampleMemory = SampleMemory::create(iStreamer, size);
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

  if(fTemporaryFileSample)
  {
    std::unique_ptr<SampleBuffers32> res = nullptr;

    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalSampleCount;

    DLOG_F(INFO, "SampleData::load ... Loading from file %s", fTemporaryFileSample->getFilePath().c_str());

    float *pSampleData = drwav_open_and_read_file_f32(fTemporaryFileSample->getFilePath().c_str(),
                                                      &channels,
                                                      &sampleRate,
                                                      &totalSampleCount);
    if(pSampleData == nullptr)
    {
      DLOG_F(ERROR, "error opening file %s", fTemporaryFileSample->getFilePath().c_str());
    }
    else
    {
      DLOG_F(INFO, "read %d/%d/%llu", channels, sampleRate, totalSampleCount);
      res = SampleBuffers32::fromInterleaved(sampleRate,
                                             channels,
                                             static_cast<int32>(totalSampleCount),
                                             pSampleData);
    }

    if(sampleRate != iSampleRate)
    {
      // TODO handle resampling

    }

    drwav_free(pSampleData);

    return std::move(res);
  }
  else
  {
    DLOG_F(INFO, "SampleData::load ... Loading from memory: NOT IMPLEMENTED YET");
    return nullptr;
  }
}

//------------------------------------------------------------------------
// SampleData::operator=
//------------------------------------------------------------------------
SampleData &SampleData::operator=(SampleData &&other) noexcept
{
  fUseFilesystem = other.fUseFilesystem;
  fFilePath = std::move(other.fFilePath);
  fTemporaryFileSample = std::move(other.fTemporaryFileSample);
  fSampleMemory = std::move(other.fSampleMemory);
  return *this;
}

//------------------------------------------------------------------------
// SampleData::copyData
//------------------------------------------------------------------------
tresult SampleData::copyData(IBStreamer &oStreamer) const
{
  oStreamer.writeInt64u(getSize());

  if(fTemporaryFileSample)
    return fTemporaryFileSample->copyTo(oStreamer);

  if(fSampleMemory)
    return fSampleMemory->copyTo(oStreamer);

  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::getSize
//------------------------------------------------------------------------
uint64 SampleData::getSize() const
{
  if(fTemporaryFileSample)
    return fTemporaryFileSample->getFileSize();

  if(fSampleMemory)
    return fSampleMemory->getSize();

  return 0;
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