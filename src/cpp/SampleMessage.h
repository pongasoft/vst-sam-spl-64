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

#ifndef SAMSPL64_SAMPLEMESSAGE_H
#define SAMSPL64_SAMPLEMESSAGE_H

#include <pongasoft/VST/ParamSerializers.h>
#include <pongasoft/VST/GUI/Params/GUIParamSerializers.h>
#include <pongasoft/Utils/Disposable.h>
#include "SampleBuffers.h"
#include "FilePath.h"

namespace pongasoft::VST::SampleSplitter {

/**
 * This class represents the message sent from the GUI to the RT when a new sample is loaded (or when the current one
 * is edited). The previous implementation was sending `SampleBuffers32` directly but it turns out to be highly
 * inefficient (especially if the samples are big see https://github.com/pongasoft/vst-sam-spl-64/issues/1),
 * due to a big number of copies involved because of the nature of messaging in VST (no way to simply "share"
 * a pointer). So when this class is serialized (see `GUISampleMessageSerializer::writeToStream`) the
 * buffers are omitted. When it is deserialized (see `GUISampleMessageSerializer::readFromStream`), the buffers are
 * read from the file provided (one way to look at it, is that the file serves the purpose of the pointer that we
 * cannot share). Although the deserialization happens on the RT side, it still happens in a GUI thread so accessing
 * the file system at this moment is not an issue.
 */
struct GUISampleMessage : public Utils::Disposable
{
  GUISampleMessage() = default;

  // Required for API (don't copy buffers)
  GUISampleMessage(GUISampleMessage const &iOther);

  //! Ensure the the buffers are cleaned
  void dispose() override;

  UTF8Path fFilePath{};
  uint64 fFileSize{};
  SampleRate fSampleRate{};
  std::unique_ptr<SampleBuffers32> fBuffers{};
};

/**
 * Serialized for `GUISampleMessage`
 */
class GUISampleMessageSerializer : public IParamSerializer<GUISampleMessage>
{
public:
  using ParamType = GUISampleMessage;

  // readFromStream
  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override;

  // writeToStream
  tresult writeToStream(const ParamType &iValue, IBStreamer &oStreamer) const override;

private:
  GUI::Params::UTF8StringParamSerializer<512> fFilePathSerializer{};
};

}

#endif //SAMSPL64_SAMPLEMESSAGE_H