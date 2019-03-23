#pragma once

#include <string>
#include <fstream>
#include <vstgui4/vstgui/lib/cstring.h>
#include <codecvt>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

/**
 * Implementation note: On Windows, file paths are wchar_t type and on macOS they are char (utf8 encoded).
 */
#if SMTG_OS_WINDOWS
using path_char_type = wchar_t;
#else
using path_char_type = char;
#endif

/**
 * Defines the aliases to use when the underlying char array (either represented as a std::string or char const *)
 * represents a properly utf-8 encoded string
 */
using UTF8CPPString = std::string;
using UTF8CString = char const *;

/**
 * Generic class to encapsulate a UTF-8 encoded path with method to convert to native path. The SDK uses UTF-8
 * encoded strings throughout. For example, the `CNewFileSelector` class returns
 * `UTF8StringPtr` filenames. This class is used in APIs to enforce the fact that the path is UTF-8 encoded
 * and in order to use it for actually opening a file, you should use the native path like this
 *
 *        std::ifstream input(path.toNativePath(), std::fstream::binary)
 *        std::ofstream output(path.toNativePath(), std::fstream::binary)
 *
 * @tparam CharT
 */
template<typename CharT>
class basic_UTF8Path
{
public:
  // actual type for the platform specific native path encoding (char or wchar_t)
  using NativePath = std::basic_string<CharT>;

public:
  basic_UTF8Path() : fPath() {}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"
  basic_UTF8Path(VSTGUI::UTF8String iPath) : fPath(std::move(iPath)) {}
  basic_UTF8Path(UTF8CPPString iPath) : fPath(std::move(iPath)) {}
  basic_UTF8Path(UTF8CString iPath) : fPath(iPath) {}
#pragma clang diagnostic pop

  inline VSTGUI::UTF8String const &utf8_str() const { return fPath; }
  inline UTF8CPPString const &cpp_str() const { return fPath.getString(); }
  inline UTF8CString c_str() const { return fPath.data(); }

  NativePath toNativePath() const;
  static basic_UTF8Path<CharT> fromNativePath(NativePath const &iPath);

private:
  VSTGUI::UTF8String fPath;
};

/**
 * Instantiation of the template with the proper type for the platform
 */
using UTF8Path = basic_UTF8Path<path_char_type>;

/**
 * @return the temporary path where temporary files can be created (os specific) */
UTF8Path getTemporaryPath();

/**
 * Creates a temporary file path from a temporary filename
 * @return the full path to the file
 */
UTF8Path createTempFilePath(UTF8Path const &iFilename);

// basic_UTF8Path::toNativePath => char implementation
template<>
std::basic_string<char> basic_UTF8Path<char>::toNativePath() const;

// basic_UTF8Path::fromNativePath => char implementation
template<>
basic_UTF8Path<char> basic_UTF8Path<char>::fromNativePath(const std::basic_string<char> &iPath);

// basic_UTF8Path::toNativePath => wchar_t implementation
template<>
std::basic_string<wchar_t> basic_UTF8Path<wchar_t>::toNativePath() const;

// basic_UTF8Path::fromNativePath => wchar_t implementation
template<>
basic_UTF8Path<wchar_t> basic_UTF8Path<wchar_t>::fromNativePath(const std::basic_string<wchar_t> &iPath);

}
}
}