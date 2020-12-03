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

#ifndef VST_SAM_SPL_64_SHAREDSAMPLEBUFFERSMGR_H
#define VST_SAM_SPL_64_SHAREDSAMPLEBUFFERSMGR_H

#include <pluginterfaces/vst/vsttypes.h>

#include <pongasoft/Utils/Concurrent/SpinLock.h>

#include "SampleBuffers.h"

namespace pongasoft::VST::SampleSplitter {

/**
 * This class is shared between the UI and the RT. It is owned and created by RT and shared to the UI via pointer
 * messaging!
 */
template<typename SampleType>
class SharedSampleBuffersMgr
{
public:
  inline SharedSampleBuffers<SampleType> getUIBuffers() const
  {
    auto lock = fLock.acquire();
    return fUIBuffers;
  }

  inline SharedSampleBuffersVersion setUIBuffers(SharedSampleBuffers<SampleType> iBuffers)
  {
    auto lock = fLock.acquire();
    fUIBuffers = iBuffers;
    fUIVersion++;
    return fUIVersion;
  }

  SharedSampleBuffers<SampleType> adjustUIBuffers(SharedSampleBuffersVersion iRTVersion)
  {
    auto lock = fLock.acquire();
    if(iRTVersion <= fRTVersion)
    {
      fUIBuffers = fRTBuffers;
      return fUIBuffers;
    }
    return nullptr;
  }

  inline SharedSampleBuffers<SampleType> getRTBuffers() const
  {
    auto lock = fLock.acquire();
    return fRTBuffers;
  }


  inline SharedSampleBuffersVersion setRTBuffers(SharedSampleBuffers<SampleType> iBuffers)
  {
    auto lock = fLock.acquire();
    fRTBuffers = std::move(iBuffers);
    fRTVersion++;
    return fRTVersion;
  }

  inline SharedSampleBuffers<SampleType> adjustRTBuffers(SharedSampleBuffersVersion iUIVersion)
  {
    auto lock = fLock.acquire();
    if(iUIVersion <= fUIVersion)
    {
      fRTBuffers = fUIBuffers;
      return fRTBuffers;
    }
    return nullptr;
  }

private:
  mutable SpinLock fLock{};

  SharedSampleBuffers<SampleType> fUIBuffers{};
  SharedSampleBuffersVersion fUIVersion;

  SharedSampleBuffers<SampleType> fRTBuffers{};
  SharedSampleBuffersVersion fRTVersion;
};

using SharedSampleBuffersMgr32 = SharedSampleBuffersMgr<Vst::Sample32>;

}

#endif //VST_SAM_SPL_64_SHAREDSAMPLEBUFFERSMGR_H
