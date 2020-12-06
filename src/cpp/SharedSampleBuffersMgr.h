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
 * messaging! It uses a (spin) lock to swap the shared buffer (and increment the version). This happens
 * only when:
 *
 * - the user samples an input
 * - the user loads a sample (drag & drop or file section)
 * - the plugin is loaded (the sample is saved part of the state)
 *
 * When the buffer changes, there is a message to notify the other side of the change providing the version
 * number of the change (mostly to make sure that things don't get out of sync: if messaging is reliable, meaning
 * messages are delivered quickly and in proper order, this should never happen, but it doesn't cost much to enforce
 * it)
 */
template<typename SampleType>
class SharedSampleBuffersMgr
{
public:
  SharedSampleBuffersMgr()
  {
    DLOG_F(INFO, "SharedSampleBuffersMgr(%p)", this);
  }

  ~SharedSampleBuffersMgr()
  {
    DLOG_F(INFO, "~SharedSampleBuffersMgr(%p)", this);
  }

  // getUIBuffers
  inline SharedSampleBuffers<SampleType> getUIBuffers() const
  {
    auto lock = fLock.acquire();
    return fUIBuffers;
  }

  /**
   * Called from the GUI layer to set the buffers (either from user drag and drop or explicit load, or from
   * loading the sample from the state when the plugin is loaded part of a song).
   */
  inline SharedSampleBuffersVersion setUIBuffers(SharedSampleBuffers<SampleType> iBuffers)
  {
    auto lock = fLock.acquire();
    fUIBuffers = iBuffers;
    fUIVersion++;
    DLOG_F(INFO, "SharedSampleBuffersMgr::setUIBuffers -> %lld", fUIVersion);
    return fUIVersion;
  }

  /**
   * Called from the GUI layer after receiving a notification from the RT that a new sample (from user sampling)
   * is available
   *
   * @see setRTBuffers
   */
  SharedSampleBuffers<SampleType> adjustUIBuffers(SharedSampleBuffersVersion iRTVersion)
  {
    auto lock = fLock.acquire();
    DLOG_F(INFO, "SharedSampleBuffersMgr::adjustUIBuffers(%lld)", iRTVersion);
    if(iRTVersion <= fRTVersion)
    {
      fUIBuffers = fRTBuffers;
      return fUIBuffers;
    }
    return nullptr;
  }

  /**
   * Called from the RT layer to set the buffers (from user sampling).
   */
  inline SharedSampleBuffersVersion setRTBuffers(SharedSampleBuffers<SampleType> iBuffers)
  {
    auto lock = fLock.acquire();
    fRTBuffers = std::move(iBuffers);
    fRTVersion++;
    DLOG_F(INFO, "SharedSampleBuffersMgr::setRTBuffers -> %lld", fRTVersion);
    return fRTVersion;
  }

  /**
   * Called from the RT layer after receiving a notification from the GUI that a new sample (user load or state)
   * is available
   *
   * @see setUIBuffers
   */
  inline SharedSampleBuffers<SampleType> adjustRTBuffers(SharedSampleBuffersVersion iUIVersion)
  {
    auto lock = fLock.acquire();
    DLOG_F(INFO, "SharedSampleBuffersMgr::adjustUIBuffers(%lld)", iUIVersion);
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
  SharedSampleBuffersVersion fUIVersion{};

  SharedSampleBuffers<SampleType> fRTBuffers{};
  SharedSampleBuffersVersion fRTVersion{};
};

using SharedSampleBuffersMgr32 = SharedSampleBuffersMgr<Vst::Sample32>;

}

#endif //VST_SAM_SPL_64_SHAREDSAMPLEBUFFERSMGR_H
