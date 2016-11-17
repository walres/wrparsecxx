/**
 * \file ExprMatch.cxx
 *
 * \brief C/C++ expression matching implementation
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
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <iostream>
#include <string>

#include <wrutil/u8string_view.h>
#include <wrparse/SPPF.h>
#include <wrparse/cxx/CXXLexer.h>
#include <wrparse/cxx/CXXParser.h>
#include <wrparse/cxx/CXXTokenKinds.h>
#include <wrparse/cxx/ExprMatch.h>


namespace wr {
namespace parse {
namespace cxx {


WRPARSECXX_API bool
matchConstExpr(
        CXXParser          &cxx,
        SPPFNode::ConstPtr  a,
        SPPFNode::ConstPtr  b,
        ExprType            target_type
)
{
        bool result = false;

        while (a->is(cxx.paren_expression)) {
                a = nonTerminals(a).node();
        }
        while (b->is(cxx.paren_expression)) {
                b = nonTerminals(b).node();
        }
        if (a->is(cxx.literal)) {
                Literal a_literal(cxx, *a);

                if (b->is(cxx.literal)) {
                        Literal b_literal(cxx, *b);
                        result = areEquivalent(a_literal, b_literal,
                                               target_type);
                }
        }

        return result;
}

//--------------------------------------

WRPARSECXX_API
ExprType::ExprType(
        Sign in_sign,
        Size in_size,
        Type in_type
) :
        sign(in_sign),
        size(in_size),
        type(in_type)
{
        if (!type && (sign || size)) {
                type = Type::INT;
        }
}

//--------------------------------------

WRPARSECXX_API
ExprType::ExprType(
        CXXParser      &cxx,
        const SPPFNode &declarator
) :
        ExprType()
{
        set(cxx, declarator);
}

//--------------------------------------

WRPARSECXX_API bool
ExprType::isSigned() const
{
        switch (type) {
        case Type::CHAR: case Type::INT:
                return sign != Sign::UNSIGNED;
        case Type::FLOAT: case Type::DOUBLE:
                return true;
        default:
                return false;
        }
}

//--------------------------------------

WRPARSECXX_API bool
ExprType::isUnsigned() const
{
        switch (type) {
        case Type::BOOL: case Type::CHAR16_T: case Type::CHAR32_T:
        case Type::WCHAR_T:
                return true;
        case Type::CHAR: case Type::INT:
                return sign == Sign::UNSIGNED;
        default:
                return false;
        }
}

//--------------------------------------

WRPARSECXX_API bool
ExprType::isNonPtrArithmeticType() const
{
        return (type >= Type::BOOL) && (type <= Type::DOUBLE);
}

//--------------------------------------

WRPARSECXX_API int
ExprType::intConvRank() const
{
        switch (type) {
        case Type::BOOL:
                return 0;
        case Type::CHAR:
                return 1;
        case Type::CHAR16_T:
                if (sizeof(char16_t) == sizeof(short)) {
                        return 2;
                } else if (sizeof(char16_t) == sizeof(int)) {
                        return 3;
                } else if (sizeof(char16_t) == sizeof(long)) {
                        return 4;
                }
                break;  // unexpected
        case Type::CHAR32_T:
                if (sizeof(char32_t) == sizeof(int)) {
                        return 3;
                } else if (sizeof(char32_t) == sizeof(long)) {
                        return 4;
                }
                break;  // unexpected
        case Type::WCHAR_T:
                if (sizeof(wchar_t) == sizeof(char)) {
                        return 1;
                } else if (sizeof(wchar_t) == sizeof(short)) {
                        return 2;
                } else if (sizeof(wchar_t) == sizeof(int)) {
                        return 3;
                } else if (sizeof(wchar_t) == sizeof(long)) {
                        return 4;
                } else if (sizeof(wchar_t) == sizeof(long long)) {
                        return 5;
                }
                break;  // unexpected
        case Type::INT:
                switch (size) {
                case Size::SHORT:
                        return 2;
                case Size::NO_SIZE:
                        return 3;
                case Size::LONG:
                        return 4;
                case Size::LONG_LONG:
                        return 5;
                default:  // invalid
                        break;
                }
        default:
                break;
        }

        return -1;
}

//--------------------------------------

WRPARSECXX_API ExprType
ExprType::bestCommonType(
        const Literal &a,
        const Literal &b
)
{
        if (a.type == b.type) {
                return a.type;
        }

        if (!a.type.isNonPtrArithmeticType()
                        || !b.type.isNonPtrArithmeticType()) {
                return {};
        }

        int a_rank = a.type.intConvRank(),
            b_rank = b.type.intConvRank();

        if ((a_rank >= 0) && (b_rank >= 0)) {
                if (a_rank > b_rank) {
                        return a.type;
                } else if (b_rank > a_rank) {
                        return b.type;
                } else {
                        /* signed vs. unsigned - prefer the unsigned type if
                           the value of signed type is >= 0, otherwise pick
                           the signed type */
                        if (a.type.isUnsigned()) {
                                if (b.i >= 0) {
                                        return a.type;
                                } else {
                                        return b.type;
                                }
                        } else {
                                if (a.i >= 0) {
                                        return b.type;
                                } else {
                                        return a.type;
                                }
                        }
                }
        } else {
                // a and/or b are of floating point type, settle for long double
                return ExprType(Sign::NO_SIGN, Size::LONG, Type::DOUBLE);
        }
}

