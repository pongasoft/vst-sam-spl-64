#include <algorithm>
#include <vstgui4/vstgui/lib/cstring.h>

#include "SampleData.h"
#include "SampleFile.h"
#include "SampleBuffers.hpp"

namespace pongasoft::VST::SampleSplitter {

// keeps the actual buffer for 10s
constexpr uint32 cBuffersCacheTTL = 10000;

//------------------------------------------------------------------------
// SampleData::init (from a file)
//------------------------------------------------------------------------
tresult SampleData::init(UTF8Path const &iFilePath)
{
  DLOG_F(INFO, "SampleData::init(%s) - from file", iFilePath.c_str());

  fOriginalFilePath = iFilePath;
  fSampleStorage = SampleFile::create(iFilePath);
  fSource = Source::kFile;
  fUpdateType = UpdateType ::kNone;
  fCache = {};

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

  fOriginalFilePath = iFilePath;
  fSampleStorage = std::move(iSamplingStorage);
  fSource = Source::kSampling;
  fUpdateType = UpdateType ::kNone;
  fCache = {};

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

  fOriginalFilePath = iFilePath;

  fSampleStorage = SampleFile::create(fOriginalFilePath,
                                      iSampleBuffers,
                                      true,
                                      SampleFile::kSampleFormatWAV,
                                      SampleFile::kSampleFormatPCM24);
  fSource = iSource;
  fUpdateType = iUpdateType;
  fCache = {};

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

  fOriginalFilePath = std::move(iFilename);
  fSampleStorage = nullptr;
  fCache = {};

  uint64 size = 0;
  tresult res = IBStreamHelper::readInt64u(iStreamer, size);

  if(res == kResultOk)
  {
    auto pos = iStreamer.tell();

    fSampleStorage = SampleFile::create(iStreamer, fOriginalFilePath, size);

    if(!fSampleStorage)
    {
      iStreamer.seek(pos, kSeekSet);

      LOG_F(WARNING, "Could not save the data in a temporary file");
    }
  }

  return exists() ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// SampleData::getNumSamples
//------------------------------------------------------------------------
int32 SampleData::getNumSamples(SampleRate iSampleRate) const
{
  return fCache.getNumSamples(fSampleStorage, iSampleRate);
}

//------------------------------------------------------------------------
// SampleData::load
//------------------------------------------------------------------------
std::shared_ptr<SampleBuffers32> SampleData::load(SampleRate iSampleRate) const
{
  DLOG_F(INFO, "SampleData::load(%f)", iSampleRate);

  return fCache.getData(fSampleStorage, iSampleRate);
}

//------------------------------------------------------------------------
// SampleData::Cache::getData
//------------------------------------------------------------------------
std::shared_ptr<SampleBuffers32> SampleData::Cache::getData(std::shared_ptr<SampleStorage> iSampleStorage,
                                                            SampleRate iSampleRate)
{
  if(!fBuffersCache || fSampleRate != iSampleRate)
  {
    fSampleRate = iSampleRate;
    fBuffersCache = iSampleStorage ? ExpiringDataCache<SampleBuffers32>{
      // loader
      [iSampleStorage, iSampleRate]() -> auto {
        return iSampleStorage->toBuffers(iSampleRate);
      },
      // ttl
      cBuffersCacheTTL
    }
                                   : ExpiringDataCache<SampleBuffers32>{};
  }

  if(fBuffersCache)
  {
    auto buffers = fBuffersCache.getData();
    fNumSamples = buffers ? buffers->getNumSamples() : 0;
    return buffers;
  }

  return nullptr;
}

//------------------------------------------------------------------------
// SampleData::Cache::getNumSamples
//------------------------------------------------------------------------
int32 SampleData::Cache::getNumSamples(std::shared_ptr<SampleStorage> iStorage, SampleRate iSampleRate)
{
  if(fNumSamples == 0 || fSampleRate != iSampleRate)
  {
    fNumSamples = 0;
    getData(iStorage, iSampleRate);
  }

  return fNumSamples;
}

//------------------------------------------------------------------------
// SampleData::load
//------------------------------------------------------------------------
std::shared_ptr<SampleBuffers32> SampleData::loadOriginal() const
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

  return kResultOk;
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
// SampleData::getFilePath
//------------------------------------------------------------------------
UTF8Path SampleData::getFilePath() const
{
  if(fSampleStorage)
    return fSampleStorage->getFilePath();

  return "";
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
  auto buffers = loadOriginal();

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

  res->fOriginalFilePath = iFilePath;
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
  fStringSerializer.writeToStream(iValue.getOriginalFilePath().utf8_str(), oStreamer);
  return iValue.copyData(oStreamer);
}

}