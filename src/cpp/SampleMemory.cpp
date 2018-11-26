#include "SampleMemory.h"
#include <fstream>
#include <pongasoft/logging/logging.h>
#include <pluginterfaces/base/ibstream.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

constexpr int32 BUFFER_SIZE = 1024;

//------------------------------------------------------------------------
// SampleMemory::create
//------------------------------------------------------------------------
std::unique_ptr<SampleMemory> SampleMemory::create(std::string const &iFilePath)
{
  std::ifstream ifs(iFilePath, std::fstream::binary);

  if(!ifs)
  {
    LOG_F(ERROR, "Could not open (R) %s", iFilePath.c_str());
    return nullptr;
  }

  char buf[BUFFER_SIZE];

  bool complete = false;

  std::vector<char> memoryBuffer;

  while(!complete)
  {
    ifs.read(buf, BUFFER_SIZE);

    if(ifs.bad())
    {
      LOG_F(ERROR, "Error while reading file %s", iFilePath.c_str());
      return nullptr;
    }

    std::streamsize count = ifs.gcount();
    if(count > 0)
    {
      memoryBuffer.insert(memoryBuffer.end(), buf, buf + count);
    }

    complete = ifs.eof();
  }

  DLOG_F(INFO, "SampleMemory::create - copied %s -> [RAM]", iFilePath.c_str());

  return std::make_unique<SampleMemory>(std::move(memoryBuffer));
}

//------------------------------------------------------------------------
// SampleMemory::create
//------------------------------------------------------------------------
std::unique_ptr<SampleMemory> SampleMemory::create(IBStreamer &iFromStream, uint64 iFileSize)
{
  char buf[BUFFER_SIZE];

  bool complete = false;
  uint64 expectedFileSize = iFileSize;

  std::vector<char> memoryBuffer;

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
      memoryBuffer.insert(memoryBuffer.end(), buf, buf + count);
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
    DLOG_F(INFO, "SampleMemory::create - copied [stream] -> [RAM]");
    return std::make_unique<SampleMemory>(std::move(memoryBuffer));
  }
  else
  {
    LOG_F(ERROR, "Corrupted stream: not enough data");
    return nullptr;
  }
}

//------------------------------------------------------------------------
// SampleMemory::copyTo
//------------------------------------------------------------------------
tresult SampleMemory::copyTo(IBStreamer &oStreamer) const
{
  int32 count{0};
  auto res = oStreamer.getStream()->write((void *) fMemoryBuffer.data(),
                                          static_cast<int32>(fMemoryBuffer.size()),
                                          &count);

  if(res == kResultOk)
  {
    if(count == fMemoryBuffer.size())
    {
      DLOG_F(INFO, "SampleFile::copyTo - copied [RAM] -> [stream]");
      return kResultOk;
    }
    else
      return kResultFalse;
  }
  else
  {
    return res;
  }
}

}
}
}