#include "Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

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
    if(fFileSample->getNumSamples() > 0)
    {
      DLOG_F(INFO, "GUIState::read - broadcasting sample / %f", fSampleRate.getValue());
      fFileSample.broadcast();
    }

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
  if(fSampleRate > 0 && fFileSample->getSampleRate() > 0)
  {
    if(fSampleRate == fFileSample->getSampleRate())
    {
      DLOG_F(INFO, "same sample rate => broadcasting original");
      return fFileSample.broadcast();
    }
    else
    {
      DLOG_F(INFO, "different sample rate => resampling and broadcasting");
      auto ptr = fFileSample->resample(fSampleRate);
      return broadcast(fParams.fFileSample, *ptr);
    }
  }

  return kResultOk;
}

}
}
}