#include <version.h>
#include "Plugin.h"
#include "SampleFile.h"

namespace pongasoft::VST::SampleSplitter {

std::array<VstString16, NUM_ROOT_KEYS> KEYS{{
  STR16("C-2"), STR16("C#-2"), STR16("D-2"), STR16("D#-2"), STR16("E-2"), STR16("F-2"), STR16("F#-2"), STR16("G-2"), STR16("G#-2"), STR16("A-2"), STR16("A#-2"), STR16("B-2"),
  STR16("C-1"), STR16("C#-1"), STR16("D-1"), STR16("D#-1"), STR16("E-1"), STR16("F-1"), STR16("F#-1"), STR16("G-1"), STR16("G#-1"), STR16("A-1"), STR16("A#-1"), STR16("B-1"),
  STR16("C0"), STR16("C#0"), STR16("D0"), STR16("D#0"), STR16("E0"), STR16("F0"), STR16("F#0"), STR16("G0"), STR16("G#0"), STR16("A0"), STR16("A#0"), STR16("B0"),
  STR16("C1"), STR16("C#1"), STR16("D1"), STR16("D#1"), STR16("E1"), STR16("F1"), STR16("F#1"), STR16("G1"), STR16("G#1"), STR16("A1"), STR16("A#1"), STR16("B1"),
  STR16("C2"), STR16("C#2"), STR16("D2"), STR16("D#2"), STR16("E2"), STR16("F2"), STR16("F#2"), STR16("G2"), STR16("G#2"), STR16("A2"), STR16("A#2"), STR16("B2"),
  STR16("C3"), STR16("C#3"), STR16("D3"), STR16("D#3"), STR16("E3"), STR16("F3"), STR16("F#3"), STR16("G3"), STR16("G#3"), STR16("A3"), STR16("A#3"), STR16("B3"),
  STR16("C4"), STR16("C#4"), STR16("D4"), STR16("D#4"), STR16("E4"), STR16("F4"), STR16("F#4"), STR16("G4"), STR16("G#4"), STR16("A4"), STR16("A#4"), STR16("B4"),
  STR16("C5"), STR16("C#5"), STR16("D5"), STR16("D#5"), STR16("E5"), STR16("F5"), STR16("F#5"), STR16("G5"), STR16("G#5"), STR16("A5"), STR16("A#5"), STR16("B5"),
  STR16("C6"), STR16("C#6"), STR16("D6"), STR16("D#6"), STR16("E6"), STR16("F6"), STR16("F#6"), STR16("G6"), STR16("G#6"), STR16("A6"), STR16("A#6"), STR16("B6"),
  STR16("C7"), STR16("C#7"), STR16("D7"), STR16("D#7"), STR16("E7"), STR16("F7"), STR16("F#7"), STR16("G7"), STR16("G#7"), STR16("A7"), STR16("A#7"), STR16("B7"),
  STR16("C8"), STR16("C#8"), STR16("D8"), STR16("D#8"), STR16("E8"), STR16("F8"), STR16("F#8"), STR16("G8")
}};

/**
 * The purpose of this serializer is to:
 * 1) make it a discrete value (so that it can be used to switch to the dialog)
 * 2) display the value to present the filename to the user
 * Note that it does not implement any of the serializer methods because this parameter is transient and not shared so
 * there is no need.
 */
struct LargeFilePathSerializer : public IParamSerializer<UTF8Path>, IDiscreteConverter<UTF8Path>
{
  void writeToStream(ParamType const &iValue, std::ostream &oStream) const override
  {
    oStream << SampleFile::extractFilename(iValue).toNativePath();
  }

  int32 getStepCount() const override
  {
    return 1;
  }

  tresult convertFromDiscreteValue(int32 iDiscreteValue, UTF8Path &oValue) const override
  {
    // not used
    return kResultFalse;
  }

