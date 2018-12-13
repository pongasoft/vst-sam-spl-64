#pragma once

#include <vector>
#include "SampleBuffers.h"
#include <sndfile.hh>
#include <pongasoft/Utils/Constants.h>
#include <pongasoft/VST/AudioUtils.h>
#include <CDSPResampler.h>
#include <cmath>

#define DEBUG_SAMPLE_BUFFER_MEMORY 0

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

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
// SampleBuffers::SampleBuffers (move constructor)
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::SampleBuffers(SampleBuffers &&other) noexcept
{
//  DLOG_F(INFO, "SampleBuffers(&&%p, %f, %d, %d) : %p", &other, other.fSampleRate, other.fNumChannels, other.fNumSamples, this);

  fSampleRate = other.fSampleRate;
  fNumChannels = other.fNumChannels;
  fNumSamples = other.fNumSamples;
  fSamples = std::move(other.fSamples);

#if DEBUG_SAMPLE_BUFFER_MEMORY
  if(hasSamples())
    DLOG_F(INFO, "SampleBuffers | [%p] -> [%p] (%d)", &other, this, fNumChannels * fNumSamples);
#endif

  other.fNumChannels = 0;
  other.fNumSamples = 0;
  other.fSamples = nullptr;
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
// SampleBuffers::operator=
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType> &SampleBuffers<SampleType>::operator=(SampleBuffers &&other) noexcept
{
//  DLOG_F(INFO, "SampleBuffers[%p]::=(&&%p, %f, %d, %d)", this, &other, other.fSampleRate, other.fNumChannels, other.fNumSamples);

  deleteBuffers();

  fSampleRate = other.fSampleRate;
  fNumChannels = other.fNumChannels;
  fNumSamples = other.fNumSamples;
  fSamples = std::move(other.fSamples);

#if DEBUG_SAMPLE_BUFFER_MEMORY
  if(hasSamples())
    DLOG_F(INFO, "SampleBuffers | [%p] -> [%p] (%d)", &other, this, fNumChannels * fNumSamples);
#endif

  other.fNumChannels = 0;
  other.fNumSamples = 0;
  other.fSamples = nullptr;

  return *this;
}

//------------------------------------------------------------------------
// SampleBuffers::copyFrom
//------------------------------------------------------------------------
template<typename SampleType>
void SampleBuffers<SampleType>::copyFrom(SampleBuffers const &other, int32 iNumSamples)
{
//  DLOG_F(INFO, "SampleBuffers[%p]::copyFrom(%p, %f, %d, %d, %d)",
//         this, &other, other.fSampleRate, other.fNumChannels, other.fNumSamples, iNumSamples);

  fSampleRate = other.fSampleRate;

  resize(other.fNumChannels, std::min(other.fNumSamples, iNumSamples));

  if(fSamples && fNumSamples > 0)
  {
    for(int32 c = 0; c < fNumChannels; c++)
    {
      std::copy(other.fSamples[c], other.fSamples[c] + fNumSamples, fSamples[c]);
    }
  }
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

  fNumChannels = std::max(iNumChannels, 0);
  fNumSamples = std::max(iNumSamples, 0);

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

constexpr sf_count_t BUFFER_SIZE_FRAMES = 1024;

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
                                                 std::vector<Sample32> &oMin,
                                                 std::vector<Sample32> &oMax,
                                                 int32 iStartOffset,
                                                 int32 iNumSamplesPerBucket,
                                                 int32 iNumBuckets) const
{
  if(!hasSamples() || iNumSamplesPerBucket <= 0 || iNumBuckets <= 0)
    return -1;

  auto samples = getChannelBuffer(iChannel);
  if(!samples)
    return -1;

  auto max = std::numeric_limits<Sample32>::min();
  auto min = std::numeric_limits<Sample32>::max();

  int bucketIndex = 0;

  int32 numBuckets = 0;

  for(int inputIndex = iStartOffset; inputIndex < fNumSamples; inputIndex++, bucketIndex++)
  {
    if(bucketIndex == iNumSamplesPerBucket)
    {
      bucketIndex = 0;
      oMin.push_back(min);
      oMax.push_back(max);
      numBuckets++;
      if(numBuckets == iNumBuckets)
        break;
      max = std::numeric_limits<Sample32>::min();
      min = std::numeric_limits<Sample32>::max();
    }

    auto sample = samples[inputIndex];
    min = std::min(min, sample);
    max = std::max(max, sample);
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

}
}
}