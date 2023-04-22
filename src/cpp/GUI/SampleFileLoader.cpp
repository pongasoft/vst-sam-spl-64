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

#include "SampleFileLoader.h"
#include <sndfile.hh>
#include <miniaudio.h>
#include "../SampleBuffers.hpp"
#include <base/source/fstring.h>

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// SndFileLoader
//------------------------------------------------------------------------
class SndFileLoader : public SampleFileLoader
{
public:
  explicit SndFileLoader(UTF8Path const &iFilePath) :
    fSndFile(iFilePath.toNativePath().c_str()),
    fValid{fSndFile.rawHandle() != nullptr},
    fError{fValid ? "" : sf_strerror(nullptr)}
  {
  }

  bool isValid() const override
  {
    return fValid;
  }

  std::string error() const override
  {
    return fError;
  }

  load_result_t load() override;

  std::optional<SampleInfo> info() override;

private:
  SndfileHandle fSndFile;
  bool fValid;
  std::string fError;
};

//------------------------------------------------------------------------
// MiniaudioLoader
//------------------------------------------------------------------------
class MiniaudioLoader : public SampleFileLoader
{
public:
  explicit MiniaudioLoader(UTF8Path const &iFilePath)
  {
    ma_decoder_config config = ma_decoder_config_init_default();
    config.format = ma_format_f32;
    ma_result result = maDecoderInitFile(iFilePath.toNativePath().c_str(), &config, &fDecoder);
    if(result != MA_SUCCESS)
    {
      fError = ma_result_description(result);
    }
    else
      fValid = true;
  }

  ~MiniaudioLoader() override
  {
    if(fValid)
      ma_decoder_uninit(&fDecoder);
  }

  bool isValid() const override
  {
    return fValid;
  }

  std::string error() const override
  {
    return fError;
  }

  load_result_t load() override;

  std::optional<SampleInfo> info() override;

private:
  inline static ma_result maDecoderInitFile(const char* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder) {
    return ma_decoder_init_file(pFilePath, pConfig, pDecoder);
  }

  inline static ma_result maDecoderInitFile(const wchar_t* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder) {
    return ma_decoder_init_file_w(pFilePath, pConfig, pDecoder);
  }

private:
  ma_decoder fDecoder{};
  bool fValid{};
  std::string fError{};
};

//------------------------------------------------------------------------
// InvalidSampleLoader
//------------------------------------------------------------------------
class InvalidSampleLoader : public SampleFileLoader
{
public:
  explicit InvalidSampleLoader(std::string iError) : fError{std::move(iError)} {}

  bool isValid() const override { return false; }
  std::string error() const override { return fError; }
  load_result_t load() override { return fError; }
  std::optional<SampleInfo> info() override { return std::nullopt; }

private:
  std::string fError{};
};

//------------------------------------------------------------------------
// SampleFileLoader::create
//------------------------------------------------------------------------
std::unique_ptr<SampleFileLoader> SampleFileLoader::create(UTF8Path const &iFilePath)
{
  auto sndFileLoader = std::make_unique<SndFileLoader>(iFilePath);
  if(sndFileLoader->isValid())
    return sndFileLoader;

  auto miniAudioLoader = std::make_unique<MiniaudioLoader>(iFilePath);
  if(miniAudioLoader->isValid())
    return miniAudioLoader;

  return std::make_unique<InvalidSampleLoader>(sndFileLoader->error());
}