  tresult convertToDiscreteValue(UTF8Path const &iValue, int32 &oDiscreteValue) const override
  {
    oDiscreteValue = iValue.toNativePath().size() == 0 ? 0 : 1;
    return kResultOk;
  }

  tresult readFromStream(IBStreamer &iStreamer, ParamType &oValue) const override
  {
    return kResultFalse;
  }

  tresult writeToStream(ParamType const &iValue, IBStreamer &oStreamer) const override
  {
    return kResultFalse;
  }
};

//------------------------------------------------------------------------
// SampleSplitterParameters::SampleSplitterParameters
//------------------------------------------------------------------------
SampleSplitterParameters::SampleSplitterParameters()
{
  // which input to use for sampling (off, 1 or 2)
  fSamplingInput =
    vst<EnumParamConverter<ESamplingInput, ESamplingInput::kSamplingInput2>>(ESampleSplitterParamID::kSamplingInput, STR16("Sampling Input"),
                                                                             {{STR16("Off"),
                                                                                STR16("Input 1"),
                                                                                STR16("Input 2")}})
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
    vst<DiscreteValueParamConverter<NUM_PAD_BANKS - 1, int>>(ESampleSplitterParamID::kPadBank, STR16("Bank"),
                                                             {{STR16("Bank A"), STR16("Bank B"), STR16("Bank C"), STR16("Bank D")}})
      .defaultValue(0)
      .shortTitle(STR16("Page"))
      .add();

  // keep track of which slice is selected (for settings editing purpose)
  fSelectedSlice =
    vst<DiscreteValueParamConverter<NUM_SLICES -1, int>>(ESampleSplitterParamID::kSelectedSlice, STR16 ("Slice"),
                                                    1) // offset
      .defaultValue(0)
      .guiOwned()
      .add();

  // whether midi input changes the selected slice
  fFollowMidiSelection =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kFollowMidiSelection, STR16("Follow Sel."))
      .defaultValue(false)
      .shortTitle(STR16("Follow"))
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

