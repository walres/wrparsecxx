/**
 * \file CXXTokenKinds.h
 *
 * \brief Definitions of C/C++ specific token types, token flags and query
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
#ifndef WRPARSECXX_TOKEN_KINDS_H
#define WRPARSECXX_TOKEN_KINDS_H

#include <wrparse/cxx/Config.h>
#include <wrparse/Grammar.h>
#include <wrparse/Token.h>


namespace wr {
namespace parse {
namespace cxx {


enum : TokenFlags
{
        TF_ALTERNATE  = TF_USER_MIN,       ///< alternate form or digraph
        TF_PREPROCESS = TF_USER_MIN << 1,  ///< part of preprocessor directive
        TF_SPLITABLE  = TF_USER_MIN << 2,  /**< \c ">=", \c ">>" or \c ">>="
                                                token that may form the end of
                                                a template parameter or
                                                argument list */
};

//--------------------------------------
/**
 * \brief C++ token IDs
 */
enum : TokenKind
{
        // revise isPunctuation() if punctuation tokens added before TOK_LPAREN
        TOK_LPAREN = TOK_USER_MIN,
        TOK_RPAREN,
        TOK_LSQUARE,
        TOK_RSQUARE,
        TOK_LBRACE,
        TOK_RBRACE,
        TOK_DOLLAR,
        TOK_DOT,
        TOK_ELLIPSIS,
        TOK_AMP,
        TOK_AMPAMP,
        TOK_AMPEQUAL,
        TOK_STAR,
        TOK_STAREQUAL,
        TOK_PLUS,
        TOK_PLUSPLUS,
        TOK_PLUSEQUAL,
        TOK_MINUS,
        TOK_ARROW,
        TOK_MINUSMINUS,
        TOK_MINUSEQUAL,
        TOK_TILDE,
        TOK_EXCLAIM,
        TOK_EXCLAIMEQUAL,
        TOK_SLASH,
        TOK_SLASHEQUAL,
        TOK_PERCENT,
        TOK_PERCENTEQUAL,
        TOK_LESS,
        TOK_LESSEQUAL,
        TOK_LSHIFT,
        TOK_LSHIFTEQUAL,
        TOK_GREATER,
        TOK_GREATEREQUAL,
        TOK_RSHIFT,
        TOK_RSHIFTEQUAL,
        TOK_CARET,
        TOK_CARETEQUAL,
        TOK_PIPE,
        TOK_PIPEPIPE,
        TOK_PIPEEQUAL,
        TOK_QUESTION,
        TOK_COLON,
        TOK_SEMI,
        TOK_EQUAL,
        TOK_EQUALEQUAL,
        TOK_COMMA,
        TOK_HASH,
        TOK_HASHHASH,
        TOK_DOTSTAR,
        TOK_ARROWSTAR,
        TOK_COLONCOLON,
        /* revise isPunctuation() if punctuation tokens added after
           TOK_COLONCOLON */

