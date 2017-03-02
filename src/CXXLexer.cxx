/**
 * \file CXXLexer.cxx
 *
 * \brief C/C++ language lexer implementation
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
#include <algorithm>
#include <bitset>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <assert.h>

#include <wrutil/ctype.h>
#include <wrutil/Format.h>
#include <wrutil/numeric_cast.h>
#include <wrutil/utf8.h>
#include <wrparse/cxx/CXXLexer.h>
#include <wrparse/cxx/CXXTokenKinds.h>


namespace wr {
namespace parse {


using namespace cxx;


WRPARSECXX_API
CXXLexer::CXXLexer(
        const CXXOptions &options
) :
        options_    (options),
        kw_id_table_(options.keywords())
{
}

//--------------------------------------

WRPARSECXX_API
CXXLexer::CXXLexer(
        const CXXOptions &options,
        std::istream     &input
) :
        Lexer       (input),
        options_    (options),
        kw_id_table_(options.keywords())
{
}

//--------------------------------------

WRPARSECXX_API Token &
CXXLexer::lex(
        Token &t
)
{
        bool again;

        do {
                again = false;
                base_t::lex(t);  // initialise token

                switch (readToken(t)) {
                case TOK_WHITESPACE:
                        again = !options_.have(cxx::KEEP_SPACE);
                        break;
                case TOK_COMMENT:
                        again = !options_.have(cxx::KEEP_COMMENTS);
                        break;
                default:
                        break;
                }
        } while (again);

        return t;
}

//--------------------------------------

void
CXXLexer::updateNextTokenFlags(
        Token &t
)
{
        switch (t.kind()) {
        case cxx::TOK_WHITESPACE:
                if (lastRead() == U'\n') {
                        // newline is always a separate token
                        setNextTokenFlags(nextTokenFlags() & ~TF_PREPROCESS);
                }
                break;
        case TOK_EOF:
                setNextTokenFlags((nextTokenFlags() & ~TF_PREPROCESS)
                                        | TF_STARTS_LINE);
                break;
        }
}

//--------------------------------------

char32_t
CXXLexer::handleTrigraph()
{
        char32_t c = lastRead();

        if (c == U'?') {
                if (base_t::read() == U'?') {
                        switch (base_t::read()) {
                        case U'<':
                                c = U'{';
                                break;
                        case U'>':
                                c = U'}';
                                break;
                        case U'(':
                                c = U'[';
                                break;
                        case U')':
                                c = U']';
                                break;
                        case U'=':
                                c = U'#';
                                break;
                        case U'/':
                                c = U'\\';
                                break;
                        case U'\'':
                                c = U'^';
                                break;
                        case U'!':
                                c = U'|';
                                break;
                        case U'-':
                                c = U'~';
                                break;
                        default:
                                backtrack(2);
                                return c;
                        }

                        replace(3, c);
                } else {
                        backtrack();
                }
        }

        return c;
}

//--------------------------------------

bool
CXXLexer::handleEscapedNewLine()
{
        if (lastRead() == U'\\') {
                if (base_t::peek() == U'\n') {
                        base_t::read();
                        erase(2);  // delete
                        return true;
                }
        }
        return false;
}

//--------------------------------------

char32_t
CXXLexer::peek()
{
        char32_t c;
        bool     repeat;

        do {
                repeat = false;
                c = base_t::peek();
                if (options_.have(cxx::TRIGRAPHS) && (c == U'?')) {
                        base_t::read();
                        c = handleTrigraph();
                        if (handleEscapedNewLine()) {
                                repeat = true;
                        } else {
                                backtrack();
                        }
                } else if (c == U'\\') {
                        base_t::read();
                        if (handleEscapedNewLine()) {
                                repeat = true;
                        } else {
                                backtrack();
                        }
                }
        } while (repeat);

        return c;
}

//--------------------------------------

char32_t
CXXLexer::read()
{
        char32_t c;

        do {
                c = base_t::read();
                if (options_.have(cxx::TRIGRAPHS) && (c == U'?')) {
                        c = handleTrigraph();
                }
        } while (handleEscapedNewLine());

        return c;
}

//--------------------------------------

TokenKind
CXXLexer::readToken(
        Token &t
)
{
        bool     eat_next = false;
        char32_t ch       = read();

        struct OnExit
        {
                CXXLexer &lex_;
                Token &t_;
                OnExit(CXXLexer &lex, Token &t) : lex_(lex), t_(t) {}
                ~OnExit() { lex_.updateNextTokenFlags(t_); }
        } on_exit(*this, t);

        if (ch == eof) {
                return setKindAndSpelling(t, TOK_EOF).kind();
        }

        switch (ch) {
        case U'#':
                if (peek() == U'#') {
                        setKindAndSpelling(t, TOK_HASHHASH);
                        eat_next = true;
                } else {
                        setKindAndSpelling(t, TOK_HASH);
                        if (!options_.have(cxx::NO_PP_DIRECTIVES)) {
                                if (t.flags() & TF_STARTS_LINE) {
                                        ppDirective(t);
                                }
                        }
                }
                break;
        case U'/':
                switch (peek()) {
                case U'=':
                        setKindAndSpelling(t, TOK_SLASHEQUAL);
                        eat_next = true;
                        break;
                case U'*':
                        comment(t);
                        break;
                case U'/':
                        if (options_.have(cxx::LINE_COMMENTS)) {
                                comment(t);
                        } else {
                                setKindAndSpelling(t, TOK_SLASH);
                        }
                        break;
                default:
                        setKindAndSpelling(t, TOK_SLASH);
                        break;
                }
                break;
        case U'.':
                if ((options_.cxx()) && (peek() == U'*')) {
                        setKindAndSpelling(t, TOK_DOTSTAR);
                        eat_next = true;
                } else if (isudigit(peek())) {
                        numericLiteral(t);
                } else if (peek() == U'.') {
                        read();     // eat 2nd '.'
                        if (peek() == U'.') {
                                setKindAndSpelling(t, TOK_ELLIPSIS);
                                eat_next = true;
                        } else {
                                backtrack();  // spit 2nd '.' back out
                                setKindAndSpelling(t, TOK_DOT);
                        }
                } else {
                        setKindAndSpelling(t, TOK_DOT);
                }
                break;
        case U'<':
                switch (peek()) {
                case U'<':
                        read();  // consume 2nd '<'
                        if (peek() == U'=') {
                                setKindAndSpelling(t, TOK_LSHIFTEQUAL);
                                eat_next = true;
                        } else {
                                setKindAndSpelling(t, TOK_LSHIFT);
                        }
                        break;
                case U'=':
                        setKindAndSpelling(t, TOK_LESSEQUAL);
                        eat_next = true;
                        break;
                case U'%':                       // "<%" digraph => '{'
                        if (options_.have(cxx::DIGRAPHS)) {
                                t.setFlags(t.flags() | cxx::TF_ALTERNATE);
                                t.setKind(TOK_LBRACE).setSpelling(u8"<%");
                                pushClosingToken(TOK_RBRACE);
                                eat_next = true;
                        } else {
                                setKindAndSpelling(t, TOK_LESS);
                                pushClosingToken(TOK_GREATER);
                        }
                        break;
                case U':':                       // "<:" digraph => '['
                        if (!options_.have(cxx::DIGRAPHS)) {
                                setKindAndSpelling(t, TOK_LESS);
                                pushClosingToken(TOK_GREATER);
                                break;
                        }
                        read();

                        /* C++11: don't misinterpret a sequence like
                           std::set<::std::string> as std::set[:std::string> */
                        if ((options_.cxx() >= cxx::CXX11) && peek() == U':') {
                                read();
                                switch (peek()) {
                                case U':': case U'>':  // treat as '['
                                        backtrack();
                                        break;
                                default:
                                        backtrack(2);
                                        setKindAndSpelling(t, TOK_LESS);
                                        pushClosingToken(TOK_GREATER);
                                        break;
                                }
                        }

                        if (t.kind() == TOK_NULL) {
                                t.setFlags(t.flags() | cxx::TF_ALTERNATE);
                                t.setKind(TOK_LSQUARE).setSpelling(u8"<:");
                                pushClosingToken(TOK_RSQUARE);
                        }
                        break;
                default:
                        setKindAndSpelling(t, TOK_LESS);
                        pushClosingToken(TOK_GREATER);
                        break;
                }
                break;
        case U'>':
                switch (peek()) {
                case U'>':
                        read();  // consume 2nd '>'
                        if (peek() == U'=') {
                                setKindAndSpelling(t, TOK_RSHIFTEQUAL);
                                eat_next = true;
                        } else {
                                setKindAndSpelling(t, TOK_RSHIFT);
                        }
                        if (nextClosingTokenIs(TOK_GREATER)) {
                                if (options_.cxx() >= cxx::CXX11) {
                                        t.addFlags(cxx::TF_SPLITABLE);
                                }
                        }
                        break;
                case U'=':
                        setKindAndSpelling(t, TOK_GREATEREQUAL);
                        if (nextClosingTokenIs(TOK_GREATER)) {
                                if (options_.cxx() >= cxx::CXX11) {
                                        t.addFlags(cxx::TF_SPLITABLE);
                                }
                        }
                        eat_next = true;
                        break;
                default:
                        setKindAndSpelling(t, TOK_GREATER);
                        popClosingTokenIf(t.kind());
                        break;
                }
                break;
        case U'+':
                switch (peek()) {
                case U'=':
                        setKindAndSpelling(t, TOK_PLUSEQUAL);
                        eat_next = true;
                        break;
                case U'+':
                        setKindAndSpelling(t, TOK_PLUSPLUS);
                        eat_next = true;
                        break;
                default:
                        setKindAndSpelling(t, TOK_PLUS);
                        break;
                }
                break;
        case U'-':
                switch (peek()) {
                case U'=':
                        setKindAndSpelling(t, TOK_MINUSEQUAL);
                        eat_next = true;
                        break;
                case U'-':
                        setKindAndSpelling(t, TOK_MINUSMINUS);
                        eat_next = true;
                        break;
                case U'>':
                        read();
                        if ((options_.cxx()) && (peek() == U'*')) {
                                setKindAndSpelling(t, TOK_ARROWSTAR);
                                eat_next = true;
                        } else {
                                setKindAndSpelling(t, TOK_ARROW);
                        }
                        break;
                default:
                        setKindAndSpelling(t, TOK_MINUS);
                        break;
                }
                break;
        case U'*':
                if (peek() == U'=') {
                        setKindAndSpelling(t, TOK_STAREQUAL);
                        eat_next = true;
                } else {
                        setKindAndSpelling(t, TOK_STAR);
                }
                break;
        case U'%':
                switch (peek()) {
                case U'=':
                        setKindAndSpelling(t, TOK_PERCENTEQUAL);
                        eat_next = true;
                        break;
                case U'>':                      // "%>" digraph => '}'
                        if (options_.have(cxx::DIGRAPHS)) {
                                t.setFlags(t.flags() | cxx::TF_ALTERNATE);
                                t.setKind(TOK_RBRACE).setSpelling(u8"%>");
                                popClosingTokenIf(t.kind());
                                eat_next = true;
                        } else {
                                setKindAndSpelling(t, TOK_PERCENT);
                        }
                        break;
                case U':':                      // "%:" digraph => '#'
                        if (!options_.have(cxx::DIGRAPHS)) {
                                setKindAndSpelling(t, TOK_PERCENT);
                                break;
                        }
                        t.setFlags(t.flags() | cxx::TF_ALTERNATE);
                        read();

                        if (peek() == U'%') {
                                read();
                                if (peek() == U':') {  // "%:%:" => "##"
                                        t.setKind(TOK_HASHHASH);
                                        t.setSpelling(u8"%:%:");
                                        eat_next = true;
                                        break;
                                } else {
                                        backtrack();
                                }
                        }

                        t.setKind(TOK_HASH).setSpelling(u8"%:");

                        if (!options_.have(cxx::NO_PP_DIRECTIVES)) {
                                if (t.flags() & TF_STARTS_LINE) {
                                        ppDirective(t);
                                }
                        }
                        break;
                default:
                        setKindAndSpelling(t, TOK_PERCENT);
                        break;
                }
                break;
        case U'&':
                switch (peek()) {
                case U'=':
                        setKindAndSpelling(t, TOK_AMPEQUAL);
                        eat_next = true;
                        break;
                case U'&':
                        if (options_.cxx() >= cxx::CXX11) {
                                setKindAndSpelling(t, TOK_AMPAMP);
                                eat_next = true;
                                break;
                        } // else fall through
                default:
                        setKindAndSpelling(t, TOK_AMP);
                        break;
                }
                break;
        case U'|':
                switch (peek()) {
                case U'=':
                        setKindAndSpelling(t, TOK_PIPEEQUAL);
                        eat_next = true;
                        break;
                case U'|':
                        setKindAndSpelling(t, TOK_PIPEPIPE);
                        eat_next = true;
                        break;
                default:
                        setKindAndSpelling(t, TOK_PIPE);
                        break;
                }
                break;
        case U'^':
                if (peek() == U'=') {
                        setKindAndSpelling(t, TOK_CARETEQUAL);
                        eat_next = true;
                } else {
                        setKindAndSpelling(t, TOK_CARET);
                }
                break;
        case U'=':
                if (peek() == U'=') {
                        setKindAndSpelling(t, TOK_EQUALEQUAL);
                        eat_next = true;
                } else {
                        setKindAndSpelling(t, TOK_EQUAL);
                }
                break;
        case U'!':
                if (peek() == U'=') {
                        setKindAndSpelling(t, TOK_EXCLAIMEQUAL);
                        eat_next = true;
                } else {
                        setKindAndSpelling(t, TOK_EXCLAIM);
                }
                break;
        case U':':
                switch (peek()) {
                case U'>':                      // ":>" digraph => ']'
                        if (options_.have(cxx::DIGRAPHS)) {
                                t.setFlags(t.flags() | cxx::TF_ALTERNATE);
                                t.setKind(TOK_RSQUARE).setSpelling(u8":>");
                                popClosingTokenIf(t.kind());
                                eat_next = true;
                        } else {
                                setKindAndSpelling(t, TOK_COLON);
                        }
                        break;
                case U':':
                        if (options_.cxx()) {
                                setKindAndSpelling(t, TOK_COLONCOLON);
                                eat_next = true;
                                break;
                        } // else fall through
                default:
                        setKindAndSpelling(t, TOK_COLON);
                        break;
                }
                break;
        case U'u':
                switch (peek()) {
                case U'8':
                        read();
                        switch (peek()) {
                        default:
                                backtrack();
                                identifierOrKeyword(t);
                                break;
                        case U'\'':
                                if (options_.have(cxx::UTF8_CHAR_LITERALS)) {
                                        read();
                                        t.setKind(TOK_U8_CHAR_LITERAL);
                                        stringOrCharLiteral(t);
                                } else {
                                        backtrack();
                                        identifierOrKeyword(t);
                                }
                                break;
                        case U'"':
                                if ((options_.c() >= cxx::C11)
                                            || (options_.cxx() >= cxx::CXX11)) {
                                        read();
                                        t.setKind(TOK_U8_STR_LITERAL);
                                        stringOrCharLiteral(t);
                                } else {
                                        identifierOrKeyword(t);
                                }
                                break;
                        case U'R':
                                read();
                                if ((peek() == U'"')
                                            && (options_.cxx() >= cxx::CXX11)) {
                                        read();
                                        t.setKind(TOK_U8_STR_LITERAL);
                                        rawStringLiteral(t);
                                } else {
                                        backtrack(2);
                                        identifierOrKeyword(t);
                                }
                                break;
                        }
                        break;
                case U'R':
                        read();
                        if ((peek() == U'"')
                                        && (options_.cxx() >= cxx::CXX11)) {
                                read();
                                t.setKind(TOK_U16_STR_LITERAL);
                                rawStringLiteral(t);
                        } else {
                                backtrack();
                                identifierOrKeyword(t);
                        }
                        break;
                case U'"':
                        if ((options_.c() >= cxx::C11)
                                        || (options_.cxx() >= cxx::CXX11)) {
                                read();
                                t.setKind(TOK_U16_STR_LITERAL);
                                stringOrCharLiteral(t);
                        } else {
                                identifierOrKeyword(t);
                        }
                        break;
                case U'\'':
                        if ((options_.c() >= cxx::C11)
                                        || (options_.cxx() >= cxx::CXX11)) {
                                read();
                                t.setKind(TOK_U16_CHAR_LITERAL);
                                stringOrCharLiteral(t);
                        }
                        break;
                default:
                        identifierOrKeyword(t);
                        break;
                }
                break;
        case U'U':
                switch (peek()) {
                case U'"':
                        if ((options_.c() >= cxx::C11)
                                        || (options_.cxx() >= cxx::CXX11)) {
                                read();
                                t.setKind(TOK_U32_STR_LITERAL);
                                stringOrCharLiteral(t);
                        }
                        break;
                case U'\'':
                        if ((options_.c() >= cxx::C11)
                                        || (options_.cxx() >= cxx::CXX11)) {
                                read();
                                t.setKind(TOK_U32_CHAR_LITERAL);
                                stringOrCharLiteral(t);
                        }
                        break;
                case U'R':
                        read();
                        if ((peek() == U'"')
                                        && (options_.cxx() >= cxx::CXX11)) {
                                read();
                                t.setKind(TOK_U32_STR_LITERAL);
                                rawStringLiteral(t);
                        } else {
                                backtrack();
                                identifierOrKeyword(t);
                        }
                        break;
                default:
                        identifierOrKeyword(t);
                        break;
                }
                break;
        case U'L':
                switch (peek()) {
                case U'"':
                        read();
                        stringOrCharLiteral(t.setKind(TOK_WSTR_LITERAL));
                        break;
                case U'\'':
                        read();
                        stringOrCharLiteral(t.setKind(TOK_WCHAR_LITERAL));
                        break;
                case U'R':
                        read();
                        if (peek() == U'"') {
                                read();
                                rawStringLiteral(t.setKind(TOK_WSTR_LITERAL));
                        } else {
                                backtrack();
                                identifierOrKeyword(t);
                        }
                        break;
                default:
                        identifierOrKeyword(t);
                        break;
                }
                break;
        case U'R':
                if (peek() == U'"') {
                        read();
                        rawStringLiteral(t.setKind(TOK_STR_LITERAL));
                } else {
                        identifierOrKeyword(t);
                }
                break;
        case U'"':  stringOrCharLiteral(t.setKind(TOK_STR_LITERAL)); break;
        case U'\'': stringOrCharLiteral(t.setKind(TOK_CHAR_LITERAL)); break;
        case U';':  setKindAndSpelling(t, TOK_SEMI); break;
        case U',':  setKindAndSpelling(t, TOK_COMMA); break;
        case U'~':  setKindAndSpelling(t, TOK_TILDE); break;
        case U'?':  setKindAndSpelling(t, TOK_QUESTION); break;
        case U'_':  identifierOrKeyword(t); break;
        case U'{':
                setKindAndSpelling(t, TOK_LBRACE);
                pushClosingToken(TOK_RBRACE);
                break;
        case U'}':
                setKindAndSpelling(t, TOK_RBRACE);
                popClosingTokenIf(t.kind());
                break;
        case U'(':
                setKindAndSpelling(t, TOK_LPAREN);
                pushClosingToken(TOK_RPAREN);
                break;
        case U')':
                setKindAndSpelling(t, TOK_RPAREN);
                popClosingTokenIf(t.kind());
                break;
        case U'[':
                setKindAndSpelling(t, TOK_LSQUARE);
                pushClosingToken(TOK_RSQUARE);
                break;
        case U']':
                setKindAndSpelling(t, TOK_RSQUARE);
                popClosingTokenIf(t.kind());
                break;
        case U'$':
                if (options_.have(cxx::IDENTIFIER_DOLLARS)) {
                        identifierOrKeyword(t);
                } else {
                        setKindAndSpelling(t, TOK_DOLLAR);
                }
                break;
        case U'\\':  // possible UCN as the start of an identifier
                switch (peek()) {
                case U'u': case U'U':
                        if (options_.have(UCNS)) {
                                ch = ucn();
                                if (isValidInitialIdentChar(ch)) {
                                        identifierOrKeyword(t);
                                }
                        }
                        break;
                default:
                        break;
                }
                break;
        default:
                if (isuspace(ch)) {
                        whitespace(t);
                } else if (isudigit(static_cast<char>(ch))) {
                        numericLiteral(t);
                } else if (isValidInitialIdentChar(ch)) {
                        identifierOrKeyword(t);
                } // otherwise leave as TOK_NULL
                break;
        }

        if (input().bad()) {
                emit(Diagnostic::FATAL_ERROR, 1, "input error");
                t.reset();
        } else if (eat_next) {
                read();
        }

        return t.kind();
}

