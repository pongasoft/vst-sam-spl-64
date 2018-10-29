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
  // Number of slice is 2 ^ ENumSlices (shortcut 1 << ENumSlices)

  kNumSlice1, // 2^0
  kNumSlice2, // 2^1
  kNumSlice4, // ...
  kNumSlice8,
  kNumSlice16,
  kNumSlice32,
  kNumSlice64,
};

//------------------------------------------------------------------------
// NumSlices
//------------------------------------------------------------------------
constexpr int MAX_SLICES = 64;
constexpr int DEFAULT_NUM_SLICES = 16;
class NumSlices
{
public:
  explicit NumSlices(int iValue = DEFAULT_NUM_SLICES) noexcept : fValue{iValue} {}

  inline int getValue() const { return fValue; }

  static NumSlices create(ENumSlices iNumSlices) noexcept { return NumSlices(1 << iNumSlices); }

private:
  int fValue;
};


//------------------------------------------------------------------------
// NumSlicesParamConverter
//------------------------------------------------------------------------
class NumSlicesParamConverter : public IParamConverter<NumSlices>
{
public:
  int getStepCount() const override { return fEnumConverter.getStepCount(); }

  inline ParamValue normalize(ParamType const &iValue) const override
  {
    int res = -1;
    int x = iValue.getValue();
    while(x) { res++; x = x >> 1; }
    return fEnumConverter.normalize(static_cast<ENumSlices>(res));
  }

  inline ParamType denormalize(ParamValue iNormalizedValue) const override
  {
    return NumSlices::create(fEnumConverter.denormalize(iNormalizedValue));
  }

  inline void toString(ParamType const &iValue, String128 oString, int32 /* iPrecision */) const override
  {
    Steinberg::UString wrapper(oString, str16BufferSize (String128));
    wrapper.printInt(iValue.getValue());
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