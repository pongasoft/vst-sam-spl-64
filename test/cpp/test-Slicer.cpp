#include <pluginterfaces/vst/vsttypes.h>

#include <gtest/gtest.h>

#include <src/cpp/Slicer.hpp>

namespace pongasoft::VST::SampleSplitter::Test {

using namespace Steinberg;
using namespace Steinberg::Vst;

// Slicer - getSampleNoCrossFade
TEST(Slicer, getSampleNoCrossFade)
{
  constexpr Sample32 FIRST_SAMPLE_VALUE = 5.0;
  constexpr int NUM_SAMPLES = 20;

  Sample32 buffer[NUM_SAMPLES];

  for(int i = 0; i < NUM_SAMPLES; i++)
    buffer[i] = FIRST_SAMPLE_VALUE + static_cast<Sample32>(i);

  Slicer<Sample32, 4> slicer{};
  slicer.crossFade(false);

  ////////////
  // range [0, NUM_SAMPLES]
  slicer.reset(0, NUM_SAMPLES);
  slicer.start();

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    ASSERT_TRUE(slicer.next());
    ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(i), slicer.getSample(buffer));
  }

  ASSERT_FALSE(slicer.next());
  ASSERT_EQ(0, slicer.getSample(buffer));

  ////////////
  // range [4, 14]
  slicer.reset(4, 14);
  slicer.start();

  for(int32 i = 4; i < 14; i++)
  {
    ASSERT_TRUE(slicer.next());
    ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(i), slicer.getSample(buffer));
  }

  // no more
  ASSERT_FALSE(slicer.next());
  ASSERT_EQ(0, slicer.getSample(buffer));

  // start
  slicer.start();
  ASSERT_TRUE(slicer.next());
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(4), slicer.getSample(buffer));

  // stop
  ASSERT_TRUE(slicer.requestEnd());
  ASSERT_FALSE(slicer.next());
  ASSERT_EQ(0, slicer.getSample(buffer));

  // start
  slicer.start();

  // reverse
  ASSERT_TRUE(slicer.next()); // 4
  ASSERT_TRUE(slicer.next()); // 5
  ASSERT_TRUE(slicer.next()); // 6
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(6), slicer.getSample(buffer));

  slicer.reverse(true);
  ASSERT_TRUE(slicer.next()); // 5
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(5), slicer.getSample(buffer));
  ASSERT_TRUE(slicer.next()); // 4
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(4), slicer.getSample(buffer));

}

// Slicer - getSampleWithCrossFade
TEST(Slicer, getSampleWithCrossFade)
{
  constexpr Sample32 FIRST_SAMPLE_VALUE = 5.0; // First Sample Value
  constexpr int NUM_SAMPLES = 20;

  Sample32 buffer[NUM_SAMPLES];

  for(int i = 0; i < NUM_SAMPLES; i++)
    buffer[i] = FIRST_SAMPLE_VALUE + static_cast<Sample32>(i);

  Slicer<Sample32, 4> slicer{}; // cross fade on by default

  ////////////
  // range [4, 14]
  slicer.reset(4, 14);

  {
    // single shot
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 4;

    // start with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.5, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.75, slicer.getSample(buffer));
    // no more x fading
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++), slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++), slicer.getSample(buffer));
    // end with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.75, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.5, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0, slicer.getSample(buffer));

    // no more
    ASSERT_FALSE(slicer.next());
    ASSERT_EQ(0, slicer.getSample(buffer));
  }

  {
    // single shot but ends before the end
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 4;

    // start with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.5, slicer.getSample(buffer));

    // stop
    ASSERT_FALSE(slicer.requestEnd()); // false because it does not stop right away but fades to 0
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0, slicer.getSample(buffer));

    // no more
    ASSERT_FALSE(slicer.next());
    ASSERT_EQ(0, slicer.getSample(buffer));
  }

  {
    // single shot but ends before the end
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 4;

    // start with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.5, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.75, slicer.getSample(buffer));
    // no more x fading
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++), slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++), slicer.getSample(buffer));
    // end with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.75, slicer.getSample(buffer));

    // since already cross fading to 0, this is essentially a noop
    ASSERT_FALSE(slicer.requestEnd());

    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.5, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s++) * 0, slicer.getSample(buffer));

    // no more
    ASSERT_FALSE(slicer.next());
    ASSERT_EQ(0, slicer.getSample(buffer));
  }

  ////////////
  // Reverse
  slicer.reverse(true);

  {
    // single shot / reverse
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 13;

    // start with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.5, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.75, slicer.getSample(buffer));
    // no more x fading
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--), slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--), slicer.getSample(buffer));
    // end with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.75, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.5, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0, slicer.getSample(buffer));

    // no more
    ASSERT_FALSE(slicer.next());
    ASSERT_EQ(0, slicer.getSample(buffer));
  }

  {
    // single shot but ends before the end
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 13;

    // start with cross fade (4 samples)
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.5, slicer.getSample(buffer));

    // stop
    ASSERT_FALSE(slicer.requestEnd()); // false because it does not stop right away but fades to 0
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0.25, slicer.getSample(buffer));
    ASSERT_TRUE(slicer.next()); ASSERT_EQ((s--) * 0, slicer.getSample(buffer));

    // no more
    ASSERT_FALSE(slicer.next());
    ASSERT_EQ(0, slicer.getSample(buffer));
  }

}

}