//--------------------------------------

WRPARSECXX_API const char *
CXXLexer::tokenKindName(
        TokenKind kind
) const
{
        return cxx::tokenKindName(kind);
}

//--------------------------------------

static std::bitset<65536>
initBMPValidIdentChars()
{
        static const std::pair<char32_t, char32_t> CHAR_RANGES[] = {
                { 0x24, 0x24 }, { 0x30, 0x39 }, { 0x41, 0x5a }, { 0x5f, 0x5f },
                { 0x61, 0x7a }, { 0xa8, 0xa8 }, { 0xaa, 0xaa }, { 0xad, 0xad },
                { 0xaf, 0xaf }, { 0xb2, 0xb5 }, { 0xb7, 0xba }, { 0xbc, 0xbe },
                { 0xc0, 0xd6 }, { 0xd8, 0xf6 }, { 0xf8, 0xff },

                { 0x0100, 0x167f }, { 0x1681, 0x180d }, { 0x180f, 0x1fff },
                { 0x200b, 0x200d }, { 0x202a, 0x202e }, { 0x203f, 0x2040 },
                { 0x2054, 0x2054 }, { 0x2060, 0x206f }, { 0x2070, 0x218f },
                { 0x2460, 0x24ff }, { 0x2776, 0x2793 }, { 0x2c00, 0x2dff },
                { 0x2e80, 0x2fff }, { 0x3004, 0x3007 }, { 0x3021, 0x302f },
                { 0x3031, 0x303f }, { 0x3040, 0xd7ff }, { 0xf900, 0xfd3d },
                { 0xfd40, 0xfdcf }, { 0xfdf0, 0xfe44 }, { 0xfe47, 0xfffd }
        };

        std::bitset<65536> bits;

        for (const auto &range: CHAR_RANGES) {
                for (char32_t c = range.first; c <= range.second; ++c) {
                        bits[c].flip();
                }
        }

        return bits;
}