//--------------------------------------

WRPARSECXX_API ExprType &
ExprType::set(
        CXXParser      &cxx,
        const SPPFNode &declarator
)
{
        auto decl_specifier_seq = declarator.find(cxx.decl_specifier_seq);

        if (!decl_specifier_seq) {
                decl_specifier_seq = declarator.find(cxx.type_specifier_seq);
                if (!decl_specifier_seq) {
                        return *this;
                }
        }

        auto *type_info
                = cxx.get<CXXParser::DeclSpecifier>(*decl_specifier_seq);

        sign = type_info->sign_spec;
        size = type_info->size_spec;
        type = type_info->type_spec;
        return *this;
}

//--------------------------------------

WRPARSECXX_API
Literal::Literal(
        CXXParser      &cxx,
        const SPPFNode &input
)
{
        if (input.is(cxx.numeric_literal)) {
                readNumericLiteral(*input.firstToken());
        } else if (input.is(cxx.character_literal)) {
                readCharacterLiteral(*input.firstToken());
        } else if (input.is(cxx.string_literal)) {
                ;
        } else if (input.is(cxx.boolean_literal)) {  // true or false
                type.type = ExprType::Type::BOOL;
                i = input.firstToken()->kind() == TOK_KW_TRUE;
        } else if (input.is(cxx.pointer_literal)) {  // nullptr
                type.type = ExprType::Type::NULLPTR_T;
                u = 0;
        } /* else input is either a user-defined literal (can't handle these)
             or not a literal at all, so leave type unset to distinguish as
             such */
}

//--------------------------------------

WRPARSECXX_API
Literal::Literal(
        CXXParser      &cxx,
        const SPPFNode &input,
        ExprType        convert_to_type
) :
        Literal(cxx, input)
{
        convertType(convert_to_type);
}

//--------------------------------------

WRPARSECXX_API
Literal::Literal(
        const Literal &other,
        ExprType       convert_to_type
) :
        Literal(other)
{
        convertType(convert_to_type);
}

//--------------------------------------

