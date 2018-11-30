#pragma once

#include <vector>
#include "SampleBuffers.h"
#include <sndfile.hh>
#include <pongasoft/Utils/Constants.h>

#define DEBUG_SAMPLE_BUFFER_MEMORY 1

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
  // TODO Not the correct implementation!!!!!!!

  auto ptr = std::make_unique<SampleBuffers<SampleType>>(*this);

  ptr->fSampleRate = iSampleRate;

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

}
}
}