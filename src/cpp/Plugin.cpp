#include "Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleSplitterParameters::SampleSplitterParameters
//------------------------------------------------------------------------
SampleSplitterParameters::SampleSplitterParameters()
{
  // which input to use for sampling (off, 1 or 2)
  fSamplingInput =
    vst<EnumParamConverter<ESamplingInput, ESamplingInput::kSamplingInput2>>(ESampleSplitterParamID::kSamplingInput, STR16("Sampling Input"),
                                                                             std::array<ConstString, 3>{STR16("Off"), STR16("Input 1"), STR16("Input 2")})
      .defaultValue(kSamplingOff)
      .shortTitle(STR16("SamplingIn"))
      .add();

  // the number of slices the sample will be split into
  fNumSlices =
    vst<NumSlicesParamConverter>(ESampleSplitterParamID::kNumSlices, STR16("Num Slices"))
      .defaultValue(DEFAULT_NUM_SLICES)
      .shortTitle(STR16("Slices"))
      .add();

  // the bank/page representing 16 pads (4 banks of 16 pads => 64 pads)
  fPadBank =
    vst<DiscreteValueParamConverter<NUM_PAD_BANKS - 1>>(ESampleSplitterParamID::kPadBank, STR16("Page"),
                                                        STR16("Page %d"), 1)
      .defaultValue(0)
      .shortTitle(STR16("Page"))
      .add();

  // keep track of which slice is selected (for settings editing purpose)
  fSelectedSlice =
    vst<DiscreteValueParamConverter<NUM_SLICES -1>>(ESampleSplitterParamID::kSelectedSlice, STR16 ("Slice"),
                                                    1) // offset
      .defaultValue(0)
      .guiOwned()
      .add();

  // play mode => hold [true] (plays as long as held) trigger [false] (plays until the end of the slice or loop)
  fPlayModeHold =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kPlayModeHold, STR16("Play Mode"),
                               STR16("Trigger"), STR16("Hold")) // BooleanParamConverter args
      .defaultValue(true)
      .shortTitle(STR16("PlayMode"))
      .add();

  // if true => multiple pads can be "played" at the same time, if false => only 1
  fPolyphonic =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kPolyphonic, STR16("Polyphony"),
                               STR16("Mono"), STR16("Multi")) // BooleanParamConverter args
      .defaultValue(true)
      .shortTitle(STR16("Poly."))
      .add();


  // whether to copy input to output when sampling so it can be heard
  fSamplingMonitor =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kSamplingMonitor, STR16("Monitor"))
      .defaultValue(false)
      .shortTitle(STR16("Monitor"))
      .add();

  // which view to display (main or edit sample)
  fViewType =
    vst<EnumParamConverter<EViewType,EViewType::kEditSampleViewType>>(ESampleSplitterParamID::kViewType, STR16("View"),
                                                                      std::array<ConstString, 2>{STR16("Main"),
                                                                                                 STR16("Edit")})
      .defaultValue(EViewType::kMainViewType)
      .shortTitle(STR16("View"))
      .guiOwned()
      .add();

  // when true, RT will sample Stereo Input
  fSampling =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kSampling, STR16("Sampling"))
      .defaultValue(false)
      .shortTitle(STR16("Sampling"))
      .transient()
      .add();

  // offset for waveform edit
  fWEOffsetPercent =
    raw(ESampleSplitterParamID::kWEOffsetPercent, STR16("Waveform Offset"))
      .defaultValue(0)
      .shortTitle(STR16("WavePos"))
      .guiOwned()
      .transient()
      .add();

  // offset for waveform edit
  fWEZoomPercent =
    raw(ESampleSplitterParamID::kWEZoomPercent, STR16("Waveform Zoom"))
      .defaultValue(0)
      .shortTitle(STR16("WaveZoom"))
      .guiOwned()
      .transient()
      .add();

  // Show/hide zero crossing
  fWEShowZeroCrossing =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kWEShowZeroCrossing, STR16("Zero Crossing"))
      .defaultValue(false)
      .shortTitle(STR16("0X"))
      .guiOwned()
      .transient()
      .add();

  // the (major) format to save the sample in
  using MajorFormat = SampleStorage::ESampleMajorFormat;
  fExportSampleMajorFormat =
    vst<EnumParamConverter<MajorFormat, MajorFormat::kSampleFormatAIFF>>(ESampleSplitterParamID::kExportSampleMajorFormat, STR16("Major Format"),
                                                                         std::array<ConstString, 2>{STR16("WAV"), STR16("AIFF")})
      .defaultValue(MajorFormat::kSampleFormatWAV)
      .guiOwned()
      .shortTitle(STR16("MajFormat"))
      .add();

  // the (minor) format to save the sample in
  using MinorFormat = SampleStorage::ESampleMinorFormat;
  fExportSampleMinorFormat =
    vst<EnumParamConverter<MinorFormat, MinorFormat::kSampleFormatPCM32>>(ESampleSplitterParamID::kExportSampleMinorFormat, STR16("Minor Format"),
                                                                          std::array<ConstString, 3>{STR16("PCM 16"), STR16("PCM 24"), STR16("PCM 32")})
      .defaultValue(MinorFormat::kSampleFormatPCM24)
      .guiOwned()
      .shortTitle(STR16("MinFormat"))
      .add();

  // 16 pads that are either on (momentary button pressed) or off
  for(int i = 0; i < NUM_PADS; i++)
  {
    // pad 0
    fPads[i] =
      vst<BooleanParamConverter>(ESampleSplitterParamID::kPad1 + i, PAD_TITLES[i])
        .defaultValue(false)
        .flags(0)
        .shortTitle(PAD_TITLES[i])
        .transient()
        .add();
  }

  // the sample rate
  fSampleRate =
    jmb<DoubleParamSerializer>(ESampleSplitterParamID::kSampleRate, STR16 ("Sample Rate"))
      .defaultValue(44100)
      .rtOwned()
      .transient()
      .shared()
      .add();

  // playing state
  fPlayingState =
    jmb<PlayingStateParamSerializer>(ESampleSplitterParamID::kPlayingState, STR16 ("Playing State"))
      .rtOwned()
      .transient()
      .shared()
      .add();

  // the sample storage (gui only, saved part of the state)
  fSampleData =
    jmb<SampleDataSerializer>(ESampleSplitterParamID::kSampleData, STR16 ("Sample Data"))
      .guiOwned()
      .add();

  // The sample buffers sent by the GUI to RT (message)
  fGUISampleMessage =
    jmb<SampleBuffersSerializer32>(ESampleSplitterParamID::kGUISampleMessage, STR16 ("GUI Sample (msg)"))
      .guiOwned()
      .shared()
      .transient()
      .add();

  // The sample buffers sent from the RT to the GUI after sampling (message)
  fRTSampleMessage =
    jmb<SampleBuffersSerializer32>(ESampleSplitterParamID::fRTSampleMessage, STR16 ("RT Sample (msg)"))
      .rtOwned()
      .shared()
      .transient()
      .add();

  // RT will update the UI with the sampling state during sampling
  fSamplingState =
    jmb<SamplingStateParamSerializer>(ESampleSplitterParamID::fSamplingState, STR16 ("Sampling State"))
      .rtOwned()
      .transient()
      .shared()
      .add();

  // the settings per slice (forward/reverse, one shot/loop)
  fSlicesSettings =
    jmb<SlicesSettingsParamSerializer>(ESampleSplitterParamID::kSlicesSettings, STR16 ("Slices Settings"))
      .guiOwned()
      .shared()
      .add();

  // the samples selected in the waveform edit window
  fWESelectedSampleRange =
    jmbFromType<SampleRange>(ESampleSplitterParamID::kWESelectedSampleRange, STR16 ("Selected Samples"))
      .defaultValue(SampleRange{-1.0})
      .guiOwned()
      .transient()
      .add();

  // RT save state order
  setRTSaveStateOrder(PROCESSOR_STATE_VERSION,
                      fNumSlices,
                      fPadBank,
                      fPlayModeHold,
                      fPolyphonic,
                      fSamplingInput,
                      fSamplingMonitor);

  // GUI save state order
  setGUISaveStateOrder(CONTROLLER_STATE_VERSION,
                       fSampleData,
                       fSelectedSlice,
                       fSlicesSettings,
                       fViewType,
                       fExportSampleMajorFormat,
                       fExportSampleMinorFormat);
}

//------------------------------------------------------------------------
// SampleSplitterGUIState::readGUIState
//------------------------------------------------------------------------
tresult SampleSplitterGUIState::readGUIState(IBStreamer &iStreamer)
{
  tresult res = GUIState::readGUIState(iStreamer);

#ifndef NDEBUG
  if(res == kResultOk)
  {
    // swap the commented line to display either on a line or in a table
    DLOG_F(INFO, "GUIState::read - %s", Debug::ParamLine::from(this, true).toString().c_str());
    //Debug::ParamTable::from(this, true).showCellSeparation().print("GUIState::read ---> ");
  }
#endif

  if(res == kResultOk)
  {
    // notifying RT of new sample
    broadcastSample();

    // notifying RT of slices settings right after loading
    fSlicesSettings.broadcast();
  }
  return res;
}

//------------------------------------------------------------------------
// SampleSplitterGUIState::broadcastSample
//------------------------------------------------------------------------
tresult SampleSplitterGUIState::broadcastSample()
{
  if(fSampleRate > 0 && fSampleData->exists())
  {
    auto ptr = fSampleData->load(fSampleRate);
    if(ptr)
      return broadcast(fParams.fGUISampleMessage, *ptr);
  }

  return kResultOk;
}


}
}
}