WRPARSECXX_API Literal &
Literal::convertType(
        ExprType to_type
)
{
        ExprType &from_type = type;

        if (to_type == from_type) {
                return *this;
        }

        bool from_signed, to_signed;
        int  from_rank, to_rank;

        switch (to_type.type) {
        case ExprType::Type::BOOL:
                switch (from_type.type) {
                case ExprType::Type::CHAR: case ExprType::Type::CHAR16_T:
                case ExprType::Type::CHAR32_T: case ExprType::Type::WCHAR_T:
                case ExprType::Type::INT: case ExprType::Type::NULLPTR_T:
                        i = (i != 0);
                        break;
                case ExprType::Type::FLOAT: case ExprType::Type::DOUBLE:
                        i = (d != 0.0);
                        break;
                default:
                        to_type = {};  // can't convert
                        break;
                }
                break;
        case ExprType::Type::CHAR: case ExprType::Type::CHAR16_T:
        case ExprType::Type::CHAR32_T: case ExprType::Type::WCHAR_T:
        case ExprType::Type::INT:
                if (from_type.type == ExprType::Type::BOOL) {
                        break;
                }
                from_signed = from_type.isSigned();
                to_signed = to_type.isSigned();
                from_rank = from_type.intConvRank();
                to_rank = to_type.intConvRank();

                if ((from_rank >= 0) && (to_rank >= from_rank)) {
                        if (to_signed == from_signed) {
                                break;  /* to same sign and either same size
                                           or widening */
                        }
                        if (!from_signed && (to_rank > from_rank)) {
                                break;  // widening unsigned-to-signed
                        }
                } else if ((from_type.type == ExprType::Type::FLOAT)
                           || (from_type.type == ExprType::Type::DOUBLE)) {
                        i = static_cast<intmax_t>(d);
                        from_type.size = ExprType::Size::LONG_LONG;
                        from_type.type = ExprType::Type::INT;
                        from_rank = from_type.intConvRank();
                } else if ((from_rank < 0) || (to_rank < 0)) {
                        to_type = {};
                        break;
                }
                        
                if (to_signed == from_signed) {  // narrowing, same sign
                        switch (to_type.type) {
                        case ExprType::Type::CHAR:
                                if (to_signed) {
                                        i = static_cast<signed char>(i);
                                } else {
                                        u = static_cast<unsigned char>(u);
                                }
                                break;
                        case ExprType::Type::CHAR16_T:
                                u = static_cast<char16_t>(u);
                                break;
                        case ExprType::Type::CHAR32_T:
                                u = static_cast<char32_t>(u);
                                break;
                        case ExprType::Type::WCHAR_T:
                                u = static_cast<wchar_t>(u);
                                break;
                        case ExprType::Type::INT:
                                if (to_signed) switch (to_type.size) {
                                case ExprType::Size::SHORT:
                                        i = static_cast<short>(i);
                                        break;
                                case ExprType::Size::NO_SIZE:
                                        i = static_cast<int>(i);
                                        break;
                                case ExprType::Size::LONG:
                                        i = static_cast<long>(i);
                                        break;
                                default:
                                        break;
                                } else switch (to_type.size) {
                                case ExprType::Size::SHORT:
                                        u = static_cast<unsigned short>(u);
                                        break;
                                case ExprType::Size::NO_SIZE:
                                        u = static_cast<unsigned int>(u);
                                        break;
                                case ExprType::Size::LONG:
                                        u = static_cast<unsigned long>(u);
                                        break;
                                default:
                                        break;
                                }
                        default:
                                break;
                        }
                } else if (from_signed) {  // signed to unsigned
                        if ((i >= 0) && (to_rank >= from_rank)) {
                                break;  /* source value guaranteed to be
                                           representable by target type */
                        }

                        switch (to_type.type) {
                        case ExprType::Type::CHAR:
                                u = static_cast<unsigned char>(u);
                                break;
                        case ExprType::Type::CHAR16_T:
                                u = static_cast<char16_t>(u);
                                break;
                        case ExprType::Type::CHAR32_T:
                                u = static_cast<char32_t>(u);
                                break;
                        case ExprType::Type::WCHAR_T:
                                u = static_cast<wchar_t>(u);
                                break;
                        case ExprType::Type::INT:
                                switch (to_type.size) {
                                case ExprType::Size::SHORT:
                                        u = static_cast<unsigned short>(u);
                                        break;
                                case ExprType::Size::NO_SIZE:
                                        u = static_cast<unsigned int>(u);
                                        break;
                                case ExprType::Size::LONG:
                                        u = static_cast<unsigned long>(u);
                                        break;
                                case ExprType::Size::LONG_LONG:
                                        u = static_cast<unsigned long long>(u);
                                        break;
                                default:
                                        break;
                                }
                        default:
                                break;
                        }
                } else if (!from_signed) {
                        // unsigned to signed, same size or narrowing
                        switch (to_type.type) {
                        case ExprType::Type::CHAR:
                                i = static_cast<signed char>(u);
                                break;
                        case ExprType::Type::INT:
                                switch (to_type.size) {
                                case ExprType::Size::SHORT:
                                        i = static_cast<short>(u);
                                        break;
                                case ExprType::Size::NO_SIZE:
                                        i = static_cast<int>(u);
                                        break;
                                case ExprType::Size::LONG:
                                        i = static_cast<long>(u);
                                        break;
                                default:
                                        break;
                                }
                        default:
                                break;
                        }
                }
                break;
        case ExprType::Type::FLOAT:
                switch (from_type.type) {
                case ExprType::Type::CHAR: case ExprType::Type::CHAR16_T:
                case ExprType::Type::CHAR32_T: case ExprType::Type::WCHAR_T:
                case ExprType::Type::INT: case ExprType::Type::BOOL:
                        if (from_type.isSigned()) {
                                d = static_cast<float>(i);
                        } else {
                                d = static_cast<float>(u);
                        }
                        break;
                case ExprType::Type::DOUBLE:
                        d = static_cast<float>(d);
                        break;
                default:
                        to_type = {};  // can't convert
                        break;
                }
                break;
        case ExprType::Type::DOUBLE:
                switch (from_type.type) {
                case ExprType::Type::CHAR: case ExprType::Type::CHAR16_T:
                case ExprType::Type::CHAR32_T: case ExprType::Type::WCHAR_T:
                case ExprType::Type::INT: case ExprType::Type::BOOL:
                        switch (to_type.size) {
                        default:
                                if (from_type.isSigned()) {
                                        d = static_cast<double>(i);
                                } else {
                                        d = static_cast<double>(u);
                                }
                                break;
                        case ExprType::Size::LONG:
                                if (from_type.isSigned()) {
                                        d = static_cast<long double>(i);
                                } else {
                                        d = static_cast<long double>(u);
                                }
                                break;
                        }
                case ExprType::Type::DOUBLE:
                        if ((to_type.size != from_type.size)
                                  && (from_type.size == ExprType::Size::LONG)) {
                                d = static_cast<double>(d);
                        }
                        break;
                default:
                        to_type = {};  // can't convert
                        break;
                }
                break;
        default:
                to_type = {};
                break;
        }

        type = to_type;
        return *this;
}

