#pragma once

#define NOMINMAX

#include <vector>
#include "SampleBuffers.h"
#include <sndfile.hh>
#include <pongasoft/Utils/Constants.h>
#include <pongasoft/VST/AudioUtils.h>
#include <CDSPResampler.h>
#include <algorithm>
#include <pongasoft/Utils/Misc.h>
#include <miniaudio.h>

#define DEBUG_SAMPLE_BUFFER_MEMORY 0

namespace pongasoft::VST::SampleSplitter {

//------------------------------------------------------------------------
// SampleBuffers::SampleBuffers
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::SampleBuffers(SampleRate iSampleRate, int32 iNumChannels, int32 iNumSamples) :
  fSampleRate{iSampleRate},
  fNumChannels{0},
  fNumSamples{0},
  fSamples{nullptr}

{
//  DLOG_F(INFO, "SampleBuffers(%f, %d, %d) : %p", iSampleRate, iNumChannels, iNumSamples, this);

  resize(iNumChannels, iNumSamples);
}

//------------------------------------------------------------------------
// SampleBuffers::SampleBuffers
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::SampleBuffers(SampleRate iSampleRate)  :
  fSampleRate{iSampleRate},
  fNumChannels{0},
  fNumSamples{0},
  fSamples{nullptr}
{
//  DLOG_F(INFO, "SampleBuffers(%f) : %p", iSampleRate, this);
}

//------------------------------------------------------------------------
// SampleBuffers::SampleBuffers (copy constructor)
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::SampleBuffers(SampleBuffers const &other) :
  fSampleRate{other.fSampleRate},
  fNumChannels{0},
  fNumSamples{0},
  fSamples{nullptr}
{
//  DLOG_F(INFO, "SampleBuffers(&%p, %f, %d, %d) : %p", &other, other.fSampleRate, other.fNumChannels, other.fNumSamples, this);

  resize(other.fNumChannels, other.fNumSamples);

  if(fSamples && fNumSamples > 0)
  {
    for(int32 c = 0; c < fNumChannels; c++)
    {
      std::copy(other.fSamples[c], other.fSamples[c] + fNumSamples, fSamples[c]);
    }
  }
}

//------------------------------------------------------------------------
// SampleBuffers::~deleteBuffers
//------------------------------------------------------------------------
template<typename SampleType>
void SampleBuffers<SampleType>::deleteBuffers()
{
  if(fSamples)
  {
//    DLOG_F(INFO, "SampleBuffers::deleteBuffers(%p, %d)", this, fNumChannels * fNumSamples);

#if DEBUG_SAMPLE_BUFFER_MEMORY
    if(hasSamples())
      DLOG_F(INFO, "SampleBuffers | [%p] -%d", this, fNumChannels * fNumSamples);
#endif

    for(int32 i = 0; i < fNumChannels; i++)
    {
      delete[]fSamples[i];
    }
    delete[]fSamples;
    fSamples = nullptr;
  }

  fNumChannels = 0;
  fNumSamples = 0;
}

//------------------------------------------------------------------------
// SampleBuffers::~SampleBuffers
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::~SampleBuffers()
{
//  DLOG_F(INFO, "~SampleBuffers(%p)", this);
  deleteBuffers();
}

//------------------------------------------------------------------------
// SampleBuffers::dispose()
//------------------------------------------------------------------------
template<typename SampleType>
void SampleBuffers<SampleType>::dispose()
{
  deleteBuffers();
}

//------------------------------------------------------------------------
// SampleBuffers::resize
//------------------------------------------------------------------------
template<typename SampleType>
void SampleBuffers<SampleType>::resize(int32 iNumChannels, int32 iNumSamples)
{
  // nothing to do when sizes are the same
  if(iNumChannels == fNumChannels && iNumSamples == fNumSamples)
    return;

  deleteBuffers();

//  DLOG_F(INFO, "SampleBuffers::resize(%p, %d, %d)", this, iNumChannels, iNumSamples);

  fNumChannels = std::max(iNumChannels, Utils::ZERO_INT32);
  fNumSamples = std::max(iNumSamples, Utils::ZERO_INT32);

  if(fNumChannels > 0)
  {
#if DEBUG_SAMPLE_BUFFER_MEMORY
    if(hasSamples())
      DLOG_F(INFO, "SampleBuffers | [%p] +%d", this, fNumChannels * fNumSamples);
#endif

    fSamples = new SampleType *[fNumChannels];
    for(int32 i = 0; i < fNumChannels; i++)
    {
      if(fNumSamples > 0)
      {
        fSamples[i] = new SampleType[fNumSamples];
      }
      else
        fSamples[i] = nullptr;
    }

  }
  else
    fSamples = nullptr;
}

//------------------------------------------------------------------------
// SampleBuffers::first
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::first(int32 iNumSamples) const
{
  if(iNumSamples < 0)
    return nullptr;

  // making sure the number of sample is not too big
  iNumSamples = std::min(iNumSamples, fNumSamples);

  auto ptr = std::make_unique<SampleBuffers<SampleType>>(fSampleRate, fNumChannels, iNumSamples);

  if(fSamples && fNumSamples > 0)
  {
    for(int32 c = 0; c < fNumChannels; c++)
    {
      std::copy(fSamples[c], fSamples[c] + iNumSamples, ptr->fSamples[c]);
    }
  }

  return ptr;
}

//------------------------------------------------------------------------
// SampleBuffers::resample
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::resample(SampleRate iSampleRate) const
{
  if(fSampleRate == iSampleRate)
  {
    DLOG_F(WARNING, "Calling resample with same sample rate");
    return nullptr;
  }

  if(iSampleRate <= 0)
  {
    DLOG_F(ERROR, "Calling resample with <= 0 sample rate");
    return nullptr;
  }

  constexpr int BUFFER_SIZE = 1024;

  // holds the samples for the library (must be doubles)
  double tmpBuffer[BUFFER_SIZE];

  auto newNumSamples = static_cast<int32>(fNumSamples * iSampleRate / fSampleRate);

  auto ptr = std::make_unique<SampleBuffers<SampleType>>(iSampleRate, fNumChannels, newNumSamples);

  r8b::CDSPResampler24 resampler(fSampleRate, iSampleRate, BUFFER_SIZE);

  for(int32 c = 0; c < fNumChannels; c++)
  {
    // according to the doc: "It is more efficient to clear the state of the resampler object than to destroy it
    // and create a new object"
    resampler.clear();

    auto thisBuffer = getChannelBuffer(c);
    auto newBuffer = ptr->getChannelBuffer(c);
    int32 inSampleIndex = 0;
    int32 outSampleIndex = 0;

    while(outSampleIndex < newNumSamples)
    {
      for(int i = 0; i < BUFFER_SIZE; i++)
      {
        if(inSampleIndex < fNumSamples)
          tmpBuffer[i] = static_cast<double>(thisBuffer[inSampleIndex++]);
        else
        {
          // although not very well documented, we may need to consume more samples (set to 0) to
          // achieve newNumSamples in the output (the code for the oneshot method demonstrates this pattern):
          // 			if( iplen == 0 )
          //			{
          //				rc = MaxInLen;
          //				p = (double*) &ZeroBuf[ 0 ];
          //			}
          tmpBuffer[i] = 0;
        }
      }

      double *out;
      auto count = resampler.process(tmpBuffer, BUFFER_SIZE, out);

      for(int i = 0; i < count; i++)
      {
        if(outSampleIndex < newNumSamples)
          newBuffer[outSampleIndex++] = static_cast<SampleType>(out[i]);
        else
          break;
      }
    }
  }

  return ptr;
}

constexpr sf_count_t BUFFER_SIZE_FRAMES = 6170;

//------------------------------------------------------------------------
// SampleBuffers::load
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::load(SndfileHandle &iFileHandle)
{
  if(!iFileHandle.rawHandle())
    return nullptr;

  const auto frameCount = iFileHandle.frames();
  const auto channelCount = iFileHandle.channels();

  auto totalNumSamples = channelCount * frameCount;

  if(totalNumSamples > Utils::MAX_INT32)
  {
    LOG_F(ERROR, "Input file is too big %llu", totalNumSamples);
    return nullptr;
  }

  auto ptr = std::make_unique<SampleBuffers<SampleType>>(iFileHandle.samplerate(),
                                                         channelCount,
                                                         frameCount);
  
  if(ptr->hasSamples())
  {
    auto buffer = ptr->getBuffer();
    std::vector<SampleType> interleavedBuffer(static_cast<unsigned long>(channelCount * BUFFER_SIZE_FRAMES));
    
    auto expectedFrames = frameCount;
    bool complete = false;
    int32 sampleIndex = 0;

    while(!complete)
    {
      // read up to BUFFER_SIZE_FRAMES frames
      auto frameCountRead = iFileHandle.readf(interleavedBuffer.data(), BUFFER_SIZE_FRAMES);

      // handle error
      if(frameCountRead == 0)
      {
        LOG_F(ERROR, "Error while loading sample %d/%s", iFileHandle.error(), iFileHandle.strError());
        return nullptr;
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
// SampleBuffers::load
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::load(ma_decoder &iDecoder)
{
  ma_format format;
  ma_uint32 channelCount;
  ma_uint32 sampleRate;
  auto result = ma_decoder_get_data_format(&iDecoder, &format, &channelCount, &sampleRate, nullptr, 0);
  if(result != MA_SUCCESS)
  {
    LOG_F(ERROR, "Error extracting data format %d/%s", result, ma_result_description(result));
    return nullptr;
  }

  ma_uint64 frameCount;
  result = ma_data_source_get_length_in_pcm_frames(&iDecoder, &frameCount);
  if(result != MA_SUCCESS)
  {
    LOG_F(ERROR, "Error extracting frameCount %d/%s", result, ma_result_description(result));
    return nullptr;
  }

  auto totalNumSamples = channelCount * frameCount;

  if(totalNumSamples > Utils::MAX_INT32)
  {
    LOG_F(ERROR, "Input file is too big %llu", totalNumSamples);
    return nullptr;
  }

  auto ptr = std::make_unique<SampleBuffers<SampleType>>(sampleRate, channelCount, frameCount);

  if(ptr->hasSamples())
  {
    auto buffer = ptr->getBuffer();
    std::vector<SampleType> interleavedBuffer(static_cast<unsigned long>(channelCount * BUFFER_SIZE_FRAMES));

    auto expectedFrames = frameCount;
    bool complete = expectedFrames == 0;
    int32 sampleIndex = 0;

    while(!complete)
    {
      ma_uint64 frameCountRead;
      result = ma_data_source_read_pcm_frames(&iDecoder, interleavedBuffer.data(), BUFFER_SIZE_FRAMES, &frameCountRead);
      if(result != MA_SUCCESS)
      {
        LOG_F(ERROR, "Error while loading sample %d/%s", result, ma_result_description(result));
        return nullptr;
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
// SampleBuffers::save
//------------------------------------------------------------------------
template<typename SampleType>
tresult SampleBuffers<SampleType>::save(SndfileHandle &iFileHandle) const
{
  if(!iFileHandle.rawHandle())
    return kInvalidArgument;

  std::vector<SampleType> interleavedBuffer(static_cast<unsigned long>(fNumChannels * BUFFER_SIZE_FRAMES));

  auto framesToWrite = fNumSamples;
  bool complete = false;
  int32 sampleIndex = 0;

  while(!complete)
  {
    // fill interleaved buffer
    auto numFrames = std::min(framesToWrite, static_cast<int32>(BUFFER_SIZE_FRAMES));
    auto numSamplesWrite = numFrames * fNumChannels;

    int32 channel = 0;
    for(int32 i = 0;  i < numSamplesWrite; i++)
    {
      interleavedBuffer[i] = fSamples[channel][sampleIndex];
      channel++;
      if(channel == fNumChannels)
      {
        channel = 0;
        sampleIndex++;
      }
    }

    auto writeCount = iFileHandle.writef(interleavedBuffer.data(), numFrames);
    if(writeCount != numFrames)
    {
      LOG_F(ERROR, "Error while writing sample %d/%s", iFileHandle.error(), iFileHandle.strError());
      return kResultFalse;
    }

    framesToWrite -= numFrames;
    complete = framesToWrite == 0;
  }

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleBuffers::convert
//------------------------------------------------------------------------
template<typename SampleType>
template<typename ToSampleType>
std::unique_ptr<SampleBuffers<ToSampleType>> SampleBuffers<SampleType>::convert() const
{
  auto ptr = std::make_unique<SampleBuffers<ToSampleType>>(fSampleRate, fNumChannels, fNumSamples);

  for(int32 c = 0; c < fNumChannels; c++)
  {
    auto bFrom = fSamples[c];
    auto bTo = ptr->getChannelBuffer(c);

    for(int i = 0; i < fNumSamples; i++)
    {
      *bTo++ = static_cast<ToSampleType>(*bFrom++);
    }
  }

  return ptr;
}

//------------------------------------------------------------------------
// SampleBuffers::toMono
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::toMono() const
{
  // already mono?
  if(fNumChannels == 1)
    return std::make_unique<SampleBuffers<SampleType>>(*this);

  auto ptr = std::make_unique<SampleBuffers<SampleType>>(fSampleRate, 1, fNumSamples);

  if(ptr->hasSamples())
  {
    auto out = ptr->getChannelBuffer(0);

    for(int i = 0; i < fNumSamples; i++)
    {
      SampleType s = 0;

      for(int c = 0; c < fNumChannels; c++)
        s += fSamples[c][i];

      out[i] = s / fNumChannels;
    }
  }

  return ptr;
}

//------------------------------------------------------------------------
// SampleBuffers::computeMinMax
//------------------------------------------------------------------------
template<typename SampleType>
int32 SampleBuffers<SampleType>::computeMinMax(int32 iChannel,
                                               std::vector<SampleType> &oMin,
                                               std::vector<SampleType> &oMax,
                                               int32 iStartOffset,
                                               double iNumSamplesPerBucket,
                                               int32 iNumBuckets,
                                               int32 *oEndOffset) const
{
  if(!hasSamples() || iNumSamplesPerBucket <= 0 || iNumBuckets <= 0)
    return -1;

  auto samples = getChannelBuffer(iChannel);
  if(!samples)
    return -1;

  iStartOffset = Utils::clamp(iStartOffset, Utils::ZERO_INT32, fNumSamples);

  Sample32 min{}, max{};

  int32 bucketIndex = 0;

  double numSamplesInBucket = iNumSamplesPerBucket;

  int32 numBuckets = 0;

  bool initMinMax{true};

  for(int inputIndex = iStartOffset; inputIndex < fNumSamples; inputIndex++, bucketIndex++)
  {
    auto sample = samples[inputIndex];

    if(initMinMax)
    {
      min = sample;
      max = sample;
      initMinMax = false;
    }

    if(numSamplesInBucket < 1.0)
    {
      if(numSamplesInBucket < 0.5)
      {
        min = std::min(min, sample);
        max = std::max(max, sample);
        oMin.push_back(min);
        oMax.push_back(max);
        if(oEndOffset)
          *oEndOffset = inputIndex;
        initMinMax = true;
      }
      else
      {
        oMin.push_back(min);
        oMax.push_back(max);
        if(oEndOffset)
          *oEndOffset = inputIndex;
        min = sample;
        max = sample;
      }
      numBuckets++;
      if(numBuckets == iNumBuckets)
        break;
      numSamplesInBucket += iNumSamplesPerBucket - 1.0;
    }
    else
    {
      min = std::min(min, sample);
      max = std::max(max, sample);
      numSamplesInBucket -= 1;
    }
  }

  return numBuckets;
}

//------------------------------------------------------------------------
// SampleBuffers::computeAvg
//------------------------------------------------------------------------
template<typename SampleType>
int32 SampleBuffers<SampleType>::computeAvg(int32 iChannel,
                                            std::vector<SampleType> &oAvg,
                                            int32 iStartOffset,
                                            double iNumSamplesPerBucket,
                                            int32 iNumBuckets,
                                            int32 *oEndOffset) const
{
  if(!hasSamples() || iNumSamplesPerBucket <= 0 || iNumBuckets <= 0)
    return -1;

  auto samples = getChannelBuffer(iChannel);
  if(!samples)
    return -1;

  iStartOffset = Utils::clamp(iStartOffset, Utils::ZERO_INT32, fNumSamples);

  double numSamplesInBucket = iNumSamplesPerBucket;

  int32 numBuckets = 0;

  Sample32 avg = 0;

  auto inputIndex = iStartOffset;

  for(; inputIndex < fNumSamples; inputIndex++)
  {
    auto sample = samples[inputIndex];
    if(numSamplesInBucket < 1.0)
    {
      auto partialSample = sample * numSamplesInBucket;
      avg += partialSample;
      avg /= iNumSamplesPerBucket;
      oAvg.push_back(avg);
      if(oEndOffset)
        *oEndOffset = inputIndex;
      avg = sample - partialSample; // remainder of the sample
      numBuckets++;
      if(numBuckets == iNumBuckets)
        break;
      numSamplesInBucket += iNumSamplesPerBucket - 1.0;
    }
    else
    {
      avg += sample;
      numSamplesInBucket -= 1;
    }
  }

  // partial last bucket
  if(numBuckets < iNumBuckets && numSamplesInBucket < iNumSamplesPerBucket)
  {
    avg /= iNumSamplesPerBucket - numSamplesInBucket;
    oAvg.push_back(avg);
    if(oEndOffset)
      *oEndOffset = inputIndex;
    numBuckets++;
  }

  return numBuckets;

}

//------------------------------------------------------------------------
// SampleBuffers::trim
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::trim() const
{
  return trim(getSampleSilentThreshold<SampleType>());
}

//------------------------------------------------------------------------
// SampleBuffers::trim
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::trim(SampleType iSilentThreshold) const
{
  if(!hasSamples())
    return nullptr;

  int32 firstIndex = fNumSamples;

  for(int32 c = 0; c < fNumChannels; c++)
  {
    if(firstIndex == 0)
      break;

    auto ptr = getChannelBuffer(c);

    for(int32 i = 0; i < firstIndex; i++)
    {
      auto value = *ptr++;

      if(value < 0)
        value = -value;

      if(value > iSilentThreshold)
        firstIndex = i;
    }
  }

  int32 lastIndex = -1;

  for(int32 c = 0; c < fNumChannels; c++)
  {
    if(lastIndex == fNumSamples - 1)
      break;

    auto ptr = getChannelBuffer(c) + fNumSamples - 1;

    for(int32 i = fNumSamples - 1; i > lastIndex; i--)
    {
      auto value = *ptr--;

      if(value < 0)
        value = -value;

      if(value > iSilentThreshold)
        lastIndex = i;
    }
  }

  if(firstIndex == 0 && lastIndex == fNumSamples - 1)
    return nullptr;

  if(firstIndex <= lastIndex)
  {
    auto newBuffers = std::make_unique<SampleBuffers<SampleType>>(fSampleRate, fNumChannels, lastIndex - firstIndex + 1);

    for(int32 c = 0; c < fNumChannels; c++)
    {
      auto ptr = getChannelBuffer(c) + firstIndex;
      auto newPtr = newBuffers->getChannelBuffer(c);
      std::copy(ptr, ptr + newBuffers->fNumSamples, newPtr);
    }

    return newBuffers;
  }
  else
  {
    // no samples
    return std::make_unique<SampleBuffers<SampleType>>(fSampleRate, fNumChannels, 0);
  }
}

//------------------------------------------------------------------------
// SampleBuffers::cut
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::cut(int32 iFromIndex, int32 iToIndex) const
{
  if(!hasSamples())
    return nullptr;

  if(iToIndex < 1 || iFromIndex >= fNumSamples)
    return nullptr;

  iFromIndex = Utils::clamp(iFromIndex, Utils::ZERO_INT32, fNumSamples - 1);
  iToIndex = Utils::clamp(iToIndex, Utils::ZERO_INT32, fNumSamples);

  if(iFromIndex >= iToIndex)
    return nullptr;

  auto numSamplesToCut = iToIndex - iFromIndex;

  if(numSamplesToCut >= fNumSamples)
  {
    // cut everything
    return std::make_unique<SampleBuffers<SampleType>>(fSampleRate, fNumChannels, 0);
  }

  auto newBuffers = std::make_unique<SampleBuffers<SampleType>>(fSampleRate, fNumChannels, fNumSamples - numSamplesToCut);

  for(int32 c = 0; c < fNumChannels; c++)
  {
    auto ptr = getChannelBuffer(c);
    auto newPtr = newBuffers->getChannelBuffer(c);
    std::copy(ptr, ptr + iFromIndex, newPtr);
    std::copy(ptr + iToIndex, ptr + fNumSamples, newPtr + iFromIndex);
  }

  return newBuffers;
}

//------------------------------------------------------------------------
// SampleBuffers::crop
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::crop(int32 iFromIndex, int32 iToIndex) const
{
  if(!hasSamples())
    return nullptr;

  if(iToIndex < 1 || iFromIndex >= fNumSamples)
    return nullptr;

  iFromIndex = Utils::clamp(iFromIndex, Utils::ZERO_INT32, fNumSamples - 1);
  iToIndex = Utils::clamp(iToIndex, Utils::ZERO_INT32, fNumSamples);

  if(iFromIndex >= iToIndex)
    return nullptr;

  auto numSamples = iToIndex - iFromIndex;

  // same buffer
  if(numSamples >= fNumSamples)
    return nullptr;

  auto newBuffers = std::make_unique<SampleBuffers<SampleType>>(fSampleRate, fNumChannels, numSamples);

  for(int32 c = 0; c < fNumChannels; c++)
  {
    auto ptr = getChannelBuffer(c);
    auto newPtr = newBuffers->getChannelBuffer(c);
    std::copy(ptr + iFromIndex, ptr + iToIndex, newPtr);
  }

  return newBuffers;
}

//------------------------------------------------------------------------
// SampleBuffers::normalize
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>> SampleBuffers<SampleType>::normalize(SampleType iMaxSample) const
{
  if(!hasSamples())
    return nullptr;

  SampleType absoluteMax = 0;

  for(int32 c = 0; c < fNumChannels; c++)
  {
    auto ptr = getChannelBuffer(c);
    for(int i = 0; i < fNumSamples; i++)
    {
      auto sample = *ptr++;
      if(sample < 0)
        sample = -sample;
      absoluteMax = std::max(absoluteMax, sample);
    }
  }

  if(absoluteMax == iMaxSample || absoluteMax == 0.0)
    return nullptr;

  auto factor = iMaxSample / absoluteMax;

  auto newBuffers = std::make_unique<SampleBuffers<SampleType>>(fSampleRate, fNumChannels, fNumSamples);

  for(int32 c = 0; c < fNumChannels; c++)
  {
    auto ptr = getChannelBuffer(c);
    auto newPtr = newBuffers->getChannelBuffer(c);
    for(int i = 0; i < fNumSamples; i++)
    {
      *newPtr++ = *ptr++ * factor;
    }
  }

  return newBuffers;
}

}