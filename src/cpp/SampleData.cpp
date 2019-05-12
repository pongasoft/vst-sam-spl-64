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
tresult SampleData::init(UTF8Path const &iFilePath,
                         std::shared_ptr<SampleStorage> iSamplingStorage)
{
  DLOG_F(INFO, "SampleData::init(%s) - from sampling", iFilePath.c_str());

  fFilePath = iFilePath;
  fSampleStorage = std::move(iSamplingStorage);
  fSource = Source::kSampling;
  fUpdateType = UpdateType ::kNone;

  return exists() ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::init (from buffers)
//------------------------------------------------------------------------
tresult SampleData::init(SampleBuffers32 const &iSampleBuffers,
                         UTF8Path const &iFilePath,
                         Source iSource,
                         UpdateType iUpdateType)
{
  DLOG_F(INFO, "SampleData::init() - from sample buffers");

  fFilePath = iFilePath;

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