#include <pongasoft/Utils/Misc.h>
#include <pluginterfaces/base/ftypes.h>

namespace pongasoft::VST::SampleSplitter {

using namespace Steinberg;

/**
 * Implements a cross fader from 0 to 1 (res. 1 to 0).
 *
 * Supposed to be used this way (java style iteration):
 *
 * ```
 * fader.xFadeTo(&buffer[0], false);
 * if(fader.hasNext())
 *  fader.next();
 * if(fader.hasNext())
 *  fader.next();
 * ...
 * ```
 */
template<typename SampleType, int32 numSamples>
class LinearCrossFader
{
public:
  //! Resets the cross fader to its original state
  inline void reset()
  {
    fFadeFrom = false;
    fCurrent = numSamples;

    // Implementation note: this is totally unnecessary because when fCurrent == numSamples, the buffer will
    // be overridden anyway... hence commented out
    // std::fill(std::begin(fBuffer), std::end(fBuffer), 0);
  }

  /**
   * Cross fade from 0 to the current buffer. Note that if there was already some cross fading happening then it will
   * cross fade from whatever was there before. As an example, stopping playing the slice puts it in "fading to 0" mode
   * but starting right away "jumps" to the beginning of the slice... this creates a click which is prevented by this
   * method.
   *
   * @tparam Iterator designed to be a "pointer" (need to support ++/--/*)
   */
  template<typename Iterator>
  void xFadeTo(Iterator iBuffer, bool iReverse)
  {
    constexpr auto factor = (static_cast<SampleType>(1) / (numSamples - 1));

    if(fCurrent < numSamples)
    {
      int current = fCurrent;

      for(int32 i = 0; i < numSamples; i++)
      {
        auto sample = current < numSamples ? fBuffer[current] : 0;
        auto f = factor * i;
        fBuffer[i] = f * (iReverse ? *iBuffer-- : *iBuffer++) + (1 - f) * sample;
        current++;
      }
    }
    else
    {
      for(int32 i = 0; i < numSamples; i++)
      {
        auto sample = iReverse ? *iBuffer-- : *iBuffer++;
        fBuffer[i] = factor * i * sample;
      }
    }

    fFadeFrom = false;
    fCurrent = 0;

  }

  /**
   * Cross fade from the buffer to 0. Also handles the case when there is already cross fading happening.
   *
   * @tparam Iterator designed to be a "pointer" (need to support ++/--/*)
   */
  template<typename Iterator>
  void xFadeFrom(Iterator iBuffer, bool iReverse)
  {
    constexpr auto factor = (static_cast<SampleType>(1) / (numSamples - 1));

    if(fCurrent < numSamples)
    {
      int current = fCurrent;

      for(int32 i = 0; i < numSamples; i++)
      {
        auto sample = iReverse ? *iBuffer-- : *iBuffer++;
        auto sample1 = current < numSamples ? fBuffer[current] : 0;
        auto sample2 = factor * static_cast<float>(numSamples - 1 - i) * sample;
        auto f = factor * i;
        fBuffer[i] = f * sample2 + (1 - f) * sample1;
        current++;
      }
    }
    else
    {
      for(int32 i = 0; i < numSamples; i++)
      {
        auto sample = iReverse ? *iBuffer-- : *iBuffer++;
        fBuffer[i] = factor * static_cast<float>(numSamples - 1 - i) * sample;
      }
    }

    fFadeFrom = true;
    fCurrent = 0;
  }

  inline bool hasNext()
  {
    return fCurrent < numSamples;
  }

  inline SampleType next()
  {
    DCHECK_F(fCurrent >= 0);
    DCHECK_F(hasNext());
    auto res = fBuffer[fCurrent];
    fCurrent++;
    return res;
  }

  /**
   * @return `true` if the cross fader is currently fading to 0 */
  inline bool isFadingTo0() { return fFadeFrom && hasNext(); }

  /**
   * @return `true` if the fader has faded to 0 and is done */
  inline bool isDoneFadingTo0() { return fFadeFrom && !hasNext(); }

private:
  bool fFadeFrom{false};
  int32 fCurrent{numSamples};
  SampleType fBuffer[numSamples]{};
};

/**
 * Slices the sample from `fStart` to `fEnd` and keep track of where we are in the buffer (`fCurrent`).
 * Handles 1 shot (plays from `fStart` to `fEnd`) and reverse. Looping is handled at a higher level
 * (call `restart` when `next` returns `false`).
 *
 * Handles cross fading when enabled.
 *
 * Supposed to be used this way (java style iteration):
 * ```
 * slicer.reset(buffer, xx, yy);
 * slicer.start();
 * if(slicer.hasNext())
 *   slicer.next();
 * if(slicer.hasNext())
 *   slicer.next();
 * if(slicer.hasNext())
 *   slicer.next();
 * ...
 * slicer.requestStop(); // optionally
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
  void reset(SampleType const *iBuffer, int32 iStart, int32 iEnd)
  {
    // sanity check
    DCHECK_F(iStart >= 0);
    DCHECK_F(iEnd >= 0);
    DCHECK_F(iStart < iEnd);

    fStart = iStart;
    fEnd = iEnd;
    fBuffer = iBuffer;

    maybeDisableCrossFader();

    if(fCurrent != NOT_PLAYING)
      fCurrent = Utils::clamp(fCurrent, fStart, fEnd -1);
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

    maybeDisableCrossFader();

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
  void start()
  {
    fCurrent = fReverse ? fEnd - 1 : fStart;

    if(fXFaderEnabled)
    {
      fXFader.xFadeTo(getBuffer(fCurrent), fReverse);
    }
  }

  /**
   * Called when we want to stop iterating. Note that it may not stop right away due to cross fading.
   *
   * @return `true` if it ended right away (of if there is more to end (cross fading to 0))
   */
  inline bool requestStop()
  {
    if(fCurrent != NOT_PLAYING)
    {
      if(fXFaderEnabled)
      {
        if(!fXFader.isFadingTo0())
          fXFader.xFadeFrom(getBuffer(fCurrent), fReverse);
      }
      else
        fCurrent = NOT_PLAYING;
    }

    return fCurrent == NOT_PLAYING;
  }

