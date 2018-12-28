#pragma once

#include <string>
#include <base/source/fstreamer.h>
#include <memory>

#include "SampleStorage.h"
#include "SampleBuffers.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

class SampleFile : public SampleStorage
{
public:
  /**
   * If iTemporary is true, the file will be automatically deleted in the destructor
   */
  SampleFile(std::string iFilePath, uint64 iFileSize, bool iTemporary) :
    fFilePath(std::move(iFilePath)),
    fTemporary{iTemporary},
    fFileSize{iFileSize} {}

    // Destructor (delete fFilePath if temporary)
  ~SampleFile () override;

  // copyTo
  tresult copyTo(IBStreamer &oStreamer) const override;

  // getSize
  uint64 getSize() const override { return fFileSize; }

  // clone
  std::unique_ptr<SampleStorage> clone() const override;

  // getFilePath
  inline std::string const &getFilePath() const { return fFilePath; }

  // toBuffers
  std::unique_ptr<SampleBuffers32> toBuffers(SampleRate iSampleRate) const override;
  std::unique_ptr<SampleBuffers32> toBuffers() const override;

  // getSampleInfo
  tresult getSampleInfo(SampleInfo &oSampleInfo) const override;

  // create / factory methods
  static std::unique_ptr<SampleFile> create(std::string const &iFromFilePath);
  static std::unique_ptr<SampleFile> create(std::string const &iToFilePath, SampleBuffers32 const &iSampleBuffers);
  static std::unique_ptr<SampleFile> create(IBStreamer &iFromStream, std::string const &iFromFilePath, uint64 iFileSize);

  // extracts the filename portion of the file path
  static std::string extractFilename(std::string const &iFilePath);

private:
  std::string fFilePath;
  bool fTemporary;
  uint64 fFileSize;
  SampleInfo fSampleInfoCache{}; // will cache the value once read
};

}
}
}