  // if true => start/stop and looping will cross fade to avoid clicks and pops
  fXFade =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kXFade, STR16("Cross Fade"))
      .defaultValue(true)
      .shortTitle(STR16("XFade"))
      .add();

  // how to handle input routing (mono->mono or mono->stereo)
  fInputRouting =
    vst<EnumParamConverter<EInputRouting, EInputRouting::kMonoInStereoOut>>(ESampleSplitterParamID::kInputRouting,
                                                                            STR16("Input Routing"),
                                                                            {{ STR16("Mono -> Mono"),
                                                                               STR16("Mono -> Stereo") }})
      .defaultValue(EInputRouting::kMonoInStereoOut)
      .shortTitle(STR16("Routing"))
      .add();

  // the root key (which is attached to first pad/slice)
  fRootKey =
    vst<RootKeyParamConverter>(ESampleSplitterParamID::kRootKey, STR16("Root Key"))
      .defaultValue(DEFAULT_ROOT_KEY)
      .shortTitle(STR16("RootK"))
      .add();

  // sampling duration in bars
  fSamplingDurationInBars =
    vst<SamplingDurationParamConverter>(ESampleSplitterParamID::kSamplingDuration, STR16("Sampling Duration"))
      .defaultValue(1)
      .shortTitle(STR16("SmplDur"))
      .add();

  // whether to copy input to output when sampling so it can be heard
  fSamplingMonitor =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kSamplingMonitor, STR16("Monitor"))
      .defaultValue(false)
      .shortTitle(STR16("Monitor"))
      .add();

  // gain to apply to sampling input
  fSamplingInputGain =
    vst<GainParamConverter>(ESampleSplitterParamID::kSamplingInputGain, STR16 ("Sampling Gain"))
      .defaultValue(DEFAULT_GAIN)
      .shortTitle(STR16 ("SmplGain"))
      .precision(2)
      .add();

  // what triggers sampling
  fSamplingTrigger =
    vst<EnumParamConverter<ESamplingTrigger, ESamplingTrigger::kSamplingTriggerOnSound>>
      (ESampleSplitterParamID::kSamplingTrigger, STR16("Sampling Trigger"),
       {{STR16("Immediate"),
          STR16("Play (Free)"),
          STR16("Play (Sync)"),
          STR16("Sound")}})
      .defaultValue(ESamplingTrigger::kSamplingTriggerOnSound)
      .shortTitle(STR16("SampTrig"))
      .add();


  // which view to display (main/edit)
  fViewType =
    vst<EnumParamConverter<EViewType,EViewType::kGlobalSettingsViewType>>(ESampleSplitterParamID::kViewType, STR16("View"),
                                                                          {{STR16("Play"),
                                                                             STR16("Edit"),
                                                                             STR16("Settings")}})
      .defaultValue(EViewType::kMainViewType)
      .shortTitle(STR16("View"))
      .guiOwned()
      .add();

  // which subtab to display (edit/sample/io)
  fEditingMode =
    vst<EnumParamConverter<EEditingMode, EEditingMode::kEditingIO>>(ESampleSplitterParamID::kEditingMode, STR16("Edit Mode"),
                                                                    {{STR16("Edit"),
                                                                       STR16("Sample"),
                                                                       STR16("IO")}})
      .defaultValue(EEditingMode::kEditingEdit)
      .shortTitle(STR16("EditMode"))
      .guiOwned()
      .transient()
      .flags(0)
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
      .flags(0)
      .add();

  // offset for waveform edit
  fWEZoomPercent =
    raw(ESampleSplitterParamID::kWEZoomPercent, STR16("Waveform Zoom"))
      .defaultValue(0)
      .shortTitle(STR16("WaveZoom"))
      .guiOwned()
      .transient()
      .flags(0)
      .add();

  // Show/hide zero crossing
  fWEShowZeroCrossing =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kWEShowZeroCrossing, STR16("Zero Crossing"))
      .defaultValue(false)
      .shortTitle(STR16("0X"))
      .guiOwned()
      .transient()
      .add();

  // play selection
  fWEPlaySelection =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kWEPlaySelection, STR16("Play Selection"))
      .defaultValue(false)
      .shortTitle(STR16("PlaySel"))
      .flags(0)
      .transient()
      .add();

  // Zoom to selection
  fWEZoomToSelection =
    vst<BooleanParamConverter>(ESampleSplitterParamID::kWEZoomToSelection, STR16("Zoom To Selection"))
      .defaultValue(false)
      .shortTitle(STR16("Zoom2Sel"))
      .flags(0)
      .transient()
      .add();

  // the (major) format to save the sample in
  using MajorFormat = SampleStorage::ESampleMajorFormat;
  fExportSampleMajorFormat =
    vst<EnumParamConverter<MajorFormat, MajorFormat::kSampleFormatAIFF>>(ESampleSplitterParamID::kExportSampleMajorFormat, STR16("Major Format"),
                                                                         {{STR16("WAV"), STR16("AIFF")}})
      .defaultValue(MajorFormat::kSampleFormatWAV)
      .guiOwned()
      .shortTitle(STR16("MajFormat"))
      .add();

  // the (minor) format to save the sample in
  using MinorFormat = SampleStorage::ESampleMinorFormat;
  fExportSampleMinorFormat =
    vst<EnumParamConverter<MinorFormat, MinorFormat::kSampleFormatPCM32>>(ESampleSplitterParamID::kExportSampleMinorFormat, STR16("Minor Format"),
                                                                          {{STR16("PCM 16"), STR16("PCM 24"), STR16("PCM 32")}})
      .defaultValue(MinorFormat::kSampleFormatPCM24)
      .guiOwned()
      .shortTitle(STR16("MinFormat"))
      .add();

  // fSamplingLeftVuPPM
  fSamplingLeftVuPPM =
    raw(ESampleSplitterParamID::kSamplingLeftVuPPM, STR16 ("Left Vu PPM"))
      .flags(ParameterInfo::kIsReadOnly)
      .transient()
      .shortTitle(STR16 ("LVuPPM"))
      .add();

  // fSamplingLeftVuPPM
  fSamplingRightVuPPM =
    raw(ESampleSplitterParamID::kSamplingRightVuPPM, STR16 ("Right Vu PPM"))
      .flags(ParameterInfo::kIsReadOnly)
      .transient()
      .shortTitle(STR16 ("RVuPPM"))
      .add();

  // keep track of which slice is selected via Midi
  fSelectedSliceViaMidi =
    vst<DiscreteValueParamConverter<NUM_SLICES -1, int>>(ESampleSplitterParamID::kSelectedSliceViaMidi, STR16 ("Midi Slice"),
                                                         1) // offset
      .defaultValue(0)
      .transient()
      .add();

  // 16 pads that are either on (momentary button pressed) or off
  for(int i = 0; i < NUM_PADS; i++)
  {
    VstString16 title(String().printf(STR16("Pad %d"), i + 1).text16());

    // pad 0
    fPads[i] =
      vst<BooleanParamConverter>(ESampleSplitterParamID::kPad1 + i, title)
        .defaultValue(false)
        .flags(0)
        .shortTitle(title)
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

  // info about the host (like bpm)
  fHostInfoMessage =
    jmb<HostInfoParamSerializer>(ESampleSplitterParamID::kHostInfo, STR16 ("Host Info"))
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

  // the sample data mgr (gui only, saved part of the state)
  fSampleData =
    jmb<SampleDataSerializer>(ESampleSplitterParamID::kSampleData, STR16 ("Sample Data"))
      .guiOwned()
      .add();

  // the sample data mgr (gui only, saved part of the state)
  fSampleDataMgr =
    jmbFromType<SampleDataMgr>(ESampleSplitterParamID::kSampleDataMgr, STR16 ("Sample Data Mgr"))
      .guiOwned()
      .transient()
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
    jmb<SampleRangeParamSerializer>(ESampleSplitterParamID::kWESelectedSampleRange, STR16 ("Selected Samples"))
      .defaultValue(SampleRange{-1.0})
      .guiOwned()
      .shared()
      .transient()
      .add();

  // quick editing for slices (all visible at once)
  fSlicesQuickEdit =
    jmbFromType<bool>(ESampleSplitterParamID::kSlicesQuickEdit, STR16 ("Quick Edit"))
      .transient()
      .add();

  // Plugin version
  fPluginVersion = jmbFromType<std::string>(ESampleSplitterParamID::kPluginVersion, STR16 ("Version"))
    .transient()
    .defaultValue(FULL_VERSION_STR)
    .add();

  // the path to a large file to maybe load
  fLargeFilePath =
    jmb<LargeFilePathSerializer>(ESampleSplitterParamID::kLargeFilePath, STR16 ("Selected Samples"))
      .defaultValue("")
      .guiOwned()
      .transient()
      .add();

  // RT save state order
  setRTSaveStateOrder(kProcessorStateLatest,
                      fNumSlices,
                      fPadBank,
                      fPlayModeHold,
                      fPolyphonic,
                      fSamplingInput,
                      fSamplingMonitor,
                      fSamplingDurationInBars,
                      fSamplingTrigger,
                      fSamplingInputGain,
                      fRootKey,
                      fFollowMidiSelection,
                      fXFade,
                      fInputRouting);

  // GUI save state order
  setGUISaveStateOrder(kControllerStateLatest,
                       fSampleData,
                       fSelectedSlice,
                       fSlicesSettings,
                       fViewType,
                       fExportSampleMajorFormat,
                       fExportSampleMinorFormat);

  // Deprecation
  // deprecated number of slices (kept for backward compatibility)
  __deprecated_fNumSlices =
    vst<__deprecated_NumSlicesParamConverter>(ESampleSplitterParamID::__deprecated_kNumSlices, STR16("Num Slices"))
      .deprecatedSince(kProcessorStateV1)
      .defaultValue(DEFAULT_NUM_SLICES.intValue())
      .shortTitle(STR16("Slices"))
      .add();

  // RT save state order
  setRTDeprecatedSaveStateOrder(kProcessorStateV1,
                                __deprecated_fNumSlices,
                                fPadBank,
                                fPlayModeHold,
                                fPolyphonic,
                                fSamplingInput,
                                fSamplingMonitor,
                                fSamplingDurationInBars,
                                fSamplingTrigger,
                                fSamplingInputGain,
                                fRootKey,
                                fFollowMidiSelection,
                                fXFade,
                                fInputRouting);
}

