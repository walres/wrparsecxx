/**
 * \file CXXParser.h
 *
 * \brief C/C++ grammar and parser interface
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
#ifndef WRPARSECXX_PARSER_H
#define WRPARSECXX_PARSER_H

#include <wrparse/SPPF.h>
#include <wrparse/Grammar.h>
#include <wrparse/Parser.h>
#include <wrparse/cxx/Config.h>


namespace wr {
namespace parse {


class CXXLexer;
class CXXOptions;
class Token;


class WRPARSECXX_API CXXParser:
        public Parser
{
public:
        using this_t = CXXParser;

        CXXParser(const CXXOptions &options);
        CXXParser(const CXXOptions &options, Lexer &lexer);
        CXXParser(CXXLexer &lexer);

        virtual ~CXXParser();

        static CXXParser &getFrom(ParseState &state)
                { return static_cast<CXXParser &>(state.parser()); }

        template <typename T> T *get(const SPPFNode &);
                // specialized in CXXParser.cxx for various data structures

        const CXXOptions &options() const { return options_; }

        static uint8_t qualifierForToken(const Token &token);

        static uint8_t
                typeQualifiersFromSeq(const SPPFNode &type_qualifier_seq);

private:
        const CXXOptions &options_;

public:
        /*
         * C++ grammar nonterminals
         */
                         // A.1 Keywords [gram.key]
        const NonTerminal typedef_name,
                         namespace_name,
                         original_namespace_name,
                         namespace_alias,
                         class_name,
                         enum_name,
                         template_name,
                         undeclared_name,

                         // A.2 Lexical conventions [gram.lex]
                         identifier,
                         literal,
                         numeric_literal,
                         character_literal,
                         string_literal,
                         boolean_literal,
                         pointer_literal,
                         user_defined_literal,
                         ud_suffix,

                         // A.3 Basic concepts [gram.basic]
                         translation_unit,

                         // A.4 Expressions [gram.expr]
                         primary_expression,
                         paren_expression,    // moved from primary-expression
                         generic_selection,   // C11
                         generic_assoc_list,  // C11
                         generic_association, // C11
                         id_expression,
                         unqualified_id,
                         qualified_id,
                         nested_name_specifier,
                         lambda_expression,
                         lambda_introducer,
                         lambda_capture,
                         capture_default,
                         capture_list,
                         capture,
                         simple_capture,
                         init_capture,
                         lambda_declarator,
                         postfix_expression,
                         array_subscript,  /* part of rule moved from
                                              postfix-expression */
                         function_call,    // ditto
                         member_access,    // ditto
                         expression_list,
                         pseudo_destructor_name,
                         unary_expression,
                         unary_operator,
                         new_expression,
                         new_placement,
                         new_type_id,
                         new_declarator,
                         noptr_new_declarator,
                         new_initializer,
                         delete_expression,
                         noexcept_expression,
                         cast_expression,
                         pm_expression,
                         multiplicative_expression,
                         additive_expression,
                         shift_expression,
                         relational_expression,
                         equality_expression,
                         and_expression,
                         exclusive_or_expression,
                         inclusive_or_expression,
                         logical_and_expression,
                         logical_or_expression,
                         conditional_expression,
                         assignment_expression,
                         assignment_operator,
                         expression,
                         constant_expression,

                         // A.5 Statements [gram.stmt]
                         statement,
                         labeled_statement,
                         expression_statement,
                         compound_statement,
                         block_declaration_seq,  // pre-C99
                         statement_seq,
                         selection_statement,
                         condition,
                         iteration_statement,
                         for_init_statement,
                         for_range_declaration,
                         for_range_initializer,
                         jump_statement,
                         declaration_statement,

                         // A.6 Declarations [gram.dcl]
                         declaration_seq,
                         declaration,
                         block_declaration,
                         alias_declaration,
                         simple_declaration,
                         static_assert_declaration,
                         empty_declaration,
                         attribute_declaration,
                         decl_specifier,
                         decl_specifier_seq,
                         storage_class_specifier,
                         function_specifier,
                         // typedef-name: see section A.1 Keywords [gram.key]
                         type_specifier,
                         trailing_type_specifier,
                         type_specifier_seq,
                         trailing_type_specifier_seq,
                         simple_type_specifier,
                         ud_type_specifier,  /* specifiers involving name of
                                                user-defined type, moved from
                                                simple-type-specifier */
                         type_name,
                         decltype_specifier,
                         elaborated_type_specifier,
                         atomic_type_specifier,  // C11
                         // enum-name: see section A.1 Keywords [gram.key]
                         enum_specifier,
                         enum_head,
                         opaque_enum_declaration,
                         enum_key,
                         enum_base,
                         enumerator_list,
                         enumerator_definition,
                         enumerator,
                         // namespace-name: see section A.1 Keywords [gram.key]
                         /* original-namespace-name: see section A.1 Keywords
                            [gram.key] */
                         namespace_definition,
                         named_namespace_definition,
                         original_namespace_definition,
                         extension_namespace_definition,
                         unnamed_namespace_definition,
                         namespace_body,
                         namespace_alias_definition,
                         qualified_namespace_specifier,
                         using_declaration,
                         using_directive,
                         asm_definition,
                         linkage_specification,
                         attribute_specifier_seq,
                         attribute_specifier,
                         alignment_specifier,
                         attribute_list,
                         attribute,
                         attribute_token,
                         attribute_scoped_token,
                         attribute_namespace,
                         attribute_argument_clause,
                         balanced_token_seq,
                         balanced_token,

                         // A.7 Declarators [gram.decl]
                         init_declarator_list,
                         init_declarator,
                         declarator,
                         ptr_declarator,
                         noptr_declarator,
                         nested_declarator,  // moved from noptr-declarator
                         array_declarator,   // ditto
                         parameters_and_qualifiers,
                         trailing_return_type,
                         ptr_operator,
                         type_qualifier_seq,
                         type_qualifier,
                         ref_qualifier,
                         declarator_id,
                         type_id,
                         abstract_declarator,
                         ptr_abstract_declarator,
                         noptr_abstract_declarator,
                         nested_abstract_declarator,
                         abstract_pack_declarator,
                         noptr_abstract_pack_declarator,
                         parameter_declaration_clause,
                         parameter_declaration_list,
                         parameter_declaration,
                         function_definition,
                         function_body,
                         initializer,
                         brace_or_equal_initializer,
                         initializer_clause,
                         initializer_list,
                         braced_init_list,
                         designation,  // C99
                         designator_list,  // C99
                         designator,  // C99

                         // A.8 Classes [gram.class]
                         class_specifier,
                         class_head,
                         class_head_name,
                         class_virt_specifier,
                         class_key,
                         member_specification,
                         member_declaration,
                         member_declarator_list,
                         member_declarator,
                         virt_specifier_seq,
                         virt_specifier,
                         pure_specifier,
                         base_clause,
                         base_specifier_list,
                         base_specifier,
                         class_or_decltype,
                         base_type_specifier,
                         access_specifier,

                         // A.10 Special member functions [gram.special]
                         conversion_function_id,
                         conversion_type_id,
                         conversion_declarator,
                         ctor_initializer,
                         mem_initializer_list,
                         mem_initializer,
                         mem_initializer_id,
                         destructor_id,  // rules moved from unqualified-id

                         // A.11 Overloading [gram.over]
                         operator_function_id,
                         overloadable_operator,
                         literal_operator_id,

                         // A.12 Templates [gram.temp]
                         template_declaration,
                         template_parameter_list,
                         template_parameter,
                         type_parameter,
                         simple_template_id,
                         template_id,
                         // template-name: see section A.1 Keywords [gram.key]
                         template_argument_list,
                         template_argument,
                         typename_specifier,
                         explicit_instantiation,
                         explicit_specialization,

                         // A.13 Exception handling [gram.except]
                         try_block,
                         function_try_block,
                         handler_seq,
                         handler,
                         exception_declaration,
                         throw_expression,
                         exception_specification,
                         dynamic_exception_specification,
                         type_id_list,
                         noexcept_specification;

        const Rule      *equal,
                        *not_equal,
                        *less,
                        *less_or_equal,
                        *greater,
                        *greater_or_equal,
                        *binary_add,
                        *binary_subtract,
                        *left_shift,
                        *right_shift,
                        *multiply,
                        *divide,
                        *modulo;

        bool langC() const    { return options_.c() != 0; }
        bool stdC99() const   { return options_.c() >= cxx::C99; }
        bool stdC11() const   { return options_.c() >= cxx::C11; }
        bool langCXX() const  { return options_.cxx() != 0; }
        bool stdCXX11() const { return options_.cxx() >= cxx::CXX11; }
        bool stdCXX14() const { return options_.cxx() >= cxx::CXX14; }
        bool stdCXX17() const { return options_.cxx() >= cxx::CXX17; }

        /**
         * \brief Bit values representing \c const, \c volatile, \c restrict
         *      and reference qualifiers
         */
        enum: uint8_t
        {
                CONST    = 0x1,
                VOLATILE = 0x2,
                RESTRICT = 0x4,
                ATOMIC   = 0x8,
                LVAL_REF = 0x40,  // functions only
                RVAL_REF = 0x80,  // ditto
        };

        /**
         * \brief Data attached to \c decl_specifier_seq nonterminals plus the
         *      similar nonterminals \c trailing_type_specifier_seq and
         *      \c type_specifier_seq
         */
        class DeclSpecifier : public AuxData
        {
        public:
                using this_t = DeclSpecifier;
                using Ptr = boost::intrusive_ptr<this_t>;

                uint8_t type_qual = 0;  /**< \c const, \c volatile, \c restrict
                                             and/or \c _Atomic (but not & or &&)
                                             qualifier(s) */

                enum Sign: uint8_t { NO_SIGN = 0, SIGNED, UNSIGNED }
                        sign_spec = NO_SIGN;  /**< \c signed / \c unsigned
                                                 specifiers for \c char / \c int
                                                 types only */

                enum Size: uint8_t { NO_SIZE = 0, SHORT, LONG, LONG_LONG }
                        size_spec = NO_SIZE;  /**< \c short, \c long and
                                                   \c {long long} specifiers for
                                                   \c int and \c double only */

                enum Type: uint8_t { NO_TYPE = 0, VOID, AUTO, DECLTYPE, BOOL,
                                     CHAR, CHAR16_T, CHAR32_T, WCHAR_T, INT,
                                     FLOAT, DOUBLE, NULLPTR_T, OTHER }
                        type_spec = NO_TYPE;  ///< core type specifier present

                SPPFNode::ConstPtr sign_spec_node,
                                   size_spec_node,
                                   type_spec_node;

                AuxData::Ptr user_data;
                                ///< for API users to hang extra data on
        private:
                friend CXXParser;

                static bool end(ParseState &state);  // nonterminal callback

                bool addDeclSpecifier(CXXParser &cxx, const SPPFNode &spec);
                        // helper for end()
        };


        /**
         * \brief Data attached to \c declarator, \c nested_declarator,
         *      \c abstract_declarator, \c nested_abstract_declarator,
         *      \c new_declarator, \c conversion_declarator and
         *      \c lambda_declarator nonterminals
         */
        class WRPARSECXX_API Declarator : public AuxData
        {
        public:
                using this_t = Declarator;
                using Ptr = boost::intrusive_ptr<this_t>;

                const Token *last_ptr    = nullptr, /**< last \c *, \c X::*,
                                                         \c & or \c && part */
                            *begin_parms = nullptr; /**< start of function
                                                         parameter list */
                bool         array       = false;   /**< \c true if declarator
                                                         ends with array */
                AuxData::Ptr user_data;  /**< for API users to hang
                                              extra data on */

                static const SPPFNode *lastPtrOperator(CXXParser &cxx,
                                                      const SPPFNode &dcl_node);

                static bool isReference(CXXParser &cxx,
                                        const SPPFNode &dcl_node);

        private:
                friend CXXParser;

                // predicates
                static bool isFunction(ParseState &state);

                // nonterminal callbacks
                static bool end(ParseState &state);

                // final checks and settings
                bool check(ParseState &state, CXXParser &cxx,
                           const SPPFNode &dcl_node);
        };

        friend Declarator;


        /**
         * \brief Data attached to \c ptr_operator and
         *      \c parameter_declaration_clause nonterminals
         */
        class WRPARSECXX_API DeclaratorPart : public AuxData
        {
        public:
                using this_t = DeclaratorPart;
                using Ptr = boost::intrusive_ptr<this_t>;

                unsigned short count      = 0;      /**< no. of function
                                                         parameters */
                bool           variadic   = false;  /**< whether parameter list
                                                         ends with \c ... */
                uint8_t        qualifiers = 0;      /**< \c const, \c volatile,
                                                         \c restrict and/or
                                                         ref-qualifier(s) */
                AuxData::Ptr   user_data;  /**< for API users to hang
                                                extra data on */

                static bool isParmPackOperator(CXXParser &cxx,
                                               const SPPFNode &part);

        private:
                friend CXXParser;

                // nonterminal callbacks
                static bool endPtrOperator(ParseState &state);
                static bool endParametersAndQualifiers(ParseState &state);
        };

