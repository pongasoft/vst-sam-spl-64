#pragma once

#include "SampleBuffers.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleBuffers::SampleBuffers
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::SampleBuffers(int32 iSampleRate, int32 iNumChannels, int32 iNumSamples) :
  fSampleRate{iSampleRate},
  fNumChannels{0},
  fNumSamples{0},
  fSamples{nullptr}

{
  DLOG_F(INFO, "SampleBuffers(%d, %d, %d) : %p", iSampleRate, iNumChannels, iNumSamples, this);

  resize(iNumChannels, iNumSamples);
}

//------------------------------------------------------------------------
// SampleBuffers::SampleBuffers
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::SampleBuffers(int32 iSampleRate)  :
  fSampleRate{iSampleRate},
  fNumChannels{0},
  fNumSamples{0},
  fSamples{nullptr}
{
  DLOG_F(INFO, "SampleBuffers(%d) : %p", iSampleRate, this);
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
  DLOG_F(INFO, "SampleBuffers(&%p, %d, %d, %d) : %p", &other, other.fSampleRate, other.fNumChannels, other.fNumSamples, this);

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
  DLOG_F(INFO, "SampleBuffers(&&%p, %d, %d, %d) : %p", &other, other.fSampleRate, other.fNumChannels, other.fNumSamples, this);

  fSampleRate = other.fSampleRate;
  fNumChannels = other.fNumChannels;
  fNumSamples = other.fNumSamples;
  fSamples = std::move(other.fSamples);

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
    DLOG_F(INFO, "SampleBuffers::deleteBuffers(%p, %d)", this, fNumChannels * fNumSamples);

  if(fSamples)
  {
    for(int32 i = 0; i < fNumChannels; i++)
    {
      delete[]fSamples[i];
    }
    delete[]fSamples;
    fSamples = nullptr;
  }

}

//------------------------------------------------------------------------
// SampleBuffers::~deleteBuffers
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType> &SampleBuffers<SampleType>::operator=(SampleBuffers &&other) noexcept
{
  DLOG_F(INFO, "SampleBuffers[%p]::=(&&%p, %d, %d, %d)", this, &other, other.fSampleRate, other.fNumChannels, other.fNumSamples);

  deleteBuffers();

  fSampleRate = other.fSampleRate;
  fNumChannels = other.fNumChannels;
  fNumSamples = other.fNumSamples;
  fSamples = std::move(other.fSamples);

  other.fNumChannels = 0;
  other.fNumSamples = 0;
  other.fSamples = nullptr;

  return *this;
}


//------------------------------------------------------------------------
// SampleBuffers::~SampleBuffers
//------------------------------------------------------------------------
template<typename SampleType>
SampleBuffers<SampleType>::~SampleBuffers()
{
  DLOG_F(INFO, "~SampleBuffers(%p)", this);
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

  DLOG_F(INFO, "SampleBuffers::resize(%p, %d, %d)", this, iNumChannels, iNumSamples);

  fNumChannels = std::max(iNumChannels, 0);
  fNumSamples = std::max(iNumSamples, 0);

  if(iNumChannels > 0)
  {
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
// SampleBuffers::fromInterleaved
//------------------------------------------------------------------------
template<typename SampleType>
std::unique_ptr<SampleBuffers<SampleType>>
SampleBuffers<SampleType>::fromInterleaved(int32 iSampleRate,
                                           int32 iNumChannels,
                                           int32 iTotalNumSamples,
                                           SampleType *iInterleavedSamples)
{

  auto ptr = std::make_unique<SampleBuffers<SampleType>>(iSampleRate,
                                                         iNumChannels,
                                                         iNumChannels > 0 ?  iTotalNumSamples / iNumChannels : 0);

  if(iNumChannels > 0 && iTotalNumSamples > 0)
  {
    auto buffer = ptr->getBuffer();
    int32 channel = 0;
    for(int32 i = 0, j = 0;  i < iTotalNumSamples; i++)
    {
      buffer[channel][j] = iInterleavedSamples[i];
      channel++;
      if(channel == iNumChannels)
      {
        channel = 0;
        j++;
      }
    }
  }

  return ptr;
}

}
}
}