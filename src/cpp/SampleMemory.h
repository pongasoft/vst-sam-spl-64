#pragma once

#include <base/source/fstreamer.h>
#include <string>
#include <vector>
#include <memory>

#include "SampleStorage.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

class SampleMemory : public SampleStorage
{
public:

  SampleMemory() = default;
  SampleMemory(SampleMemory const &iOther) : fMemoryBuffer{iOther.fMemoryBuffer} {}
  explicit SampleMemory(std::vector<char> &&iMemoryBuffer) : fMemoryBuffer{std::move(iMemoryBuffer)} {}

  uint64 getSize() const override { return fMemoryBuffer.size(); }

  tresult copyTo(IBStreamer &oStreamer) const override;

  std::unique_ptr<SampleStorage> clone() const override;

  std::unique_ptr<SampleBuffers32> toBuffers() const override;

  static std::unique_ptr<SampleMemory> create(std::string const &iFilePath);
  static std::unique_ptr<SampleMemory> create(IBStreamer &iFromStream, uint64 iFileSize);

private:
  std::vector<char> fMemoryBuffer{};
};

}
}
}

