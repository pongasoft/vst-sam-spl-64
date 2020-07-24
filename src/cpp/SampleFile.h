#pragma once

#include <string>
#include <base/source/fstreamer.h>
#include <memory>

#include "SampleBuffers.h"
#include "FilePath.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

/**
 * Information relating to the sample */
struct SampleInfo
{
  SampleRate fSampleRate{-1};
  int32 fNumChannels{-1};
  int32 fNumSamples{-1};
};

class SampleFile
{
public:
  enum ESampleMajorFormat
  {
    kSampleFormatWAV,
    kSampleFormatAIFF
  };

  enum ESampleMinorFormat
  {
    kSampleFormatPCM16,
    kSampleFormatPCM24,
    kSampleFormatPCM32
  };

public:
  /**
   * If iTemporary is true, the file will be automatically deleted in the destructor
   */
  SampleFile(UTF8Path iFilePath, uint64 iFileSize, bool iTemporary) :
    fFilePath(std::move(iFilePath)),
    fTemporary{iTemporary},
    fFileSize{iFileSize} {}

    // Destructor (delete fFilePath if temporary)
  ~SampleFile ();

  // copyTo
  tresult copyTo(IBStreamer &oStreamer) const;

  // getSize
  uint64 getSize() const { return fFileSize; }

  // clone
  std::unique_ptr<SampleFile> clone() const;

  // getFilePath
  inline UTF8Path const &getFilePath() const { return fFilePath; }

  // toBuffers
  std::unique_ptr<SampleBuffers32> toBuffers(SampleRate iSampleRate) const;
  std::unique_ptr<SampleBuffers32> toBuffers() const;

  // getSampleInfo
  tresult getSampleInfo(SampleInfo &oSampleInfo) const;

  /**
 * Retrieves information about the sample (without reading the whole buffer)
 *
 * @return the info about the sample if could read it, `nullptr` otherwise
 */
  virtual std::unique_ptr<SampleInfo> getSampleInfo() const
  {
    SampleInfo sampleInfo;
    if(getSampleInfo(sampleInfo) == kResultOk)
      return std::make_unique<SampleInfo>(sampleInfo);
    else
      return nullptr;
  }

  // create / factory methods
  static std::unique_ptr<SampleFile> create(UTF8Path const &iFromFilePath);
  static std::unique_ptr<SampleFile> create(UTF8Path const &iToFilePath,
                                            SampleBuffers32 const &iSampleBuffers,
                                            bool iTemporaryFile,
                                            ESampleMajorFormat iMajorFormat,
                                            ESampleMinorFormat iMinorFormat);
  static std::unique_ptr<SampleFile> create(IBStreamer &iFromStream, UTF8Path const &iFromFilePath, uint64 iFileSize);

  // extracts the filename portion of the file path
  static UTF8Path extractFilename(UTF8Path const &iFilePath);

private:
  UTF8Path fFilePath;
  bool fTemporary;
  uint64 fFileSize;
  SampleInfo fSampleInfoCache{}; // will cache the value once read
};

}
}
}

