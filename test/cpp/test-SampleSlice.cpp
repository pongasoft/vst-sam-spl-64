#include <array>
#include <pongasoft/logging/logging.h>
#include <pongasoft/Utils/StringUtils.h>

#include <gtest/gtest.h>

#include <src/cpp/SampleSlices.hpp>
#include <src/cpp/SampleBuffers.hpp>
//#include <src/cpp/SampleFile.h>
//#include <pongasoft/Utils/Clock/Clock.h>

namespace pongasoft::VST::SampleSplitter::Test {

template<int NumChannels, int NumSamples>
struct AudioOut
{
  AudioOut()
  {
    fAudioBusBuffers.numChannels = NumChannels;
    fAudioBusBuffers.channelBuffers32 = new Sample32*[NumChannels];
    for(int i = 0; i < NumChannels; i++)
    {
      fAudioBusBuffers.channelBuffers32[i] = new Sample32[NumSamples]{};
      fAudioBuffers.setSilenceFlag(i, true);
    }
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

  AudioBuffers32 &getBuffers(bool clearBuffer = true)
  {
    if(clearBuffer)
      fAudioBuffers.clear();
    return fAudioBuffers;
  }

  void fillBuffers(Sample32 iValue)
  {
    for(int c = 0; c < NumChannels; c++)
    {
      auto channel = fAudioBusBuffers.channelBuffers32[c];
      std::fill(&channel[0], &channel[NumSamples], iValue);
      fAudioBuffers.getAudioChannel(c).setSilenceFlag(isSilent(iValue));
    }
  }

  bool checkBuffers(std::array<Sample32, NumChannels * NumSamples> iSamples)
  {
    bool expectedSilent = true;
    
    for(int c = 0; c < NumChannels; c++)
    {
      bool expectedSilentChannel = true;
      
      for(int i = 0; i < NumSamples; i++)
      {
        auto expectedSample = iSamples.at(c * NumSamples + i);
        if(!isSilent(expectedSample))
        {
          expectedSilentChannel = false;
          expectedSilent = false;
        }
        if(expectedSample != fAudioBusBuffers.channelBuffers32[c][i])
        {
          DLOG_F(ERROR, "Channel=%d, Sample=%d, Expected=%f, Got=%f",
                 c, i, expectedSample, fAudioBusBuffers.channelBuffers32[c][i]);
          return false;
        }
      }

      if(expectedSilentChannel != fAudioBuffers.getAudioChannel(c).isSilent())
      {
        DLOG_F(ERROR, "Channel=%d, expectedSilentChannel=%s, Got=%s",
               c, Utils::to_string(expectedSilentChannel), Utils::to_string(fAudioBuffers.getAudioChannel(c).isSilent()));
        return false;
      }
    }

    if(expectedSilent != fAudioBuffers.isSilent())
    {
      DLOG_F(ERROR, "expectedSilent=%s, Got=%s",
             Utils::to_string(expectedSilent), Utils::to_string(fAudioBuffers.isSilent()));
      return false;
    }

    return true;
  }

  /**
   * Expand the provided buffer from {x,y,z} to {x,y,z,-x,-y,-z} */
  bool checkBuffers2(std::array<Sample32, NumChannels / 2 * NumSamples> iSamples)
  {
    std::array<Sample32, NumChannels * NumSamples> newArray{};

    for(int c = 0; c < NumChannels / 2; c++)
    {
      for(int i = 0; i < NumSamples; i++)
      {
        auto sample = iSamples.at(c * NumSamples + i);
        newArray.at(c * NumSamples + i) = sample;
        newArray.at((c + NumChannels / 2) * NumSamples + i) = -sample;
      }
    }
    return checkBuffers(newArray);
  }

  void appendTo(std::vector<Sample32> &oBuffer)
  {
    auto channel = fAudioBusBuffers.channelBuffers32[0];
    std::copy(&channel[0], &channel[NumSamples], std::back_inserter((oBuffer)));
  }


  AudioBusBuffers fAudioBusBuffers{};
  AudioBuffers32 fAudioBuffers{fAudioBusBuffers, NumSamples};
};

// SampleSlice - playNoCrossFade
TEST(SampleSlice, playNoCrossFade)
{
  constexpr int NUM_CHANNELS = 2;
  constexpr int NUM_SAMPLES = 20;
  constexpr Sample32 FIRST_SAMPLE = 5.0;

  SampleBuffers32 sampleBuffers{44100, NUM_CHANNELS, NUM_SAMPLES};

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    auto sample = static_cast<Sample32>(i) + FIRST_SAMPLE;
    sampleBuffers.getBuffer()[0][i] = sample;
    sampleBuffers.getBuffer()[1][i] = -sample;
  }

