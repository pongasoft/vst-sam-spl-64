/*
 * Copyright (c) 2020 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include "SampleFile.h"
#include "../SampleBuffers.hpp"

#include <pongasoft/logging/logging.h>

#include <sndfile.hh>

namespace pongasoft::VST::SampleSplitter::GUI {

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
// SampleFile::computeFileSize
//------------------------------------------------------------------------
int64 SampleFile::computeFileSize(const UTF8Path &iFilePath)
{
  std::ifstream in(iFilePath.toNativePath(), std::ifstream::ate | std::ifstream::binary);
  return static_cast<int64>(in.tellg());
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

  return std::make_unique<SampleFile>(iFromFilePath, toFilePath, static_cast<uint64>(fileSize));
}

//------------------------------------------------------------------------
// SampleFile::save
//------------------------------------------------------------------------
tresult SampleFile::save(UTF8Path const &iToFilePath,
                         SampleBuffers32 const &iSampleBuffers,
                         SampleFile::ESampleMajorFormat iMajorFormat,
                         SampleFile::ESampleMinorFormat iMinorFormat)
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

  SndfileHandle sndFile(iToFilePath.toNativePath().c_str(),
                        SFM_WRITE, // open for writing
                        format,
                        iSampleBuffers.getNumChannels(),
                        static_cast<int>(iSampleBuffers.getSampleRate()));

  if(!sndFile.rawHandle())
  {
    LOG_F(ERROR, "Could not open (W) %s", iToFilePath.c_str());
    return kResultFalse;
  }

  return iSampleBuffers.save(sndFile);
}


//------------------------------------------------------------------------
// SampleFile::create (from RT sampling)
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(UTF8Path const &iOriginalFilePath,
                                               SampleBuffers32 const &iSampleBuffers,
                                               SampleFile::ESampleMajorFormat iMajorFormat,
                                               SampleFile::ESampleMinorFormat iMinorFormat)
{
  auto toFilePath = createTempFilePath(iOriginalFilePath);

  if(save(toFilePath, iSampleBuffers, iMajorFormat, iMinorFormat) == kResultOk)
    return std::make_unique<SampleFile>(iOriginalFilePath, toFilePath, computeFileSize(toFilePath.toNativePath()));
  else
    return nullptr;
}

//------------------------------------------------------------------------
// SampleFile::create (restoring state)
//------------------------------------------------------------------------
std::unique_ptr<SampleFile> SampleFile::create(IBStreamer &iFromStream, UTF8Path const &iFromFilePath, uint64 iFileSize)
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
    return std::make_unique<SampleFile>(iFromFilePath, toFilePath, iFileSize);
  }
  else
  {
    LOG_F(ERROR, "Corrupted stream: not enough data");
    return nullptr;
  }
}

//------------------------------------------------------------------------
// SampleFile::load
//------------------------------------------------------------------------
std::pair<std::shared_ptr<SampleBuffers32>, SampleRate> SampleFile::load(SampleRate iSampleRate) const
{
  auto buffers = loadOriginal();
  SampleRate originalSampleRate{};

  if(buffers)
  {
    originalSampleRate = buffers->getSampleRate();

    if(buffers->getSampleRate() != iSampleRate)
    {
      DLOG_F(INFO, "Resampling %f -> %f", buffers->getSampleRate(), iSampleRate);
      buffers = buffers->resample(iSampleRate);
    }

  }

  return std::make_pair(std::move(buffers), originalSampleRate);
}

//------------------------------------------------------------------------
// SampleFile::loadOriginal
//------------------------------------------------------------------------
std::unique_ptr<SampleBuffers32> SampleFile::loadOriginal() const
{
  // if there is no file, we cannot load it
  if(empty())
    return nullptr;

  auto const &filePath = getTemporaryFilePath();

  DLOG_F(INFO, "SampleFile::toBuffers ... Loading from file %s", filePath.c_str());

  SndfileHandle sndFile(filePath.toNativePath().c_str());

  if(!sndFile.rawHandle())
  {
    DLOG_F(ERROR, "error opening file %s", filePath.c_str());
    return nullptr;
  }
  else
  {
    DLOG_F(INFO, "read (libsndfile) %d/%d/%llu | %d",
           sndFile.channels(), sndFile.samplerate(), sndFile.frames(), sndFile.format());

    return SampleBuffers32::load(sndFile);
  }
}

//------------------------------------------------------------------------
// SampleFile::copyTo
//------------------------------------------------------------------------
tresult SampleFile::copyTo(IBStreamer &oStreamer) const
{
  DCHECK_F(!empty());

  auto const &filePath = getTemporaryFilePath();

  std::ifstream ifs(filePath.toNativePath(), std::fstream::binary);

  if(!ifs)
  {
    LOG_F(ERROR, "Could not open (R) %s", filePath.c_str());
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
      LOG_F(ERROR, "Error while reading file %s", filePath.c_str());
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

  DLOG_F(INFO, "SampleFile::copyTo - copied %s -> [stream]", filePath.c_str());

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleFileSerializer::readFromStream
//------------------------------------------------------------------------
tresult SampleFileSerializer::readFromStream(IBStreamer &iStreamer, SampleFileSerializer::ParamType &oValue) const
{
  VSTGUI::UTF8String filename;
  auto res = fStringSerializer.readFromStream(iStreamer, filename);
  if(res == kResultOk)
  {
    uint64 size = 0;
    res = IBStreamHelper::readInt64u(iStreamer, size);

    if(res == kResultOk)
    {
      auto pos = iStreamer.tell();

      auto sampleFile = SampleFile::create(iStreamer, filename, size);

      if(!sampleFile)
      {
        iStreamer.seek(pos, kSeekSet);

        LOG_F(WARNING, "Could not save the data in a temporary file");
        res = kResultFalse;
      }

      oValue = *sampleFile;
    }
  }
  return res;
}

//------------------------------------------------------------------------
// SampleFileSerializer::writeToStream
//------------------------------------------------------------------------
tresult SampleFileSerializer::writeToStream(SampleFileSerializer::ParamType const &iValue, IBStreamer &oStreamer) const
{
  if(iValue.empty())
  {
    fStringSerializer.writeToStream("", oStreamer);
    return oStreamer.writeInt64u(0);
  }
  else
  {
    fStringSerializer.writeToStream(iValue.getOriginalFilePath().utf8_str(), oStreamer);
    oStreamer.writeInt64u(iValue.getFileSize());
    return iValue.copyTo(oStreamer);
  }
}

//------------------------------------------------------------------------
// SampleFile::TemporaryFile::~TemporaryFile
//------------------------------------------------------------------------
SampleFile::TemporaryFile::~TemporaryFile()
{
  DLOG_F(INFO, "TemporaryFile::~TemporaryFile() deleting %s ", fFilePath.c_str());

  if(remove(fFilePath.c_str()) != 0)
    LOG_F(WARNING, "Could not delete %s", fFilePath.c_str());
}

}