#include <src/cpp/SampleBuffers.hpp>
#include <gtest/gtest.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace Test {

using V32 = std::vector<Sample32>;

inline V32 toVector(std::unique_ptr<SampleBuffers32> const &iBuffer, int32 iChannel = 0)
{
  V32 res{};

  if(iBuffer)
  {
    auto b = iBuffer->getChannelBuffer(iChannel);
    std::copy(b, b + iBuffer->getNumSamples(), std::back_inserter(res));
  }

  return res;
}

// Helper function to compare two vectors of floats with ASSERT_FLOAT_EQ
void ASSERT_FLOAT_VECTOR_EQ(V32 const &expected, V32 const &actual)
{
  ASSERT_EQ(expected.size(), actual.size()) << "Vectors are of unequal length";
  for(size_t i = 0; i < expected.size(); ++i)
  {
    ASSERT_FLOAT_EQ(expected[i], actual[i]) << "Vectors differ at index " << i;
  }
}

// SampleBuffers - computeAvg
TEST(SampleBuffers, computeAvg)
{
  constexpr int NUM_SAMPLES = 10;

  SampleBuffers32 sampleBuffers{44100, 1, NUM_SAMPLES};

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    sampleBuffers.getBuffer()[0][i] = i + 1;
  }

  V32 avg, expected;
  avg.reserve(NUM_SAMPLES);

  // making sure it works for bucket of size 1 -> avg is value
  ASSERT_EQ(5, sampleBuffers.computeAvg(0, avg, 0, 1, 5));
  expected = V32{1,2,3,4,5};
  ASSERT_FLOAT_VECTOR_EQ(expected, avg);

  // making sure we can ask for more buckets but get back only what is available
  avg.clear();
  ASSERT_EQ(2, sampleBuffers.computeAvg(0, avg, 8, 1, 5));
  expected = V32{9,10};
  ASSERT_FLOAT_VECTOR_EQ(expected, avg);

  // make sure it works with fractional bucket size
  avg.clear();
  ASSERT_EQ(3, sampleBuffers.computeAvg(0, avg, 0, 1.25, 3));
  expected = V32{static_cast<float>((1.0 * 1.0  + 2.0 * 0.25) / 1.25),
                 static_cast<float>((2.0 * 0.75 + 3.0 * 0.5)  / 1.25),
                 static_cast<float>((3.0 * 0.5  + 4.0 * 0.75) / 1.25)};
  ASSERT_FLOAT_VECTOR_EQ(expected, avg);

  // make sure it works with fractional bucket size when there is less available
  avg.clear();
  ASSERT_EQ(1, sampleBuffers.computeAvg(0, avg, 7, 3.25, 2));
  expected = V32{static_cast<float>((8.0 * 1.0  + 9.0 * 1.0 + 10.0 * 1.0) / 3.0)};
  ASSERT_FLOAT_VECTOR_EQ(expected, avg);

  // make sure it works with fractional bucket size when there is less available
  avg.clear();
  ASSERT_EQ(2, sampleBuffers.computeAvg(0, avg, 8, 1.25, 2));
  expected = V32{static_cast<float>((9.0 * 1.0  + 10.0 * 0.25) / 1.25),
                 static_cast<float>((10.0 * 0.75) / 0.75)};
  ASSERT_FLOAT_VECTOR_EQ(expected, avg);
}

// SampleBuffers - cut
TEST(SampleBuffers, cut)
{
  constexpr int NUM_SAMPLES = 10;

  SampleBuffers32 sampleBuffers{44100, 1, NUM_SAMPLES};

  //  0  1  2  3  4  5  6  7  8  9
  // [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    sampleBuffers.getBuffer()[0][i] = i + 1;
  }

  V32 expected;

  {
    auto b = sampleBuffers.cut(2, 5);
    expected = V32{1,2,6,7,8,9,10};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.cut(5, 6);
    expected = V32{1,2,3,4,5,7,8,9,10};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.cut(0, 6);
    expected = V32{7,8,9,10};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.cut(6, 100);
    expected = V32{1,2,3,4,5,6};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.cut(0, 10);
    expected = V32{};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.cut(-1, 12);
    expected = V32{};
    ASSERT_EQ(expected, toVector(b));
  }

  ASSERT_TRUE(sampleBuffers.cut(-1, -1) == nullptr);
  ASSERT_TRUE(sampleBuffers.cut(5, 5) == nullptr);
  ASSERT_TRUE(sampleBuffers.cut(12, 12) == nullptr);
  ASSERT_TRUE(sampleBuffers.cut(10, 12) == nullptr);
  ASSERT_TRUE(sampleBuffers.cut(-5, 0) == nullptr);

}

// SampleBuffers - crop
TEST(SampleBuffers, crop)
{
  constexpr int NUM_SAMPLES = 10;

  SampleBuffers32 sampleBuffers{44100, 1, NUM_SAMPLES};

  //  0  1  2  3  4  5  6  7  8  9
  // [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    sampleBuffers.getBuffer()[0][i] = i + 1;
  }

  V32 expected;

  {
    auto b = sampleBuffers.crop(2, 5);
    expected = V32{3,4,5};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.crop(5, 6);
    expected = V32{6};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.crop(0, 6);
    expected = V32{1,2,3,4,5,6};
    ASSERT_EQ(expected, toVector(b));
  }

  {
    auto b = sampleBuffers.crop(6, 100);
    expected = V32{7,8,9,10};
    ASSERT_EQ(expected, toVector(b));
  }

  ASSERT_TRUE(sampleBuffers.crop(-1, -1) == nullptr);
  ASSERT_TRUE(sampleBuffers.crop(0, 10) == nullptr);
  ASSERT_TRUE(sampleBuffers.crop(5, 5) == nullptr);
  ASSERT_TRUE(sampleBuffers.crop(12, 12) == nullptr);
  ASSERT_TRUE(sampleBuffers.crop(10, 12) == nullptr);
  ASSERT_TRUE(sampleBuffers.crop(-5, 0) == nullptr);

}


}
}
}
}