//--------------------------------------

WRPARSECXX_API bool
CXXLexer::isValidIdentChar(
        char32_t c
) const
{
        if ((c == U'$') && !options_.have(cxx::IDENTIFIER_DOLLARS)) {
                return false;
        }

        static const std::bitset<65536> BMP_VALID = initBMPValidIdentChars();

        return ((c <= 0xffff) && BMP_VALID.test(c)) ||
                ((c >= 0x10000) && (c <= 0xefffd) && ((c & 0xffff) <= 0xfffd));
}

//--------------------------------------

WRPARSECXX_API bool
CXXLexer::isValidInitialIdentChar(
        char32_t c
) const
{
        return isValidIdentChar(c) && ((c < 0x30) || (c > 0x39))
                                   && ((c < 0x300) || (c > 0x36f))
                                   && ((c < 0x1dc0) || (c > 0x1dff))
                                   && ((c < 0x20d0) || (c > 0x20ff))
                                   && ((c < 0xfe20) || (c > 0xfe2f));
}

//--------------------------------------

WRPARSECXX_API CXXLexer &
CXXLexer::clearStorage()
{
        kw_id_table_ = options_.keywords();
        base_t::clearStorage();
        return *this;
}

//--------------------------------------

char32_t
CXXLexer::ucn()
{
        size_t n;
        auto   start_line = line();
        auto   start_column = column();
        auto   start_offset = offset();

        switch (read()) {
        case U'u':
                n = 4;
                break;
        case U'U':
                n = 8;
                break;
        default:
                backtrack();
                return eof;
        }

        char32_t c = 0;
        size_t   i = 0;

        for (; i < n; ++i) {
                char32_t digit = peek();
                if (!isuxdigit(digit)) {
                        break;
                }
                read();
                c <<= 4;
                c |= uxdigitval(digit);
        }

        if (i < n) {
                emit(Diagnostic::ERROR, start_offset, offset() - start_offset,
                     start_line, start_column,
                     "Not a UCN: insufficient digits given");
                backtrack(i + 1);
                c = eof;
        } else if ((c >= 0xd800) && (c <= 0xdfff)) {
                emit(Diagnostic::ERROR, start_offset, offset() - start_offset,
                     start_line, start_column,
                     "Illegal UCN: surrogate code point");
                c = eof;
        } else if (static_cast<uint32_t>(c) > 0x1fffff) {  // signed char32_t?
                emit(Diagnostic::ERROR, start_offset, offset() - start_offset,
                     start_line, start_column,
                     "Not a UCN: code point out of range 0 - 0x1fffff");
                c = eof;
        } else {
                replace(n + 2, c);
        }

        return c;
}

