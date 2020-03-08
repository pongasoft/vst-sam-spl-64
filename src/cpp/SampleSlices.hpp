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
  void setPlayMode(EPlayMode iValue) { fPlayMode = iValue; };
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

  void setWESliceSelected(bool iSelected) { fWESlice.setSelected(true, iSelected); }

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

    bool played = false;

    // play each slice
    for(int32 i = 0; i < fNumActiveSlices; i++)
    {
      auto &slice = getSlice(i);

      if(!fPolyphonic && i != fMostRecentSlicePlayed)
        slice.requestStop();

      if(slice.play(fPlayMode, out, iOverride))
      {
        played = true;
        iOverride = false;
      }
    }

    // play the WE slice
    {
      if(fWESlice.play(EPlayMode::kHold, out, iOverride))
      {
        played = true;
      }
    }

    return played;
  }

protected:
  using SampleSliceImpl = SampleSlice<numChannels, numXFadeSamples>;

  void splitSample()
  {
    DCHECK_F(fNumActiveSlices <= numSlices);

    if(fSampleBuffers.hasSamples())
    {
      int32 numSamplesPerSlice = fSampleBuffers.getNumSamples() / fNumActiveSlices;

      DLOG_F(INFO, "SampleSlices::splitSample(%d)", fNumActiveSlices);

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
//    DLOG_F(INFO, "SampleSlices::setSelected(%d,%s,%s,%d)", iSlice, iPad ? "pad" : "note", iSelected ? "true" : "false", iStartFrame);

    // only handle active slices
    if(iSlice >= fNumActiveSlices)
      return;

    getSlice(iSlice).setSelected(iPad, iSelected);

    if(iSelected)
    {
      if(fMostRecentFrame < iStartFrame && iSlice < fNumActiveSlices)
      {
        fMostRecentSlicePlayed = iSlice;
        fMostRecentFrame = iStartFrame;
      }
    }
  }

  inline SampleSliceImpl &getSlice(int32 iSlice) { DCHECK_F(iSlice < numSlices); return fSampleSlices[iSlice]; }
  inline SampleSliceImpl const &getSlice(int32 iSlice) const { DCHECK_F(iSlice < numSlices); return fSampleSlices[iSlice]; }

private:
  bool fPolyphonic{true};
  EPlayMode fPlayMode{EPlayMode::kHold};

  uint32 fMostRecentFrame{};
  int32 fMostRecentSlicePlayed{};

  SampleBuffers32 fSampleBuffers{0};

  int32 fNumActiveSlices{numSlices};
  SampleSliceImpl fSampleSlices[numSlices]{};

  // represents the slice on the edit tab which can be "played" by holding the "Play" pad
  SampleSliceImpl fWESlice{};
};

}

