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

#ifndef VST_SAM_SPL_64_SAMPLEFILE_H
#define VST_SAM_SPL_64_SAMPLEFILE_H

#include <base/source/fstreamer.h>

#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/VST/GUI/Params/GUIParamSerializers.h>

#include "../FilePath.h"
#include "../SampleBuffers.h"
#include "../Model.h"

#include <variant>

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace Steinberg;

class SampleFile
{
public:
  enum class ESampleMajorFormat
  {
    kSampleFormatWAV,
    kSampleFormatAIFF
  };

  enum class ESampleMinorFormat
  {
    kSampleFormatPCM16,
    kSampleFormatPCM24,
    kSampleFormatPCM32
  };

public:
  using load_result_t = std::variant<std::unique_ptr<SampleBuffers32>, std::string>;

public:
  SampleFile() = default; // for param API

  SampleFile(SampleFile const &iOther) = default; // for param API

  /**
   * Handle the sample as a (temporary) file which will be deleted when the destructor runs */
  SampleFile(UTF8Path iOriginalFilePath, UTF8Path iTemporaryFilePath, uint64 iFileSize) :
    fOriginalFilePath(std::move(iOriginalFilePath)),
    fTemporaryFile{std::make_shared<TemporaryFile>(iTemporaryFilePath)},
    fFileSize{iFileSize} {}

  // Return `true` if this object is pointing to a valid sample file
  bool empty() const { return fTemporaryFile == nullptr; }

  // getFilePath
  UTF8Path const &getTemporaryFilePath() const { DCHECK_F(!empty()); return fTemporaryFile->fFilePath; }

  // getFilePath
  UTF8Path const &getOriginalFilePath() const { DCHECK_F(!empty()); return fOriginalFilePath; }

  // getFileSize
  uint64 getFileSize() const { return fFileSize; }

  // Loads the sample from the file and make sure it is the proper sample rate
  std::pair<std::shared_ptr<SampleBuffers32>, SampleRate> load(SampleRate iSampleRate, IErrorHandler *iErrorHandler) const;

  // Loads the sample from the file without resampling
  std::unique_ptr<SampleBuffers32> loadOriginal(IErrorHandler *iErrorHandler) const;

  // copyTo
  tresult copyTo(IBStreamer &oStreamer) const;

  // create (from user selected sample)
  static std::unique_ptr<SampleFile> create(UTF8Path const &iFromFilePath);

  // create (from RT sampling)
  static std::unique_ptr<SampleFile> create(UTF8Path const &iOriginalFilePath,
                                            SampleBuffers32 const &iSampleBuffers,
                                            ESampleMajorFormat iMajorFormat = ESampleMajorFormat::kSampleFormatWAV,
                                            ESampleMinorFormat iMinorFormat = ESampleMinorFormat::kSampleFormatPCM24);

  // Saves the sample to a file using the provided formats
  static tresult save(UTF8Path const &iToFilePath,
                      SampleBuffers32 const &iSampleBuffers,
                      ESampleMajorFormat iMajorFormat = ESampleMajorFormat::kSampleFormatWAV,
                      ESampleMinorFormat iMinorFormat = ESampleMinorFormat::kSampleFormatPCM24);


  // create (from when the state is restored)
  static std::unique_ptr<SampleFile> create(IBStreamer &iFromStream, UTF8Path const &iFromFilePath, uint64 iFileSize);

  // extracts the filename portion of the file path
  static UTF8Path extractFilename(UTF8Path const &iFilePath);

  // Computes the size of the given file (-1 if file does not exist)
  static int64 computeFileSize(UTF8Path const &iFilePath);

private:
  struct TemporaryFile
  {
    TemporaryFile(UTF8Path iFilePath) : fFilePath{std::move(iFilePath)} {
      DLOG_F(INFO, "TemporaryFile::TemporaryFile(%s)", fFilePath.c_str());
    }
    ~TemporaryFile();
    UTF8Path fFilePath{};
  };

private:
  UTF8Path fOriginalFilePath;
  std::shared_ptr<TemporaryFile> fTemporaryFile{};
  uint64 fFileSize{};
};

/**
 * Serializes the sample file */
class SampleFileSerializer : public IParamSerializer<SampleFile>
{
public:
  using ParamType = SampleFile;

  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override;

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override;

private:
  VST::GUI::Params::UTF8StringParamSerializer<128> fStringSerializer{};
};

}

#endif //VST_SAM_SPL_64_SAMPLEFILE_H