//--------------------------------------

void
CXXLexer::whitespace(
        Token &t
)
{
        t.setKind(TOK_WHITESPACE);

        if (lastRead() == U'\n') {
                t.setSpelling(u8"\n");
        } else if (options_.have(cxx::KEEP_SPACE)) {
                tmp_spelling_buf_.clear();
                utf8_append(tmp_spelling_buf_, lastRead());
        } else {
                t.setSpelling(u8" ");
        }

        // return newline as individual token to aid preprocessing
        if (lastRead() != U'\n') {
                if (options_.have(cxx::KEEP_SPACE)) {
                        while (isuspace(peek()) && (peek() != U'\n')) {
                                utf8_append(tmp_spelling_buf_, read());
                        }
                        t.setSpelling(store(tmp_spelling_buf_));
                } else {
                        while (isuspace(peek()) && (peek() != U'\n')) {
                                read();
                        }
                }
        }
}

//--------------------------------------

void
CXXLexer::numericLiteral(
        Token &t
)
{
        tmp_spelling_buf_.clear();
        utf8_append(tmp_spelling_buf_, lastRead());

        bool octal = false;

        switch (lastRead()) {
        case U'0':
                switch (peek()) {
                case U'b': case U'B':
                        if (options_.have(cxx::BINARY_LITERALS)) {
                                read();
                                if (udigitval(peek()) <= 1) {
                                        binaryLiteral(t);
                                        return;
                                }
                                backtrack();
                                return;
                        } else {
                                octal = true;
                        }
                        break;
                case U'x': case U'X':
                        read();
                        if (isuxdigit(peek())) {
                                hexadecimalLiteral(t);
                                return;
                        }
                        backtrack();
                        return;
                case U'.':
                        floatingLiteral(t);
                        return;
                default:
                        octal = true;
                        break;
                }

                break;
        case U'.':
                floatingLiteral(t);
                return;
        default:
                break;  // digit (already checked by readToken())
        }

        while (true) {
                switch (peek()) {
                case U'.': case U'E': case U'e':
                        utf8_append(tmp_spelling_buf_, read());
                        floatingLiteral(t);
                        return;
                case U'\'':             // grouping separator
                        read();
                        if (isudigit(peek())) {
                                utf8_append(tmp_spelling_buf_, lastRead());
                        } else {
                                backtrack();
                        }
                        break;
                default:
                        break;
                }

                if (isudigit(peek())) {
                        octal = octal && (udigitval(peek()) < 8);
                        utf8_append(tmp_spelling_buf_, read());
                } else {
                        break;
                }
        }

        checkForIntegerSuffix();

        if (octal) {
                t.setKind(TOK_OCT_INT_LITERAL);
        } else {
                t.setKind(TOK_DEC_INT_LITERAL);
        }

        t.setSpelling(store(tmp_spelling_buf_));
}

