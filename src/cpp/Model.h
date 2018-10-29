#pragma once

#include <pluginterfaces/base/ftypes.h>

#include <pongasoft/VST/ParamConverters.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

enum ENumSlices
{
  // Number of slice is 2 ^ ENumSlices (shortcut 1 << ENumSlices)

  kNumSlice1, // 2^0
  kNumSlice2, // 2^1
  kNumSlice4, // ...
  kNumSlice8,
  kNumSlice16,
  kNumSlice32,
  kNumSlice64,
};

constexpr int numSlices(ENumSlices iNumSlices) noexcept { return 1 << iNumSlices; }

}
}
}