        // revise isKeyword() if keywords added before TOK_KW_ALIGNAS
        TOK_KW_ALIGNAS,
        TOK_KW_ALIGNOF,
        TOK_KW_ASM,
        TOK_KW_ATOMIC,  // C11
        TOK_KW_AUTO,
        TOK_KW_BOOL,
        TOK_KW_BREAK,
        TOK_KW_CASE,
        TOK_KW_CATCH,
        TOK_KW_CHAR,
        TOK_KW_CHAR16_T,
        TOK_KW_CHAR32_T,
        TOK_KW_CLASS,
        TOK_KW_COMPLEX,  // C99
        TOK_KW_CONST,
        TOK_KW_CONST_CAST,
        TOK_KW_CONSTEXPR,
        TOK_KW_CONTINUE,
        TOK_KW_DECLTYPE,
        TOK_KW_DEFAULT,
        TOK_KW_DELETE,
        TOK_KW_DO,
        TOK_KW_DOUBLE,
        TOK_KW_DYNAMIC_CAST,
        TOK_KW_ELSE,
        TOK_KW_ENUM,
        TOK_KW_EXPLICIT,
        TOK_KW_EXPORT,
        TOK_KW_EXTERN,
        TOK_KW_FALSE,
        TOK_KW_FLOAT,
        TOK_KW_FOR,
        TOK_KW_FRIEND,
        TOK_KW_FUNC,
        TOK_KW_GENERIC,  // C11
        TOK_KW_GOTO,
        TOK_KW_IF,
        TOK_KW_IMAGINARY,  // C99
        TOK_KW_INLINE,
        TOK_KW_INT,
        TOK_KW_LONG,
        TOK_KW_MUTABLE,
        TOK_KW_NEW,
        TOK_KW_NAMESPACE,
        TOK_KW_NOEXCEPT,
        TOK_KW_NORETURN,
        TOK_KW_NULLPTR,
        TOK_KW_OPERATOR,
        TOK_KW_PRIVATE,
        TOK_KW_PROTECTED,
        TOK_KW_PUBLIC,
        TOK_KW_REGISTER,
        TOK_KW_REINTERPRET_CAST,
        TOK_KW_RESTRICT,
        TOK_KW_RETURN,
        TOK_KW_SHORT,
        TOK_KW_SIGNED,
        TOK_KW_SIZEOF,
        TOK_KW_STATIC,
        TOK_KW_STATIC_ASSERT,
        TOK_KW_STATIC_CAST,
        TOK_KW_STRUCT,
        TOK_KW_SWITCH,
        TOK_KW_TEMPLATE,
        TOK_KW_THIS,
        TOK_KW_THREAD_LOCAL,
        TOK_KW_THROW,
        TOK_KW_TRUE,
        TOK_KW_TRY,
        TOK_KW_TYPEDEF,
        TOK_KW_TYPEID,
        TOK_KW_TYPENAME,
        TOK_KW_UNION,
        TOK_KW_UNSIGNED,
        TOK_KW_USING,
        TOK_KW_VIRTUAL,
        TOK_KW_VOID,
        TOK_KW_VOLATILE,
        TOK_KW_WCHAR_T,
        TOK_KW_WHILE,
        // revise isKeyword() if keywords added after TOK_KW_WHILE

        /* revise isMultiSpelling() if multi-spelling tokens added
           before TOK_IDENTIFIER */
        TOK_IDENTIFIER,
        TOK_DEC_INT_LITERAL,
        TOK_HEX_INT_LITERAL,
        TOK_OCT_INT_LITERAL,
        TOK_BIN_INT_LITERAL,
        TOK_FLOAT_LITERAL,
        TOK_CHAR_LITERAL,
        TOK_WCHAR_LITERAL,
        TOK_U8_CHAR_LITERAL,
        TOK_U16_CHAR_LITERAL,
        TOK_U32_CHAR_LITERAL,
        TOK_STR_LITERAL,
        TOK_WSTR_LITERAL,
        TOK_U8_STR_LITERAL,
        TOK_U16_STR_LITERAL,
        TOK_U32_STR_LITERAL,

        TOK_WHITESPACE,
        TOK_COMMENT,

        /* revise isPreprocessorToken() if preprocessor tokens added
           before TOK_PP_NUMBER */
        TOK_PP_NUMBER,
        /* revise isMultiSpelling() if multi-spelling tokens added
           after TOK_PP_NUMBER */
        /* revise isPreprocessorDirective() if preprocessor tokens
           added before TOK_PP_INCLUDE */
        TOK_PP_INCLUDE,
        TOK_PP_INCLUDE_NEXT,
        TOK_PP_DEFINE,
        TOK_PP_UNDEF,
        TOK_PP_IF,
        TOK_PP_IFDEF,
        TOK_PP_IFNDEF,
        TOK_PP_ELIF,
        TOK_PP_ELSE,
        TOK_PP_ENDIF,
        TOK_PP_LINE,
        TOK_PP_ERROR,
        TOK_PP_WARNING,
        TOK_PP_PRAGMA,
        TOK_PP_NULL,
        /* revise isPreprocessorToken() / isPreprocessorDirective()
           if preprocessor tokens added after TOK_PP_NULL */
};

//--------------------------------------

WRPARSECXX_API const char *tokenKindName(TokenKind kind);
WRPARSECXX_API const char *defaultSpelling(TokenKind kind);
WRPARSECXX_API Token &setKindAndSpelling(Token &token, TokenKind kind);
WRPARSECXX_API bool isKeyword(TokenKind kind);
WRPARSECXX_API bool isPunctuation(TokenKind kind);
WRPARSECXX_API bool isMultiSpelling(TokenKind kind);
WRPARSECXX_API bool isDeclSpecifier(TokenKind kind);
WRPARSECXX_API bool isPreprocessorToken(TokenKind kind);
WRPARSECXX_API bool isPreprocessorDirective(TokenKind kind);


} // namespace cxx
} // namespace parse
} // namespace wr


#endif // !WRPARSECXX_TOKEN_KINDS_H
