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