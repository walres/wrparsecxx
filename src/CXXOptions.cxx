/**
 * \file CXXOptions.cxx
 *
 * \brief Implementation of C/C++ language options data type and definition of
 *      C/C++ keywords
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
#include <stdexcept>
#include <wrutil/Format.h>
#include <wrparse/cxx/CXXOptions.h>


namespace wr {
namespace parse {


struct CXXOptions::Internals
{
        template <typename Data, size_t N> static void
        initLanguage(
                CXXOptions     &me,
                const Data    (&data)[N],
                cxx::Language   selected,
                const char     *lang_name
        )
        {
                if (selected) {
                        bool ok = false;

                        for (const auto &lang: data) {
                                if (lang.standard == selected) {
                                        ok = true;
                                        me.features_ |= lang.features;
                                        (*lang.add_keywords)(me.keywords_);
                                        break;
                                }
                        }

                        if (!ok) {
                                throw std::runtime_error(
                                        printStr("invalid %s language standard",
                                                 lang_name));
                        }
                }
        }
};

//--------------------------------------

WRPARSECXX_API
CXXOptions::CXXOptions(
        cxx::Languages languages,
        cxx::Features  extra_features
) :
        languages_(languages),
        features_ (0)
{
        if (!c() && !cxx()) {
                throw std::invalid_argument("no language selected");
        }

        if ((c() && (c() < cxx::C11)) || (cxx() && (cxx() < cxx::CXX11))) {
                if ((c() >= cxx::C11) || (cxx() >= cxx::CXX11)) {
                        ;  // higher of the two standards is satisfactory
                } else if (extra_features & cxx::UTF8_CHAR_LITERALS) {
                        throw std::invalid_argument(printStr(
                                "UTF-8 character literals not available before C11/C++11"));
                }
        }

        static const struct {
                cxx::Language        standard;
                cxx::Features        features;
                cxx::KeywordTable &(*add_keywords)(cxx::KeywordTable &);
        } C_LANG_DATA[] = {
                { cxx::C89, cxx::C89_STD_FEATURES, &cxx::addC89Keywords },
                { cxx::C90, cxx::C90_STD_FEATURES, &cxx::addC89Keywords },
                { cxx::C95, cxx::C95_STD_FEATURES, &cxx::addC89Keywords },
                { cxx::C99, cxx::C99_STD_FEATURES, &cxx::addC99Keywords },
                { cxx::C11, cxx::C11_STD_FEATURES, &cxx::addC11Keywords },
        }, CXX_LANG_DATA[] = {
                { cxx::CXX98, cxx::CXX98_STD_FEATURES, &cxx::addCXX98Keywords },
                { cxx::CXX03, cxx::CXX03_STD_FEATURES, &cxx::addCXX98Keywords },
                { cxx::CXX11, cxx::CXX11_STD_FEATURES, &cxx::addCXX11Keywords },
                { cxx::CXX14, cxx::CXX14_STD_FEATURES, &cxx::addCXX11Keywords },
                { cxx::CXX17, cxx::CXX17_STD_FEATURES, &cxx::addCXX11Keywords },
        };

        Internals::initLanguage(*this, C_LANG_DATA, c(), "C");
        Internals::initLanguage(*this, CXX_LANG_DATA, cxx(), "C++");

        if (extra_features & cxx::INLINE_FUNCTIONS) {
                keywords_.insert({ u8"inline", cxx::TOK_KW_INLINE });
        }
        if (extra_features & cxx::NO_PP_DIRECTIVES) {
                features_ |= cxx::NO_PP_DIRECTIVES;
        }
}

//--------------------------------------

std::pair<cxx::Language, cxx::Language>
CXXOptions::language(
        const string_view &name
)
{
        static const struct {
                const char * const name;
                cxx::Language      lang,
                                   mask;
        } NAMES[] = {
                { "c", cxx::C_LATEST, cxx::C_LANG },
                { "c++", cxx::CXX_LATEST, cxx::CXX_LANG },
        };

        std::string low_name = name.to_lower();

        for (const auto &data: NAMES) {
                if (low_name == data.name) {
                        return { data.lang, data.mask };
                }
        }

        return { 0, 0 };
}

//--------------------------------------

std::pair<cxx::Language, cxx::Language>
CXXOptions::standard(
        const string_view &name
)
{
        static const struct {
                const char * const name;
                cxx::Language      std,
                                   mask;
        } NAMES[] = {
                { "c89", cxx::C89, cxx::C_LANG },
                { "c90", cxx::C90, cxx::C_LANG },
                { "c95", cxx::C95, cxx::C_LANG },
                { "c99", cxx::C99, cxx::C_LANG },
                { "c11", cxx::C11, cxx::C_LANG },
                { "c++98", cxx::CXX98, cxx::CXX_LANG },
                { "c++03", cxx::CXX03, cxx::CXX_LANG },
                { "c++0x", cxx::CXX11, cxx::CXX_LANG },
                { "c++11", cxx::CXX11, cxx::CXX_LANG },
                { "c++1y", cxx::CXX14, cxx::CXX_LANG },
                { "c++14", cxx::CXX14, cxx::CXX_LANG },
                { "c++1z", cxx::CXX17, cxx::CXX_LANG },
                { "c++17", cxx::CXX17, cxx::CXX_LANG },
        };

        std::string low_name = name.to_lower();

        for (const auto &data: NAMES) {
                if (name == data.name) {
                        return { data.std, data.mask };
                }
        }

        return { 0, 0 };
}

//--------------------------------------

std::string
CXXOptions::langName(
        cxx::Languages languages
)
{
        std::string name;

        if (languages & cxx::C_LANG) {
                name += "C";
        }

        if (languages & cxx::CXX_LANG) {
                if (!name.empty()) {
                        name += '/';
                }
                name += "C++";
        }

        if (name.empty()) {
                name = "unknown";
        }

        return name;
}

//--------------------------------------

std::string
CXXOptions::stdName(
        cxx::Languages languages
)
{
        std::string name;

        switch (languages & cxx::C_LANG) {
        case cxx::C89:
                name += "C89";
                break;
        case cxx::C90:
                name += "C90";
                break;
        case cxx::C95:
                name += "C95";
                break;
        case cxx::C99:
                name += "C99";
                break;
        case cxx::C11:
                name += "C11";
                break;
        default:
                return "unknown";
        }

        if (!name.empty() && (languages & cxx::CXX_LANG)) {
                name += '/';
        }

        switch (languages & cxx::CXX_LANG) {
        case cxx::CXX98:
                name += "C++98";
                break;
        case cxx::CXX03:
                name += "C++03";
                break;
        case cxx::CXX11:
                name += "C++11";
                break;
        case cxx::CXX14:
                name += "C++14";
                break;
        case cxx::CXX17:
                name += "C++17";
                break;
        default:
                return "unknown";
        }

        return name;
}

//--------------------------------------

WRPARSECXX_API cxx::KeywordTable &
cxx::addC89Keywords(
        KeywordTable &keywords
)
{
        static const std::pair<const char *, TokenKind> ENTRIES[] = {
                { u8"auto", TOK_KW_AUTO },
                { u8"break", TOK_KW_BREAK },
                { u8"case", TOK_KW_CASE },
                { u8"char", TOK_KW_CHAR },
                { u8"const", TOK_KW_CONST },
                { u8"continue", TOK_KW_CONTINUE },
                { u8"default", TOK_KW_DEFAULT },
                { u8"do", TOK_KW_DO },
                { u8"double", TOK_KW_DOUBLE },
                { u8"else", TOK_KW_ELSE },
                { u8"enum", TOK_KW_ENUM },
                { u8"extern", TOK_KW_EXTERN },
                { u8"float", TOK_KW_FLOAT },
                { u8"for", TOK_KW_FOR },
                { u8"goto", TOK_KW_GOTO },
                { u8"if", TOK_KW_IF },
                { u8"int", TOK_KW_INT },
                { u8"long", TOK_KW_LONG },
                { u8"register", TOK_KW_REGISTER },
                { u8"return", TOK_KW_RETURN },
                { u8"short", TOK_KW_SHORT },
                { u8"signed", TOK_KW_SIGNED },
                { u8"sizeof", TOK_KW_SIZEOF },
                { u8"static", TOK_KW_STATIC },
                { u8"struct", TOK_KW_STRUCT },
                { u8"switch", TOK_KW_SWITCH },
                { u8"typedef", TOK_KW_TYPEDEF },
                { u8"union", TOK_KW_UNION },
                { u8"unsigned", TOK_KW_UNSIGNED },
                { u8"void", TOK_KW_VOID },
                { u8"volatile", TOK_KW_VOLATILE },
                { u8"while", TOK_KW_WHILE },
        };

        for (const auto &entry: ENTRIES) {
                keywords[entry.first] = entry.second;
        }

        return keywords;
}

//--------------------------------------

WRPARSECXX_API cxx::KeywordTable &
cxx::addC99Keywords(
        KeywordTable &keywords
)
{
        addC89Keywords(keywords);

        static const std::pair<const char *, TokenKind> ENTRIES[] = {
                { u8"_Bool", TOK_KW_BOOL },
                { u8"_Complex", TOK_KW_COMPLEX },
                { u8"_Imaginary", TOK_KW_IMAGINARY },
                { u8"inline", TOK_KW_INLINE },
                { u8"restrict", TOK_KW_RESTRICT },
        };

        for (const auto &entry: ENTRIES) {
                keywords[entry.first] = entry.second;
        }

        return keywords;
}

//--------------------------------------

WRPARSECXX_API cxx::KeywordTable &
cxx::addC11Keywords(
        KeywordTable &keywords
)
{
        addC99Keywords(keywords);

        static const std::pair<const char *, TokenKind> ENTRIES[] = {
                { u8"_Alignas", TOK_KW_ALIGNAS },
                { u8"_Alignof", TOK_KW_ALIGNOF },
                { u8"_Atomic", TOK_KW_ATOMIC },
                { u8"_Generic", TOK_KW_GENERIC },
                { u8"_Noreturn", TOK_KW_NORETURN },
                { u8"_Static_assert", TOK_KW_STATIC_ASSERT },
                { u8"_Thread_local", TOK_KW_THREAD_LOCAL },
        };

        for (const auto &entry: ENTRIES) {
                keywords[entry.first] = entry.second;
        }

        return keywords;
}

//--------------------------------------

WRPARSECXX_API cxx::KeywordTable &
cxx::addCXX98Keywords(
        KeywordTable &keywords
)
{
        addC89Keywords(keywords);

        static const std::pair<const char *, TokenKind> ENTRIES[] = {
                { u8"and", TOK_AMPAMP },
                { u8"and_eq", TOK_AMPEQUAL },
                { u8"asm", TOK_KW_ASM },
                { u8"bitand", TOK_AMP },
                { u8"bitor", TOK_PIPE },
                { u8"bool", TOK_KW_BOOL },
                { u8"catch", TOK_KW_CATCH },
                { u8"class", TOK_KW_CLASS },
                { u8"compl", TOK_TILDE },
                { u8"const_cast", TOK_KW_CONST_CAST },
                { u8"delete", TOK_KW_DELETE },
                { u8"dynamic_cast", TOK_KW_DYNAMIC_CAST },
                { u8"explicit", TOK_KW_EXPLICIT },
                { u8"export", TOK_KW_EXPORT },
                { u8"false", TOK_KW_FALSE },
                { u8"friend", TOK_KW_FRIEND },
                { u8"inline", TOK_KW_INLINE },
                { u8"mutable", TOK_KW_MUTABLE },
                { u8"namespace", TOK_KW_NAMESPACE },
                { u8"new", TOK_KW_NEW },
                { u8"not", TOK_EXCLAIM },
                { u8"not_eq", TOK_EXCLAIMEQUAL },
                { u8"operator", TOK_KW_OPERATOR },
                { u8"or", TOK_PIPEPIPE },
                { u8"or_eq", TOK_PIPEEQUAL },
                { u8"private", TOK_KW_PRIVATE },
                { u8"protected", TOK_KW_PROTECTED },
                { u8"public", TOK_KW_PUBLIC },
                { u8"reinterpret_cast", TOK_KW_REINTERPRET_CAST },
                { u8"static_cast", TOK_KW_STATIC_CAST },
                { u8"template", TOK_KW_TEMPLATE },
                { u8"this", TOK_KW_THIS },
                { u8"throw", TOK_KW_THROW },
                { u8"true", TOK_KW_TRUE },
                { u8"try", TOK_KW_TRY },
                { u8"typeid", TOK_KW_TYPEID },
                { u8"typename", TOK_KW_TYPENAME },
                { u8"using", TOK_KW_USING },
                { u8"virtual", TOK_KW_VIRTUAL },
                { u8"wchar_t", TOK_KW_WCHAR_T },
                { u8"__wchar_t", TOK_KW_WCHAR_T },
                { u8"xor", TOK_CARET },
                { u8"xor_eq", TOK_CARETEQUAL },
        };

        for (const auto &entry: ENTRIES) {
                keywords[entry.first] = entry.second;
        }

        return keywords;
}

//--------------------------------------

WRPARSECXX_API cxx::KeywordTable &
cxx::addCXX11Keywords(
        KeywordTable &keywords
)
{
        addCXX98Keywords(keywords);

        static const std::pair<const char *, TokenKind> ENTRIES[] = {
                { u8"alignas", TOK_KW_ALIGNAS },
                { u8"alignof", TOK_KW_ALIGNOF },
                { u8"char16_t", TOK_KW_CHAR16_T },
                { u8"char32_t", TOK_KW_CHAR32_T },
                { u8"constexpr", TOK_KW_CONSTEXPR },
                { u8"decltype", TOK_KW_DECLTYPE },
                { u8"noexcept", TOK_KW_NOEXCEPT },
                { u8"nullptr", TOK_KW_NULLPTR },
                { u8"static_assert", TOK_KW_STATIC_ASSERT },
                { u8"thread_local", TOK_KW_THREAD_LOCAL },
        };

        for (const auto &entry: ENTRIES) {
                keywords[entry.first] = entry.second;
        }

        return keywords;
}

//--------------------------------------

WRPARSECXX_API cxx::KeywordTable &
cxx::addCKeywords(
        KeywordTable &keywords
)
{
        return addC11Keywords(keywords);
}

//--------------------------------------

WRPARSECXX_API cxx::KeywordTable &
cxx::addCXXKeywords(
        KeywordTable &keywords
)
{
        return addCXX11Keywords(keywords);
}


} // namespace parse
} // namespace wr