private:
        // predicates
        static bool isTypedefName(ParseState &state);
        static bool isClassName(ParseState &state);
        static bool isEnumName(ParseState &state);
        static bool isNamespaceName(ParseState &state);
        static bool isNamespaceAliasName(ParseState &state);
        static bool isTemplateName(ParseState &state);
        static bool isUndeclaredName(ParseState &state);
        static bool isFinalSpecifier(ParseState &state);
        static bool isBalancedToken(ParseState &state);
        static bool processTemplParmArgListEndToken(ParseState &state);
};


} // namespace parse

//--------------------------------------

namespace fmt {


struct Arg;
template <typename> struct TypeHandler;

/**
 * \brief support for CXXParser::DeclSpecifier::Sign arguments to
 *      wr::print() functions
 */
template <>
struct WRPARSE_API TypeHandler<parse::CXXParser::DeclSpecifier::Sign>
{
        static void set(Arg &arg, parse::CXXParser::DeclSpecifier::Sign val);
};

/**
 * \brief support for CXXParser::DeclSpecifier::Size arguments to
 *      wr::print() functions
 */
template <>
struct WRPARSE_API TypeHandler<parse::CXXParser::DeclSpecifier::Size>
{
        static void set(Arg &arg, parse::CXXParser::DeclSpecifier::Size val);
};

/**
 * \brief support for CXXParser::DeclSpecifier::Type arguments to
 *      wr::print() functions
 */
template <>
struct WRPARSE_API TypeHandler<parse::CXXParser::DeclSpecifier::Type>
{
        static void set(Arg &arg, parse::CXXParser::DeclSpecifier::Type val);
};


} // namespace fmt
} // namespace wr


#endif // !WRPARSECXX_PARSER_H
