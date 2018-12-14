#include <src/cpp/SampleBuffers.hpp>
#include <gtest/gtest.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace Test {

// SampleBuffers - computeAvg
TEST(SampleBuffers, computeAvg)
{
  constexpr int NUM_SAMPLES = 10;

  SampleBuffers32 sampleBuffers{44100, 1, NUM_SAMPLES};

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    sampleBuffers.getBuffer()[0][i] = i + 1;
  }

  using V32 = std::vector<Sample32>;

  V32 avg, expected;
  avg.reserve(NUM_SAMPLES);

  // making sure it works for bucket of size 1 -> avg is value
  ASSERT_EQ(5, sampleBuffers.computeAvg(0, avg, 0, 1, 5));
  expected = V32{1,2,3,4,5};
  ASSERT_EQ(expected, avg);

  // making sure we can ask for more buckets but get back only what is available
  avg.clear();
  ASSERT_EQ(2, sampleBuffers.computeAvg(0, avg, 8, 1, 5));
  expected = V32{9,10};
  ASSERT_EQ(expected, avg);

  // make sure it works with fractional bucket size
  avg.clear();
  ASSERT_EQ(3, sampleBuffers.computeAvg(0, avg, 0, 1.25, 3));
  expected = V32{static_cast<float>((1.0 * 1.0  + 2.0 * 0.25) / 1.25),
                 static_cast<float>((2.0 * 0.75 + 3.0 * 0.5)  / 1.25),
                 static_cast<float>((3.0 * 0.5  + 4.0 * 0.75) / 1.25)};
  ASSERT_EQ(expected, avg);

  // make sure it works with fractional bucket size when there is less available
  avg.clear();
  ASSERT_EQ(1, sampleBuffers.computeAvg(0, avg, 7, 3.25, 2));
  expected = V32{static_cast<float>((8.0 * 1.0  + 9.0 * 1.0 + 10.0 * 1.0) / 3.0)};
  ASSERT_EQ(expected, avg);

  // make sure it works with fractional bucket size when there is less available
  avg.clear();
  ASSERT_EQ(2, sampleBuffers.computeAvg(0, avg, 8, 1.25, 2));
  expected = V32{static_cast<float>((9.0 * 1.0  + 10.0 * 0.25) / 1.25),
                 static_cast<float>((10.0 * 0.75) / 0.75)};
  ASSERT_EQ(expected, avg);

}

}
}
}
}