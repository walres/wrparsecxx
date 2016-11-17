# wrutil - General Purpose C++ Toolbox


## Introduction

`wrutil` is a small library of reusable C++ components intended to make multi-platform Unicode-friendly C++ applications easier and more pleasant to write, with a permissive license ([Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0)) and with minimal external dependencies. It aims to provide a consistent set of modern C++ library features across development environments and to augment the C++ standard library rather than trying to replace it. It takes advantage of and depends on features of the C++11 standard; *C++03, C++98 and earlier standards are not supported*.

`wrutil` attempts to smooth over differences between C++ standard library implementations of modern features (*e.g.* UTF-8/16/32 `codecvt` facets, `std::optional`, `std::filesystem`, `std::basic_string_view`) while making implementations available to environments missing them (typically by re-using code from [libc++](http://libcxx.llvm.org/) or [Boost](http://www.boost.org/), or its own implementation in the case of `basic_string_view`).


## Current Status

The current version of `wrutil` is 0.1.0.

**At present `wrutil` is in an experimental state and therefore neither its API nor its ABI are stable. Some components are more mature than others.** The library is structured such that it is straightforward to transplant portions of its code into other projects if that is preferred over linking with a pre-built `wrutil` library.

The library has a fledgling test suite which must be expanded urgently.

Documentation outside of this file is almost nonexistent at the moment; this must be tackled as a top priority.


## Features

### Type-Safe `printf()`-Style Text Formatting: `<wrutil/Format.h>`

`wr::print()` and `wr::printStr()` family of functions providing a type-safe, extensible and performant C++11-based version of the old favourite `printf()` function family. Follows the POSIX standard for `printf()` including numbered conversion specifiers (*i.e.* `%n$`) with some extensions. Directly usable with standard iostreams, C stdio `FILE` handles, fixed buffers, `std::string` objects and extendable to other targets. These functions do not use `std::iostream` objects for their implementation and do not require construction of separate objects to parse format strings.

### Flexible Command-Line Option Processing: `<wrutil/Option.h>`

`wr::Option` class and functions for concise declaration and handling of command-line options of various styles with minimal boilerplate code required. GNU `--long-option` style, traditional single-letter `-o` style (including direct concatenation of them where allowable), the `-option` style as used elsewhere, more esoteric styles like `+o` as used by some compilers are supported, plus the `/o` and `/option` style is supported on Windows. Arguments may be delimited by whitespace or joined directly to their options as desired. This was required to parse popular C++ compilers' options which often use a mixture of these styles.

### Unicode Code-Point Handling: `<wrutil/ctype.h>`, `<wrutil/UnicodeData.h>`

Better support for the `char32_t` type introduced in C++11, with an independent specialization of the `std::ctype` class (`std::ctype<char32_t>`) which works with Unicode code point values. Standalone functions `wr::isuspace(c)`, `wr::isualpha(c)`, etc. mirroring those provided by the C library are also provided plus extras such as `wr::digitval(c)` and `wr::xdigitval(c)` for obtaining decimal and hex digit values for appropriate code points.

### UTF-8 <-> Narrow String Transcoding: `<wrutil/codecvt.h>`

A subclass of `std::codecvt` (`wr::codecvt_utf8_narrow`) provides direct conversion between locale-dependent encoded 'narrow' `char *` strings and UTF-8 `char *` strings using the default locale or another explicitly specified locale. A utility class `wr::u8string_convert` based on `wr::wstring_convert`, with a thread-local instance available via the function `wr::utf8_narrow_cvt()`, can be used for easy conversion between narrow and UTF-8 strings. Likewise, a thread-local instance of a `wr::wstring_convert<wchar_t>` class is available via the function `wr::wide_narrow_cvt()` for conversion between narrow and wide strings.

### Platform Independent Unicode Input/Output/Error iostreams: `<wrutil/uiostream.h>`

A set of iostreams `wr::uin`, `wr::uout`, `wr::uerr` and `wr::ulog` mirroring `std::cout` etc. which expect all `char *` based strings to be encoded as UTF-8. Automatic conversion to/from the external encoding is performed where necessary; raw Unicode input/output (*i.e.* no codepage conversion) is used on the Windows console.

### UTF-8 String View: `<wrutil/u8string_view.h>`

A workalike of the `basic_string_view` template, `wr::u8string_view` wraps a `char *` string and presents it as a sequence of 32-bit Unicode code points of type `char32_t`. It is interoperable with raw `char *` arrays, `std::string` objects and `wr::basic_string_view` (plus `std::basic_string_view` when available).

### Missing C++ Library `codecvt` Classes: `<wrutil/codecvt.h>`

For older not-quite-compliant versions of the C++11 standard library, `wrutil` provides an implementation of `codecvt_utf8` under the `::wr` namespace and implementations of the `std::codecvt<char16_t, char, std::mbstate_t>` and `std::codecvt<char32_t, char, std::mbstate_t>` specializations if they are missing from the host C++ library. When `codecvt_utf8` is available in the standard library `wr::codecvt_utf8` is simply aliased to `std::codecvt_utf8`.
        
### C++17 Library Features

`<wrutil/string_view.h>`: `wr::basic_string_view`, an extended version of the C++17 `std::basic_string_view` template with extra functionality such as case conversion, whitespace trimming and tokenisation.

`<wrutil/optional.h>`: `wr::optional`, an alias of `std::optional` where available, but an internal implementation (taken from the libcxx library) is used in other cases.

`<wrutil/filesystem.h>`: `wr::path` and filesystem functions - aliased to `std::filesystem` where available, otherwise `boost::filesystem`; provides all the functionality described as part of the C++17 filesystem module and fills in any missing features such as `relative()`, `weakly_canonical()`, `unique_path()` and UTF-8 path transcoding.

### Circular Singly Linked Lists: `<wrutil/circ_fwd_list.h>`

Intrusive and non-intrusive circular singly-linked lists are implemented by the class templates `wr::intrusive_circ_fwd_list` and `wr::circ_fwd_list` respectively. The interface is similar to that of `std::forward_list` except it provides constant-time `push_back()` and unlike the equivalent classes in the Boost.Intrusive library it is possible to use a smart pointer type for the link pointers by specializing the `wr::intrusive_list_traits` template.

### CityHash Functions: `<wrutil/CityHash.h>`

Google's [CityHash](https://github.com/google/cityhash) functions `CityHash32()`, `CityHash64()`, `CityHash128()` and companions made available under the `::wr` namespace along with a functor class `wr::CityHash` for use with standard library hash containers such as `std::unordered_map` which works directly with `const char *` arrays, `std::string`, `wr::string_view` and `wr::u8string_view` objects.

### SHA-256 Hashing: `<wrutil/SHA256.h>`

`wr::SHA256` incrementally generates an 256-bit SHA-2 cryptographic hash from textual or binary input using [the algorithm described by NIST](https://web.archive.org/web/20130526224224/http://csrc.nist.gov/groups/STM/cavp/documents/shs/sha256-384-512.pdf).

### Unit Testing: `<wrutil/TestManager.h>`

The class `wr::TestManager` provides a quick and easy means of writing test suite programs, each test case being uniquely identifiable and executed inside its own child process with time limiting and crash detection. Program options allow tests to be singled out and executed individually for easier debugging.

### Debugging Support - Independent Exception Stack Traces: `<wrutil/debug.h>`

When the application is linked with the shared library `wrdebug` (built alongside the `wrutil` library) `wr::dumpException()` can be used within any `catch` statement to obtain a stack trace of the exception source, including within `catch (...)` statements. Functions with all existing exception types.

**NOTE:** At present code must be compiled with debugging information to get meaningful output; optimisation can significantly disrupt the output and executables on Unix-like systems generally must be linked with a dynamic symbol export option (*e.g.* `-E` option for many linkers)

### Checked Numeric Casting: `<wrutil/numeric_cast.h>`

Imports `boost::numeric_cast` into the `::wr` namespace along with its associated exception types: `boost::numeric::bad_numeric_cast`, `boost::numeric::positive_overflow` and `boost::numeric::negative_overflow` from the Boost.Numeric library.

### Restore Variable Values On Scope Exit: `<wrutil/VarGuard.h>`

The `wr::VarGuard` class template can be used to save a variable's value and selectively restore the original value automatically upon exiting the scope in which it was declared.

### `fopen()` and `popen()` C Wrappers: `<wrutil/StdioFilePtr.h>`

The function `wr::fopen()` wraps `::fopen()`, taking its path as a `wr::path` object and calling the underlying function most appropriate for the platform (*e.g.* `_wfopen()` for Windows) to avoid data loss due to path encoding issues when non-ASCII paths are specified. A similar function `wr::popen()` is provided for opening a pipe to a child process. Both functions return their `FILE` pointers enclosed in `std::unique_ptr` objects which are set up to call the appropriate function (`fclose()` or `pclose()`) to automatically close the handle on expiry.


## Prerequisites

* C++11-capable C++ compiler (*e.g.* GCC 4.7 or later, Clang 3.3 or later, Visual C++ 2015 or later)
* CMake 2.6 or later for the build system
* Boost 1.44 or later


## Tested Platforms and Compilers

### Linux (any distro):

* GCC 4.7 or later
* Clang 3.3 or later

### Windows 7 (x86/x64)

* Visual Studio 2015 or later (earlier versions very unlikely to work)

`wrutil` should work with Windows XP or later versions.

### Others

Ports to other platforms (Mac OS X, FreeBSD) are very much desired. Most Unix or Unix-like systems should be straightforward to port to. Beware there may be trickiness with the encoding of `wchar_t` being locale-dependent on some platforms *e.g.* Solaris and BSDs. Another potentially tricky area is the exception tracing code; platforms using the Itanium C++ ABI should be pretty much taken care of already but a potential issue is non-availability of the `libunwind` API.


## Building and Installation

CMake 2.6 or later is required to generate makefiles or IDE project files.

These instructions assume that you have already downloaded the `wrutil` source code (and unpacked it if necessary) to an accessible filesystem directory.

### Linux with GNU Make

From a terminal with the current directory set to the wrutil source tree's top directory, issue these commands:

1. `mkdir build`
2. `cd build`
3. `cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=<install-base-path> ..`
4. `make`
5. `make test`          (omit if you don't wish to run tests)
6. `make install`       (omit if you don't wish to install)

For step 3 it may be necessary to add `-DBOOST_ROOT=<boost-base-path>` if Boost is not installed at a location where the C++ compiler and linker can automatically find Boost's header files and libraries. The `-DCMAKE_INSTALL_PREFIX=` option can be omitted if also omitting step 6.

Please refer to the section *Influential CMake Settings* for more details on build configuration settings.

### Windows with Visual Studio 2015

If building 64-bit code then open a *VS???? x64 Native Tools Command Prompt*; this is located under the Start menu entry *Visual Studio ????* -> *Visual Studio Tools* -> *Windows Desktop Command Prompts* (where *????* is your Visual Studio version).

If building 32-bit code then open a *VS???? x86 Native Tools Command Prompt*; this is located under the Start menu entry *Visual Studio ????* -> *Visual Studio Tools* -> *Windows Desktop Command Prompts*.

Within the command prompt change to the top directory of the `wrutil` source tree and issue the following commands:

1. `mkdir build`
2. `cd build`
3. *for 64-bit build:* `cmake -G "Visual Studio 14 2015 Win64" -DBOOST_ROOT=<boost-base-path> ..`
   *for 32-bit build:* `cmake -G "Visual Studio 14 2015" -DBOOST_ROOT=<boost-base-path> ..`

From Windows Explorer or Visual Studio itself, open the file `wrutil.sln` that is generated by CMake. You should now be able to build all targets.

To run and debug tests the DLL search path must be set for the projects as follows so they can find the `wrutil` and `wrdebug` DLLs produced by the build:

1. Select all projects (but not the solution) in the *Solution Explorer* (all projects must be collapsed first); right click the selected projects and go to *Properties*
2. Ensure *All Configurations* is set in the *Configuration* combo box of the *Property Pages* dialog.
3. Select the *VC++ Directories* section under *Configuration Properties*
4. Add the following path to *Executable Directories*: `$(SolutionDir)\bin\$(Configuration)`
5. Select the *Debugging* section under *Configuration Properties*
6. Ensure *Merge Environment* is set to `Yes`
7. Add the following string to *Environment*: `PATH=$(SolutionDir)\bin\$(Configuration)`
8. Click *OK*

The tests can be then be run by explicitly building the `RUN_TESTS` pseudo-project. Steps 3-4 are necessary for `RUN_TESTS` to work; steps 5-7 are necessary to allow debugging.

Please refer to the section *Influential CMake Settings* for more details on build configuration settings.

### Windows with Visual C++ compiler and NMake

If building 64-bit code then open a *VS???? x64 Native Tools Command Prompt*; this is located under the Start menu entry *Visual Studio ????* -> *Visual Studio Tools* -> *Windows Desktop Command Prompts* (where *????* is your Visual Studio version).

If building 32-bit code then open a *VS???? x86 Native Tools Command Prompt*; this is located under the Start menu entry *Visual Studio ????* -> *Visual Studio Tools* -> *Windows Desktop Command Prompts*.

Within the command prompt change to the top directory of the `wrutil` source tree and issue the following commands:

1. `mkdir build`
2. `cd build`
3. `cmake -G "NMake Makefiles" -DCMAKE_INSTALL_PREFIX=<install-base-path> -DBOOST_ROOT=<boost-base-path> ..`
4. `nmake`
5. `nmake test`         (omit if you don't wish to run tests)
6. `nmake install`      (omit if you don't wish to install)

For step 3 the `-DCMAKE_INSTALL_PREFIX=` option can be omitted if also omitting step 6.

Please refer to the section *Influential CMake Settings* for more details on build configuration settings.

### Windows Notes

When compiling programs that link with `wrutil.dll` ensure that `wrutil_IMPORTS` is defined by the preprocessor. If a program links with `wrdebug.dll` ensure that `wrdebug_IMPORTS` is defined by the preprocessor. If this is not done then the linker will emit unresolved symbol errors.

If your own project uses CMake, includes the installed file `share\wrutil\wrutil.cmake` underneath the top-level installation directory and references the `wrutil` and `wrdebug` projects as dependencies then this should be done automatically.

### Influential CMake Settings

* `-DCMAKE_CXX_COMPILER=<compiler>`: C++ compiler executable, *e.g.* `g++`, `clang++`; prefix with directory if necessary
* `-DCMAKE_BUILD_TYPE=<type>`: where `<type>` is `Debug`, `RelWithDebInfo` or `Release`
* `-DCMAKE_INSTALL_PREFIX=<install-base-path>`: set to intended base directory of the `wrutil` installation; header files will be installed in `include/wrutil`, libraries in `lib` and CMake project files in `share/wrutil` inside the base directory
* `-DBOOST_ROOT=<boost-base-path>`: set to base directory of Boost installation
* `-DUSE_CXX14=<1|ON|YES|TRUE|0|OFF|NO|FALSE>`: set to `1`, `ON`, `YES` or `TRUE` to impose C++14 language standard (necessary to use `std::optional`, `std::basic_string_view` and/or `std::filesystem` with some standard C++ libraries)
* `-DUSE_CXX17=<1|ON|YES|TRUE|0|OFF|NO|FALSE>`: set to `1`, `ON`, `YES` or `TRUE` to impose C++17 language standard


## Future Work

* **Priority! Documentation**
* **Priority! More unit tests**
* Improve handling of different character types with `wr::print()`; new overloads for direct printing to UTF-16/wide strings
* Pattern matching function (like POSIX `fnmatch()`) for `wr::path` objects
* More convenience functions for converting between UTF-8/UTF-16/wide strings
* Greater use of `constexpr` and `noexcept`, language standard permitting
* Nice to have: More C++17 library features (*e.g.* `any`, `polymorphic_allocator`)
* Nice to have: Decimal floating-point type support using [decNumber](http://speleotrove.com/decimal/decnumber.html) or [Intel](https://software.intel.com/en-us/articles/intel-decimal-floating-point-math-library) library implementations: provide basic types `decimal32`/`decimal64`/`decimal128` and `BigDecimal` class (like C#/Java equivalents); provide support for using these types with `wr::print()` functions