  SampleBuffers32 emptyBuffers{44100, NUM_CHANNELS, NUM_SAMPLES};

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    emptyBuffers.getBuffer()[0][i] = 0;
    emptyBuffers.getBuffer()[1][i] = 0;
  }

  std::array<Sample32, NUM_CHANNELS * 3> silent{{0.0,0.0,0.0,0.0,0.0,0.0}};

  AudioOut<NUM_CHANNELS, 3> audioOut{};
  ASSERT_TRUE(audioOut.checkBuffers(silent));

  SampleSlices<> ss;
  // 5 active slices (4 samples each). First slice represents samples from 0 to 3
  ss.setNumActiveSlices(5);
  ss.setCrossFade(false);
  ss.setPolyphonic(true);
  ss.setPlayMode(EPlayMode::kHold);

  ss.setBuffers(&sampleBuffers);

  // play without doing anything => should not play
  ASSERT_FALSE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers(silent));

  // play the first 3 samples of slice 0 => still playing
  ss.setPadSelected(0, true);
  ss.setLoop(0, false); // non looping
  ASSERT_TRUE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 5.0,6.0,7.0, /* R */ -5.0,-6.0f,-7.0f}}));

  // play the next 2 samples then no more => done playing
  ASSERT_TRUE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 8.0,0.0,0.0, /* R */ -8.0f,-0.0f,0.0f}}));

  // nothing else to play
  auto out = audioOut.getBuffers();
  audioOut.fillBuffers(12.345);
  ASSERT_FALSE(ss.play(out));
  // because nothing was played, the buffers have been left untouched
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 12.345,12.345,12.345, /* R */ 12.345,12.345,12.345}}));

  // resets pad 0 and add pad 1
  ss.setPadSelected(0, false);
  ss.setPadSelected(0, true);
  ss.setPadSelected(1, true);

  ASSERT_TRUE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 5.0 + 9.0, 6.0 + 10.0 ,7.0 + 11.0, /* R */ -5.0 - 9.0,-6.0 - 10.0,-7.0 - 11.0}}));

  // stop pad 0. Pad 1 continues.
  ss.setPadSelected(0, false);
  ASSERT_TRUE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 12.0,0.0,0.0, /* R */ -12.0f,-0.0f,0.0f}}));

  // nothing else to play
  ASSERT_FALSE(ss.play(audioOut.getBuffers()));

  // play empty buffer
  ss.setBuffers(&emptyBuffers);

  ss.setPadSelected(0, false);
  ss.setPadSelected(0, true);
  ss.setPadSelected(1, false);

  // although the buffer is silent, it was still "played"
  ASSERT_TRUE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 0.0,0.0,0.0, /* R */ -0.0f,-0.0f,0.0f}}));

  // reset to normal buffer and
  ss.setBuffers(&sampleBuffers);

  {
    // set looping mode on
    ss.setPadSelected(0, false);
    ss.setPadSelected(0, true);
    ss.setLoop(0, true);

    ASSERT_TRUE(ss.play(audioOut.getBuffers())); // first 3 samples
    ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 5.0,6.0,7.0, /* R */ -5.0,-6.0f,-7.0f}}));
    ASSERT_TRUE(ss.play(audioOut.getBuffers())); // next 1 sample then loop
    ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 8.0,5.0,6.0, /* R */ -8.0,-5.0,-6.0f}}));
    ASSERT_TRUE(ss.play(audioOut.getBuffers())); // next 2 sample then loop
    ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 7.0,8.0,5.0, /* R */ -7.0,-8.0,-5.0}}));

    ss.setPadSelected(0, false);
    // nothing else to play
    ASSERT_FALSE(ss.play(audioOut.getBuffers()));
  }

  // set reverse mode on
  ss.setLoop(0, false);
  ss.setReverse(0, true);
  ss.setPadSelected(0, true);

  ASSERT_TRUE(ss.play(audioOut.getBuffers())); // last 3 samples in reverse order
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 8.0,7.0,6.0, /* R */ -8.0,-7.0f,-6.0f}}));
  ASSERT_TRUE(ss.play(audioOut.getBuffers())); // 1 more
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 5.0,0.0,0.0, /* R */ -5.0,-0.0f,-0.0f}}));
  // nothing else to play
  ASSERT_FALSE(ss.play(audioOut.getBuffers()));

  // now looping
  ss.setLoop(0, true);
  ss.setPadSelected(0, false);
  ss.setPadSelected(0, true);

  ASSERT_TRUE(ss.play(audioOut.getBuffers())); // last 3 samples in reverse order
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 8.0,7.0,6.0, /* R */ -8.0,-7.0f,-6.0f}}));
  ASSERT_TRUE(ss.play(audioOut.getBuffers())); // 1 more then loop
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 5.0,8.0,7.0, /* R */ -5.0,-8.0f,-7.0f}}));

  ss.setPadSelected(0, false);
  // nothing else to play
  ASSERT_FALSE(ss.play(audioOut.getBuffers()));

  // reverting to normal play and 1 shot (plays until the end)
  ss.setReverse(0, false);
  ss.setPlayMode(EPlayMode::kTrigger);
  ss.setPadSelected(0, true);

  ASSERT_TRUE(ss.play(audioOut.getBuffers())); // first 3 samples
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 5.0,6.0,7.0, /* R */ -5.0,-6.0f,-7.0f}}));

  // release the pad (but will play until the end)
  ss.setPadSelected(0, false);
  ASSERT_TRUE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 8.0,0.0,0.0, /* R */ -8.0f,-0.0f,0.0f}}));

  // nothing else to play
  ASSERT_FALSE(ss.play(audioOut.getBuffers()));

  // setting looping (which is a noop in 1 shot mode)
  ss.setLoop(0, true);
  ss.setPadSelected(0, true);
  ASSERT_TRUE(ss.play(audioOut.getBuffers())); // first 3 samples
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 5.0,6.0,7.0, /* R */ -5.0,-6.0f,-7.0f}}));
  ASSERT_TRUE(ss.play(audioOut.getBuffers()));
  ASSERT_TRUE(audioOut.checkBuffers({{ /* L */ 8.0,0.0,0.0, /* R */ -8.0f,-0.0f,0.0f}}));

  // nothing else to play
  ASSERT_FALSE(ss.play(audioOut.getBuffers()));

}

