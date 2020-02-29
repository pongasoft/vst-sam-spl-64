#include <pongasoft/Utils/Misc.h>
#include <pluginterfaces/base/ftypes.h>

namespace pongasoft::VST::SampleSplitter {

using namespace Steinberg;

/**
 * Implements a cross fader from 0 to 1 (res. 1 to 0).
 *
 * Supposed to be used this way:
 *
 * ```
 * fader.next()
 * fader.compute(sample);
 * fader.next()
 * fader.compute(sample);
 * ...
 * ```
 */
template<typename SampleType, int32 numSamples>
class LinearCrossFader
{
public:

  //! Resets the cross fader to its original state (pass through)
  inline void reset()
  {
    fFadeToZero = false;
    fCurrent = numSamples;
  }

  /**
   * Sets the cross fader into cross fading to 0. Note that if it was already in the middle of cross fading to 1.0,
   * it will simply go back down to 0 from this point.
   *
   * @return `true` if there is more cross fading to do
   */
  inline bool xFadeTo0()
  {
    fFadeToZero = true;
    return fCurrent >= 0;
  }

  /**
   * Sets the cross fader into cross fading to 1. Always start at 0.
   */
  inline void xFadeTo1()
  {
    fFadeToZero = false;
    fCurrent = -1;
  }

  /**
   * Computes the cross faded sample based on the state of the cross fader
   */
  inline SampleType compute(SampleType iSample)
  {
    if(fCurrent >= numSamples) // including shortcut when fCurrent == numSamples
      return iSample;

    if(fCurrent <= 0) // including shortcut when fCurrent == 0
      return 0;

    auto factor = (static_cast<SampleType>(1) / numSamples) * fCurrent;
    return iSample * factor;
  }

  /**
   * This method advances the state of the cross fader to handle the next invocation if `compute`.
   *
   * @return `false` when cross fade to 0 is done (meaning any further invocation of `compute` will return 0) */
  inline bool next()
  {
    if(fFadeToZero)
    {
      if(fCurrent >= 0)
        fCurrent--;
    }
    else
    {
      if(fCurrent < numSamples)
        fCurrent++;
    }

    return fCurrent >= 0;
  }

private:
  bool fFadeToZero{false};
  int32 fCurrent{numSamples};
};

/**
 * Slices the sample from `fStart` to `fEnd` and keep track of where we are in the buffer (`fCurrent`).
 * Handles 1 shot (plays from `fStart` to `fEnd`) and reverse. Looping is handled at a higher level
 * (call `restart` when `next` returns `false`).
 *
 * Handles cross fading when enabled.
 *
 * Supposed to be used this way:
 * ```
 * slicer.reset(xx, yy);
 * slicer.start();
 * if(slicer.next())
 *   slicer.getSample(buffer);
 * if(slicer.next())
 *   slicer.getSample(buffer);
 * if(slicer.next())
 *   slicer.getSample(buffer);
 * ...
 * slicer.requestEnd(); // optionally
 * ```
 *
 * Note that this class does NOT handle buffer management and assumes that `fStart`, `fEnd - 1` are within the boundary
 * of `buffer`. */
template<typename SampleType, int32 numXFadeSamples>
class Slicer
{
private:
  static constexpr int32 NOT_PLAYING = -2;

public:
  /**
   * Called to reset `fStart` and `fEnd`. Note that `fStart` is part of the range and `fEnd` is *not*. */
  inline void reset(int32 iStart, int32 iEnd)
  {
    // sanity check
    DCHECK_F(iStart >= 0);
    DCHECK_F(iEnd >= 0);
    DCHECK_F(iStart < iEnd);

    fStart = iStart;
    fEnd = iEnd;

    if(fCurrent != NOT_PLAYING)
      fCurrent = Utils::clamp(fCurrent, fStart - 1, fEnd);
  }

  inline int32 startIdx() const { return fStart; }
  inline int32 endIdx() const { return fEnd; }

  /**
   * Specifies whether cross fading should happen or not (in order to avoid pops and clicks when starting/stopping or
   * looping.
   */
  inline void crossFade(bool iEnabled)
  {
    fXFaderEnabled = iEnabled;

    if(fXFaderEnabled)
      fXFader.reset();
  }

  //! Specifies the iteration direction
  inline void reverse(bool iReverse) { fReverse = iReverse; }

  // reverse
  inline bool reverse() const { return fReverse; }

  // Returns the total number of slices to play
  inline int32 numSlices() const { return fEnd - fStart; }

  // Returns the number of slices played so far
  inline int32 numSlicesPlayed() const
  {
    if(fCurrent == NOT_PLAYING)
      return 0;

    return fCurrent - (fReverse ? fEnd - 1 : fStart);
  }

  //! Must be called prior to iteration
  inline void start()
  {
    // we start "before" because next() is called first and will increment (resp. decrement) first
    fCurrent = fReverse ? fEnd : fStart - 1;

    if(fXFaderEnabled)
    {
      fXFader.xFadeTo1();
    }
  }

  //! Shortcut which calls `start` and `next`
  inline bool restart()
  {
    start();
    return next();
  }

  /**
   * Called when we want to stop iterating. Note that it may not stop right away due to cross fading.
   *
   * @return `true` if it ended right away (of if there is more to end (cross fading to 0))
   */
  inline bool requestEnd()
  {
    if(fCurrent != NOT_PLAYING)
    {
      if(!fXFaderEnabled || !fXFader.xFadeTo0())
        fCurrent = NOT_PLAYING;
    }

    return fCurrent == NOT_PLAYING;
  }

  /**
   * Retrieves the current sample (applies cross fading if necessary)
   *
   * \note This class assumes that `fStart` and `fEnd - 1` are valid for the provided buffer! Be very cautious when
   *       calling this method!
   */
  inline SampleType getSample(SampleType *iBuffer)
  {
    if(fCurrent == NOT_PLAYING)
      return 0;

    // with this API it is not guaranteed that fCurrent represents a valid index in iBuffer,
    // but we assume that fStart and fEnd (set in reset) have been set appropriately
    DCHECK_F(fCurrent >= 0);
    DCHECK_F(fCurrent >= fStart);
    DCHECK_F(fCurrent < fEnd);

    auto sample = iBuffer[fCurrent];

    return fXFaderEnabled ? fXFader.compute(sample) : sample;
  }

  /**
   * Advances the state of the slicer to the next sample (takes looping, reverse and cross fading into consideration)
   *
   * @return `true` if there is more playing or `false` if done (at which point `getSample` will always return 0)
   */
  inline bool next()
  {
    if(fCurrent == NOT_PLAYING)
      return false;

    if(fReverse)
    {
      fCurrent--;

      if(fXFaderEnabled && fCurrent == fStart + numXFadeSamples - 1)
        fXFader.xFadeTo0();

      if(fCurrent < fStart)
        fCurrent = NOT_PLAYING;
    }
    else
    {
      fCurrent++;

      if(fXFaderEnabled && fCurrent == fEnd - numXFadeSamples)
        fXFader.xFadeTo0();

      if(fCurrent >= fEnd)
        fCurrent = NOT_PLAYING;
    }

    if(fXFaderEnabled && !fXFader.next())
      fCurrent = NOT_PLAYING;

    return fCurrent != NOT_PLAYING;
  }

private:
  int32 fStart{-1};
  int32 fEnd{-1};
  int32 fCurrent{NOT_PLAYING};

  bool fReverse{false};

  bool fXFaderEnabled{true};
  LinearCrossFader<SampleType, numXFadeSamples> fXFader{};
};

}