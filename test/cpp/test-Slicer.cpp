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
  slicer.reset(buffer, 0, NUM_SAMPLES);
  slicer.start();

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    ASSERT_TRUE(slicer.hasNext());
    ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(i), slicer.next());
  }

  ASSERT_FALSE(slicer.hasNext());

  ////////////
  // range [4, 14]
  slicer.reset(buffer, 4, 14);
  slicer.start();

  for(int32 i = 4; i < 14; i++)
  {
    ASSERT_TRUE(slicer.hasNext());
    ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(i), slicer.next());
  }

  // no more
  ASSERT_FALSE(slicer.hasNext());

  // start
  slicer.start();
  ASSERT_TRUE(slicer.hasNext());
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(4), slicer.next());

  // stop
  ASSERT_TRUE(slicer.requestEnd());
  ASSERT_FALSE(slicer.hasNext());

  // start
  slicer.start();

  // reverse
  ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(4), slicer.next());
  ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(5), slicer.next());
  ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(6), slicer.next());

  slicer.reverse(true);
  ASSERT_TRUE(slicer.hasNext()); // 7 (note that slicer.next has moved the state by 1 already...)
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(7), slicer.next());
  ASSERT_TRUE(slicer.hasNext()); // 6
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(6), slicer.next());
  ASSERT_TRUE(slicer.hasNext()); // 5
  ASSERT_EQ(FIRST_SAMPLE_VALUE + static_cast<Sample32>(5), slicer.next());

}

// Slicer - getSampleWithCrossFade
TEST(Slicer, getSampleWithCrossFade)
{
  constexpr Sample32 FIRST_SAMPLE_VALUE = 5.0; // First Sample Value
  constexpr int NUM_SAMPLES = 20;

  Sample32 buffer[NUM_SAMPLES];

  for(int i = 0; i < NUM_SAMPLES; i++)
    buffer[i] = FIRST_SAMPLE_VALUE + static_cast<Sample32>(i);

  Slicer<Sample32, 5> slicer{}; // cross fade on by default

  ////////////
  // range [4, 16[ => values [9, 21[
  slicer.reset(buffer, 4, 16);

  {
    // single shot
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 4;

    // start with cross fade (5 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.00, slicer.next()); // 9
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.25, slicer.next()); // 10
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.50, slicer.next()); // 11
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.75, slicer.next()); // 12
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 1.00, slicer.next()); // 13
    // no more x fading
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++)       , slicer.next()); // 14
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++)       , slicer.next()); // 15
    // end with cross fade (5 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 1.00, slicer.next()); // 16
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.75, slicer.next()); // 17
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.50, slicer.next()); // 18
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.25, slicer.next()); // 19
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.00, slicer.next()); // 20

    // no more
    ASSERT_FALSE(slicer.hasNext());
  }

  {
    // single shot but ends before the end
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 4;

    // start with cross fade (4 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.00, slicer.next()); // 9  -> 0
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.25, slicer.next()); // 10 -> 2.5
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.50, slicer.next()); // 11 -> 5.5

    // stop
    ASSERT_FALSE(slicer.requestEnd()); // false because it does not stop right away but fades to 0

    // at this stage the cross fade buffer is [0, 2.5, 5.5, 9, 13]
    //                         fCurrent --------------------^
    // the main buffer is [..., 9, 10, 11, 12, 13, 14, 15, ...]
    //        fCurrent --------------------^
    // The cross fade algorithm is thus cross fading [12, 13, 14, 15, 16] * [1.0, 0.75, 0.5, 0.25, 0] with [9, 13, 0, 0, 0]
    // which is cross fading [12, 9.75, 7, 3.75, 0] with [9, 13, 0, 0, 0]
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((9  * 1.0  + 12   * 0.0), slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((13 * 0.75 + 9.75 * 0.25), slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((0  * 0.5  + 7    * 0.5), slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((0  * 0.25 + 3.75 * 0.75), slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ(0, slicer.next());

    // no more
    ASSERT_FALSE(slicer.hasNext());
  }

  {
    // single shot but ends before the end
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 4;

    // start with cross fade (5 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.00, slicer.next()); // 9
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.25, slicer.next()); // 10
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.50, slicer.next()); // 11
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.75, slicer.next()); // 12
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 1.00, slicer.next()); // 13
    // no more x fading
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++)       , slicer.next()); // 14
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++)       , slicer.next()); // 15
    // end with cross fade (5 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 1.00, slicer.next()); // 16

    // since already cross fading to 0, this is essentially a noop
    ASSERT_FALSE(slicer.requestEnd());

    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.75, slicer.next()); // 17
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.50, slicer.next()); // 18
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.25, slicer.next()); // 19
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s++) * 0.00, slicer.next()); // 20

    // no more
    ASSERT_FALSE(slicer.hasNext());
  }

  ////////////
  // Reverse
  slicer.reverse(true);

  {
    // single shot / reverse
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 15;

    // start with cross fade (5 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0   ,slicer.next()); // 20
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.25,slicer.next()); // 19
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.5 ,slicer.next()); // 18
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.75,slicer.next()); // 17
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 1.0 ,slicer.next()); // 16
    // no more x fading
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--)      , slicer.next()); // 15
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--)      , slicer.next()); // 14
    // end with cross fade (5 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 1.00, slicer.next()); // 16
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.75, slicer.next()); // 17
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.50, slicer.next()); // 18
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.25, slicer.next()); // 19
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.00, slicer.next()); // 20

    // no more
    ASSERT_FALSE(slicer.hasNext());
  }

  {
    // single shot but ends before the end
    slicer.start();

    Sample32 s = FIRST_SAMPLE_VALUE + 15;

    // start with cross fade (4 samples)
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.00, slicer.next()); // 20  -> 0
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.25, slicer.next()); // 19 -> 4.75
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((s--) * 0.50, slicer.next()); // 18 -> 5.5

    // stop
    ASSERT_FALSE(slicer.requestEnd()); // false because it does not stop right away but fades to 0

    // at this stage the cross fade buffer is [0, 4.75, 9, 12.75, 16]
    //                         fCurrent ----------------------^
    // the main buffer is [..., 12, 13, 14, 15, 16, 17, 18, 19...]
    //                 fCurrent ---------------------^
    // The cross fade algorithm is thus cross fading [17, 16, 15, 14, 13] * [1.0, 0.75, 0.5, 0.25, 0] with [12.75, 16, 0, 0, 0]
    // which is cross fading [17, 12, 7.5, 3.5, 0] with [12.75, 16, 0, 0, 0]
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((12.75 * 1.0  + 17  * 0.0) , slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((16    * 0.75 + 12  * 0.25), slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((0     * 0.5  + 7.5 * 0.5) , slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ((0     * 0.25 + 3.5 * 0.75), slicer.next());
    ASSERT_TRUE(slicer.hasNext()); ASSERT_EQ(0, slicer.next());

    // no more
    ASSERT_FALSE(slicer.hasNext());
  }

}

}
