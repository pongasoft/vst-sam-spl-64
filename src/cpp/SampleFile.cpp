#include <fstream>
#include <algorithm>
#include "SampleFile.h"
#include <sstream>
#include <pongasoft/logging/logging.h>
#include <pongasoft/Utils/Clock/Clock.h>
#include <pluginterfaces/base/ibstream.h>
#include "mackron/dr_libs/dr_wav.h"
#include "SampleBuffers.hpp"

#if SMTG_OS_WINDOWS
#include <windows.h>
#include <tchar.h>
#endif

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

constexpr uint64 BUFFER_SIZE = 1024;

//------------------------------------------------------------------------
// ::createTempFilePath
//------------------------------------------------------------------------
std::string createTempFilePath(std::string const &iFilename)
{
  // this should be unique across multiple instances of the plugin running in the same DAW
  static std::atomic<int32> unique_id(0);

  std::string tempFilePath;

#if SMTG_OS_WINDOWS
  // From https://docs.microsoft.com/en-us/windows/desktop/FileIO/creating-and-using-a-temporary-file
  TCHAR lpTempPathBuffer[MAX_PATH];
  auto dwRetVal = GetTempPath(MAX_PATH,lpTempPathBuffer);
  if(dwRetVal > MAX_PATH || (dwRetVal == 0))
  {
    LOG_F(ERROR, "Cannot get access to temporary folder");
    return nullptr;
  }
  tempFilePath = lpTempPathBuffer;
#else
  tempFilePath = "/tmp/";
#endif

  std::ostringstream tempFilename;

  tempFilename << "sam_spl64_" << Clock::getCurrentTimeMillis() << "_" << unique_id.fetch_add(1);

  auto found = iFilename.rfind('.');
  if(found == std::string::npos)
    tempFilename << ".raw";
  else
    tempFilename << iFilename.substr(found);

  tempFilePath += tempFilename.str();

  return tempFilePath;
}

//------------------------------------------------------------------------
// SampleFile::~SampleFile()
//------------------------------------------------------------------------
SampleFile::~SampleFile()
{
  if(fTemporary)
  {
    DLOG_F(INFO, "SampleFile::~SampleFile() deleting %s ", getFilePath().c_str());

    if(remove(getFilePath().c_str()) != 0)
      LOG_F(WARNING, "Could not delete %s", getFilePath().c_str());
  }
}

//------------------------------------------------------------------------
// SampleFile::create
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(std::string const &iFromFilePath)
{
  std::string toFilePath = createTempFilePath(iFromFilePath);

  std::ifstream ifs(iFromFilePath, std::fstream::binary);
  if(!ifs)
  {
    LOG_F(ERROR, "Could not open (R) %s", iFromFilePath.c_str());
    return nullptr;
  }

  std::ofstream ofs(toFilePath, std::fstream::binary);

  if(!ofs)
  {
    LOG_F(ERROR, "Could not open (W) %s", toFilePath.c_str());
    return nullptr;
  }

  char buf[BUFFER_SIZE];

  bool complete = false;

  uint64 fileSize = 0;

  while(!complete)
  {
    ifs.read(buf, BUFFER_SIZE);

    if(ifs.bad())
    {
      LOG_F(ERROR, "Error while reading file %s", iFromFilePath.c_str());
      return nullptr;
    }

    if(ifs.gcount() > 0)
    {
      ofs.write(buf, ifs.gcount());
      if(ofs.bad())
      {
        LOG_F(ERROR, "Error while writing file %s", toFilePath.c_str());
        return nullptr;
      }
      fileSize += ifs.gcount();
    }

    complete = ifs.eof();
  }

  DLOG_F(INFO, "SampleFile::create - copied %s -> %s", iFromFilePath.c_str(), toFilePath.c_str());

  return std::make_unique<SampleFile>(toFilePath, fileSize, true);
}

