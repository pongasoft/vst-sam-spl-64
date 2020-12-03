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

#ifndef VST_SAM_SPL_64_CURRENTSAMPLE_H
#define VST_SAM_SPL_64_CURRENTSAMPLE_H

#include "../SampleBuffers.h"

namespace pongasoft::VST::SampleSplitter::GUI {

class CurrentSample
{
public:
  enum class Source
  {
    kUnknown, kFile, kSampling
  };
  enum class UpdateType
  {
    kNone, kAction, kUndo
  };

public:

  CurrentSample() = default; // param API

  CurrentSample(CurrentSample const &iOther) = default; // param API

  CurrentSample(std::shared_ptr<SampleBuffers32> iBuffers,
                SampleRate iOriginalSampleRate,
                Source iSource,
                UpdateType iUpdateType) :
    fBuffers{std::move(iBuffers)}, fOriginalSampleRate{iOriginalSampleRate}, fSource{iSource}, fUpdateType{iUpdateType}
  {
    // empty
  }

  // empty
  inline bool empty() const { return fBuffers == nullptr; }

  // hasSamples
  inline bool hasSamples() const { return !empty() && fBuffers->hasSamples(); }

  // getSampleRate
  inline SampleRate getSampleRate() const { DCHECK_F(!empty()); return fBuffers->getSampleRate(); }

  // getOriginalSampleRate
  inline SampleRate getOriginalSampleRate() const { DCHECK_F(!empty()); return fOriginalSampleRate; }

  // getNumChannels
  inline int32 getNumChannels() const { DCHECK_F(!empty()); return fBuffers->getNumChannels(); };

  // getNumSamples
  inline int32 getNumSamples() const { DCHECK_F(!empty()); return fBuffers->getNumSamples(); }

  // getBuffers
  inline SampleBuffers32 const *getBuffers() const { return fBuffers != nullptr ? fBuffers.get() : nullptr; }

  // getSharedBuffers
  inline std::shared_ptr<SampleBuffers32> getSharedBuffers() const { return fBuffers; }

  // getSource
  inline Source getSource() const { return fSource; }

  // getUpdateType
  inline UpdateType getUpdateType() const { return fUpdateType; }

private:
  std::shared_ptr<SampleBuffers32> fBuffers{};
  SampleRate fOriginalSampleRate{};
  Source fSource{Source::kUnknown};
  UpdateType fUpdateType{UpdateType::kNone};
};



}

#endif //VST_SAM_SPL_64_CURRENTSAMPLE_H