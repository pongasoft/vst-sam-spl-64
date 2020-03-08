#pragma once

#include "SampleSlice.hpp"
#include "Model.h"

namespace pongasoft::VST::SampleSplitter {

template<int32 numSlices = NUM_SLICES, int32 numChannels = 2, int32 numXFadeSamples = 65>
class SampleSlices
{
public:
  // Constructor (simply sets the WE slice in looping mode)
  SampleSlices() { fWESlice.loop(true); }

  inline bool empty() const { return !fSampleBuffers.hasSamples(); }

  void setBuffers(SampleBuffers32 &&iBuffers) { fSampleBuffers = std::move(iBuffers); splitSample(); }

  template<typename BuffersUpdater>
  void updateBuffers(BuffersUpdater const &iUpdater) { iUpdater(&fSampleBuffers); splitSample(); }

  void setNumActiveSlices(int32 iValue) { fNumActiveSlices = iValue; splitSample(); }
  void setPolyphonic(bool iValue) { fPolyphonic = iValue; }
  void setPlayModeHold(bool iValue) { fPlayModeHold = iValue; };
  void setLoop(int32 iSlice, bool iLoop) { getSlice(iSlice).loop(iLoop); }
  void setReverse(int32 iSlice, bool iReverse) { getSlice(iSlice).reverse(iReverse); }
  float getPercentPlayed(int32 iSlice) const { return getSlice(iSlice).getPercentPlayed(); }


  void setCrossFade(bool iEnabled)
  {
    for(auto &slice: fSampleSlices)
      slice.crossFade(iEnabled);

    fWESlice.crossFade(iEnabled);
  }

  inline void setPadSelected(int32 iSlice, bool iSelected, uint32 iStartFrame = 0) { setSelected(iSlice, true, iSelected, iStartFrame); }
  inline void setNoteSelected(int32 iSlice, bool iSelected, uint32 iStartFrame = 0) { setSelected(iSlice, false, iSelected, iStartFrame); }

  void setWESliceSelection(int32 iStart, int32 iEnd)
  {
    // should not be called without samples (cannot select in the UI...)
    DCHECK_F(fSampleBuffers.hasSamples());

    if(iStart == -1)
      iStart = 0;

    if(iEnd == -1)
      iEnd = fSampleBuffers.getNumSamples();

    iStart = Utils::clamp<int32>(iStart, 0, fSampleBuffers.getNumSamples() - 1);
    iEnd = Utils::clamp<int32>(iEnd, 0, fSampleBuffers.getNumSamples());

    fWESlice.reset(&fSampleBuffers, iStart, iEnd);
  }

  void setWESliceSelected(bool iSelected)
  {
    bool wasSelected = fWESlice.isSelected();
    fWESlice.setSelected(true, iSelected);
    bool isSelected = fWESlice.isSelected();

    if(wasSelected != isSelected)
      fWESliceTransition = adjustTransition(fWESliceTransition, isSelected);
  }

  float getWESlicePercentPlayed() const { return fWESlice.getPercentPlayed(); }

  /**
   * Play all the slices that needs to be played.
   *
   * @param iOverride if `out` should be overridden or added to
   * @return `true` if anything was played at all or `false` if `out` was left untouched */
  template<typename SampleType>
  bool play(AudioBuffers<SampleType> &out, bool iOverride = true)
  {
    if(out.getNumSamples() <= 0 || out.getNumChannels() == 0 || !fSampleBuffers.hasSamples())
      return false;

    // if we are not in play mode hold, we adjust any stopping transition to none since the slice
    // should play until it ends at the end of the slice (not because a pad is released)
    if(!fPlayModeHold)
    {
      for(int32 i = 0; i < fNumActiveSlices; i++)
      {
        auto transition = fSliceTransitions[i];
        if(fSliceTransitions[i] == ETransition::kStopping)
          fSliceTransitions[i] = ETransition::kNone;
      }
    }

    bool played = false;

    // play each slice
    for(int32 i = 0; i < fNumActiveSlices; i++)
    {
      auto &slice = prepareSliceForPlaying(i);

      if(slice.isPlaying())
      {
        slice.play(out, iOverride, fPlayModeHold ? slice.loop() : false);
        played = true;
        iOverride = false;
      }
    }

    // play the WE slice
    {
      auto &slice = prepareSliceForPlaying(fWESlice, fWESliceTransition);
      if(slice.isPlaying())
      {
        slice.play(out, iOverride, true);
        played = true;
      }
    }

    return played;
  }

protected:
  using SampleSliceImpl = SampleSlice<numChannels, numXFadeSamples>;

  enum class ETransition
  {
    kNone, kStarting, kStopping, kRestarting
  };

