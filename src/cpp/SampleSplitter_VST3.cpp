//------------------------------------------------------------------------------------------------------------
// This file contains the standard boilerplate code that VST3 sdk requires to instantiate the plugin
// components
//------------------------------------------------------------------------------------------------------------
#include "SampleSplitterCIDs.h"

#include "version.h"
#include "RT/SampleSplitterProcessor.h"
#include "GUI/SampleSplitterController.h"

#include <pongasoft/VST/PluginFactory.h>

#if SMTG_OS_WINDOWS
#include "FilePath.h"
#include <windows.h>
#endif

using namespace pongasoft::VST;

//------------------------------------------------------------------------
//  Module init/exit
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// called after library was loaded
bool InitModule()
{
#ifndef NDEBUG
#if SMTG_OS_WINDOWS
  auto stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE); 
  if(stdoutHandle <= 0)
  {
    auto logFilePath = pongasoft::VST::SampleSplitter::createTempFilePath("out.log");
    // this will NOT work if the path contains UT8-encoded characters but is a workaround to the
    // fact that loguru does not support wchar_t... see https://github.com/emilk/loguru/issues/100
    // Should be using logFilePath.toNativePath() if/when loguru supports windows files...
    loguru::add_file(logFilePath.c_str(), loguru::Truncate, loguru::Verbosity_MAX);
  }
#endif
#endif

  return true;
}

//------------------------------------------------------------------------
// called after library is unloaded
bool DeinitModule()
{
  return true;
}

//------------------------------------------------------------------------
//  VST3 Plugin Main entry point
//------------------------------------------------------------------------
EXPORT_FACTORY Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory()
{
  return JambaPluginFactory::GetVST3PluginFactory<
    pongasoft::VST::SampleSplitter::RT::SampleSplitterProcessor,  // processor class (Real Time)
    pongasoft::VST::SampleSplitter::GUI::SampleSplitterController // controller class (GUI)
  >("pongasoft",                      // company/vendor
    "https://www.pongasoft.com",      // url
    "support@pongasoft.com",          // email
    stringPluginName,                 // plugin name
    FULL_VERSION_STR,                 // plugin version
    Vst::PlugType::kInstrumentSampler // Instrument type!
  );
}
