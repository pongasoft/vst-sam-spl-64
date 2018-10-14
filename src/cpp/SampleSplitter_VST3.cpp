//------------------------------------------------------------------------------------------------------------
// This file contains the standard boilerplate code that VST3 sdk requires to instantiate the plugin
// components
//------------------------------------------------------------------------------------------------------------
#include "SampleSplitterCIDs.h"

#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <public.sdk/source/main/pluginfactoryvst3.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>

#include "version.h"
#include "RT/SampleSplitterProcessor.h"
#include "GUI/SampleSplitterController.h"

using namespace Steinberg::Vst;


#define stringPluginName "SampleSplitter"

//------------------------------------------------------------------------
//  Module init/exit
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// called after library was loaded
bool InitModule()
{
  return true;
}

//------------------------------------------------------------------------
// called after library is unloaded
bool DeinitModule()
{
  return true;
}

//------------------------------------------------------------------------
//  VST Plug-in Entry
//------------------------------------------------------------------------
BEGIN_FACTORY_DEF ("pongasoft",
                   "https://www.pongasoft.com",
                   "support@pongasoft.com")

    // JSGainProcessor processor
    DEF_CLASS2 (INLINE_UID_FROM_FUID(pongasoft::VST::SampleSplitter::SampleSplitterProcessorUID),
                PClassInfo::kManyInstances,  // cardinality
                kVstAudioEffectClass,    // the component category (do not changed this)
                stringPluginName,      // here the Plug-in name (to be changed)
                Vst::kDistributable,  // means that component and controller could be distributed on different computers
                "Fx",          // Subcategory for this Plug-in (to be changed)
                FULL_VERSION_STR,    // Plug-in version (to be changed)
                kVstVersionString,    // the VST 3 SDK version (do not changed this, use always this define)
                pongasoft::VST::SampleSplitter::RT::SampleSplitterProcessor::createInstance)  // function pointer called when this component should be instantiated

    // JSGainController controller
    DEF_CLASS2 (INLINE_UID_FROM_FUID(pongasoft::VST::SampleSplitter::SampleSplitterControllerUID),
                PClassInfo::kManyInstances,  // cardinality
                kVstComponentControllerClass,// the Controller category (do not changed this)
                stringPluginName
                "Controller",  // controller name (could be the same than component name)
                0,            // not used here
                "",            // not used here
                FULL_VERSION_STR,    // Plug-in version (to be changed)
                kVstVersionString,    // the VST 3 SDK version (do not changed this, use always this define)
                pongasoft::VST::SampleSplitter::GUI::SampleSplitterController::createInstance)// function pointer called when this component should be instantiated

END_FACTORY