// SampleSlice - playWithCrossFade
TEST(SampleSlice, playWithCrossFade)
{
  constexpr int NUM_CHANNELS = 2;
  constexpr int NUM_SAMPLES = 12;
  constexpr Sample32 FIRST_SAMPLE = 9.0;

  SampleBuffers32 sampleBuffers{44100, NUM_CHANNELS, NUM_SAMPLES};

  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    auto sample = static_cast<Sample32>(i) + FIRST_SAMPLE;
    sampleBuffers.getBuffer()[0][i] = sample;
    sampleBuffers.getBuffer()[1][i] = -sample;
  }

  AudioOut<NUM_CHANNELS, 3> audioOut{};

  SampleSlices<64, 2, 5> ss;
  // using only 1 slice (all samples)
  ss.setNumActiveSlices(1);
  ss.setCrossFade(true);
  ss.setPolyphonic(true);
  ss.setPlayMode(EPlayMode::kHold);

  ss.setBuffers(&sampleBuffers);

  // play without doing anything => should not play
  ASSERT_FALSE(ss.play(audioOut.getBuffers()));

  {
    // cross fade at beginning and end
    ss.setPadSelected(0, true);
    ss.setLoop(0, false); // non looping

    // expected result
    // [9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
    // [+++++++++++++++++ =======  ------------------] // 5 samples of xfade to 1, 2 normal, 5 xfade to 0
    ASSERT_TRUE(ss.play(audioOut.getBuffers()));
    ASSERT_TRUE(audioOut.checkBuffers2({{ 9.00 * 0.00,
                                          10.0 * 0.25,
                                          11   * 0.50 }}));
    ASSERT_TRUE(ss.play(audioOut.getBuffers()));
    ASSERT_TRUE(audioOut.checkBuffers2({{ 12.0 * 0.75,
                                          13.0 * 1.00,
                                          14        }}));
    ASSERT_TRUE(ss.play(audioOut.getBuffers()));
    ASSERT_TRUE(audioOut.checkBuffers2({{ 15,
                                          16.0 * 1.00,
                                          17   * 0.75 }}));
    ASSERT_TRUE(ss.play(audioOut.getBuffers()));
    ASSERT_TRUE(audioOut.checkBuffers2({{ 18   * 0.50,
                                          19.0 * 0.25,
                                          20   * 0.00 }}));

    // play without doing anything => should not play
    ASSERT_FALSE(ss.play(audioOut.getBuffers()));
  }

  {
    ss.setPadSelected(0, false);
    ss.setPadSelected(0, true);
    ss.setLoop(0, true);

    // expected result
    // [9, 10, 11, 12, 13, 14, 15, 16]
    // [+++++++++, ------------------] // start with xFading to 1 (3 samples) then xFade to 0
    ASSERT_TRUE(ss.play(audioOut.getBuffers()));
    ASSERT_TRUE(audioOut.checkBuffers2({{ 9.00 * 0.00,
                                          10.0 * 0.25,
                                          11 * 0.50 }}));

    // we request stop
    ss.setPadSelected(0, false);


    // at this stage the cross fade buffer is [0, 2.5, 5.5, 9, 13]
    //                         fCurrent --------------------^
    // the main buffer is [..., 9, 10, 11, 12, 13, 14, 15, ...]
    //        fCurrent --------------------^
    // The cross fade algorithm is thus cross fading [12, 13, 14, 15, 16] * [1.0, 0.75, 0.5, 0.25, 0] with [9, 13, 13, 13, 13]
    // which is cross fading [12, 9.75, 7, 3.75, 0] with [9, 13, 13, 13, 13]

    ASSERT_TRUE(ss.play(audioOut.getBuffers()));
    ASSERT_TRUE(audioOut.checkBuffers2({{ 9  * 1.0  + 12   * 0.0,
                                          13 * 0.75 + 9.75 * 0.25,
                                          13  * 0.5  + 7    * 0.50}}));
    ASSERT_TRUE(ss.play(audioOut.getBuffers()));
    ASSERT_TRUE(audioOut.checkBuffers2({{ 13  * 0.25 + 3.75 * 0.75,
                                          0,
                                          0}}));

    // play without doing anything => should not play
    ASSERT_FALSE(ss.play(audioOut.getBuffers()));

  }


}

