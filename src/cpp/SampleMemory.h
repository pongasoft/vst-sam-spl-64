#pragma once

#include <base/source/fstreamer.h>
#include <string>
#include <vector>
#include <memory>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

class SampleMemory
{
public:

  SampleMemory() {}
  explicit SampleMemory(SampleMemory const &iOther) : fMemoryBuffer{iOther.fMemoryBuffer} {}
  explicit SampleMemory(std::vector<char> &&iMemoryBuffer) : fMemoryBuffer{std::move(iMemoryBuffer)} {}

  inline uint64 getSize() const { return fMemoryBuffer.size(); }

  tresult copyTo(IBStreamer &oStreamer) const;

  static std::unique_ptr<SampleMemory> create(std::string const &iFilePath);
  static std::unique_ptr<SampleMemory> create(IBStreamer &iFromStream, uint64 iFileSize);

private:
  std::vector<char> fMemoryBuffer{};
};

}
}
}

