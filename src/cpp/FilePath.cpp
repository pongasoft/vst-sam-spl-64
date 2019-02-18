#include <pongasoft/logging/logging.h>
#include <pongasoft/Utils/Clock/Clock.h>
#include <pluginterfaces/base/ftypes.h>
#include <sstream>

#include "FilePath.h"

#if SMTG_OS_WINDOWS
#include <windows.h>
#include <tchar.h>
#else
#include <cstdlib>
#endif

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace Steinberg;

//------------------------------------------------------------------------
// getTemporaryPath
//------------------------------------------------------------------------
Path getTemporaryPath()
{
// Implementation notes:
// * this code is inspired by boost library
//   https://github.com/boostorg/filesystem/blob/boost-1.69.0/src/operations.cpp#L1857
//   Although it would be ideally preferable to use the boost library directly
//   as of this writing (Feb 2019), having to download and compile the library in order to use it
//   does not fit very well with how this project is structured (dependencies are downloaded).
// * the boost library does not use GetTempPath which is available in Windows but rely on
//   environment variables instead

#if SMTG_OS_WINDOWS
  // Windows Implementation

  // From https://docs.microsoft.com/en-us/windows/desktop/FileIO/creating-and-using-a-temporary-file
  path_char_type lpTempPathBuffer[MAX_PATH];
  auto dwRetVal = GetTempPath(MAX_PATH,lpTempPathBuffer);
  if(dwRetVal > MAX_PATH || (dwRetVal == 0))
  {
    LOG_F(ERROR, "Cannot get access to temporary folder");
    return L"C:\\Temp";
  }
  return lpTempPathBuffer;
#else
  // other implementation
  const char* val = nullptr;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
  (val = std::getenv("TMPDIR" )) ||
  (val = std::getenv("TMP"    )) ||
  (val = std::getenv("TEMP"   )) ||
  (val = std::getenv("TEMPDIR"));
#pragma clang diagnostic pop

  if(!val)
    val = "/tmp";

  return val;

#endif
}

//------------------------------------------------------------------------
// createTempFilePath
//------------------------------------------------------------------------
Path createTempFilePath(UTF8Path const &iFilename)
{
  // this id will be unique across multiple instances of the plugin running in the same DAW
  static std::atomic<int32> unique_id(0);
  // this id will be unique to a DAW (very unlikely that 2 DAWs could start at exactly the same time)
  static auto time_id = Clock::getCurrentTimeMillis();

  Path tempFilePath = getTemporaryPath();

  std::basic_ostringstream<path_char_type> tempFilename;

  auto now = Clock::getCurrentTimeMillis();

  tempFilename << "sam_spl64_" << time_id << "_" << (now - time_id) << "_" << unique_id.fetch_add(1);

  auto filepath = iFilename.toNativePath();

  auto found = filepath.rfind('.');
  if(found == std::string::npos)
    tempFilename << ".raw";
  else
    tempFilename << filepath.substr(found);

  tempFilePath += tempFilename.str();

  return tempFilePath;
}

//------------------------------------------------------------------------
// basic_UTF8Path<char>::toNativePath
//------------------------------------------------------------------------
template<>
std::basic_string<char> basic_UTF8Path<char>::toNativePath() const
{
  return data();
}

//------------------------------------------------------------------------
// basic_UTF8Path<char>::fromNativePath
//------------------------------------------------------------------------
template<>
basic_UTF8Path<char> basic_UTF8Path<char>::fromNativePath(const std::basic_string<char> &iPath)
{
  return basic_UTF8Path<char>(iPath);
}

//------------------------------------------------------------------------
// basic_UTF8Path<wchar_t>::toNativePath
//------------------------------------------------------------------------
template<>
std::basic_string<wchar_t> basic_UTF8Path<wchar_t>::toNativePath() const
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  return converter.from_bytes(data());
}

//------------------------------------------------------------------------
// basic_UTF8Path<wchar_t>::fromNativePath
//------------------------------------------------------------------------
template<>
basic_UTF8Path<wchar_t> basic_UTF8Path<wchar_t>::fromNativePath(const std::basic_string<wchar_t> &iPath)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  return converter.to_bytes(iPath);
}


}
}
}