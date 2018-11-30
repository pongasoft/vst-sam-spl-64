#pragma once

#include <string>
#include <base/source/fstreamer.h>
#include <memory>

#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/VST/GUI/Params/GUIParamSerializers.h>
#include "SampleStorage.h"
#include "SampleBuffers.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

/**
 * This class maintains the sample itself as it will be saved (for example a wav format).
 *
 * During the life of the plugin the loaded sample will be maintained in a temporary file. The fSampleMemory is only
 * used if, for some reason, it is not possible to save a temporary file.
 */
class SampleData
{
public:
  SampleData() = default;

  // Copy Contructor (for param API)
  SampleData(SampleData const& iOther);

  // Move assignment => buf2 = buf1
  SampleData &operator=(SampleData &&other) noexcept;

  // init from file (when user selects a file in the GUI)
  tresult init(std::string const &iFilePath);

  // init from samples (when user does sampling)
  tresult init(SampleBuffers32 const &iSampleBuffers);

  // init from saved state
  tresult init(std::string iFilename, IBStreamer &iStreamer);

  std::string const& getFilePath() const { return fFilePath; }
  bool exists() const { return fSampleStorage != nullptr; }
  uint64 getSize() const;

  std::unique_ptr<SampleBuffers32> load(SampleRate iSampleRate) const;

  tresult copyData(IBStreamer &oStreamer) const;

private:
  std::string fFilePath{};
  std::unique_ptr<SampleStorage> fSampleStorage{};
};

/**
 * Serializes the sample data (for example the original file)
 */
class SampleDataSerializer : public IParamSerializer<SampleData>
{
public:
  using ParamType = SampleData;

  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override;

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override;

private:
  GUI::Params::UTF8StringParamSerializer<128> fStringSerializer{};
};

}
}
}

