#pragma once

#include "Sampler.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// Sampler::start
//------------------------------------------------------------------------
template<typename SampleType>
void Sampler<SampleType>::start(bool iResetCurrent)
{
  fState = ESamplerState::kSampling;
  if(iResetCurrent)
    fCurrent = 0;
}

//------------------------------------------------------------------------
// Sampler::init
//------------------------------------------------------------------------
template<typename SampleType>
void Sampler<SampleType>::init(SampleRate iSampleRate, int32 iMaxSamples)
{
  fBuffers = std::make_unique<SampleBuffersT>(iSampleRate, fNumChannels, iMaxSamples);
  fCurrent = 0;
  fState = ESamplerState::kNotSampling;
}

//------------------------------------------------------------------------
// Sampler::getPercentSampled
//------------------------------------------------------------------------
template<typename SampleType>
float Sampler<SampleType>::getPercentSampled() const
{
  if(fState == ESamplerState::kSampling)
  {
    return fCurrent / static_cast<float>(fBuffers->getNumSamples());
  }
  return 0;
}


//------------------------------------------------------------------------
// Sampler::sample
//------------------------------------------------------------------------
template<typename SampleType>
template<typename InputSampleType>
ESamplerState Sampler<SampleType>::sample(AudioBuffers<InputSampleType> &iIn, int32 iStartOffset, int32 iEndOffset)
{
  if(fState != ESamplerState::kSampling)
    return fState;

  auto localBuffersNumSamples = fBuffers->getNumSamples();
  auto inNumSamples = iIn.getNumSamples();

  if(iStartOffset == -1)
    iStartOffset = 0;

  if(iEndOffset == -1)
    iEndOffset = inNumSamples;

  // making sure offset remain within the bounds
  iStartOffset = clamp(iStartOffset, ZERO_INT32, inNumSamples - 1);
  iEndOffset = clamp(iEndOffset, ZERO_INT32, inNumSamples);

  auto numChannels = std::min(fBuffers->getNumChannels(), iIn.getNumChannels());

  int32 newCurrent = fCurrent;

  for(int32 c = 0; c < numChannels; c++)
  {
    int32 current = fCurrent;

    auto channel = iIn.getAudioChannel(c);
    if(!channel.isActive())
      continue;

    auto audioBuffer = channel.getBuffer(); // we know it is not null here
    auto sampleBuffer = fBuffers->getChannelBuffer(c);

    for(int32 i = iStartOffset; i < iEndOffset; i++)
    {
      if(current == localBuffersNumSamples)
        break;

      sampleBuffer[current++] = static_cast<SampleType>(audioBuffer[i]);
    }

    newCurrent = current;
  }

  fCurrent = newCurrent;

  if(localBuffersNumSamples == fCurrent)
    fState = ESamplerState::kDoneSampling;

  return fState;
}

//------------------------------------------------------------------------
// Sampler::dispose
//------------------------------------------------------------------------
template<typename SampleType>
void Sampler<SampleType>::dispose()
{
  fBuffers = nullptr;
  fCurrent = 0;
  fState = ESamplerState::kNotSampling;
}

//------------------------------------------------------------------------
// Sampler::acquireBuffers
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<typename Sampler<SampleType>::SampleBuffersT> Sampler<SampleType>::acquireBuffers()
{
  if(!fBuffers)
    return nullptr;

  auto buffers = std::make_unique<SampleBuffersT>(fBuffers->getSampleRate(),
                                                  fBuffers->getNumChannels(),
                                                  fBuffers->getNumSamples());

  std::swap(fBuffers, buffers);

  return buffers;
}

}
}
}