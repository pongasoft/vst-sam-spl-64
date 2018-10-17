#pragma once

#include <pongasoft/VST/AudioBuffer.h>
#include <pluginterfaces/base/ftypes.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

struct Sample
{
  uint32 fSampleRate{44100};
  AudioBuffers32 fSamples;
};

// should use AudioBusBuffers

}
}
}