//--------------------------------------

void
Literal::readNumericLiteral(
        const Token &input
)
{
        u8string_view spelling = input.spelling();

        if (spelling.empty()) {  // empty spelling is invalid
                return;
        }

        auto begin    = spelling.begin(), end = spelling.end(), pos = begin;
        bool negative = (*pos == '-');

        if (negative) {
                ++pos;
        }

        switch (input.kind()) {
        case TOK_BIN_INT_LITERAL:
        case TOK_HEX_INT_LITERAL:
                std::advance(pos, 2);  // skip '0B', '0b', '0X' or '0x' prefix
                break;
        case TOK_OCT_INT_LITERAL:
        case TOK_DEC_INT_LITERAL:
        case TOK_FLOAT_LITERAL:
                break;
        default:  // not a numeric literal
                return;
        }

        if (pos >= end) {  // invalid spelling
                return;
        }

        // set default type characteristics
        if (input.kind() == TOK_FLOAT_LITERAL) {
                type.type = ExprType::Type::DOUBLE;
                type.sign = ExprType::Sign::NO_SIGN;
                d = 0.0;
        } else {
                type.type = ExprType::Type::INT;
                type.sign = ExprType::Sign::SIGNED;
                i = 0;
        }

        bool      overflow = false;
        long long top_digit_mask;

        switch (input.kind()) {
        case TOK_BIN_INT_LITERAL:
                top_digit_mask = 1LL << ((sizeof(top_digit_mask) * 8) - 1);

                for (; pos < end; ++pos) {
                        char32_t c = *pos;
                        if (c == '\'') {  // separator
                                continue;
                        } else if ((c < '0') || (c > '1')) {
                                break;
                        }
                        overflow = overflow || ((i & top_digit_mask) != 0);
                        i = (i << 1) | (c - '0');
                }
                break;
        case TOK_OCT_INT_LITERAL:
                top_digit_mask = 7LL << ((sizeof(top_digit_mask) * 8) - 3);

                for (; pos < end; ++pos) {
                        char32_t c = *pos;
                        if (c == '\'') {
                                continue;
                        } else if ((c < '0') || (c > '7')) {
                                break;
                        }
                        overflow = overflow || ((i & top_digit_mask) != 0);
                        i = (i << 3) | (c - '0');
                }
                break;
        case TOK_DEC_INT_LITERAL:
                for (; pos < end; ++pos) {
                        char32_t c = *pos;
                        if (c == '\'') {
                                continue;
                        } else if ((c < '0') || (c > '9')) {
                                break;
                        }
                        overflow = overflow || (u > (ULLONG_MAX - 10));
                        i = (i * 10) + (c - '0');
                }
                break;
        case TOK_HEX_INT_LITERAL:
                top_digit_mask = 15LL << ((sizeof(top_digit_mask) * 8) - 4);

                for (; pos < end; ++pos) {
                        char32_t c = *pos;
                        if (c == '\'') {
                                continue;
                        } else if ((c >= 0x80)
                                        || !isxdigit(static_cast<char>(c))) {
                                break;
                        }
                        overflow = overflow || ((i & top_digit_mask) != 0);
                        i <<= 4;
                        if (c <= '9') {
                                i |= (c - '0');
                        } else {
                                i |= tolower(static_cast<char>(c)) - 'a' + 10;
                        }
                }
                break;
        case TOK_FLOAT_LITERAL:
                {
                        size_t stop_ix;
                        d = std::stold(spelling.to_string(), &stop_ix);
                        pos = std::next(begin, stop_ix);
                }
        default:  // not reached
                return;
        }

        if (input.kind() != TOK_FLOAT_LITERAL) {
                if (negative) {
                        i = -i;
                }

                /*
                 * infer type from magnitude of value
                 */
                if (overflow) {  // just use biggest allowable size
                        type.size = ExprType::Size::LONG_LONG;
                        if (!negative) {
                                type.sign = ExprType::Sign::UNSIGNED;
                        }
                } else if ((i >= INT_MIN) && (i <= INT_MAX)) {
                        ;  // leave alone
                } else if (!negative && (u <= UINT_MAX)) {
                        type.sign = ExprType::Sign::UNSIGNED;
                } else if ((i >= LONG_MIN) && (i <= LONG_MAX)) {
                        type.size = ExprType::Size::LONG;
                } else if (!negative && (u <= ULONG_MAX)) {
                        type.sign = ExprType::Sign::UNSIGNED;
                        type.size = ExprType::Size::LONG;
                } else {
                        type.size = ExprType::Size::LONG_LONG;
                        if (!negative && (u > LLONG_MAX)) {
                                type.sign = ExprType::Sign::UNSIGNED;
                        }
                }
        }

        /*
         * apply suffix if present
         */
        for (; pos < end; ++pos) {
                switch (*pos) {
                case 'U': case 'u':
                        if (input.kind() != TOK_FLOAT_LITERAL) {
                                type.sign = ExprType::Sign::UNSIGNED;
                        } else {
                                pos = end;
                        }
                        break;
                case 'L': case 'l':
                        if (type.size == ExprType::Size::NO_SIZE) {
                                type.size = ExprType::Size::LONG;
                                if (input.kind() == TOK_FLOAT_LITERAL) {
                                        pos = end;
                                }
                        } else if (type.size == ExprType::Size::LONG) {
                                type.size = ExprType::Size::LONG_LONG;
                        } else {
                                pos = end;
                        }
                        break;
                case 'F': case 'f':
                        if (input.kind() == TOK_FLOAT_LITERAL) {
                                type.type = ExprType::Type::FLOAT;
                        }
                        pos = end;
                        break;
                default:
                        pos = end;
                        break;
                }
        }
}