//--------------------------------------

void
CXXLexer::binaryLiteral(
        Token &t
)
{
        utf8_append(tmp_spelling_buf_, lastRead());  // consume 'b' or 'B'

        while (true) {
                char32_t ch = peek();

                if (udigitval(ch) <= 1) {
                        utf8_append(tmp_spelling_buf_, read());
                } else if (ch == U'\'') {
                        read();
                        ch = peek();
                        if ((ch == U'0') || (ch == U'1')) {
                                utf8_append(tmp_spelling_buf_, lastRead());
                        } else {
                                backtrack();
                                break;
                        }
                } else {
                        break;
                }
        }

        checkForIntegerSuffix();
        t.setKind(TOK_BIN_INT_LITERAL);
        t.setSpelling(store(tmp_spelling_buf_));
}

//--------------------------------------

void
CXXLexer::hexadecimalLiteral(
        Token &t
)
{
        utf8_append(tmp_spelling_buf_, lastRead());  // consume 'x' or 'X'

        while (isuxdigit(peek())) {
                utf8_append(tmp_spelling_buf_, read());
                if (peek() == U'\'') {  // grouping separator
                        read();
                        if (isuxdigit(peek())) {
                                utf8_append(tmp_spelling_buf_, lastRead());
                        } else {
                                backtrack();
                        }
                }
        }

        checkForIntegerSuffix();
        t.setKind(TOK_HEX_INT_LITERAL);
        t.setSpelling(store(tmp_spelling_buf_));
}

