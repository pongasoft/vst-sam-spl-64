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

#include "SharedObjectMgr.h"
#include "SampleBuffers.h"

namespace pongasoft::VST::SampleSplitter {

template<typename SampleType>
using SharedSampleBuffersMgr = SharedObjectMgr<SampleBuffers<SampleType>, int64>;

using SharedSampleBuffersMgr32 = SharedSampleBuffersMgr<Vst::Sample32>;

using SharedSampleBuffersVersion = SharedSampleBuffersMgr32::version_type;

}

#endif //VST_SAM_SPL_64_SHAREDSAMPLEBUFFERSMGR_H
