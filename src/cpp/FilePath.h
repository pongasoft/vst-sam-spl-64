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
 * Represents a "native" path. Note that C++17 has the concept of `std::filesystem::path` but unfortunately
 * it is not supported with XCode 9.2 (or 10...). `std::filesystem::path` is a class which uses the proper
 * char type. For the purpose of this code, simply creating an alias to the proper `string` class.
 */
using Path = std::basic_string<path_char_type>;

/**
 * Generic class to encapsulates the fact that a `std::string` which is UT8 encoded (which is what the VST
 * SDK uses) can be turned into a native path. For example, the `CNewFileSelector` class returns
 * `UTF8StringPtr` filenames. This class can then turn those into a native path for use when opening files.
 *
 * @tparam CharT
 */
template<typename CharT>
class basic_UTF8Path : public VSTGUI::UTF8String
{
public:
  using VSTGUI::UTF8String::UTF8String;

  std::basic_string<CharT> toNativePath() const;
  static basic_UTF8Path<CharT> fromNativePath(std::basic_string<CharT> const &iPath);
};

/**
 * Instantiation of the template with the proper type for the platform
 */
using UTF8Path = basic_UTF8Path<path_char_type>;

/**
 * @return the temporary path where temporary files can be created (os specific) */
Path getTemporaryPath();

/**
 * Creates a temporary file path from a temporary filename
 * @return the full path to the file
 */
Path createTempFilePath(UTF8Path const &iFilename);

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