//--------------------------------------

void
CXXLexer::checkForIntegerSuffix()
{
        switch (peek()) {
        case U'u': case U'U':
                utf8_append(tmp_spelling_buf_, read());
                if (toulower(peek()) == U'l') {
                        utf8_append(tmp_spelling_buf_, read());
                        if (options_.have(cxx::LONG_LONG)
                                       && (peek() == lastRead())) {  // LL or ll
                                utf8_append(tmp_spelling_buf_, read());
                        }
                }
                break;
        case U'l': case U'L':
                utf8_append(tmp_spelling_buf_, read());
                if (options_.have(cxx::LONG_LONG)
                                       && (peek() == lastRead())) {  // LL or ll
                        utf8_append(tmp_spelling_buf_, read());
                }
                if (toulower(peek()) == U'u') {
                        utf8_append(tmp_spelling_buf_, read());
                }
                break;
        default:
                break;
        }
}

//--------------------------------------

void
CXXLexer::floatingLiteral(
        Token &t
)
{
        bool int_part = (lastRead() != U'.'), exp_part = false, again = true;

        while (again) {
                char32_t ch = peek();

                switch (ch) {
                case U'.':
                        if (int_part) {
                                utf8_append(tmp_spelling_buf_, read());
                                int_part = false;
                        } else {
                                again = false;
                        }
                        break;
                case U'E': case U'e':
                        if (exp_part) {
                                again = false;
                                break;
                        }
                        utf8_append(tmp_spelling_buf_, read());
                        exp_part = true;
                        ch = peek();
                        if ((ch == U'+') || (ch == U'-')) {
                                utf8_append(tmp_spelling_buf_, read());
                        }
                        break;
                default:
                        if (isudigit(ch)) {
                                utf8_append(tmp_spelling_buf_, read());
                        } else {
                                again = false;
                        }
                        break;
                }
        }

        switch (peek()) {
        case U'F': case U'f': case U'L': case U'l':
                utf8_append(tmp_spelling_buf_, read());  // consume suffix
                break;
        default:
                break;
        }

        t.setKind(TOK_FLOAT_LITERAL);
        t.setSpelling(store(tmp_spelling_buf_));
}

//--------------------------------------

void
CXXLexer::stringOrCharLiteral(
        Token &t
)
{
        tmp_spelling_buf_.clear();

        char32_t delimiter = lastRead();

        for (char32_t c = read(); c != delimiter; c = read()) {
                if (c == eof || c == '\n') {
                        const char *kind;
                        if (delimiter == '"') {
                                kind = "string";
                        } else {
                                kind = "character";
                        }
                        emit(Diagnostic::ERROR, t,
                                "unterminated %s literal", kind);
                        break;
                } else if (c != U'\\') {
                        utf8_append(tmp_spelling_buf_, lastRead());
                } else {
                        switch (c = read()) {
                        default:
                                if (udigitval(c) < 8) {
                                        // up to 3-digit octal character value
                                        backtrack();
                                        c = octalEscapeSequence();
                                        break;
                                } // else unrecognised escape sequence...
                                // for now just take next character
                        case U'\'': case U'"': case U'?': case U'\\':
                                break;
                        case U'a':
                                c = U'\a';
                                break;
                        case U'b':
                                c = U'\b';
                                break;
                        case U'f':
                                c = U'\f';
                                break;
                        case U'n':
                                c = U'\n';
                                break;
                        case U'r':
                                c = U'\r';
                                break;
                        case U't':
                                c = U'\t';
                                break;
                        case U'v':
                                c = U'\v';
                                break;
                        case U'x':
                                if (isuxdigit(peek())) {
                                        // up to 2-digit hex character value
                                        c = hexEscapeSequence();
                                }
                                break;
                        case U'u': case U'U':
                                if (!options_.have(cxx::UCNS)) {
                                        break;
                                }
                                backtrack();
                                c = ucn();
                                /* if (c == eof), ucn() will have backtracked 
                                   so just append 'U'/'u' plus whatever
                                   follows */
                                break;
                        }

                        utf8_append(tmp_spelling_buf_, c);
                }
        }

        t.setSpelling(store(tmp_spelling_buf_));
}

