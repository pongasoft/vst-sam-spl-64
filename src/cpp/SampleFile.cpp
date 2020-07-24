#include <fstream>
#include <algorithm>
#include "SampleFile.h"
#include <sstream>
#include <pongasoft/logging/logging.h>
#include <pongasoft/Utils/Clock/Clock.h>
#include <pluginterfaces/base/ibstream.h>
#include "FilePath.h"
#include <atomic>

#if SMTG_OS_WINDOWS
#include <windows.h>
#include <tchar.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#endif

#include "SampleBuffers.hpp"
#include <sndfile.hh>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

constexpr int32 BUFFER_SIZE = 1024;

//------------------------------------------------------------------------
// SampleFile::extractFilename
//------------------------------------------------------------------------
UTF8Path SampleFile::extractFilename(UTF8Path const &iFilePath)
{
  auto const &path = iFilePath.cpp_str();

  // first we look for /
  auto found = path.rfind('/');

  // not found? look for backslash
  if(found == std::string::npos)
    found = path.rfind('\\');

  if(found == std::string::npos)
    return iFilePath;
  else
    return path.substr(found + 1);
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
// SampleFile::create (read a user provided file)
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(UTF8Path const &iFromFilePath)
{
  auto toFilePath = createTempFilePath(iFromFilePath);

  std::ifstream ifs(iFromFilePath.toNativePath(), std::fstream::binary);
  if(!ifs)
  {
    LOG_F(ERROR, "Could not open (R) %s", iFromFilePath.c_str());
    return nullptr;
  }

  std::ofstream ofs(toFilePath.toNativePath(), std::fstream::binary);

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

  ofs.close();

  DLOG_F(INFO, "SampleFile::create - copied %s -> %s", iFromFilePath.c_str(), toFilePath.c_str());

  return std::make_unique<SampleFile>(toFilePath, static_cast<int32>(fileSize), true);
}

//------------------------------------------------------------------------
// SampleFile::create (from user sampling)
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(UTF8Path const &iToFilePath,
                                               SampleBuffers32 const &iSampleBuffers,
                                               bool iTemporaryFile,
                                               ESampleMajorFormat iMajorFormat,
                                               ESampleMinorFormat iMinorFormat)
{
  auto toFilePath = iTemporaryFile ? createTempFilePath(iToFilePath) : iToFilePath;

  tresult res;
  {
    int format = iMajorFormat == ESampleMajorFormat::kSampleFormatWAV ? SF_FORMAT_WAV : SF_FORMAT_AIFF;
    switch(iMinorFormat)
    {
      case ESampleMinorFormat::kSampleFormatPCM16:
        format |= SF_FORMAT_PCM_16;
        break;

      case ESampleMinorFormat::kSampleFormatPCM24:
        format |= SF_FORMAT_PCM_24;
        break;

      case ESampleMinorFormat::kSampleFormatPCM32:
        format |= SF_FORMAT_PCM_32;
        break;
    }

    SndfileHandle sndFile(toFilePath.toNativePath().c_str(),
                          SFM_WRITE, // open for writing
                          format,
                          iSampleBuffers.getNumChannels(),
                          static_cast<int>(iSampleBuffers.getSampleRate()));

    if(!sndFile.rawHandle())
    {
      LOG_F(ERROR, "Could not open (W) %s", toFilePath.c_str());
      return nullptr;
    }

    res = iSampleBuffers.save(sndFile);
  }

  // the previous block ensures that sndFile will be deleted hence the file is closed at this point

  if(res == kResultOk)
  {
    // open the file for read at the end which will provide the size of the file
    std::ifstream ifs(toFilePath.toNativePath(), std::ifstream::ate | std::ifstream::binary);
    auto fileSize = ifs.tellg();
    ifs.close();
    return std::make_unique<SampleFile>(toFilePath, fileSize, iTemporaryFile);
  }
  else
    return nullptr;
}

//------------------------------------------------------------------------
// SampleFile::create (from the state)
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(IBStreamer &iFromStream,
                                               UTF8Path const &iFromFilePath,
                                               uint64 iFileSize)
{
  auto toFilePath = createTempFilePath(iFromFilePath);

  std::ofstream ofs(toFilePath.toNativePath(), std::fstream::binary);

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
  std::ifstream ifs(fFilePath.toNativePath(), std::fstream::binary);

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
std::unique_ptr<SampleFile> SampleFile::clone() const
{
  if(fTemporary)
    return SampleFile::create(fFilePath);
  else
    return std::make_unique<SampleFile>(fFilePath, fFileSize, false);
}

//------------------------------------------------------------------------
// SampleFile::toBuffers
//------------------------------------------------------------------------
std::unique_ptr<SampleBuffers32> SampleFile::toBuffers() const
{
  DLOG_F(INFO, "SampleFile::toBuffers ... Loading from file %s", getFilePath().c_str());

  SndfileHandle sndFile(getFilePath().toNativePath().c_str());

  if(!sndFile.rawHandle())
  {
    DLOG_F(ERROR, "error opening file %s", getFilePath().c_str());
    return nullptr;
  }
  else
  {
    DLOG_F(INFO, "read (libsndfile) %d/%d/%llu | %d", sndFile.channels(), sndFile.samplerate(), sndFile.frames(), sndFile.format());

    // TODO improvement: could load and resample at the same time (much more complex)
    return SampleBuffers32::load(sndFile);
  }
}

//------------------------------------------------------------------------
// SampleFile::toBuffers
//------------------------------------------------------------------------
std::unique_ptr<SampleBuffers32> SampleFile::toBuffers(SampleRate iSampleRate) const
{
  auto res = toBuffers();

  if(res && res->getSampleRate() != iSampleRate)
  {
    DLOG_F(INFO, "Resampling %f -> %f", res->getSampleRate(), iSampleRate);
    res = res->resample(iSampleRate);
  }

  return res;
}

//------------------------------------------------------------------------
// SampleFile::getSampleInfo
//------------------------------------------------------------------------
tresult SampleFile::getSampleInfo(SampleInfo &oSampleInfo) const
{
  if(fSampleInfoCache.fSampleRate == -2)
    return kResultFalse;

  if(fSampleInfoCache.fSampleRate == -1)
  {
    SndfileHandle sndFile(getFilePath().toNativePath().c_str());

    if(!sndFile.rawHandle())
    {
      DLOG_F(ERROR, "error opening file %s", getFilePath().c_str());
      const_cast<SampleFile *>(this)->fSampleInfoCache.fSampleRate = -2;
      return kResultFalse;
    }
    else
    {
      // Implementation note: this method should be const and the fact that we are using a cache
      // (so that we don't open the file over and over) should not be exposed
      const_cast<SampleFile *>(this)->fSampleInfoCache = SampleInfo{static_cast<SampleRate>(sndFile.samplerate()),
                                                                    sndFile.channels(),
                                                                    static_cast<int32>(sndFile.frames())};
    }
  }

  oSampleInfo = fSampleInfoCache;
  return kResultOk;
}

}
}
}