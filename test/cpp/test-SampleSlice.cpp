#include <array>
#include <pongasoft/logging/logging.h>

#include <gtest/gtest.h>

#include <src/cpp/SampleSlice.hpp>
#include <src/cpp/SampleBuffers.hpp>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace Test {

template<int NumChannels, int NumSamples>
struct AudioOut
{
  AudioOut()
  {
    fAudioBusBuffers.numChannels = NumChannels;
    fAudioBusBuffers.channelBuffers32 = new Sample32*[NumChannels];
    for(int i = 0; i < NumChannels; i++)
      fAudioBusBuffers.channelBuffers32[i] = new Sample32[NumSamples]{};
  }

  ~AudioOut()
  {
    for(int i = 0; i < NumChannels; i++)
      delete [] fAudioBusBuffers.channelBuffers32[i];
    delete [] fAudioBusBuffers.channelBuffers32;
  }

  void init(std::array<Sample32, NumSamples> iSamples)
  {
    for(int c = 0; c < NumChannels; c++)
      for(unsigned long i = 0; i < NumSamples; i++)
        fAudioBusBuffers.channelBuffers32[c][i] = iSamples.at(i);
  }

  AudioBuffers32 &getBuffers()
  {
    return fAudioBuffers;
  }

  void testBuffers(std::array<Sample32, NumChannels * NumSamples> iSamples)
  {
    for(int c = 0; c < NumChannels; c++)
      for(int i = 0; i < NumSamples; i++)
        ASSERT_EQ(iSamples.at(c * NumSamples + i), fAudioBusBuffers.channelBuffers32[c][i]);
  }

  AudioBusBuffers fAudioBusBuffers{};
  AudioBuffers32 fAudioBuffers{fAudioBusBuffers, NumSamples};
};