// SampleSlice - crossFadeIssue
// Commented out as it is was used for debugging code
//TEST(SampleSlice, crossFadeIssue)
//{
//  auto file = SampleFile::create("/Volumes/Vault/tmp/vst-sam-spl-64/sam-spl-64-click-issue.wav");
//
//  SampleSlices<NUM_SLICES, 1, NUM_XFADE_SAMPLES> fSampleSlices;
//  SampleRate sampleRate;
//  int32 numSamples;
//
//  {
//    auto buffers = file->toBuffers();
//    DLOG_F(INFO, "numChannels=%d; numSamples=%d", buffers->getNumChannels(), buffers->getNumSamples());
//
//    sampleRate = buffers->getSampleRate();
//    numSamples = buffers->getNumSamples();
//
//    fSampleSlices.setPlayMode(EPlayMode::kHold);
//    fSampleSlices.setCrossFade(true);
//    fSampleSlices.setNumActiveSlices(4);
//    for(int32 s = 0; s < 4; s++)
//      fSampleSlices.setLoop(s, true);
//    fSampleSlices.setBuffers(std::move(*buffers));
//  }
//
//  std::vector<Sample32> out{};
//  out.reserve(numSamples);
//
//  AudioOut<1, 512> audioOut{};
//
//  // frame 5368
//  fSampleSlices.setNoteSelected(1, true, 5368);
//  for(int32 i = 5368; i < 5415; i++)
//  {
//    fSampleSlices.play(audioOut.getBuffers(), true);
//    audioOut.appendTo(out);
//  }
//
//  // frame 5415
//  fSampleSlices.setNoteSelected(1, false, 5415);
//  for(int32 i = 5415; i < 5430; i++)
//  {
//    fSampleSlices.play(audioOut.getBuffers(), true);
//    audioOut.appendTo(out);
//  }
//
//  SampleBuffers32 outBuffer(sampleRate, 1, out.size());
//
//  std::copy(std::begin(out), std::end(out), outBuffer.getChannelBuffer(0));
//
//  auto time = Clock::getCurrentTimeMillis();
//
//  auto sampleFile = SampleFile::create("/Volumes/Vault/tmp/vst-sam-spl-64/crossFadeIssue-result_" + std::to_string(time) + ".wav",
//                                       outBuffer,
//                                       false,
//                                       SampleStorage::kSampleFormatWAV,
//                                       SampleStorage::kSampleFormatPCM24);
//
//}


}
