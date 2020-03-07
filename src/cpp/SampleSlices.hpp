#pragma once

#include "SampleSlice.hpp"
#include "Model.h"

namespace pongasoft::VST::SampleSplitter {

template<int32 numSlices = NUM_SLICES, int32 numChannels = 2, int32 numXFadeSamples = 65>
class SampleSlices
{
public:
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
  }

  inline void setPadSelected(int32 iSlice, bool iSelected, uint32 iStartFrame = 0) { setSelected(iSlice, true, iSelected, iStartFrame); }
  inline void setNoteSelected(int32 iSlice, bool iSelected, uint32 iStartFrame = 0) { setSelected(iSlice, false, iSelected, iStartFrame); }

  /**
   * Play all the slices that needs to be played.
   *
   * @return `true` if anything was played at all or `false` if `out` was left untouched */
  template<typename SampleType>
  bool play(AudioBuffers<SampleType> &out)
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
        slice.play(out, !played, fPlayModeHold ? slice.loop() : false);
        played = true;
      }
    }

    if(played)
    {
      // handle mono -> stereo (copy channel 0 to other channels if necessary)
      // TODO...
    }

    // reset transitions for next frame
    std::fill(std::begin(fSliceTransitions), std::end(fSliceTransitions), ETransition::kNone);

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

    auto transition = fSliceTransitions[iSlice];

    switch(transition)
    {
      case ETransition::kNone:
        transition = iStarting ? ETransition::kStarting : ETransition::kStopping;
        break;

      case ETransition::kStarting:
        transition = iStarting ? ETransition::kStarting : ETransition::kNone;
        break;

      case ETransition::kStopping:
      case ETransition::kRestarting:
        transition = iStarting ? ETransition::kRestarting : ETransition::kStopping;
        break;

    }

    DLOG_F(INFO, "SampleSlices::adjustSliceTransition(%d) %d -> %d", iSlice, fSliceTransitions[iSlice], transition);

    fSliceTransitions[iSlice] = transition;
  }

  //------------------------------------------------------------------------
  // prepareSlice
  //------------------------------------------------------------------------
  SampleSliceImpl &prepareSliceForPlaying(int32 iSlice)
  {
    DCHECK_F(iSlice <= fNumActiveSlices);

    auto &slice = getSlice(iSlice);
    auto transition = fSliceTransitions[iSlice];

    // when monophonic mode
    if(!fPolyphonic && iSlice != fMostRecentSlicePlayed)
      transition = slice.isPlaying() ? ETransition::kStopping : ETransition::kNone;

    switch(transition)
    {
      case ETransition::kStarting:
        DLOG_F(INFO, "SampleSlices::prepareSliceForPlaying(%d) starting", iSlice);
        if(slice.isPlaying())
          slice.requestStop();
        slice.start();
        break;

      case ETransition::kStopping:
        DLOG_F(INFO, "SampleSlices::prepareSliceForPlaying(%d) stopping", iSlice);
        slice.requestStop();
        break;

      case ETransition::kRestarting:
        DLOG_F(INFO, "SampleSlices::prepareSliceForPlaying(%d) restarting", iSlice);
        slice.requestStop();
        slice.start();
        break;

      default:
        // nothing to do
        break;
    }

    return slice;
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
};

}