//------------------------------------------------------------------------
// SampleSplitterParameters::handleRTStateUpgrade
//------------------------------------------------------------------------
tresult SampleSplitterParameters::handleRTStateUpgrade(NormalizedState const &iDeprecatedState,
                                                       NormalizedState &oNewState) const
{
  DCHECK_F(oNewState.getVersion() == kProcessorStateLatest);

  switch(iDeprecatedState.getVersion())
  {
    case kProcessorStateV1:
    {
      // we handle number of slices (read it from old state / write to new)
      auto oldNumSlices = __deprecated_fNumSlices->readFromState(iDeprecatedState);
      fNumSlices->writeToState(NumSlice{static_cast<NumSlice::int_type>(oldNumSlices)}, oNewState);

//      DLOG_F(INFO, "======>>>>> handleRTStateUpgrade:: upgraded from \n[%s]\n ==> \n[%s]",
//             iDeprecatedState.toString().c_str(),
//             oNewState.toString().c_str());

      return kResultTrue;
    }

    default:
      DLOG_F(ERROR, "Unexpected deprecated version %d", iDeprecatedState.fSaveOrder->fVersion);
      return kResultFalse;
  }
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
    auto ptr = fSampleData->load(*fSampleRate);
    if(ptr)
    {
      auto res = broadcast(fParams.fGUISampleMessage, *ptr);
      // we reset the selection because after setting a sample in RT, the entire sample is selected for playing
      if(res == kResultOk)
        fWESelectedSampleRange.resetToDefault();
      return res;
    }
  }

  return kResultOk;
}

