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

#include "SampleMessage.h"
#include "SampleFile.h"
#include <pongasoft/logging/logging.h>

namespace pongasoft::VST::SampleSplitter {

//------------------------------------------------------------------------
// GUISampleMessageSerializer::writeToStream()
//------------------------------------------------------------------------
tresult GUISampleMessageSerializer::writeToStream(GUISampleMessageSerializer::ParamType const &iValue,
                                                  IBStreamer &oStreamer) const
{
  DLOG_F(INFO, "GUISampleMessageSerializer::writeToStream(%s, %llu, %f)", iValue.fFilePath.c_str(), iValue.fFileSize, iValue.fSampleRate);
  tresult res = fFilePathSerializer.writeToStream(iValue.fFilePath.utf8_str(), oStreamer);

  if(res != kResultOk)
    return res;
  if(!oStreamer.writeInt64u(iValue.fFileSize))
    return kResultFalse;
  if(!oStreamer.writeDouble(iValue.fSampleRate))
    return kResultFalse;

  // on purpose we are NOT writing the content of the buffers.. they will be loaded in readFromStream() from
  // the file

  return kResultOk;
}

//------------------------------------------------------------------------
// GUISampleMessageSerializer::readFromStream()
//------------------------------------------------------------------------
tresult GUISampleMessageSerializer::readFromStream(IBStreamer &iStreamer,
                                                   GUISampleMessageSerializer::ParamType &oValue) const
{
  DLOG_F(INFO, "GUISampleMessageSerializer::readFromStream");

  VSTGUI::UTF8String filename;

  auto res = fFilePathSerializer.readFromStream(iStreamer, filename);
  if(res != kResultOk)
    return res;
  oValue.fFilePath = filename.getString();

  res = IBStreamHelper::readInt64u(iStreamer, oValue.fFileSize);
  if(res != kResultOk)
    return res;

  res = IBStreamHelper::readDouble(iStreamer, oValue.fSampleRate);
  if(res != kResultOk)
    return res;

  SampleFile file{oValue.fFilePath, oValue.fFileSize, false};

  oValue.fBuffers = file.toBuffers(oValue.fSampleRate);

  DLOG_F(INFO, "GUISampleMessageSerializer::readFromStream(%s, %llu, %f) => %d",
         oValue.fFilePath.c_str(), oValue.fFileSize, oValue.fSampleRate, oValue.fBuffers ? oValue.fBuffers->getNumSamples() : -1);

  return oValue.fBuffers ? kResultOk : kResultFalse;
}

//------------------------------------------------------------------------
// GUISampleMessage::dispose()
//------------------------------------------------------------------------
void GUISampleMessage::dispose()
{
  fBuffers = nullptr;
}

//------------------------------------------------------------------------
// GUISampleMessage::GUISampleMessage()
//------------------------------------------------------------------------
GUISampleMessage::GUISampleMessage(GUISampleMessage const &iOther) :
  fFilePath{iOther.fFilePath},
  fFileSize{iOther.fFileSize},
  fSampleRate{iOther.fSampleRate}
{

}

}