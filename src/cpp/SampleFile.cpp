#include <fstream>
#include <algorithm>
#include "SampleFile.h"
#include <sstream>
#include <pongasoft/logging/logging.h>
#include <pongasoft/Utils/Clock/Clock.h>
#include <pluginterfaces/base/ibstream.h>

#if SMTG_OS_WINDOWS
#include <windows.h>
#include <tchar.h>
#endif

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

constexpr uint64 BUFFER_SIZE = 1024;

//------------------------------------------------------------------------
// SampleFile::create
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(std::string const &iFromFilePath, std::string const &iToFilePath)
{
  std::ifstream ifs(iFromFilePath, std::fstream::binary);
  if(!ifs)
  {
    LOG_F(ERROR, "Could not open (R) %s", iFromFilePath.c_str());
    return nullptr;
  }

  std::ofstream ofs(iToFilePath, std::fstream::binary);

  if(!ofs)
  {
    LOG_F(ERROR, "Could not open (W) %s", iToFilePath.c_str());
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
        LOG_F(ERROR, "Error while writing file %s", iToFilePath.c_str());
        return nullptr;
      }
      fileSize += ifs.gcount();
    }

    complete = ifs.eof();
  }

  DLOG_F(INFO, "SampleFile::create - copied %s -> %s", iFromFilePath.c_str(), iToFilePath.c_str());

  return std::make_unique<SampleFile>(iToFilePath, fileSize);
}

//------------------------------------------------------------------------
// SampleFile::create
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(IBStreamer &iFromStream,
                                               std::string const &iToFilePath,
                                               uint64 iFileSize)
{
  std::ofstream ofs(iToFilePath, std::fstream::binary);

  if(!ofs)
  {
    LOG_F(ERROR, "Could not open (W) %s", iToFilePath.c_str());
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
        LOG_F(ERROR, "Error while writing file %s", iToFilePath.c_str());
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
    DLOG_F(INFO, "SampleFile::create - copied [stream] -> %s", iToFilePath.c_str());
    return std::make_unique<SampleFile>(iToFilePath, iFileSize);
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
// TemporarySampleFile::~TemporarySampleFile
//------------------------------------------------------------------------
TemporarySampleFile::~TemporarySampleFile()
{
  if(fSampleFile)
  {
    DLOG_F(INFO, "TemporarySampleFile::~TemporarySampleFile() deleting %s ", fSampleFile->getFilePath().c_str());

    if(remove(fSampleFile->getFilePath().c_str()) != 0)
      LOG_F(WARNING, "Could not delete %s", fSampleFile->getFilePath().c_str());
  }
}

// this should be unique across multiple instances of the plugin
std::atomic<int32> unique_id(0);

//------------------------------------------------------------------------
// TemporarySampleFile::createTempFilePath
//------------------------------------------------------------------------
std::string TemporarySampleFile::createTempFilePath(std::string const &iFilename)
{
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
// TemporarySampleFile::create
//------------------------------------------------------------------------
std::unique_ptr<TemporarySampleFile> TemporarySampleFile::create(std::string const &iFromFilePath)
{
  std::string tempFilePath = createTempFilePath(iFromFilePath);

  DLOG_F(INFO, "TemporarySampleFile::create %s -> %s", iFromFilePath.c_str(), tempFilePath.c_str());

  auto sampleFile = SampleFile::create(iFromFilePath, tempFilePath);
  if(sampleFile)
    return std::make_unique<TemporarySampleFile>(std::move(sampleFile));
  else
    return nullptr;
}

//------------------------------------------------------------------------
// TemporarySampleFile::create
//------------------------------------------------------------------------
std::unique_ptr<TemporarySampleFile> TemporarySampleFile::create(IBStreamer &iFromStream,
                                                                 std::string const &iFromFilename,
                                                                 uint64 iFileSize)
{
  std::string tempFilePath = createTempFilePath(iFromFilename);

  DLOG_F(INFO, "TemporarySampleFile::create %s -> %s", iFromFilename.c_str(), tempFilePath.c_str());

  auto sampleFile = SampleFile::create(iFromStream, tempFilePath, iFileSize);
  if(sampleFile)
    return std::make_unique<TemporarySampleFile>(std::move(sampleFile));
  else
    return nullptr;

}


}
}
}