//--------------------------------------

char
CXXLexer::octalEscapeSequence()
{
        char value = 0;
        auto digit = udigitval(read());

        if (digit < 8) {
                value += static_cast<char>(digit);
                digit = udigitval(read());
                if (digit < 8) {
                        value = (value << 3) + static_cast<char>(digit);
                        digit = udigitval(read());
                        if (digit < 8) {
                                value = (value << 3) + static_cast<char>(digit);
                        } else {
                                backtrack();
                        }
                } else {
                        backtrack();
                }
        } else {
                value = std::char_traits<char>::eof();
                backtrack();
        }

        return value;
}

//--------------------------------------

char
CXXLexer::hexEscapeSequence()
{
        char value = 0;
        auto digit = xdigitval(read());

        if (digit >= 0) {
                value += static_cast<char>(digit);
                digit = xdigitval(read());
                if (digit >= 0) {
                        value = (value << 4) + static_cast<char>(digit);
                } else {
                        backtrack();
                }
        } else {
                value = std::char_traits<char>::eof();
                backtrack();
        }

        return value;
}

//--------------------------------------

void
CXXLexer::rawStringLiteral(
        Token &t
)
{
        enum { MAX_DELIMITER_LEN = 16 };

        char32_t c,
                 delimiter[MAX_DELIMITER_LEN];
        int      delimiter_len = 0;
        auto     start_offset = offset();
        auto     start_line = line();
        auto     start_column = column();

        /*
         * read optional delimiter between '"' and '('
         */
        do {
                switch (c = read()) {
                case eof:
                        emit(Diagnostic::ERROR, 1,
                               "end of file in raw string literal delimiter");
                        t.reset();
                        return;
                case U'(':
                        break;
                default:
                        if (!isuspace(c)) {
                                if (delimiter_len >= MAX_DELIMITER_LEN) {
                                        emit(Diagnostic::FATAL_ERROR,
                                             start_offset,
                                             offset() - start_offset,
                                             start_line, start_column,
                                             "raw string literal delimiter length (%d) longer than maximum (%d)",
                                             delimiter_len,
                                             int(MAX_DELIMITER_LEN));
                                        t.reset();
                                        return;
                                }
                                delimiter[delimiter_len++] = c;
                        } else {
                                backtrack();
                                emit(Diagnostic::ERROR, utf8_seq_size(c),
                                     "illegal whitespace character in raw string literal delimiter");
                                read();
                        }
                        break;
                case U'\\': case ')':
                        backtrack();
                        emit(Diagnostic::ERROR, utf8_seq_size(c),
                             "illegal character '%c' in raw string literal delimiter",
                             c);
                        read();
                        break;
                }
        } while ((c != U'(') && (c != eof));

        tmp_spelling_buf_.clear();

        int    delimiter2_len = -1;
        size_t tentative_spelling_len;

        /*
         * read string contents
         */
        while (true) {
                c = base_t::read();
                        // don't interpret trigraphs or escaped newlines

                if (c == eof) {
                        emit(Diagnostic::ERROR, t,
                             "unterminated raw string literal");
                        break;
                } else if (c == U')') {
                        if (delimiter2_len < 0) {
                                tentative_spelling_len
                                        = tmp_spelling_buf_.size();
                                delimiter2_len = 0;
                        } else {
                                delimiter2_len = -1;
                        }
                } else if (c == U'"') {
                        if (delimiter2_len == delimiter_len) {
                                tmp_spelling_buf_.resize(
                                                tentative_spelling_len);
                                break;
                        }
                } else if (delimiter2_len >= 0) {
                        if ((delimiter2_len < delimiter_len) && 
                                        (delimiter[delimiter2_len] == c)) {
                                ++delimiter2_len;
                        } else {
                                delimiter2_len = -1;
                        }
                }

                utf8_append(tmp_spelling_buf_, c);
        }

        t.setSpelling(store(tmp_spelling_buf_));
}

//--------------------------------------

void
CXXLexer::identifierOrKeyword(
        Token &t
)
{
        tmp_spelling_buf_.clear();
        utf8_append(tmp_spelling_buf_, lastRead());

        while (true) {
                char32_t c = read();

                if ((c == U'\\') && (toulower(peek()) == U'u')
                                 && options_.have(cxx::UCNS)) {
                        c = ucn();
                        if (isValidIdentChar(c)) {
                                utf8_append(tmp_spelling_buf_, c);
                        } else if (c == eof) {  // not a UCN
                                break;
                        } else {  // valid UCN but not a legal identifier char
                                backtrack();
                                break;
                        }
                } else if (isValidIdentChar(c)) {
                        utf8_append(tmp_spelling_buf_, c);
                } else {
                        backtrack();
                        break;
                }
        }

        auto i_kw = kw_id_table_.find(tmp_spelling_buf_);

        if (i_kw != kw_id_table_.end()) {
                t.setKind(i_kw->second).setSpelling(i_kw->first);
                if (cxx::isPunctuation(t.kind())) {
                        t.setFlags(t.flags() | cxx::TF_ALTERNATE);
                                /* one of the alternate tokens "and", "bitand",
                                   "or", "bitor", etc. */
                }
        } else {
                t.setKind(TOK_IDENTIFIER);
                t.setSpelling(store(tmp_spelling_buf_));
                kw_id_table_.insert({ t.spelling(), t.kind() });
        }
}

