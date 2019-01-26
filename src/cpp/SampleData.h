#pragma once

#include <string>
#include <base/source/fstreamer.h>
#include <memory>

#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/VST/GUI/Params/GUIParamSerializers.h>
#include "SampleStorage.h"
#include "SampleBuffers.h"
#include "Model.h"

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
  enum class Source { kUnknown, kFile, kSampling };
  enum class UpdateType { kNone, kAction, kUndo };

  struct Action
  {
    enum class Type { kCut, kCrop, kTrim, kNormalize0, kNormalize3, kNormalize6, kResample };

    explicit Action(Type iType) : fType{iType} {}

    Type fType;
    SampleRange fSelectedSampleRange{-1};
    Percent fOffsetPercent{};
    Percent fZoomPercent{};
    SampleRate fSampleRate{};
  };

  struct ExecutedAction
  {
    ExecutedAction(Action const &iAction, std::unique_ptr<SampleData> iSampleData) :
      fAction{iAction},
      fSampleData(std::move(iSampleData)){}

    Action fAction;
    std::unique_ptr<SampleData> fSampleData{};
  };

public:
  SampleData() = default;

  // Copy Constructor (for param API)
  SampleData(SampleData const& iOther);

  // Move Constructor
  SampleData(SampleData &&iOther) noexcept;

  // Move assignment => buf2 = buf1
  SampleData &operator=(SampleData &&other) noexcept;

  // init from file (when user selects a file in the GUI)
  tresult init(std::string const &iFilePath);

  // init from samples (when user does sampling)
  tresult init(SampleBuffers32 const &iSampleBuffers,
               std::string const *iFilePath = nullptr,
               Source iSource = Source::kSampling,
               UpdateType iUpdateType = UpdateType::kNone);

  // init from saved state
  tresult init(std::string iFilename, IBStreamer &iStreamer);

  // getSource => where the sample comes from (loaded, sampling...).
  Source getSource() const { return fSource; };

  // getUpdateType => what type of update was executed to generate this sample data
  UpdateType getUpdateType() const { return fUpdateType; }

  /**
   * Executes the provided action and add it to the undo history if successful.
   *
   * @return kResultOk if action succeeded, kResultFalse otherwise
   */
  tresult execute(Action const &iAction);

  /**
   * @return true if there is an undo history (which means calling undo will revert the previous operation)
   */
  bool hasUndoHistory() const { return fUndoHistory != nullptr; }

  /**
   * Empties the undo history (to save space mostly)
   */
  void clearUndoHistory() { fUndoHistory = nullptr; fUpdateType = UpdateType::kNone; }

  /**
   * @return the undo history or nullptr if none
   */
  ExecutedAction const *getUndoHistory() const { return fUndoHistory ? fUndoHistory.get() : nullptr; }

  /**
   * Undo the last operation.
   *
   * @return the last action that was undone, or nullptr if there was none
   */
  std::unique_ptr<Action> undo();

  std::string const& getFilePath() const { return fFilePath; }
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
  std::unique_ptr<SampleData> save(std::string const &iFilePath,
                                   SampleStorage::ESampleMajorFormat iMajorFormat,
                                   SampleStorage::ESampleMinorFormat iMinorFormat) const;

  tresult copyData(IBStreamer &oStreamer) const;

protected:
  tresult addExecutedAction(Action const &iAction, std::unique_ptr<SampleBuffers32> iSampleBuffers);

private:
  std::string fFilePath{};
  std::unique_ptr<SampleStorage> fSampleStorage{};
  Source fSource{Source::kUnknown};
  UpdateType fUpdateType{UpdateType::kNone};
  std::unique_ptr<ExecutedAction> fUndoHistory{};
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

