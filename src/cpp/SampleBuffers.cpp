#include "SampleBuffers.hpp"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleBuffersSerializer32::writeToStream
//------------------------------------------------------------------------
tresult SampleBuffersSerializer32::writeToStream(const SampleBuffersSerializer32::ParamType &iValue,
                                                 IBStreamer &oStreamer) const
{
  oStreamer.writeDouble(iValue.fSampleRate);
  oStreamer.writeInt32(iValue.fNumChannels);
  oStreamer.writeInt32(iValue.fNumSamples);

  if(iValue.fNumSamples > 0)
  {
    for(int32 c = 0; c < iValue.fNumChannels; c++)
    {
      oStreamer.writeFloatArray(iValue.getChannelBuffer(c), int32(iValue.fNumSamples));
    }
  }

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleBuffersSerializer32::readFromStream
//------------------------------------------------------------------------
tresult SampleBuffersSerializer32::readFromStream(IBStreamer &iStreamer,
                                                  SampleBuffersSerializer32::ParamType &oValue) const
{
  // DLOG_F(INFO, "SampleBuffersSerializer32::readFromStream(%p) ", &oValue);

  tresult res = IBStreamHelper::readDouble(iStreamer, oValue.fSampleRate);

  if(res == kResultOk)
  {
    int32 numChannels = 0;
    res |= IBStreamHelper::readInt32(iStreamer, numChannels);
    int32 numSamples = 0;
    res |= IBStreamHelper::readInt32(iStreamer, numSamples);

    if(res == kResultOk)
    {
      oValue.resize(numChannels, numSamples);

      if(numSamples > 0)
      {
        for(int32 c = 0; c < numChannels; c++)
        {
          res |= IBStreamHelper::readFloatArray(iStreamer, oValue.getChannelBuffer(c), numSamples);
        }
      }
    }
  }

  return res;
}

}
}
}