//------------------------------------------------------------------------
// SampleSplitterGUIState::maybeLoadSample
//------------------------------------------------------------------------
tresult SampleSplitterGUIState::maybeLoadSample(UTF8Path const &iFilePath)
{
  DLOG_F(INFO, "SampleSplitterGUIState::maybeLoadSample");

  auto fileSize = SampleFile::computeFileSize(iFilePath);
  if(fileSize > 0)
  {
    if(fileSize > LARGE_SAMPLE_FILE_SIZE)
    {
      DLOG_F(WARNING, "file is big %lld", fileSize);
      fLargeFilePath.update(iFilePath);
    }
    else
    {
      return loadSample(iFilePath);
    }
  }
  return kResultFalse;
}

//------------------------------------------------------------------------
// SampleSplitterGUIState::loadSample
//------------------------------------------------------------------------
tresult SampleSplitterGUIState::loadSample(UTF8Path const &iFilePath)
{
  DLOG_F(INFO, "SampleSplitterGUIState::loadSample");

  if(fSampleDataMgr.updateIf([&iFilePath] (SampleDataMgr *iMgr) -> bool
                             {
                               return iMgr->load(iFilePath);
                             }))
  {
    auto res = broadcastSample();
    // after loading the sample (drag'n'drop or file interface) we automatically split in DEFAULT_NUM_SLICES
    if(res == kResultOk)
    {
      // reset number of slices
      getGUIVstParameter(fParams.fNumSlices)->resetToDefault();

      // reset slice settings
      fSlicesSettings.resetToDefault();
      fSlicesSettings.broadcast();
    }
    return res;
  }
  return kResultFalse;
}



}