#pragma once

#include <string>
#include <base/source/fstreamer.h>
#include <memory>
#include <forward_list>

#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/VST/GUI/Params/GUIParamSerializers.h>
#include <pongasoft/VST/GUI/Params/GUIJmbParameter.h>
#include "SampleStorage.h"
#include "SampleBuffers.h"
#include "Model.h"
#include "FilePath.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

/**
 * This class maintains the sample itself as it will be saved (for example a wav format).
 *
 * During the life of the plugin the loaded sample will be maintained in a temporary file.
 */
class SampleData
{
public:
  enum class Source { kUnknown, kFile, kSampling };
  enum class UpdateType { kNone, kAction, kUndo };

public:
  SampleData() = default;

  // Copy Constructor (for param API)
  SampleData(SampleData const& iOther) = default;

  // Move Constructor (for param API)
  SampleData(SampleData &&iOther) noexcept = default;

  SampleData &operator=(SampleData &&iOther) noexcept = default;

  // init from file (when user selects a file in the GUI)
  tresult init(UTF8Path const &iFilePath);

  // init from sampling
  tresult init(UTF8Path const &iFilePath, std::shared_ptr<SampleStorage> iSamplingStorage);

  // init from buffers (after user action like cut/crop, etc...)
  tresult init(SampleBuffers32 const &iSampleBuffers,
               UTF8Path const &iFilePath,
               Source iSource,
               UpdateType iUpdateType);

  // init from saved state
  tresult init(std::string iFilename, IBStreamer &iStreamer);

  // getSource => where the sample comes from (loaded, sampling...).
  Source getSource() const { return fSource; };

  // getUpdateType => what type of update was executed to generate this sample data
  UpdateType getUpdateType() const { return fUpdateType; }

  UTF8Path const& getFilePath() const { return fFilePath; }
  bool exists() const { return fSampleStorage != nullptr; }
  uint64 getSize() const;

  /**
   * Reads the sample info and populate the argument when possible
   *
   * @return kResultOk if the sample info could be read
   */
  tresult getSampleInfo(SampleInfo &oSampleInfo) const;
  std::unique_ptr<SampleInfo> getSampleInfo() const;

  std::unique_ptr<SampleBuffers32> load(SampleRate iSampleRate) const;
  std::unique_ptr<SampleBuffers32> load() const;

  /**
   * Save this sample to another file
   *
   * @param iFilePath
   * @return the new sample data or nullptr if there is a problem saving the file
   */
  std::unique_ptr<SampleData> save(UTF8Path const &iFilePath,
                                   SampleStorage::ESampleMajorFormat iMajorFormat,
                                   SampleStorage::ESampleMinorFormat iMinorFormat) const;

  /**
   * Delegate to the storage to copy its content
   */
  tresult copyData(IBStreamer &oStreamer) const;

private:
  UTF8Path fFilePath{};
  std::shared_ptr<SampleStorage> fSampleStorage{};
  Source fSource{Source::kUnknown};
  UpdateType fUpdateType{UpdateType::kNone};
};

/**
 * Encapsulates an action on SampleData as an object (in order to be able to handle undo/redo) */
struct SampleDataAction
{
  enum class Type { kCut, kCrop, kTrim, kNormalize0, kNormalize3, kNormalize6, kResample, kLoad, kSample };

  explicit SampleDataAction(Type iType) : fType{iType} {}

  Type fType;
  SampleRange fSelectedSampleRange{-1};
  Percent fOffsetPercent{};
  Percent fZoomPercent{};
  SampleRate fSampleRate{};
  UTF8Path fFilePath{};
  std::shared_ptr<SampleStorage> fSamplingStorage{};
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

