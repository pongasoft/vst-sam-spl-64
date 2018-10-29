#pragma once

#include <pluginterfaces/base/ftypes.h>

#include <pongasoft/VST/ParamConverters.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

//------------------------------------------------------------------------
// ENumSlices
//------------------------------------------------------------------------
enum ENumSlices
{
  kNumSlice1,
  kNumSlice2,
  kNumSlice4,
  kNumSlice8,
  kNumSlice16,
  kNumSlice32,
  kNumSlice48,
  kNumSlice64,
};

//------------------------------------------------------------------------
// NumSlicesParamConverter
//------------------------------------------------------------------------
constexpr int MAX_SLICES = 64;
constexpr int DEFAULT_NUM_SLICES = 16;
class NumSlicesParamConverter : public IParamConverter<int>
{
public:
  int getStepCount() const override { return fEnumConverter.getStepCount(); }

  inline ParamValue normalize(int const &iValue) const override
  {
    ENumSlices numSlices = ENumSlices::kNumSlice16;
    switch(iValue)
    {
      case 1: numSlices = ENumSlices::kNumSlice1; break;
      case 2: numSlices = ENumSlices::kNumSlice2; break;
      case 3: numSlices = ENumSlices::kNumSlice4; break;
      case 8: numSlices = ENumSlices::kNumSlice8; break;
      case 16: numSlices = ENumSlices::kNumSlice16; break;
      case 32: numSlices = ENumSlices::kNumSlice32; break;
      case 48: numSlices = ENumSlices::kNumSlice48; break;
      case 64: numSlices = ENumSlices::kNumSlice64; break;
      default: ENumSlices::kNumSlice16; break;
    }
    return fEnumConverter.normalize(numSlices);
  }

  inline int denormalize(ParamValue iNormalizedValue) const override
  {
    ENumSlices numSlices = fEnumConverter.denormalize(iNormalizedValue);
    switch(numSlices)
    {
      case ENumSlices::kNumSlice1: return 1;
      case ENumSlices::kNumSlice2: return 2;
      case ENumSlices::kNumSlice4: return 4;
      case ENumSlices::kNumSlice8: return 8;
      case ENumSlices::kNumSlice16: return 16;
      case ENumSlices::kNumSlice32: return 32;
      case ENumSlices::kNumSlice48: return 48;
      case ENumSlices::kNumSlice64: return 64;
      default: return 16;
    }
  }

  inline void toString(ParamType const &iValue, String128 oString, int32 /* iPrecision */) const override
  {
    Steinberg::UString wrapper(oString, str16BufferSize (String128));
    wrapper.printInt(iValue);
  }

  EnumParamConverter<ENumSlices, ENumSlices::kNumSlice64> fEnumConverter;
};

//------------------------------------------------------------------------
// Pads
//------------------------------------------------------------------------
constexpr int NUM_PADS = 16;
constexpr int NUM_PAD_BANKS = 4;

}
}
}