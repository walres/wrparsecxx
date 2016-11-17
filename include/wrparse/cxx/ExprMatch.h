/**
 * \file ExprMatch.h
 *
 * \brief C/C++ expression matching APIs
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
#ifndef WRPARSECXX_EXPR_MATCH_H
#define WRPARSECXX_EXPR_MATCH_H

#include <stdint.h>
#include <wrparse/cxx/Config.h>
#include <wrparse/cxx/CXXParser.h>


namespace wr {
namespace parse {
namespace cxx {


struct Literal;


struct WRPARSECXX_API ExprType
{
        typedef CXXParser::DeclSpecifier::Sign Sign;
        typedef CXXParser::DeclSpecifier::Size Size;
        typedef CXXParser::DeclSpecifier::Type Type;

        Sign sign = Sign::NO_SIGN;
        Size size = Size::NO_SIZE;
        Type type = Type::NO_TYPE;

        ExprType() = default;
        ExprType(const ExprType &) = default;
        ExprType(Sign in_sign, Size in_size, Type in_type);
        ExprType(CXXParser &cxx, const SPPFNode &parsed_declarator);

        bool isSigned() const;
        bool isUnsigned() const;
        bool isNonPtrArithmeticType() const;
        int intConvRank() const;

        static ExprType bestCommonType(const Literal &a, const Literal &b);

        ExprType &operator=(const ExprType &) = default;

        ExprType &set(CXXParser &cxx, const SPPFNode &parsed_declarator);

        explicit operator bool() const { return type != Type::NO_TYPE; }
        bool operator!() const         { return type == Type::NO_TYPE; }

        bool operator==(const ExprType &other) const
                { return (type == other.type) && (size == other.size)
                         && (sign == other.sign); }

        bool operator!=(const ExprType &other) const
                { return (type != other.type) || (size != other.size)
                         || (sign != other.sign); }
};

//--------------------------------------

struct Literal
{
        ExprType type;

        union
        {
                intmax_t    i;
                uintmax_t   u;
                long double d;
        };


        WRPARSECXX_API Literal(CXXParser &cxx, const SPPFNode &input);
        WRPARSECXX_API Literal(CXXParser &cxx, const SPPFNode &input,
                               ExprType convert_to_type);
        WRPARSECXX_API Literal(const Literal &other) = default;
        WRPARSECXX_API Literal(const Literal &other, ExprType convert_to_type);

        WRPARSECXX_API Literal &convertType(ExprType to_type);

private:
        void readNumericLiteral(const Token &input);
        void readCharacterLiteral(const Token &input);
};

//--------------------------------------

WRPARSECXX_API bool matchConstExpr(CXXParser &cxx, SPPFNode::ConstPtr a,
                                   SPPFNode::ConstPtr b, ExprType target_type);

WRPARSECXX_API bool areEquivalent(const Literal &a, const Literal &b,
                                  ExprType target_type);


} // namespace cxx
} // namespace parse
} // namespace wr


#endif // !WRPARSECXX_EXPR_MATCH_H