namespace fmt {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif
/*
 * Copied from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf */
template<typename... Args>
std::string printf(const std::string& format, Args... args )
{
  int size_s = snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
  if(size_s <= 0) { throw std::runtime_error("Error during formatting."); }
  auto size = static_cast<size_t>( size_s );
  auto buf = std::make_unique<char[]>(size);
  snprintf(buf.get(), size_s, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

}

//------------------------------------------------------------------------
// SndFileLoader::load
//------------------------------------------------------------------------
SampleFileLoader::load_result_t SndFileLoader::load()
{
  if(!isValid())
    return fError;

  const auto frameCount = fSndFile.frames();
  const auto channelCount = fSndFile.channels();

  auto totalNumSamples = channelCount * frameCount;

  if(totalNumSamples > Utils::MAX_INT32)
  {
    return fmt::printf("Input file is too big %llu", totalNumSamples);

  }

  auto ptr = std::make_unique<SampleBuffers32>(fSndFile.samplerate(),
                                               channelCount,
                                               frameCount);

  if(ptr->hasSamples())
  {
    auto buffer = ptr->getBuffer();
    std::vector<Vst::Sample32> interleavedBuffer(static_cast<unsigned long>(channelCount * BUFFER_SIZE_FRAMES));

    auto expectedFrames = frameCount;
    bool complete = false;
    int32 sampleIndex = 0;

    while(!complete)
    {
      // read up to BUFFER_SIZE_FRAMES frames
      auto frameCountRead = fSndFile.readf(interleavedBuffer.data(), BUFFER_SIZE_FRAMES);

      // handle error
      if(frameCountRead == 0)
      {
        return fmt::printf("Error while loading sample %d/%s", fSndFile.error(), fSndFile.strError());
      }

      // de-interleave buffer
      auto numSamplesRead = frameCountRead * channelCount;
      int32 channel = 0;
      for(int32 i = 0;  i < numSamplesRead; i++)
      {
        buffer[channel][sampleIndex] = interleavedBuffer[i];
        channel++;
        if(channel == channelCount)
        {
          channel = 0;
          sampleIndex++;
        }
      }

      // adjust number of frames to read
      expectedFrames -= frameCountRead;
      complete = expectedFrames == 0;
    }
  }

  return ptr;
}

//------------------------------------------------------------------------
// SndFileLoader::info
//------------------------------------------------------------------------
std::optional<SampleFileLoader::SampleInfo> SndFileLoader::info()
{
  if(isValid())
  {
    return SampleFileLoader::SampleInfo{
      static_cast<SampleRate>(fSndFile.samplerate()),
      fSndFile.channels(),
      static_cast<int32>(fSndFile.frames())
    };
  }
  else
    return std::nullopt;
}

//------------------------------------------------------------------------
// MiniaudioLoader::load
//------------------------------------------------------------------------
SampleFileLoader::load_result_t MiniaudioLoader::load()
{
  if(!isValid())
    return fError;

  ma_format format;
  ma_uint32 channelCount;
  ma_uint32 sampleRate;
  auto result = ma_decoder_get_data_format(&fDecoder, &format, &channelCount, &sampleRate, nullptr, 0);
  if(result != MA_SUCCESS)
  {
    return fmt::printf("Error extracting data format %d/%s", result, ma_result_description(result));
  }

  ma_uint64 frameCount;
  result = ma_data_source_get_length_in_pcm_frames(&fDecoder, &frameCount);
  if(result != MA_SUCCESS)
  {
    return fmt::printf("Error extracting frameCount %d/%s", result, ma_result_description(result));
  }

  auto totalNumSamples = channelCount * frameCount;

  if(totalNumSamples > Utils::MAX_INT32)
  {
    return fmt::printf("Input file is too big %llu", totalNumSamples);
  }

  auto ptr = std::make_unique<SampleBuffers32>(sampleRate, channelCount, frameCount);

  if(ptr->hasSamples())
  {
    auto buffer = ptr->getBuffer();
    std::vector<Vst::Sample32> interleavedBuffer(static_cast<unsigned long>(channelCount * BUFFER_SIZE_FRAMES));

    auto expectedFrames = frameCount;
    bool complete = expectedFrames == 0;
    int32 sampleIndex = 0;

    while(!complete)
    {
      ma_uint64 frameCountRead;
      result = ma_data_source_read_pcm_frames(&fDecoder, interleavedBuffer.data(), BUFFER_SIZE_FRAMES, &frameCountRead);
      if(result != MA_SUCCESS)
      {
        return fmt::printf("Error while loading sample %d/%s", result, ma_result_description(result));
      }

      // de-interleave buffer
      auto numSamplesRead = frameCountRead * channelCount;
      int32 channel = 0;
      for(int32 i = 0;  i < numSamplesRead; i++)
      {
        buffer[channel][sampleIndex] = interleavedBuffer[i];
        channel++;
        if(channel == channelCount)
        {
          channel = 0;
          sampleIndex++;
        }
      }

      // adjust number of frames to read
      expectedFrames -= frameCountRead;
      complete = expectedFrames == 0;
    }
  }

  return ptr;
}

//------------------------------------------------------------------------
// MiniaudioLoader::info
//------------------------------------------------------------------------
std::optional<SampleFileLoader::SampleInfo> MiniaudioLoader::info()
{
  if(isValid())
  {
    ma_format format;
    ma_uint32 channelCount;
    ma_uint32 sampleRate;
    auto result = ma_decoder_get_data_format(&fDecoder, &format, &channelCount, &sampleRate, nullptr, 0);
    if(result != MA_SUCCESS)
      return std::nullopt;

    ma_uint64 frameCount;
    result = ma_data_source_get_length_in_pcm_frames(&fDecoder, &frameCount);
    if(result != MA_SUCCESS)
      return std::nullopt;

    return SampleFileLoader::SampleInfo{
      static_cast<SampleRate>(sampleRate),
      static_cast<int32>(channelCount),
      static_cast<int32>(frameCount)
    };
  }
  else
    return std::nullopt;
}


}