//------------------------------------------------------------------------
// SampleFile::create
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(IBStreamer &iFromStream,
                                               std::string const &iFromFilePath,
                                               uint64 iFileSize)
{
  std::string toFilePath = createTempFilePath(iFromFilePath);

  std::ofstream ofs(toFilePath, std::fstream::binary);

  if(!ofs)
  {
    LOG_F(ERROR, "Could not open (W) %s", toFilePath.c_str());
    return nullptr;
  }

  char buf[BUFFER_SIZE];

  bool complete = false;

  uint64 expectedFileSize = iFileSize;

  while(!complete)
  {
    int32 count{0};
    auto res = iFromStream.getStream()->read(buf,
                                             static_cast<int32>(std::min(static_cast<uint64>(BUFFER_SIZE), expectedFileSize)),
                                             &count);

    if(res != kResultOk)
    {
      LOG_F(ERROR, "Error while reading stream [%d]", res);
      return nullptr;
    }

    if(count > 0)
    {
      ofs.write(buf, count);
      if(ofs.bad())
      {
        LOG_F(ERROR, "Error while writing file %s", toFilePath.c_str());
        return nullptr;
      }

      expectedFileSize -= count;
      complete = expectedFileSize == 0;
    }
    else
    {
      complete = true;
    }

  }

  if(expectedFileSize == 0)
  {
    DLOG_F(INFO, "SampleFile::create - copied [stream] -> %s", toFilePath.c_str());
    return std::make_unique<SampleFile>(toFilePath, iFileSize, true);
  }
  else
  {
    LOG_F(ERROR, "Corrupted stream: not enough data");
    return nullptr;
  }
}

//------------------------------------------------------------------------
// SampleFile::copyTo
//------------------------------------------------------------------------
tresult SampleFile::copyTo(IBStreamer &oStreamer) const
{
  std::ifstream ifs(fFilePath, std::fstream::binary);

  if(!ifs)
  {
    LOG_F(ERROR, "Could not open (R) %s", fFilePath.c_str());
    return kResultFalse;
  }

  char buf[BUFFER_SIZE];

  bool complete = false;

  int64 expectedFileSize = fFileSize;

  while(!complete)
  {
    ifs.read(buf, BUFFER_SIZE);

    if(ifs.bad())
    {
      LOG_F(ERROR, "Error while reading file %s", fFilePath.c_str());
      return kResultFalse;
    }

    auto count = static_cast<int32>(ifs.gcount());
    if(count > 0)
    {
      int32 streamCount{0};
      auto res = oStreamer.getStream()->write(buf, count, &streamCount);

      if(res == kResultOk)
      {
        if(count != streamCount)
        {
          DLOG_F(ERROR, "Error while writing to stream");
          return kResultFalse;
        }
      }
      else
      {
        DLOG_F(ERROR, "Error while writing to stream");
        return res;
      }

      expectedFileSize -= count;
    }

    complete = ifs.eof();
  }

  if(expectedFileSize != 0)
    return kResultFalse;

  DLOG_F(INFO, "SampleFile::copyTo - copied %s -> [stream]", fFilePath.c_str());

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleFile::clone
//------------------------------------------------------------------------
std::unique_ptr<SampleStorage> SampleFile::clone() const
{
  if(fTemporary)
    return SampleFile::create(fFilePath);
  else
    return std::unique_ptr<SampleStorage>(new SampleFile(fFilePath, fFileSize, false));
}

//------------------------------------------------------------------------
// SampleFile::toBuffers
//------------------------------------------------------------------------
std::unique_ptr<SampleBuffers32> SampleFile::toBuffers() const
{
  std::unique_ptr<SampleBuffers32> res = nullptr;

  unsigned int channels;
  unsigned int sampleRate;
  drwav_uint64 totalSampleCount;

  DLOG_F(INFO, "SampleFile::toBuffers ... Loading from file %s", getFilePath().c_str());

  float *pSampleData = drwav_open_and_read_file_f32(getFilePath().c_str(),
                                                    &channels,
                                                    &sampleRate,
                                                    &totalSampleCount);
  if(pSampleData == nullptr)
  {
    DLOG_F(ERROR, "error opening file %s", getFilePath().c_str());
  }
  else
  {
    DLOG_F(INFO, "read %d/%d/%llu", channels, sampleRate, totalSampleCount);
    res = SampleBuffers32::fromInterleaved(sampleRate,
                                           channels,
                                           static_cast<int32>(totalSampleCount),
                                           pSampleData);
  }

  drwav_free(pSampleData);

  return std::move(res);
}

}
}
}