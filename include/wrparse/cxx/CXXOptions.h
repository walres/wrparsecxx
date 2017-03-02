/**
 * \file CXXOptions.h
 *
 * \brief C/C++ language option constants and representative data type
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2014-2016 James S. Waller
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 * \endparblock
 */
#ifndef WRPARSECXX_OPTIONS_H
#define WRPARSECXX_OPTIONS_H

#include <cstdint>
#include <unordered_map>
#include <wrutil/CityHash.h>
#include <wrutil/string_view.h>
#include <wrutil/u8string_view.h>
#include <wrparse/cxx/Config.h>
#include <wrparse/cxx/CXXTokenKinds.h>


namespace wr {
namespace parse {


namespace cxx {


/**
 * \brief Constants denoting language standards
 */
enum : uint64_t
{
        C89 = 1,
        C90 = 2,
        C95 = 3,
        C99 = 4,
        C11 = 5,
        C_LATEST = C11,
        C_LANG = UINT64_C(0xff),

        CXX98 = 1 << 8,
        CXX03 = 2 << 8,
        CXX11 = 3 << 8,
        CXX14 = 4 << 8,
        CXX17 = 5 << 8,
        CXX_LATEST = CXX17,
        CXX_LANG = UINT64_C(0xff00)
};

using Languages = uint64_t;
using Language = uint64_t;

/**
 * \brief optional features for various language standards
 *
 * Unless otherwise specified these options are valid for any C or C++
 * language standard.
 */
enum : uint64_t
{
        KEEP_SPACE = UINT64_C(1),
                        ///< Lexer: record full content for whitespace tokens
        KEEP_COMMENTS = UINT64_C(1) << 1,
                        ///< Lexer: record full content for comments
        LINE_COMMENTS = UINT64_C(1) << 2,
                        /**< Lexer: recognise one-line comments prefixed
                             by <code>//</code> */
        LONG_LONG = UINT64_C(1) << 3,
                        /**< <code>long long</code> integer type;
                             standard from C99 and C++11 */
        DIGRAPHS = UINT64_C(1) << 4,
                        ///< digraph tokens; standard in C++ and from C95
        TRIGRAPHS = UINT64_C(1) << 5,
                        /**< interpret trigraph sequences;
                             standard until C++17 */
        BINARY_LITERALS = UINT64_C(1) << 6,
                        /**< <code>0b</code>-prefixed binary integer literals;
                             standard from C++14 */
        UTF8_CHAR_LITERALS = UINT64_C(1) << 7,
                        /**< UTF-8 character literals (<code>u8'...'</code>
                             syntax); standard from C++17, optional for C++11/14
                             and C11 only */
        HEX_FLOAT_LITERALS = UINT64_C(1) << 8,
                        /**< <code>0x</code>-prefixed hexadecimal floating point
                             literals; standard from C99 and C++17 */
        UCNS = UINT64_C(1) << 9,
                        /**< Allow use of universal <code>\u<i>xxxx</i></code>
                             and <code>\U<i>xxxxxxxx</i></code> character names;
                             standard from C99 and C++11 */
        IDENTIFIER_DOLLARS = UINT64_C(1) << 10,
                        ///< Allow use of dollar characters in identifiers
        INLINE_FUNCTIONS = UINT64_C(1) << 11,
                        /**< Inline function specifier;
                             standard from C99 and in C++ */
        NO_PP_DIRECTIVES = UINT64_C(1) << 12,
                        ///< Lexer: do not interpret preprocessor directives

        C89_STD_FEATURES = TRIGRAPHS,
        C90_STD_FEATURES = C89_STD_FEATURES,
        C95_STD_FEATURES = DIGRAPHS | TRIGRAPHS,
        C99_STD_FEATURES = C95_STD_FEATURES | LINE_COMMENTS | UCNS
                           | LONG_LONG | HEX_FLOAT_LITERALS | INLINE_FUNCTIONS,
        C11_STD_FEATURES = C99_STD_FEATURES,

        CXX98_STD_FEATURES = LINE_COMMENTS | DIGRAPHS | TRIGRAPHS
                             | INLINE_FUNCTIONS,
        CXX03_STD_FEATURES = CXX98_STD_FEATURES,
        CXX11_STD_FEATURES = CXX03_STD_FEATURES | LONG_LONG | UCNS,
        CXX14_STD_FEATURES = CXX11_STD_FEATURES | BINARY_LITERALS,
        CXX17_STD_FEATURES = (CXX14_STD_FEATURES ^ TRIGRAPHS)
                                | UTF8_CHAR_LITERALS | HEX_FLOAT_LITERALS
};

using Features = uint64_t;

using KeywordTable = std::unordered_map<u8string_view, TokenKind, CityHash>;


WRPARSECXX_API KeywordTable &addC89Keywords(KeywordTable &keywords);
WRPARSECXX_API KeywordTable &addC99Keywords(KeywordTable &keywords);
WRPARSECXX_API KeywordTable &addC11Keywords(KeywordTable &keywords);
WRPARSECXX_API KeywordTable &addCXX98Keywords(KeywordTable &keywords);
WRPARSECXX_API KeywordTable &addCXX11Keywords(KeywordTable &keywords);

// add latest version of language keywords
WRPARSECXX_API KeywordTable &addCKeywords(KeywordTable &keywords);
WRPARSECXX_API KeywordTable &addCXXKeywords(KeywordTable &keywords);


} // namespace cxx

//--------------------------------------

class WRPARSECXX_API CXXOptions
{
public:
        CXXOptions(cxx::Languages languages, cxx::Features extra_features = 0);

        cxx::Languages languages() const { return languages_; }

        cxx::Language c() const   { return languages_ & cxx::C_LANG; }
        cxx::Language cxx() const { return languages_ & cxx::CXX_LANG; }

        cxx::Features features() const            { return features_; }
        const cxx::KeywordTable &keywords() const { return keywords_; }

        bool have(cxx::Features want) const
                { return (features_ & want) == want; }

        static std::pair<cxx::Language, cxx::Language>
                language(const string_view &name);

        static std::pair<cxx::Language, cxx::Language>
                standard(const string_view &name);

        static std::string langName(cxx::Languages languages);
        static std::string stdName(cxx::Languages languages);

private:
        struct Internals;
        friend Internals;

        cxx::Languages    languages_;
        cxx::Features     features_;
        cxx::KeywordTable keywords_;
};


} // namespace parse
} // namespace wr


#endif // !WRPARSECXX_OPTIONS_H
