//------------------------------------------------------------------------------------------------------------
// This file contains the standard boilerplate code that VST3 sdk requires to instantiate the plugin
// components
//------------------------------------------------------------------------------------------------------------
#include "FilePath.h"

#include "SampleSplitterCIDs.h"

#include "version.h"
#include "RT/SampleSplitterProcessor.h"
#include "GUI/SampleSplitterController.h"

#include <pongasoft/VST/PluginFactory.h>

using namespace pongasoft::VST;

#ifndef NDEBUG
#if SMTG_OS_WINDOWS
#include <public.sdk/source/main/moduleinit.h>
//------------------------------------------------------------------------
// called after library was loaded
static Steinberg::ModuleInitializer InitPlugin([] () {
  auto stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  if(stdoutHandle <= 0)
  {
    auto logFilePath = pongasoft::VST::SampleSplitter::createTempFilePath("out.log");
    // this will NOT work if the path contains UT8-encoded characters but is a workaround to the
    // fact that loguru does not support wchar_t... see https://github.com/emilk/loguru/issues/100
    // Should be using logFilePath.toNativePath() if/when loguru supports windows files...
    loguru::add_file(logFilePath.c_str(), loguru::Truncate, loguru::Verbosity_MAX);
  }
});
#endif
#endif

//------------------------------------------------------------------------
//  VST3 Plugin Main entry point
//------------------------------------------------------------------------
SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory()
{
  return JambaPluginFactory::GetNonDistributableVST3PluginFactory<
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