// SampleSlice - play
TEST(SampleSlice, play)
{
  constexpr int NUM_CHANNELS = 2;
  constexpr int NUM_SAMPLES = 10;

  SampleBuffers32 sampleBuffers{44100, NUM_CHANNELS, NUM_SAMPLES};

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    sampleBuffers.getBuffer()[0][i] = i;
    sampleBuffers.getBuffer()[1][i] = -i;
  }

  SampleBuffers32 emptyBuffers{44100, NUM_CHANNELS, NUM_SAMPLES};

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    emptyBuffers.getBuffer()[0][i] = 0;
    emptyBuffers.getBuffer()[1][i] = 0;
  }

  std::array<Sample32, NUM_CHANNELS * 3> silent{0.0,0.0,0.0,0.0,0.0,0.0};

  AudioOut<NUM_CHANNELS, 3> audioOut{};
  audioOut.testBuffers(silent);

  SampleSlice ss;

  // slice represents samples from 0 to 3 (4 samples)
  ss.reset(0, 4);

  // not playing
  ASSERT_EQ(SampleSlice::State::kNotPlaying, ss.getState());

  // play the first 3 samples => still playing
  ss.play(sampleBuffers, audioOut.getBuffers(), true);
  audioOut.testBuffers({ /* L */ 0.0,1.0,2.0, /* R */ 0.0,-1.0f,-2.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ASSERT_EQ(false, audioOut.getBuffers().getLeftChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().getRightChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().isSilent());

  // play the next 2 samples then no more => done playing
  ss.play(sampleBuffers, audioOut.getBuffers(), true);
  audioOut.testBuffers({ /* L */ 3.0,0.0,0.0, /* R */ -3.0f,0.0f,0.0f});
  ASSERT_EQ(SampleSlice::State::kDonePlaying, ss.getState());
  ASSERT_EQ(false, audioOut.getBuffers().getLeftChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().getRightChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().isSilent());

  // it resets itself... play the first 3 samples => still playing
  ss.play(sampleBuffers, audioOut.getBuffers(), true);
  audioOut.testBuffers({ /* L */ 0.0,1.0,2.0, /* R */ 0.0,-1.0f,-2.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ASSERT_EQ(false, audioOut.getBuffers().getLeftChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().getRightChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().isSilent());

  // reset and replay the first 3 samples this time with no override (addition)
  ss.resetCurrent();
  ss.play(sampleBuffers, audioOut.getBuffers(), false);
  audioOut.testBuffers({ /* L */ 0.0,2.0,4.0, /* R */ 0.0f,-2.0f,-4.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ASSERT_EQ(false, audioOut.getBuffers().getLeftChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().getRightChannel().isSilent());
  ASSERT_EQ(false, audioOut.getBuffers().isSilent());

  // play empty buffer
  ss.resetCurrent();
  ss.play(emptyBuffers, audioOut.getBuffers(), true);
  audioOut.testBuffers(silent);
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ASSERT_EQ(true, audioOut.getBuffers().getLeftChannel().isSilent());
  ASSERT_EQ(true, audioOut.getBuffers().getRightChannel().isSilent());
  ASSERT_EQ(true, audioOut.getBuffers().isSilent());

  // set looping mode on
  ss.resetCurrent();
  ss.setLoop(true);
  ss.setPadSelected(true);
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // first 3 samples
  audioOut.testBuffers({ /* L */ 0.0,1.0,2.0, /* R */ 0.0,-1.0f,-2.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // next 1 sample then loop
  audioOut.testBuffers({ /* L */ 3.0,0.0,1.0, /* R */ -3.0f,0.0f,-1.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // next 2 samples then loop
  audioOut.testBuffers({ /* L */ 2.0,3.0,0.0, /* R */ -2.0f,-3.0f,0.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());

  // set looping mode on
  ss.resetCurrent();
  ss.setLoop(true);
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // first 3 samples
  audioOut.testBuffers({ /* L */ 0.0,1.0,2.0, /* R */ 0.0,-1.0f,-2.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // next 1 sample then loop
  audioOut.testBuffers({ /* L */ 3.0,0.0,1.0, /* R */ -3.0f,0.0f,-1.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ss.setPadSelected(false);
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // next 2 samples then finish
  audioOut.testBuffers({ /* L */ 2.0,3.0,0.0, /* R */ -2.0f,-3.0f,0.0f});
  ASSERT_EQ(SampleSlice::State::kDonePlaying, ss.getState());

  // set reverse mode on
  ss.setLoop(false);
  ss.setReverse(true);
  ss.setPadSelected(true);
  ss.reset(0, 5);
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // last 3 samples
  audioOut.testBuffers({ /* L */ 4.0,3.0,2.0, /* R */ -4.0f,-3.0f,-2.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // next samples (reverse)
  audioOut.testBuffers({ /* L */ 1.0,0.0,0.0, /* R */ -1.0f,0.0f,0.0f});
  ASSERT_EQ(SampleSlice::State::kDonePlaying, ss.getState());
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // automatic reset
  audioOut.testBuffers({ /* L */ 4.0,3.0,2.0, /* R */ -4.0f,-3.0f,-2.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());

  // set reverse/loop mode on
  ss.setLoop(true);
  ss.setReverse(true);
  ss.setPadSelected(true);
  ss.reset(0, 5);
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // last 3 samples
  audioOut.testBuffers({ /* L */ 4.0,3.0,2.0, /* R */ -4.0f,-3.0f,-2.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // next samples (reverse and loop)
  audioOut.testBuffers({ /* L */ 1.0,0.0,4.0, /* R */ -1.0f,0.0f,-4.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
  ss.play(sampleBuffers, audioOut.getBuffers(), true); // next samples (reverse and loop)
  audioOut.testBuffers({ /* L */ 3.0,2.0,1.0, /* R */ -3.0f,-2.0f,-1.0f});
  ASSERT_EQ(SampleSlice::State::kPlaying, ss.getState());
}

}
}
}
}
