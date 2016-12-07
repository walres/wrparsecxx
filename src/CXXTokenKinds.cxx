/**
 * \file CXXTokenKinds.cxx
 *
 * \brief Definitions of C/C++ token type names, token spellings, and query
 *      functions
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
#include <map>
#include <wrparse/cxx/CXXTokenKinds.h>


using namespace std;


namespace wr {
namespace parse {
namespace cxx {


struct TokenKindInfo
{
        const char * const name_,
                   * const default_spelling_or_desc_;

        TokenKindInfo(const char *name, const char *default_spelling_or_desc) :
                name_(name),
                default_spelling_or_desc_(default_spelling_or_desc) {}

        TokenKindInfo(const char *name) :
                name_(name), default_spelling_or_desc_(name) {}
};

//--------------------------------------

static const std::map<TokenKind, TokenKindInfo> TOKEN_KINDS = {
        { TOK_NULL, { u8"NULL", u8"" }},
        { TOK_EOF, { u8"EOF", u8"" }},
        { TOK_LPAREN, u8"(" },
        { TOK_RPAREN, u8")" },
        { TOK_LSQUARE, u8"[" },
        { TOK_RSQUARE, u8"]" },
        { TOK_LBRACE, u8"{" },
        { TOK_RBRACE, u8"}" },
        { TOK_DOLLAR, u8"$" },
        { TOK_DOT, u8"." },
        { TOK_ELLIPSIS, u8"..." },
        { TOK_AMP, u8"&" },
        { TOK_AMPAMP, u8"&&" },
        { TOK_AMPEQUAL, u8"&=" },
        { TOK_STAR, u8"*" },
        { TOK_STAREQUAL, u8"*=" },
        { TOK_PLUS, u8"+" },
        { TOK_PLUSPLUS, u8"++" },
        { TOK_PLUSEQUAL, u8"+=" },
        { TOK_MINUS, u8"-" },
        { TOK_ARROW, u8"->" },
        { TOK_MINUSMINUS, u8"--" },
        { TOK_MINUSEQUAL, u8"-=" },
        { TOK_TILDE, u8"~" },
        { TOK_EXCLAIM, u8"!" },
        { TOK_EXCLAIMEQUAL, u8"!=" },
        { TOK_SLASH, u8"/" },
        { TOK_SLASHEQUAL, u8"/=" },
        { TOK_PERCENT, u8"%" },
        { TOK_PERCENTEQUAL, u8"%=" },
        { TOK_LESS, u8"<" },
        { TOK_LESSEQUAL, u8"<=" },
        { TOK_LSHIFT, u8"<<" },
        { TOK_LSHIFTEQUAL, u8"<<=" },
        { TOK_GREATER, u8">" },
        { TOK_GREATEREQUAL, u8">=" },
        { TOK_RSHIFT, u8">>" },
        { TOK_RSHIFTEQUAL, u8">>=" },
        { TOK_CARET, u8"^" },
        { TOK_CARETEQUAL, u8"^=" },
        { TOK_PIPE, u8"|" },
        { TOK_PIPEPIPE, u8"||" },
        { TOK_PIPEEQUAL, u8"|=" },
        { TOK_QUESTION, u8"?" },
        { TOK_COLON, u8":" },
        { TOK_SEMI, u8";" },
        { TOK_EQUAL, u8"=" },
        { TOK_EQUALEQUAL, u8"==" },
        { TOK_COMMA, u8"," },
        { TOK_HASH, u8"#" },
        { TOK_HASHHASH, u8"##" },
        { TOK_DOTSTAR, u8".*" },
        { TOK_ARROWSTAR, u8"->*" },
        { TOK_COLONCOLON, u8"::" },
        { TOK_KW_ALIGNAS, u8"alignas" },
        { TOK_KW_ALIGNOF, u8"alignof" },
        { TOK_KW_ASM, u8"asm" },
        { TOK_KW_ATOMIC, u8"_Atomic" },
        { TOK_KW_AUTO, u8"auto" },
        { TOK_KW_BOOL, u8"bool" },
        { TOK_KW_BREAK, u8"break" },
        { TOK_KW_CASE, u8"case" },
        { TOK_KW_CATCH, u8"catch" },
        { TOK_KW_CHAR, u8"char" },
        { TOK_KW_CHAR16_T, u8"char16_t" },
        { TOK_KW_CHAR32_T, u8"char32_t" },
        { TOK_KW_CLASS, u8"class" },
        { TOK_KW_COMPLEX, u8"_Complex" },
        { TOK_KW_CONST, u8"const" },
        { TOK_KW_CONST_CAST, u8"const_cast" },
        { TOK_KW_CONSTEXPR, u8"constexpr" },
        { TOK_KW_CONTINUE, u8"continue" },
        { TOK_KW_DECLTYPE, u8"decltype" },
        { TOK_KW_DEFAULT, u8"default" },
        { TOK_KW_DELETE, u8"delete" },
        { TOK_KW_DO, u8"do" },
        { TOK_KW_DOUBLE, u8"double" },
        { TOK_KW_DYNAMIC_CAST, u8"dynamic_cast" },
        { TOK_KW_ELSE, u8"else" },
        { TOK_KW_ENUM, u8"enum" },
        { TOK_KW_EXPLICIT, u8"explicit" },
        { TOK_KW_EXPORT, u8"export" },
        { TOK_KW_EXTERN, u8"extern" },
        { TOK_KW_FALSE, u8"false" },
        { TOK_KW_FLOAT, u8"float" },
        { TOK_KW_FOR, u8"for" },
        { TOK_KW_FRIEND, u8"friend" },
        { TOK_KW_FUNC, u8"func" },
        { TOK_KW_GENERIC, u8"_Generic" },
        { TOK_KW_GOTO, u8"goto" },
        { TOK_KW_IF, u8"if" },
        { TOK_KW_IMAGINARY, u8"_Imaginary" },
        { TOK_KW_INLINE, u8"inline" },
        { TOK_KW_INT, u8"int" },
        { TOK_KW_LONG, u8"long" },
        { TOK_KW_MUTABLE, u8"mutable" },
        { TOK_KW_NEW, u8"new" },
        { TOK_KW_NAMESPACE, u8"namespace" },
        { TOK_KW_NOEXCEPT, u8"noexcept" },
        { TOK_KW_NORETURN, u8"_Noreturn" },
        { TOK_KW_NULLPTR, u8"nullptr" },
        { TOK_KW_OPERATOR, u8"operator" },
        { TOK_KW_PRIVATE, u8"private" },
        { TOK_KW_PROTECTED, u8"protected" },
        { TOK_KW_PUBLIC, u8"public" },
        { TOK_KW_REGISTER, u8"register" },
        { TOK_KW_REINTERPRET_CAST, u8"reinterpret_cast" },
        { TOK_KW_RESTRICT, u8"restrict" },
        { TOK_KW_RETURN, u8"return" },
        { TOK_KW_SHORT, u8"short" },
        { TOK_KW_SIGNED, u8"signed" },
        { TOK_KW_SIZEOF, u8"sizeof" },
        { TOK_KW_STATIC, u8"static" },
        { TOK_KW_STATIC_ASSERT, u8"static_assert" },
        { TOK_KW_STATIC_CAST, u8"static_cast" },
        { TOK_KW_STRUCT, u8"struct" },
        { TOK_KW_SWITCH, u8"switch" },
        { TOK_KW_TEMPLATE, u8"template" },
        { TOK_KW_THIS, u8"this" },
        { TOK_KW_THREAD_LOCAL, u8"thread_local" },
        { TOK_KW_THROW, u8"throw" },
        { TOK_KW_TRUE, u8"true" },
        { TOK_KW_TRY, u8"try" },
        { TOK_KW_TYPEDEF, u8"typedef" },
        { TOK_KW_TYPEID, u8"typeid" },
        { TOK_KW_TYPENAME, u8"typename" },
        { TOK_KW_UNION, u8"union" },
        { TOK_KW_UNSIGNED, u8"unsigned" },
        { TOK_KW_USING, u8"using" },
        { TOK_KW_VIRTUAL, u8"virtual" },
        { TOK_KW_VOID, u8"void" },
        { TOK_KW_VOLATILE, u8"volatile" },
        { TOK_KW_WCHAR_T, u8"wchar_t" },
        { TOK_KW_WHILE, u8"while" },
        { TOK_IDENTIFIER, u8"identifier" },
        { TOK_DEC_INT_LITERAL, { u8"dec_int_literal",
                                 u8"decimal integer literal" }},
        { TOK_HEX_INT_LITERAL, { u8"hex_int_literal",
                                 u8"hexadecimal integer literal" }},
        { TOK_OCT_INT_LITERAL, { u8"oct_int_literal",
                                 u8"octal integer literal" }},
        { TOK_BIN_INT_LITERAL, { u8"bin_int_literal",
                                 u8"binary integer literal" }},
        { TOK_FLOAT_LITERAL, { u8"float_literal", u8"floating literal" }},
        { TOK_CHAR_LITERAL, { u8"char_literal", u8"character literal" }},
        { TOK_WCHAR_LITERAL, { u8"wchar_literal", u8"wide-character literal" }},
        { TOK_U8_CHAR_LITERAL, { u8"u8_char_literal",
                                 u8"UTF-8 character literal" }},
        { TOK_U16_CHAR_LITERAL, { u8"u16_char_literal",
                                  u8"UTF-16 character literal" }},
        { TOK_U32_CHAR_LITERAL, { u8"u32_char_literal",
                                  u8"UTF-32 character literal" }},
        { TOK_STR_LITERAL, { u8"str_literal", u8"string literal" }},
        { TOK_WSTR_LITERAL, { u8"wstr_literal", u8"wide string literal" }},
        { TOK_U8_STR_LITERAL, { u8"u8_str_literal", u8"UTF-8 string literal" }},
        { TOK_U16_STR_LITERAL, { u8"u16_str_literal",
                                 u8"UTF-16 string literal" }},
        { TOK_U32_STR_LITERAL, { u8"u32_str_literal",
                                 u8"UTF-32 string literal" }},
        { TOK_WHITESPACE, u8"whitespace" },
        { TOK_COMMENT, u8"comment" },
        { TOK_PP_NUMBER, { u8"pp-number", u8"preprocessing-number" }},
        { TOK_PP_INCLUDE, u8"#include" },
        { TOK_PP_INCLUDE_NEXT, u8"#include_next" },
        { TOK_PP_DEFINE, u8"#define" },
        { TOK_PP_UNDEF, u8"#undef" },
        { TOK_PP_IF, u8"#if" },
        { TOK_PP_IFDEF, u8"#ifdef" },
        { TOK_PP_IFNDEF, u8"#ifndef" },
        { TOK_PP_ELIF, u8"#elif" },
        { TOK_PP_ELSE, u8"#else" },
        { TOK_PP_ENDIF, u8"#endif" },
        { TOK_PP_LINE, u8"#line" },
        { TOK_PP_ERROR, u8"#error" },
        { TOK_PP_WARNING, u8"#warning" },
        { TOK_PP_PRAGMA, u8"#pragma" },
        { TOK_PP_NULL, u8"#" },
};

//--------------------------------------

WRPARSECXX_API const char *
tokenKindName(
        TokenKind kind
)
{
        auto i = TOKEN_KINDS.find(kind);
        if (i != TOKEN_KINDS.end()) {
                return i->second.name_;
        } else {
                return "unknown";
        }
}

//--------------------------------------

WRPARSECXX_API const char *
defaultSpelling(
        TokenKind kind
)
{
        auto i = TOKEN_KINDS.find(kind);
        if (i != TOKEN_KINDS.end()) {
                return i->second.default_spelling_or_desc_;
        } else {
                return "";
        }
}

//--------------------------------------

WRPARSECXX_API Token &
setKindAndSpelling(
        Token     &token,
        TokenKind  kind
)
{
        return token.setKind(kind).setSpelling(defaultSpelling(kind));
}

//--------------------------------------

WRPARSECXX_API bool
isKeyword(
        TokenKind kind
)
{
        return (kind >= TOK_KW_ALIGNAS) && (kind <= TOK_KW_WHILE);
}

//--------------------------------------

WRPARSECXX_API bool
isPunctuation(
        TokenKind kind
)
{
        return (kind >= TOK_LPAREN) && (kind <= TOK_COLONCOLON);
}

//--------------------------------------

WRPARSECXX_API bool
isMultiSpelling(
        TokenKind kind
)
{
        return (kind >= TOK_IDENTIFIER) && (kind <= TOK_PP_NUMBER);
}

//--------------------------------------

WRPARSECXX_API bool
isDeclSpecifier(
        TokenKind kind
)
{
        switch (kind) {
        case TOK_KW_ATOMIC: case TOK_KW_AUTO: case TOK_KW_BOOL:
        case TOK_KW_CHAR: case TOK_KW_CHAR16_T: case TOK_KW_CHAR32_T:
        case TOK_KW_COMPLEX: case TOK_KW_CONST: case TOK_KW_DOUBLE:
        case TOK_KW_FLOAT: case TOK_KW_IMAGINARY: case TOK_KW_INT:
        case TOK_KW_LONG: case TOK_KW_REGISTER: case TOK_KW_RESTRICT:
        case TOK_KW_SHORT: case TOK_KW_SIGNED: case TOK_KW_THREAD_LOCAL:
        case TOK_KW_UNSIGNED: case TOK_KW_VIRTUAL: case TOK_KW_VOID:
        case TOK_KW_VOLATILE: case TOK_KW_WCHAR_T:
                return true;
        default:
                return false;
        }
}

//--------------------------------------

WRPARSECXX_API bool
isPreprocessorToken(
        TokenKind kind
)
{
        return (kind == TOK_HASH) || (kind == TOK_HASHHASH)
                || ((kind >= TOK_PP_NUMBER) && (kind <= TOK_PP_PRAGMA));
}

//--------------------------------------

WRPARSECXX_API bool
isPreprocessorDirective(
        TokenKind kind
)
{
        return (kind >= TOK_PP_INCLUDE) && (kind <= TOK_PP_PRAGMA);
}


} // namespace cxx
} // namespace parse
} // namespace wr