  void splitSample()
  {
    DCHECK_F(fNumActiveSlices <= numSlices);

    if(fSampleBuffers.hasSamples())
    {
      int32 numSamplesPerSlice = fSampleBuffers.getNumSamples() / fNumActiveSlices;

      DLOG_F(INFO, "SampleSplitterProcessor::splitSample(%d)", fNumActiveSlices);

      for(auto &slice : fSampleSlices)
        slice.hardStop();

      int32 start = 0;
      for(int32 i = 0; i < fNumActiveSlices; i++, start += numSamplesPerSlice)
        getSlice(i).reset(&fSampleBuffers, start, start + numSamplesPerSlice);

      // select the entire sample by default
      fWESlice.reset(&fSampleBuffers, 0, fSampleBuffers.getNumSamples());
    }
  }

  //------------------------------------------------------------------------
  // setSelected
  //------------------------------------------------------------------------
  void setSelected(int32 iSlice, bool iPad, bool iSelected, uint32 iStartFrame)
  {
    DLOG_F(INFO, "SampleSlices::setSelected(%d,%s,%s,%d)", iSlice, iPad ? "pad" : "note", iSelected ? "true" : "false", iStartFrame);

    // only handle active slices
    if(iSlice >= fNumActiveSlices)
      return;

    auto &slice = getSlice(iSlice);
    bool wasSelected = slice.isSelected();
    slice.setSelected(iPad, iSelected);
    bool isSelected = slice.isSelected();

    if(wasSelected != isSelected)
      adjustSliceTransition(iSlice, isSelected);

    if(iSelected)
    {
      if(fMostRecentFrame < iStartFrame && iSlice < fNumActiveSlices)
      {
        fMostRecentSlicePlayed = iSlice;
        fMostRecentFrame = iStartFrame;
      }
    }
  }

  //------------------------------------------------------------------------
  // adjustSliceTransition
  //------------------------------------------------------------------------
  void adjustSliceTransition(int32 iSlice, bool iStarting)
  {
    DCHECK_F(iSlice >= 0 && iSlice < numSlices);
    fSliceTransitions[iSlice] = adjustTransition(fSliceTransitions[iSlice], iStarting);
  }

  //------------------------------------------------------------------------
  // adjustTransition
  //------------------------------------------------------------------------
  ETransition adjustTransition(ETransition iCurrentTransition, bool iStarting)
  {
    switch(iCurrentTransition)
    {
      case ETransition::kNone:
        return iStarting ? ETransition::kStarting : ETransition::kStopping;

      case ETransition::kStarting:
        return iStarting ? ETransition::kStarting : ETransition::kNone;

      case ETransition::kStopping:
      case ETransition::kRestarting:
        return iStarting ? ETransition::kRestarting : ETransition::kStopping;
    }
  }

  //------------------------------------------------------------------------
  // prepareSliceForPlaying
  //------------------------------------------------------------------------
  SampleSliceImpl &prepareSliceForPlaying(int32 iSlice)
  {
    DCHECK_F(iSlice <= fNumActiveSlices);

    auto &slice = getSlice(iSlice);

    // when monophonic mode
    if(!fPolyphonic && iSlice != fMostRecentSlicePlayed)
      fSliceTransitions[iSlice] = slice.isPlaying() ? ETransition::kStopping : ETransition::kNone;

    return prepareSliceForPlaying(slice, fSliceTransitions[iSlice]);
  }

  //------------------------------------------------------------------------
  // prepareSliceForPlaying
  //------------------------------------------------------------------------
  SampleSliceImpl &prepareSliceForPlaying(SampleSliceImpl &iSlice, ETransition &iTransition)
  {
    switch(iTransition)
    {
      case ETransition::kStarting:
        if(iSlice.isPlaying())
          iSlice.requestStop();
        iSlice.start();
        iTransition = ETransition::kNone;
        break;

      case ETransition::kStopping:
        iSlice.requestStop();
        iTransition = ETransition::kNone;
        break;

      case ETransition::kRestarting:
        iSlice.requestStop();
        iSlice.start();
        iTransition = ETransition::kNone;
        break;

      default:
        // nothing to do
        break;
    }

    return iSlice;
  }

  inline SampleSliceImpl &getSlice(int32 iSlice) { DCHECK_F(iSlice < numSlices); return fSampleSlices[iSlice]; }
  inline SampleSliceImpl const &getSlice(int32 iSlice) const { DCHECK_F(iSlice < numSlices); return fSampleSlices[iSlice]; }

private:
  bool fPolyphonic{true};
  bool fPlayModeHold{true};

  uint32 fMostRecentFrame{};
  int32 fMostRecentSlicePlayed{};

  SampleBuffers32 fSampleBuffers{0};

  int32 fNumActiveSlices{numSlices};
  SampleSliceImpl fSampleSlices[numSlices]{};
  ETransition fSliceTransitions[numSlices]{};

  // represents the slice on the edit tab which can be "played" by holding the "Play" pad
  SampleSliceImpl fWESlice{};
  ETransition fWESliceTransition{};
};

}

