#pragma once

#include <base/source/fstreamer.h>
#include <memory>

#include "SampleBuffers.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

/**
 * Defines the interface to how a sample is stored while the plugin is running (memory or temporary file). The
 * sample is maintained in its original form (ex wav, aiff, etc..). */
class SampleStorage
{
public:
  // Destructor
  virtual ~SampleStorage() = default;

  // The size of the sample (in number of bytes)
  virtual uint64 getSize() const = 0;

  // Called to save the sample to the plugin state
  virtual tresult copyTo(IBStreamer &oStreamer) const = 0;

  // clone
  virtual std::unique_ptr<SampleStorage> clone() const = 0;

  // "reads" the sample and convert it into buffers of individual 32 bits samples
  virtual std::unique_ptr<SampleBuffers32> toBuffers(SampleRate iSampleRate) const = 0;
};

}
}
}