  /**
   * Forces end no matter what */
  inline void hardStop() { fCurrent = NOT_PLAYING; }

  /**
   * @return `true` if there is a next step (which means it is ok to call `next`)
   */
  inline bool hasNext() { return fCurrent != NOT_PLAYING; }

  /**
   * Retrieves the current sample (applies cross fading if necessary)
   */
  inline SampleType next()
  {
    DCHECK_F(hasNext());

    // sanity check
    DCHECK_F(fCurrent >= 0);
    DCHECK_F(fCurrent >= fStart);
    DCHECK_F(fCurrent < fEnd);

    auto res = fXFaderEnabled && fXFader.hasNext() ? fXFader.next() : fBuffer[fCurrent];

    computeNext();

    return res;
  }

private:
  /**
   * Advances the state of the slicer to the next sample (takes looping, reverse and cross fading into consideration)
   *
   * @return `true` if there is more playing or `false` if done (at which point `hasNext` will always return `false`)
   */
  bool computeNext()
  {
    if(fCurrent == NOT_PLAYING)
      return false;

    if(fReverse)
    {
      fCurrent--;

      if(fCurrent < fStart)
        fCurrent = NOT_PLAYING;
      else
      {
        if(fXFaderEnabled && !fXFader.isFadingTo0() && fCurrent == fStart + numXFadeSamples - 1)
          fXFader.xFadeFrom(getBuffer(fCurrent), true);
      }
    }
    else
    {
      fCurrent++;

      if(fCurrent >= fEnd)
        fCurrent = NOT_PLAYING;
      else
      {
        if(fXFaderEnabled && !fXFader.isFadingTo0() && fCurrent == fEnd - numXFadeSamples)
          fXFader.xFadeFrom(getBuffer(fCurrent), false);
      }
    }

    if(fXFaderEnabled)
    {
      if(fXFader.isDoneFadingTo0())
        fCurrent = NOT_PLAYING;
    }

    return fCurrent != NOT_PLAYING;
  }

  /**
   * In the even there are not enough samples, we disable the cross fader */
  void maybeDisableCrossFader()
  {
    if(fStart != -1 && fEnd != -1 && fXFaderEnabled && fEnd - fStart < numXFadeSamples)
    {
      DLOG_F(WARNING, "not enough samples... disabling cross fader");
      fXFaderEnabled = false;
      fCurrent = NOT_PLAYING;
    }
  }

private:
#ifndef NDEBUG
  struct SafeBufferAccessor
  {
    inline SafeBufferAccessor& operator++() { fCurrent++; return *this; }
    inline SafeBufferAccessor operator++(int)
    {
      SafeBufferAccessor retval = *this;
      ++(*this);
      return retval;
    }

    inline SafeBufferAccessor& operator--() { fCurrent--; return *this; }
    inline SafeBufferAccessor operator--(int)
    {
      SafeBufferAccessor retval = *this;
      --(*this);
      return retval;
    }

    inline SampleType operator*() const
    {
      DCHECK_F(fCurrent >= fStart && fCurrent < fEnd);
      return fBuffer[fCurrent];
    }

    int32 fStart{-1};
    int32 fEnd{-1};
    SampleType const *fBuffer{};

    int32 fCurrent{-1};
  };

  // In dev mode, wraps the buffer access into an object to check boundaries
  inline SafeBufferAccessor getBuffer(int idx) { return {fStart, fEnd, fBuffer, idx}; }
#else
  // in production mode, simply returns the pointer directly
  inline SampleType const *getBuffer(int idx)
  {
    return &fBuffer[idx];
  }
#endif

private:
  int32 fStart{-1};
  int32 fEnd{-1};
  SampleType const *fBuffer{};
  int32 fCurrent{NOT_PLAYING};

  bool fReverse{false};

  bool fXFaderEnabled{true};
  LinearCrossFader<SampleType, numXFadeSamples> fXFader{};
};

}