//--------------------------------------

void
CXXLexer::comment(
        Token &t
)
{
        t.setKind(TOK_COMMENT);

        if (options_.have(cxx::KEEP_COMMENTS)) {
                tmp_spelling_buf_.clear();
                utf8_append(tmp_spelling_buf_, lastRead());
                utf8_append(tmp_spelling_buf_, peek());
        }

        read();  // consume '*' or '/' character

        bool is_bcpl = (lastRead() == U'/');

        if (options_.have(cxx::KEEP_COMMENTS)) {
                while (true) {
                        if (is_bcpl) {
                                if (input().eof() || (peek() == U'\n')) {
                                        break;
                                }
                                utf8_append(tmp_spelling_buf_, read());
                        } else if (read() == eof) {
                                emit(Diagnostic::ERROR, t,
                                     "unexpected end of file encountered in comment");
                                break;
                        } else {
                                utf8_append(tmp_spelling_buf_, lastRead());
                                if ((lastRead() == U'*') && (peek() == U'/')) {
                                        utf8_append(tmp_spelling_buf_, read());
                                        break;
                                }
                        }
                }
                t.setSpelling(store(tmp_spelling_buf_));
        } else while (true) {
                if (is_bcpl) {
                        if (input().eof() || (peek() == U'\n')) {
                                break;
                        }
                        read();
                } else if (read() == eof) {
                        emit(Diagnostic::ERROR, t,
                             "unexpected end of file encountered in comment");
                        break;
                } else {
                        if ((lastRead() == U'*') && (peek() == U'/')) {
                                read();
                                break;
                        }
                }
        }
}

//--------------------------------------

void
CXXLexer::ppDirective(
        Token &t
)
{
        std::string &name = tmp_spelling_buf_;
        name.clear();

        size_t n = 0;

        for (char32_t c = read(); isualpha(c) && (c < 0x80); c = read()) {
                name += static_cast<char>(c);
                ++n;
        }

        if (n >= 2) {
                switch (name[0]) {
                case U'd':
                        if (name == u8"define") {
                                t.setKind(cxx::TOK_PP_DEFINE);
                        }
                        break;
                case U'e':
                        switch (name[1]) {
                        case U'l':
                                if (n != 4) {
                                        ;
                                } else if (name == u8"elif") {
                                        t.setKind(cxx::TOK_PP_ELIF);
                                } else if (name == u8"else") {
                                        t.setKind(cxx::TOK_PP_ELSE);
                                }
                                break;
                        case U'n':
                                if (name == u8"endif") {
                                        t.setKind(cxx::TOK_PP_ENDIF);
                                }
                                break;
                        case U'r':
                                if (name == u8"error") {
                                        t.setKind(cxx::TOK_PP_ERROR);
                                }
                                break;
                        default:
                                break;
                        }
                case U'i':
                        switch (name[1]) {
                        case U'f':
                                if (n == 2) {
                                        t.setKind(cxx::TOK_PP_IF);
                                } else if (name == u8"ifdef") {
                                        t.setKind(cxx::TOK_PP_IFDEF);
                                } else if (name == u8"ifndef") {
                                        t.setKind(cxx::TOK_PP_IFNDEF);
                                }
                                break;
                        case U'n':
                                if ((n < 7) ||
                                    name.compare(0, 7, u8"include")) {
                                        ;
                                } else if (n == 7) {
                                        t.setKind(cxx::TOK_PP_INCLUDE);
                                } else if (name == u8"include_next") {
                                        t.setKind(cxx::TOK_PP_INCLUDE_NEXT);
                                }
                                break;
                        default:
                                break;
                        }
                case U'l':
                        if (name == u8"line") {
                                t.setKind(cxx::TOK_PP_LINE);
                        }
                case U'p':
                        if (name == u8"pragma") {
                                t.setKind(cxx::TOK_PP_PRAGMA);
                        }
                        break;
                case U'u':
                        if (name == u8"undef") {
                                t.setKind(cxx::TOK_PP_UNDEF);
                        }
                        break;
                case U'w':
                        if (name == u8"warning") {
                                t.setKind(cxx::TOK_PP_WARNING);
                        }
                        break;
                default:
                        break;
                }
        }

        if (!cxx::isPreprocessorDirective(t.kind())) {
                emit(Diagnostic::WARNING, t,
                     "unrecognised preprocessor directive \"#%s\"", name);
                backtrack(n);
                t.setKind(cxx::TOK_PP_NULL);
        }

        t.setFlags(t.flags() | cxx::TF_PREPROCESS);
        t.setSpelling(cxx::defaultSpelling(t.kind()));
        setNextTokenFlags(nextTokenFlags() | cxx::TF_PREPROCESS);
}

//--------------------------------------

void
CXXLexer::pushClosingToken(
        TokenKind k
)
{
        closing_tokens_.push_front(k);
}

//--------------------------------------

bool
CXXLexer::popClosingTokenIf(
        TokenKind k
)
{
        bool result = false;

        if (!closing_tokens_.empty()) {
                if (k != TOK_GREATER) {
                        while (popClosingTokenIf(TOK_GREATER)) {
                                ;
                        }
                }
                if (closing_tokens_.front() == k) {
                        closing_tokens_.pop_front();
                        result = true;
                }
        }

        return result;
}

//--------------------------------------

bool
CXXLexer::nextClosingTokenIs(
        TokenKind k
) const
{
        return !closing_tokens_.empty() && (closing_tokens_.front() == k);
}


} // namespace parse
} // namespace wr