//--------------------------------------

void
Literal::readCharacterLiteral(
        const Token &input
)
{
        u8string_view spelling = input.spelling();

        if (spelling.empty()) {  // empty spelling is invalid
                return;
        }

        auto           end = spelling.end(), pos = spelling.begin();
        ExprType::Type t   = ExprType::Type::NO_TYPE;

        switch (input.kind()) {
        case TOK_CHAR_LITERAL:
                t = ExprType::Type::CHAR;
                break;
        case TOK_WCHAR_LITERAL:
                t = ExprType::Type::WCHAR_T;
                break;
        case TOK_U16_CHAR_LITERAL:
                t = ExprType::Type::CHAR16_T;
                break;
        case TOK_U32_CHAR_LITERAL:
                t = ExprType::Type::CHAR32_T;
                break;
        default:  // not a character literal
                return;
        }

        if ((++pos) >= end) {  // invalid spelling
                return;
        }

        i = 0;

        u8string_view_iterator begin = pos, stop;

        if (*pos == '\\') {
                ++begin;
                if ((++pos) >= end) {
                        return;
                }

                switch (*pos) {
                case '\'': case '"': case '?': case '\\':
                        i = *pos;
                        ++pos;
                        break;
                case 'a':
                        i = '\a';
                        ++pos;
                        break;
                case 'b':
                        i = '\b';
                        ++pos;
                        break;
                case 'f':
                        i = '\f';
                        ++pos;
                        break;
                case 'n':
                        i = '\n';
                        ++pos;
                        break;
                case 'r':
                        i = '\r';
                        ++pos;
                        break;
                case 't':
                        i = '\t';
                        ++pos;
                        break;
                case 'v':
                        i = '\v';
                        ++pos;
                        break;
                case 'u': case 'U': case 'x':
                        if (*pos == 'u') {
                                stop = std::next(pos, 5);
                        } else if (*pos == 'U') {
                                stop = std::next(pos, 9);
                        } else { // 'x'
                                stop = std::prev(end);
                        }

                        if (stop >= end) {
                                return;
                        }

                        for (++pos; pos < stop; ++pos) {
                                char32_t c = *pos;
                                if (c >= 0x80) {
                                        break;
                                } else if (!isxdigit(static_cast<char>(c))) {
                                        break;
                                }
                                i <<= 4;
                                if (c <= '9') {
                                        i |= c - '0';
                                } else {
                                        i |= tolower(static_cast<char>(c))
                                                - 'a' + 10;
                                }
                        }
                        break;
                default:
                        stop = std::next(pos, 3);
                        if (stop >= end) {
                                return;
                        }

                        for (; pos < stop; ++pos) {
                                char32_t c = *pos;
                                if ((c < '0') || (c > '7')) {
                                        break;
                                }
                                i = (i << 3) | (c - '0');
                        }
                        break;
                }
        } else if (*pos != '\'') {
                i = *pos;
        }

        if ((pos > begin) && (pos < end) && (*pos == '\'')) {
                type.type = t;
        }
}

//--------------------------------------

WRPARSECXX_API bool
areEquivalent(
        const Literal &a,
        const Literal &b,
        ExprType       target_type
)
{
        if (!target_type || target_type.type == ExprType::Type::OTHER) {
                target_type = ExprType::bestCommonType(a, b);
        }

        Literal a2(a, target_type);

        if (!a2.type.type) {
                return false;
        }

        Literal b2(b, target_type);

        if (!b2.type.type) {
                return false;
        }

        switch (target_type.type) {
        case ExprType::Type::BOOL: case ExprType::Type::CHAR:
        case ExprType::Type::CHAR16_T: case ExprType::Type::CHAR32_T:
        case ExprType::Type::WCHAR_T: case ExprType::Type::INT:
        case ExprType::Type::NULLPTR_T:
                return a2.i == b2.i;
        case ExprType::Type::FLOAT: case ExprType::Type::DOUBLE:
                return a2.d == b2.d;
        default:
                return false;
        }
}


} // namespace cxx
} // namespace parse
} // namespace wr
