/**
 * \file CXXLexer.h
 *
 * \brief C/C++ language lexer interface
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
#ifndef WRPARSECXX_LEXER_H
#define WRPARSECXX_LEXER_H

#include <iosfwd>
#include <forward_list>
#include <string>
#include <unordered_map>
#include <wrutil/CityHash.h>
#include <wrparse/Lexer.h>
#include <wrparse/Token.h>
#include <wrparse/cxx/Config.h>
#include <wrparse/cxx/CXXOptions.h>


namespace wr {
namespace parse {


class WRPARSECXX_API CXXLexer :
        public Lexer
{
public:
        using this_t = CXXLexer;
        using base_t = Lexer;

        CXXLexer(const CXXOptions &options);
        CXXLexer(const CXXOptions &options, std::istream &input);

        // core Lexer methods
        virtual Token &lex(Token &token) override;
        virtual const char *tokenKindName(TokenKind kind) const override;

        const CXXOptions &options() const { return options_; }

        bool isValidIdentChar(char32_t c) const;
        bool isValidInitialIdentChar(char32_t c) const;
        bool nextClosingTokenIs(TokenKind k) const;

        virtual this_t &clearStorage();

protected:
        char32_t peek();  // interprets trigraphs and escaped newline
        char32_t read();  // ditto

private:
        void updateNextTokenFlags(Token &t);
        TokenKind readToken(Token &t);

        char32_t handleTrigraph();
        bool handleEscapedNewLine();
        char32_t ucn();
        void whitespace(Token &t);
        void numericLiteral(Token &t);
        void binaryLiteral(Token &t);
        void hexadecimalLiteral(Token &t);
        void checkForIntegerSuffix();
        void floatingLiteral(Token &t);
        void stringOrCharLiteral(Token &t);
        char octalEscapeSequence();
        char hexEscapeSequence();
        void rawStringLiteral(Token &t);
        void identifierOrKeyword(Token &t);
        void comment(Token &t);
        void ppDirective(Token &t);
        void pushClosingToken(TokenKind k);
        bool popClosingTokenIf(TokenKind k);


        const CXXOptions             &options_;
        cxx::KeywordTable             kw_id_table_;
        std::string                   tmp_spelling_buf_;
        std::forward_list<TokenKind>  closing_tokens_;
                /**< stack of expected matching closing token kind(s) to match
                     "opening" tokens \c "(", \c "{", \c "[" and \c "<" */
};


} // namespace parse
} // namespace wr


#endif // !WRPARSECXX_LEXER_H
