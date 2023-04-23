/*
 * Copyright (c) 2023 pongasoft
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

#ifndef VST_SAM_SPL_64_SAMPLE_FILE_LOADER_H
#define VST_SAM_SPL_64_SAMPLE_FILE_LOADER_H

#include <memory>
#include <variant>
#include <optional>
#include "../FilePath.h"
#include <pluginterfaces/vst/vsttypes.h>

using namespace Steinberg;

namespace pongasoft::VST::SampleSplitter {
template<typename SampleType>
class SampleBuffers;
using SampleBuffers32 = SampleBuffers<Vst::Sample32>;
}

namespace pongasoft::VST::SampleSplitter::GUI {

/**
 * The point of this (abstract) class is to allow for different loaders */
class SampleFileLoader
{
public:
  struct SampleInfo
  {
    Vst::SampleRate fSampleRate;
    int32 fNumChannels;
    int32 fNumSamples; // an int32 can contain over 3h worth of samples at 192000

    constexpr int64 getTotalSize() const { return static_cast<int64>(fNumChannels) * static_cast<int64>(fNumSamples); }
  };

public:
  using load_result_t = std::variant<std::unique_ptr<SampleBuffers32>, std::string>;

public:
  virtual ~SampleFileLoader() = default;

  virtual bool isValid() const = 0;
  virtual std::string error() const = 0;
  virtual load_result_t load() = 0;
  virtual std::optional<SampleInfo> info() = 0;

  static std::unique_ptr<SampleFileLoader> create(UTF8Path const &iFilePath);

  // Checks if the file is supported by Sndfile or miniaudio
  static inline bool isSupportedFileType(UTF8Path const &iFilePath) { return create(iFilePath)->isValid(); }
};



}

#endif //VST_SAM_SPL_64_SAMPLE_FILE_LOADER_H