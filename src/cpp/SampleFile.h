#pragma once

#include <string>
#include <base/source/fstreamer.h>
#include <memory>

#include "SampleBuffers.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

class SampleFile
{
public:
  SampleFile(std::string iFilePath, uint64 iFileSize) : fFilePath(std::move(iFilePath)), fFileSize{iFileSize} {}

  tresult copyTo(IBStreamer &oStreamer) const;

  inline std::string const &getFilePath() const { return fFilePath; }
  inline uint64 getFileSize() const { return fFileSize; }

  static std::unique_ptr<SampleFile> create(std::string const &iFromFilePath, std::string const &iToFilePath);
  static std::unique_ptr<SampleFile> create(IBStreamer &iFromStream, std::string const &iToFilePath, uint64 iFileSize);

private:
  std::string fFilePath;
  uint64 fFileSize;
};

class TemporarySampleFile
{
public:
  explicit TemporarySampleFile(std::unique_ptr<SampleFile> iSampleFile) : fSampleFile{std::move(iSampleFile)} {}

  ~TemporarySampleFile();

  inline std::string const &getFilePath() const { return fSampleFile->getFilePath(); }
  inline uint64 getFileSize() const { return fSampleFile->getFileSize(); }

  inline tresult copyTo(IBStreamer &oStreamer) const { return fSampleFile->copyTo(oStreamer); }

  static std::unique_ptr<TemporarySampleFile> create(std::string const &iFromFilePath);
  static std::unique_ptr<TemporarySampleFile> create(IBStreamer &iFromStream, std::string const &iFromFilename, uint64 iFileSize);

  static std::string createTempFilePath(std::string const &iFilename);

private:
  std::unique_ptr<SampleFile> fSampleFile;
};

}
}
}

