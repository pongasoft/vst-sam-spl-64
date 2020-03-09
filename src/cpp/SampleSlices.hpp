#pragma once

#include "SampleSlice.hpp"
#include "Model.h"

namespace pongasoft::VST::SampleSplitter {

/**
 * Manager of the all the slices the RT is playing (including the slice from the Edit tab)
 *
 * @tparam numSlices the maximum number of slices the sample will be split into (currently 64)
 * @tparam numChannels the number of input channels supported (coming from the sample itself, currently only support
 *                     mono or stereo)
 * @tparam numXFadeSamples number of samples used in the cross fading algorithm
 */
template<int32 numSlices = NUM_SLICES, int32 numChannels = MAX_NUM_INPUT_CHANNELS, int32 numXFadeSamples = NUM_XFADE_SAMPLES>
class SampleSlices
{
public:
  // Constructor (simply sets the WE slice in looping mode)
  SampleSlices() { fWESlice.loop(true); }

  /**
   * @return `true` if there is no sample to play (meaning `play` will never do anything) */
  inline bool empty() const { return !fSampleBuffers.hasSamples(); }

  /**
   * Sets the sample buffers. Note that this api moves the buffers. */
  void setBuffers(SampleBuffers32 &&iBuffers) { fSampleBuffers = std::move(iBuffers); splitSample(); }

  /**
   * Updates the sample buffers. Note that this api copies the buffers. */
  template<typename BuffersUpdater>
  void updateBuffers(BuffersUpdater const &iUpdater) { iUpdater(&fSampleBuffers); splitSample(); }

  /**
   * Changes the number of slices that are active: the sample will be split into `iNumActiveSlices` slices */
  void setNumActiveSlices(int32 iNumActiveSlices) { fNumActiveSlices = iNumActiveSlices; splitSample(); }

  /**
   * @return number of active channels (1 for mono, 2 for stereo at the moment) */
  inline int32 getNumActiveChannels() const { return fSampleBuffers.getNumChannels(); }

  /**
   * Sets whether slices should be played in a polyphonic fashion (any number slices playing at the same time) or
   * a monophonic fashion (only 1 at a time, the "last" one played).
   *
   * \note When cross fading is enabled, even in monophonic mode, there might be other slices playing: only one slice
   *       is playing, but others might be stopping hence playing at most `numXFadeSamples` until they
   *       completely stop
   */
  void setPolyphonic(bool iValue) { fPolyphonic = iValue; }

  /**
   * Sets the play mode for each slice: either play as long as being help (`EPlayMode::kHold`) or play until the end
   * once triggered (`EPlayMode::kTrigger`) */
  void setPlayMode(EPlayMode iValue) { fPlayMode = iValue; };

  /**
   * Sets the looping mode for a given slice (start over when reaches the end when play mode is set
   * to `EPlayMode::kHold`) */
  void setLoop(int32 iSlice, bool iLoop) { getSlice(iSlice).loop(iLoop); }

  /**
   * Sets the direction in which a given slice is played */
  void setReverse(int32 iSlice, bool iReverse) { getSlice(iSlice).reverse(iReverse); }

  /**
   * @return how much of a slice has been playing (in percentage) or `PERCENT_PLAYED_NOT_PLAYING` when not playing */
  float getPercentPlayed(int32 iSlice) const { return getSlice(iSlice).getPercentPlayed(); }

  /**
   * Enable/disable cross fading when starting/stopping to play slices. Enabling cross fading requires more computing
   * power (to compute the cross fading), but removes any pops and clicks triggered by starting/stopping to play
   * a sample at random position in the sample */
  void setCrossFade(bool iEnabled)
  {
    for(auto &slice: fSampleSlices)
      slice.crossFade(iEnabled);

    fWESlice.crossFade(iEnabled);
  }

  /**
   * Select/Deselect the slice (via pressing the pad in the UI)
   *
   * @param iStartFrame at which frame this event happened (used for monophonic mode) */
  inline void setPadSelected(int32 iSlice, bool iSelected, uint32 iStartFrame = 0) { setSelected(iSlice, true, iSelected, iStartFrame); }

  /**
   * Select/Deselect the slice (via MIDI event)
   *
   * @param iStartFrame at which frame this event happened (used for monophonic mode) */
  inline void setNoteSelected(int32 iSlice, bool iSelected, uint32 iStartFrame = 0) { setSelected(iSlice, false, iSelected, iStartFrame); }

  /**
   * Changes the section of the sample that will be played on the Edit tab (the user can select a range (highlighted
   * in white) with the mouse).
   */
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

  /**
   * Called when the user presses/releases the "Play" pad on the Edit tab to play either what is selected,
   * or if nothing is selected, the entire sample. */
  void setWESliceSelected(bool iSelected) { fWESlice.setSelected(true, iSelected); }

  /**
   * @return how much of the sample on the Edit tab has been playing (in percentage) or
   *         `PERCENT_PLAYED_NOT_PLAYING` when not playing */
  float getWESlicePercentPlayed() const { return fWESlice.getPercentPlayed(); }

  /**
   * Play all the slices that needs to be played. Called on every RT frame with the `out` buffer.
   *
   * \note This class handles mono inputs by playing the mono input channel in the left output
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

      // handle monophonic case: any slice which is not the last one played must stop (requestStop is a noop
      // if not playing)
      if(!fPolyphonic && i != fMostRecentSlicePlayed)
        slice.requestStop();

      if(slice.play(fPlayMode, out, iOverride))
      {
        played = true;
        iOverride = false;
      }
    }

    // play the WE slice
    if(fWESlice.play(EPlayMode::kHold, out, iOverride))
    {
      played = true;
    }

    return played;
  }

protected:
  using SampleSliceImpl = SampleSlice<numChannels, numXFadeSamples>;

  //------------------------------------------------------------------------
  // splitSample
  //------------------------------------------------------------------------
  void splitSample()
  {
    DCHECK_F(fNumActiveSlices <= numSlices);

    if(fSampleBuffers.hasSamples())
    {
      int32 numSamplesPerSlice = fSampleBuffers.getNumSamples() / fNumActiveSlices;

      DLOG_F(INFO, "SampleSlices::splitSample(%d)", fNumActiveSlices);

      // enforcing that ALL slices are stopped (may generate pops and clicks but only when the sample changes while
      // being "played" which is clearly not a "usual" use case)
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

    // we record the most recently played slice (for monophonic case)
    if(iSelected)
    {
      if(fMostRecentFrame < iStartFrame && iSlice < fNumActiveSlices)
      {
        fMostRecentSlicePlayed = iSlice;
        fMostRecentFrame = iStartFrame;
      }
    }
  }

  // Adds checks to make sure we don't request an unavailable slice
  inline SampleSliceImpl &getSlice(int32 iSlice) { DCHECK_F(iSlice < numSlices); return fSampleSlices[iSlice]; }
  inline SampleSliceImpl const &getSlice(int32 iSlice) const { DCHECK_F(iSlice < numSlices); return fSampleSlices[iSlice]; }

private:
  bool fPolyphonic{true};
  EPlayMode fPlayMode{EPlayMode::kHold};

  uint32 fMostRecentFrame{};
  int32 fMostRecentSlicePlayed{};

  // the sample to be played
  SampleBuffers32 fSampleBuffers{0};

  // the slices
  int32 fNumActiveSlices{numSlices};
  SampleSliceImpl fSampleSlices[numSlices]{};

  // represents the slice on the edit tab which can be "played" by holding the "Play" pad
  SampleSliceImpl fWESlice{};
};

}

