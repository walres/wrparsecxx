/**
 * \file CXXParser.cxx
 *
 * \brief C/C++ grammar and parser implementation
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
#include <wrutil/numeric_cast.h>
#include <wrparse/cxx/CXXLexer.h>
#include <wrparse/cxx/CXXParser.h>
#include <wrparse/cxx/CXXTokenKinds.h>


namespace wr {
namespace parse {


using namespace cxx;


WRPARSECXX_API
CXXParser::CXXParser(
        const CXXOptions &options
) :
        options_(options),

        /*
         * A.1 Keywords [gram.key]
         */
        typedef_name { "typedef-name", {
                { pred(identifier, &isTypedefName) }
        }},

        class_name { "class-name", {
                { pred(identifier, &isClassName) },
                {{ simple_template_id }, langCXX() }
        }},

        enum_name { "enum-name", {
                { pred(identifier, &isEnumName) },
        }},

        namespace_name { "namespace-name", langCXX(), {
                { original_namespace_name },
                { namespace_alias }
        }},

        original_namespace_name { "original-namespace-name", langCXX(), {
                { pred(identifier, &isNamespaceName) }
        }},

        namespace_alias { "namespace-alias", langCXX(), {
                { pred(identifier, &isNamespaceAliasName) }
        }},

        template_name { "template-name", langCXX(), {
                { pred(identifier, &isTemplateName) },
        }},

        undeclared_name { "undeclared-name", {
                { pred(identifier, &isUndeclaredName) }
        }},

        /*--------------------------------------
         * A.2 Lexical conventions [gram.lex]
         * NB: many of the elements are handled by the lexer
         */
        identifier { "identifier", {
                { TOK_IDENTIFIER }
        }},

        literal { "literal", {
                { numeric_literal },
                { character_literal },
                { string_literal },
                {{ boolean_literal }, langCXX() },
                {{ pointer_literal }, stdCXX11() },
                {{ user_defined_literal }, stdCXX11() },
        }},

        boolean_literal { "boolean-literal", langCXX(), {
                { TOK_KW_FALSE },
                { TOK_KW_TRUE }
        }},

        pointer_literal { "pointer-literal", stdCXX11(), {
                { TOK_KW_NULLPTR }
        }},

        user_defined_literal { "user-defined-literal", stdCXX11(), {
                { numeric_literal, ud_suffix },
                { character_literal, ud_suffix },
                { string_literal, ud_suffix }
        }},

        ud_suffix { "ud-suffix", stdCXX11(), {
                { identifier }
        }},

        numeric_literal { "numeric-literal", {
                { TOK_DEC_INT_LITERAL },
                { TOK_HEX_INT_LITERAL },
                { TOK_OCT_INT_LITERAL },
                {{ TOK_BIN_INT_LITERAL }, options.have(cxx::BINARY_LITERALS) },
                { TOK_FLOAT_LITERAL }
        }},

        character_literal { "character-literal", {
                { TOK_CHAR_LITERAL },
                { TOK_WCHAR_LITERAL },
                {{ TOK_U8_CHAR_LITERAL },
                        options.have(cxx::UTF8_CHAR_LITERALS) },
                {{ TOK_U16_CHAR_LITERAL }, stdC11() || stdCXX11() },
                {{ TOK_U32_CHAR_LITERAL }, stdC11() || stdCXX11() }
        }},

        string_literal { "string-literal", {
                { TOK_STR_LITERAL },
                { TOK_WSTR_LITERAL },
                {{ TOK_U8_STR_LITERAL }, stdC11() || stdCXX11() },
                {{ TOK_U16_STR_LITERAL }, stdC11() || stdCXX11() },
                {{ TOK_U32_STR_LITERAL }, stdC11() || stdCXX11() }
        }},

        /*--------------------------------------
         * A.3 Basic concepts [gram.basic]
         */
        translation_unit { "translation-unit", {
                { opt(declaration_seq) }
        }},

        /*--------------------------------------
         * A.4 Expressions [gram.expr]
         */
        primary_expression { "primary-expression", {
                { literal },
                { paren_expression },  // moved into distinct nonterminals below
                { id_expression },
                {{ TOK_KW_THIS }, langCXX() },
                {{ lambda_expression }, stdCXX11() },
                {{ generic_selection }, stdC11() }
        }, NonTerminal::HIDE_IF_DELEGATE },

        generic_selection { "generic-selection", stdC11(), {
                { TOK_KW_GENERIC, TOK_LPAREN, assignment_expression,
                        TOK_COMMA, generic_assoc_list, TOK_RPAREN },
        }},

        generic_assoc_list { "generic-assoc-list", stdC11(), {
                { generic_association },
                { generic_assoc_list, TOK_COMMA, generic_association },
        }, NonTerminal::TRANSPARENT },

        generic_association { "generic-association", stdC11(), {
                { TOK_KW_DEFAULT, TOK_COLON, assignment_expression },
                { type_id, TOK_COLON, assignment_expression },
        }},

        paren_expression { "paren-expression", {
                { TOK_LPAREN, expression, TOK_RPAREN }
        }},

        id_expression { "id-expression", {
                { unqualified_id },
                {{ qualified_id }, langCXX() }
        }},

        unqualified_id { "unqualified-id", {
                { identifier },
                {{ operator_function_id }, langCXX() },  // e.g. "operator="
                {{ conversion_function_id }, langCXX() }, // e.g. "operator int"
                {{ literal_operator_id }, stdCXX11() },  // operator""
                // destructor IDs moved to separate nonterminals */
                {{ destructor_id }, langCXX() },
                {{ template_id }, langCXX() }
        }},

        postfix_expression { "postfix-expression", {
                { primary_expression },
                { postfix_expression, array_subscript },
                { postfix_expression, function_call },
                { postfix_expression, member_access },

                // C++ function-style type casting / object construction
                {{ simple_type_specifier, TOK_LPAREN,
                        opt(expression_list), TOK_RPAREN }, langCXX() },
                {{ typename_specifier, TOK_LPAREN, opt(expression_list),
                        TOK_RPAREN }, langCXX() },

                // C++11 uniform initialisation
                {{ simple_type_specifier, braced_init_list }, stdCXX11() },
                {{ typename_specifier, braced_init_list }, stdCXX11() },

                // post-increment and decrement
                { postfix_expression, TOK_PLUSPLUS },
                { postfix_expression, TOK_MINUSMINUS },

                // C++ dynamic_/static_/reinterpret_/const_cast<T>(x)
                {{ TOK_KW_DYNAMIC_CAST, TOK_LESS, type_id, TOK_GREATER,
                        TOK_LPAREN, expression, TOK_RPAREN }, langCXX() },
                {{ TOK_KW_STATIC_CAST, TOK_LESS, type_id, TOK_GREATER,
                        TOK_LPAREN, expression, TOK_RPAREN }, langCXX() },
                {{ TOK_KW_REINTERPRET_CAST, TOK_LESS, type_id,
                        TOK_GREATER, TOK_LPAREN, expression,
                        TOK_RPAREN }, langCXX() },
                {{ TOK_KW_CONST_CAST, TOK_LESS, type_id, TOK_GREATER,
                        TOK_LPAREN, expression, TOK_RPAREN }, langCXX() },

                // C++ typeid(x) and typeid(T)
                {{ TOK_KW_TYPEID, TOK_LPAREN, expression,
                        TOK_RPAREN }, langCXX() },
                {{ TOK_KW_TYPEID, TOK_LPAREN, type_id,
                        TOK_RPAREN }, langCXX() },

                // C99 compound literal
                {{ TOK_LPAREN, type_id, TOK_RPAREN, TOK_LBRACE,
                        initializer_list, opt(TOK_COMMA),
                        TOK_RBRACE }, stdC99() }
        }, NonTerminal::HIDE_IF_DELEGATE },

        array_subscript { "array-subscript", {
                { TOK_LSQUARE, expression, TOK_RSQUARE },
                {{ TOK_LSQUARE, braced_init_list, TOK_RSQUARE }, stdCXX11() }
        }},

        function_call { "function-call", {
                { TOK_LPAREN, opt(expression_list), TOK_RPAREN }
        }},

        member_access { "member-access", {
                { TOK_DOT, opt(TOK_KW_TEMPLATE), id_expression },
                {{ TOK_DOT, pseudo_destructor_name }, langCXX() },
                { TOK_ARROW, opt(TOK_KW_TEMPLATE), id_expression },
                {{ TOK_ARROW, pseudo_destructor_name }, langCXX() },
        }},

        expression_list { "expression-list", {
                { initializer_list }
        }},

        unary_expression { "unary-expression", {
                { postfix_expression },

                // pre-increment and decrement
                { TOK_PLUSPLUS, cast_expression },
                { TOK_MINUSMINUS, cast_expression },

                { unary_operator, cast_expression },

                // sizeof(expr) and sizeof(type)
                { TOK_KW_SIZEOF, unary_expression },
                { TOK_KW_SIZEOF, TOK_LPAREN, type_id, TOK_RPAREN },

                // C++11 sizeof template parameter pack
                {{ TOK_KW_SIZEOF, TOK_ELLIPSIS, TOK_LPAREN, identifier,
                        TOK_RPAREN }, stdCXX11() },

                {{ TOK_KW_ALIGNOF, TOK_LPAREN, type_id,
                        TOK_RPAREN }, stdC11() || stdCXX11() },

                {{ noexcept_expression }, stdCXX11() },
                {{ new_expression }, langCXX() },
                {{ delete_expression }, langCXX() }
        }, NonTerminal::HIDE_IF_DELEGATE },

        unary_operator { "unary-operator", {
                { TOK_STAR },
                { TOK_AMP },
                { TOK_PLUS },
                { TOK_MINUS },
                { TOK_EXCLAIM },
                { TOK_TILDE }
        }},

        qualified_id { "qualified-id", langCXX(), {
                { nested_name_specifier, opt(TOK_KW_TEMPLATE), unqualified_id }
        }},

        nested_name_specifier { "nested-name-specifier", langCXX(), {
                { TOK_COLONCOLON },
                { type_name, TOK_COLONCOLON },
                { namespace_name, TOK_COLONCOLON },
                { decltype_specifier, TOK_COLONCOLON },
                { nested_name_specifier, identifier, TOK_COLONCOLON },
                { nested_name_specifier, opt(TOK_KW_TEMPLATE),
                        simple_template_id, TOK_COLONCOLON }
        }},

        pseudo_destructor_name { "pseudo-destructor-name", langCXX(), {
                { opt(nested_name_specifier), type_name, TOK_COLONCOLON,
                        TOK_TILDE, type_name },
                { nested_name_specifier, TOK_KW_TEMPLATE, simple_template_id,
                        TOK_COLONCOLON, TOK_TILDE, type_name },
                { opt(nested_name_specifier), TOK_TILDE, type_name },
                {{ TOK_TILDE, decltype_specifier }, stdCXX11() }
        }},

        new_expression { "new-expression", langCXX(), {
                { opt(TOK_COLONCOLON), TOK_KW_NEW, opt(new_placement),
                        new_type_id, opt(new_initializer) },
                { opt(TOK_COLONCOLON), TOK_KW_NEW, opt(new_placement),
                        TOK_LPAREN, type_id, TOK_RPAREN, opt(new_initializer) }
        }},

        new_placement { "new-placement", langCXX(), {
                { TOK_LPAREN, expression_list, TOK_RPAREN }
        }},

        new_type_id { "new-type-id", langCXX(), {
                { type_specifier_seq, opt(new_declarator) }
        }},

        new_declarator { "new-declarator", langCXX(), {
                { ptr_operator, opt(new_declarator) },
                { noptr_new_declarator }
        }},

        noptr_new_declarator { "noptr-new-declarator", langCXX(), {
                { TOK_LSQUARE, expression, TOK_RSQUARE,
                        opt(attribute_specifier_seq) },
                { noptr_new_declarator, TOK_LSQUARE, constant_expression,
                        TOK_RSQUARE, opt(attribute_specifier_seq) }
        }},

        new_initializer { "new-initializer", langCXX(), {
                { TOK_LPAREN, opt(expression_list), TOK_RPAREN },
                { braced_init_list }
        }},

        delete_expression { "delete-expression", langCXX(), {
                { opt(TOK_COLONCOLON), TOK_KW_DELETE, cast_expression },
                { opt(TOK_COLONCOLON), TOK_KW_DELETE, TOK_LSQUARE, TOK_RSQUARE,
                        cast_expression }
        }},

        lambda_expression { "lambda-expression", stdCXX11(), {
                { lambda_introducer, opt(lambda_declarator),
                        compound_statement }
        }},

        lambda_introducer { "lambda-introducer", stdCXX11(), {
                { TOK_LSQUARE, opt(lambda_capture), TOK_RSQUARE }
        }},

        lambda_capture { "lambda-capture", stdCXX11(), {
                { capture_default },
                { capture_list },
                { capture_default, TOK_COMMA, capture_list }
        }},

        capture_default { "capture-default", stdCXX11(), {
                { TOK_AMP },
                { TOK_EQUAL }
        }},

        capture_list { "capture-list", stdCXX11(), {
                { capture, opt(TOK_ELLIPSIS) },
                { capture_list, TOK_COMMA, capture, opt(TOK_ELLIPSIS) }
        }, NonTerminal::TRANSPARENT },

        capture { "capture", stdCXX11(), {
                { simple_capture },
                { init_capture }
        }},

        simple_capture { "simple-capture", stdCXX11(), {
                { identifier },
                { TOK_AMP, identifier },
                { TOK_KW_THIS }
        }},

        init_capture { "init-capture", stdCXX11(), {
                { identifier, initializer },
                { TOK_AMP, identifier, initializer }
        }},

        lambda_declarator { "lambda-declarator", stdCXX11(), {
                { TOK_LPAREN, parameter_declaration_clause, TOK_RPAREN,
                       opt(TOK_KW_MUTABLE), opt(exception_specification),
                       opt(attribute_specifier_seq), opt(trailing_return_type) }
        }},

        noexcept_expression { "noexcept-expression", stdCXX11(), {
                { TOK_KW_NOEXCEPT, TOK_LBRACE, expression, TOK_RBRACE }
        }},

        cast_expression { "cast-expression", {
                { unary_expression },
                { TOK_LPAREN, type_id, TOK_RPAREN, cast_expression }
        }, NonTerminal::HIDE_IF_DELEGATE },

        pm_expression { "pm-expression", {
                { cast_expression },
                {{ pm_expression, TOK_DOTSTAR, cast_expression }, langCXX() },
                {{ pm_expression, TOK_ARROWSTAR, cast_expression }, langCXX() },
        }, NonTerminal::HIDE_IF_DELEGATE },

        multiplicative_expression { "multiplicative-expression", {
                { pm_expression },
                {{ multiplicative_expression, TOK_STAR, pm_expression },
                        multiply },
                {{ multiplicative_expression, TOK_SLASH, pm_expression },
                        divide },
                {{ multiplicative_expression, TOK_PERCENT, pm_expression },
                        modulo },
        }, NonTerminal::HIDE_IF_DELEGATE },

        additive_expression { "additive-expression", {
                { multiplicative_expression },
                {{ additive_expression, TOK_PLUS, multiplicative_expression },
                        binary_add },
                {{ additive_expression, TOK_MINUS, multiplicative_expression },
                        binary_subtract },
        }, NonTerminal::HIDE_IF_DELEGATE },

        shift_expression { "shift-expression", {
                { additive_expression },
                {{ shift_expression, TOK_LSHIFT, additive_expression },
                        left_shift },
                {{ shift_expression, TOK_RSHIFT, additive_expression },
                        right_shift },
        }, NonTerminal::HIDE_IF_DELEGATE },

        relational_expression { "relational-expression", {
                { shift_expression },
                {{ relational_expression, TOK_LESS, shift_expression }, less },
                {{ relational_expression, TOK_GREATER, shift_expression },
                        greater },
                {{ relational_expression, TOK_LESSEQUAL, shift_expression },
                        less_or_equal },
                {{ relational_expression, TOK_GREATEREQUAL, shift_expression },
                        greater_or_equal }
        }, NonTerminal::HIDE_IF_DELEGATE },

        equality_expression { "equality-expression", {
                { relational_expression },
                {{ equality_expression, TOK_EQUALEQUAL, relational_expression },
                        equal },
                {{ equality_expression, TOK_EXCLAIMEQUAL,
                        relational_expression }, not_equal },
        }, NonTerminal::HIDE_IF_DELEGATE },

        and_expression { "and-expression", {
                { equality_expression },
                { and_expression, TOK_AMP, equality_expression }
        }, NonTerminal::HIDE_IF_DELEGATE },

        exclusive_or_expression { "exclusive-or-expression", {
                { and_expression },
                { exclusive_or_expression, TOK_CARET, and_expression }
        }, NonTerminal::HIDE_IF_DELEGATE },

        inclusive_or_expression { "inclusive-or-expression", {
                { exclusive_or_expression },
                { inclusive_or_expression, TOK_PIPE, exclusive_or_expression }
        }, NonTerminal::HIDE_IF_DELEGATE },

        logical_and_expression { "logical-and-expression", {
                { inclusive_or_expression },
                { logical_and_expression, TOK_AMPAMP, inclusive_or_expression }
        }, NonTerminal::HIDE_IF_DELEGATE },

        logical_or_expression { "logical-or-expression", {
                { logical_and_expression },
                { logical_or_expression, TOK_PIPEPIPE, logical_and_expression }
        }, NonTerminal::HIDE_IF_DELEGATE },

        conditional_expression { "conditional-expression", {
                { logical_or_expression },
                {{ logical_or_expression, TOK_QUESTION, expression, TOK_COLON,
                        assignment_expression }, langCXX() },
                {{ logical_or_expression, TOK_QUESTION, expression, TOK_COLON,
                        conditional_expression }, !langCXX() }
        }, NonTerminal::HIDE_IF_DELEGATE },

        assignment_expression { "assignment-expression", {
                { conditional_expression },
                {{ logical_or_expression, assignment_operator,
                        initializer_clause }, langCXX() },
                {{ throw_expression }, langCXX() },
                {{ unary_expression, assignment_operator,
                        assignment_expression }, !langCXX() }
        }, NonTerminal::HIDE_IF_DELEGATE },

        assignment_operator { "assignment-operator", {
                { TOK_EQUAL },
                { TOK_STAREQUAL },
                { TOK_SLASHEQUAL },
                { TOK_PERCENTEQUAL },
                { TOK_PLUSEQUAL },
                { TOK_MINUSEQUAL },
                { TOK_RSHIFTEQUAL },
                { TOK_LSHIFTEQUAL },
                { TOK_AMPEQUAL },
                { TOK_CARETEQUAL },
                { TOK_PIPEEQUAL }
        }},

        expression { "expression", {
                { assignment_expression },
                { expression, TOK_COMMA, assignment_expression }
        }},

        constant_expression { "constant-expression", {
                { conditional_expression }
        }},

        /*--------------------------------------
         * A.5 Statements [gram.stmt]
         */
        statement { "statement", {
                { labeled_statement },
                { opt(attribute_specifier_seq), expression_statement },
                { opt(attribute_specifier_seq), compound_statement },
                { opt(attribute_specifier_seq), selection_statement },
                { opt(attribute_specifier_seq), iteration_statement },
                { opt(attribute_specifier_seq), jump_statement },
                {{ declaration_statement }, langCXX() || stdC99() },
                                // intermixing of declarations with statements
                {{ opt(attribute_specifier_seq), try_block }, langCXX() }
        }},

        labeled_statement { "labeled-statement", {
                { opt(attribute_specifier_seq), identifier, TOK_COLON,
                        statement },
                { opt(attribute_specifier_seq), TOK_KW_CASE,
                        constant_expression, TOK_COLON, statement },
                { opt(attribute_specifier_seq), TOK_KW_DEFAULT, TOK_COLON,
                        statement }
        }},

        expression_statement { "expression-statement", {
                { opt(expression), TOK_SEMI }
        }},

        compound_statement { "compound-statement", {
                {{ TOK_LBRACE, opt(statement_seq),
                        TOK_RBRACE }, langCXX() || stdC99() },
                {{ TOK_LBRACE, opt(block_declaration_seq), opt(statement_seq),
                        TOK_RBRACE }, !langCXX() && !stdC99() }
                                // pre-C99: declarations at top of block only
        }},

        block_declaration_seq { "block-declaration-seq",
                                  !langCXX() && !stdC99(), {
                { block_declaration },
                { block_declaration_seq, block_declaration }
        }, NonTerminal::TRANSPARENT },

        statement_seq { "statement-seq", {
                { statement },
                { statement_seq, statement }
        }, NonTerminal::TRANSPARENT },

        selection_statement { "selection-statement", {
                { TOK_KW_IF, TOK_LPAREN, condition, TOK_RPAREN, statement },
                { TOK_KW_IF, TOK_LPAREN, condition, TOK_RPAREN, statement,
                        TOK_KW_ELSE, statement },
                { TOK_KW_SWITCH, TOK_LPAREN, condition, TOK_RPAREN, statement }
        }},

        condition { "condition", {
                { expression },

                /*
                 * C++: variable decls inside if/for/while/switch condition
                 */
                // pre-C++11
                {{ decl_specifier_seq, declarator, TOK_EQUAL,
                        assignment_expression }, langCXX() && !stdCXX11() },

                // C++11 uniform initialisation
                {{ opt(attribute_specifier_seq), decl_specifier_seq, declarator,
                        TOK_EQUAL, initializer_clause }, stdCXX11() },
                {{ opt(attribute_specifier_seq), decl_specifier_seq, declarator,
                        braced_init_list }, stdCXX11() }
        }},

        iteration_statement { "iteration-statement", {
                // while ...
                { TOK_KW_WHILE, TOK_LPAREN, condition, TOK_RPAREN, statement },

                // do ... while
                { TOK_KW_DO, statement, TOK_KW_WHILE, TOK_LPAREN, expression,
                        TOK_RPAREN, TOK_SEMI },

                // original C/C++-style for (;;)
                { TOK_KW_FOR, TOK_LPAREN, for_init_statement, opt(condition),
                        TOK_SEMI, opt(expression), TOK_RPAREN, statement },

                // C++11 range-based for
                {{ TOK_KW_FOR, TOK_LPAREN, for_range_declaration, TOK_COLON,
                        for_range_initializer, TOK_RPAREN,
                        statement }, stdCXX11() }
        }},

        for_init_statement { "for-init-statement", {
                { expression_statement },

                // C++/C99: enable variable declarations in 'for' statements
                {{ simple_declaration }, langCXX() || stdC99() }
        }},

        for_range_declaration { "for-range-declaration", {
                { opt(attribute_specifier_seq), decl_specifier_seq, declarator }
        }},

        for_range_initializer { "for-range-initializer", {
                { expression },
                { braced_init_list }
        }},

        jump_statement { "jump-statement", {
                { TOK_KW_BREAK, TOK_SEMI },
                { TOK_KW_CONTINUE, TOK_SEMI },
                { TOK_KW_GOTO, identifier, TOK_SEMI },
                { TOK_KW_RETURN, opt(expression), TOK_SEMI },
                {{ TOK_KW_RETURN, braced_init_list, TOK_SEMI }, stdCXX11() }
        }},

        declaration_statement { "declaration-statement", {
                { block_declaration }
        }},

        /*--------------------------------------
         * A.6 Declarations [gram.dcl]
         */
        declaration_seq { "declaration-seq", {
                { declaration },
                { declaration_seq, declaration }
        }, NonTerminal::TRANSPARENT },

        declaration { "declaration", {
                { block_declaration },
                { function_definition },
                { empty_declaration },
                {{ template_declaration }, langCXX() },
                {{ explicit_instantiation }, langCXX() },
                {{ explicit_specialization }, langCXX() },
                {{ linkage_specification }, langCXX() },
                {{ namespace_definition }, langCXX() },
                {{ attribute_declaration }, stdCXX11() }
        }},

        block_declaration { "block-declaration", {
                { simple_declaration },
                { asm_definition },
                {{ static_assert_declaration }, stdC11() || stdCXX11() },
                {{ namespace_alias_definition }, langCXX() },
                {{ using_declaration }, langCXX() },
                {{ using_directive }, langCXX() },
                {{ alias_declaration }, stdCXX11() },
                {{ opaque_enum_declaration }, stdCXX11() }
        }},

        /* split simple_declaration into 4 rules, making decl_specifier_seq
           mandatory in two of them (reason: the declarator-id's of constructor
           declarations are otherwise mistaken for a decl-specifier-seq) */
        simple_declaration { "simple-declaration", {
                { decl_specifier_seq, opt(init_declarator_list), TOK_SEMI },

                // C++ constructors and pre-C99 implicit int functions
                {{ init_declarator_list, TOK_SEMI }, langCXX() || !stdC99() },

                {{ attribute_specifier_seq, decl_specifier_seq,
                        init_declarator_list, TOK_SEMI }, stdCXX11() },
                {{ attribute_specifier_seq, init_declarator_list,
                        TOK_SEMI }, stdCXX11() },
        }},

        static_assert_declaration { "static_assert-declaration",
                                      stdC11() || stdCXX11(), {
                { TOK_KW_STATIC_ASSERT, TOK_LPAREN, constant_expression,
                       TOK_COMMA, string_literal, TOK_RPAREN, TOK_SEMI }
        }},

        empty_declaration { "empty-declaration", {{ TOK_SEMI }} },

        decl_specifier { "decl-specifier", {
                { storage_class_specifier },
                { type_specifier },
                { function_specifier },
                { TOK_KW_TYPEDEF },
                {{ TOK_KW_FRIEND }, langCXX() },
                {{ TOK_KW_CONSTEXPR }, stdCXX11() },
                {{ alignment_specifier }, stdC11() }
                        /* NB: alignment-specifier is parsed via
                           attribute-specifier in C++11 */
        }},

        decl_specifier_seq { "decl-specifier-seq", {
                { decl_specifier, opt(attribute_specifier_seq) },
                { decl_specifier, decl_specifier_seq }
        }},

        storage_class_specifier { "storage-class-specifier", {
                { TOK_KW_REGISTER },
                { TOK_KW_STATIC },
                { TOK_KW_THREAD_LOCAL },
                { TOK_KW_EXTERN },
                { TOK_KW_MUTABLE },
                {{ TOK_KW_AUTO }, langC() && !stdCXX11() }
                                /* 'auto' has different meaning in C++11;
                                   see simple-type-specifier */
        }},

        function_specifier { "function-specifier", {
                {{ TOK_KW_INLINE }, options.have(cxx::INLINE_FUNCTIONS) },
                {{ TOK_KW_VIRTUAL }, langCXX() },
                {{ TOK_KW_EXPLICIT }, langCXX() },
                {{ TOK_KW_NORETURN }, stdC11() }
        }},

        // typedef-name: see section A.1 Keywords [gram.key]

        type_specifier { "type-specifier", {
                { trailing_type_specifier },
                { class_specifier },
                { enum_specifier }
        }},

        trailing_type_specifier { "trailing-type-specifier", {
                { simple_type_specifier },
                { elaborated_type_specifier },
                { type_qualifier },
                {{ typename_specifier }, stdCXX11() },
                {{ atomic_type_specifier }, stdC11() }
        }, NonTerminal::TRANSPARENT },

        type_specifier_seq { "type-specifier-seq", {
                { type_specifier, opt(attribute_specifier_seq) },
                { type_specifier, type_specifier_seq }
        }},

        trailing_type_specifier_seq { "trailing-type-specifier-seq", {
                { trailing_type_specifier, opt(attribute_specifier_seq) },
                { trailing_type_specifier, trailing_type_specifier_seq }
        }},

        simple_type_specifier { "simple-type-specifier", {
                // user-defined type specifiers moved into separate nonterminal
                {{ ud_type_specifier }, langCXX() },
                {{ typedef_name }, !langCXX() },
                {{ undeclared_name }, !langCXX() },
                { TOK_KW_CHAR },
                { TOK_KW_WCHAR_T },
                {{ TOK_KW_CHAR16_T }, stdC11() || stdCXX11() },
                {{ TOK_KW_CHAR32_T }, stdC11() || stdCXX11() },
                { TOK_KW_SIGNED },
                { TOK_KW_UNSIGNED },
                { TOK_KW_FLOAT },
                { TOK_KW_DOUBLE },
                { TOK_KW_VOID },
                { TOK_KW_SHORT },
                { TOK_KW_INT },
                { TOK_KW_LONG },
                // parse "long long" separately
                {{ TOK_KW_LONG, TOK_KW_LONG }, options.have(cxx::LONG_LONG) },
                {{ TOK_KW_BOOL }, langCXX() || stdC99() },
                {{ TOK_KW_AUTO }, stdCXX11() },
                {{ decltype_specifier }, stdCXX11() },
                {{ TOK_KW_COMPLEX }, stdC99() }
        }},

        ud_type_specifier { "ud-type-specifier", langCXX(), {
                { opt(nested_name_specifier), type_name },
                { nested_name_specifier, TOK_KW_TEMPLATE,
                        simple_template_id }
        }},

        type_name { "type-name", langCXX(), {  // C: see type_id
                { class_name },
                { enum_name },
                { typedef_name },
                { undeclared_name },
                { simple_template_id }
        }},

        elaborated_type_specifier { "elaborated-type-specifier", {
                { class_key, opt(attribute_specifier_seq),
                        opt(nested_name_specifier), identifier },
                {{ class_key, opt(nested_name_specifier),
                        TOK_KW_TEMPLATE, simple_template_id }, langCXX() },
                { TOK_KW_ENUM, opt(nested_name_specifier), identifier }
        }},

        atomic_type_specifier { "atomic-type-specifier", stdC11(), {
                { TOK_KW_ATOMIC, TOK_LPAREN, type_id, TOK_RPAREN }
        }},

        // enum-name: see section A.1 Keywords [gram.key]

        enum_specifier { "enum-specifier", {
                { enum_head, TOK_LBRACE, opt(enumerator_list), TOK_RBRACE },
                { enum_head, TOK_LBRACE, enumerator_list, TOK_COMMA,
                        TOK_RBRACE },
                {{ enum_head }, langC() }
        }},

        enum_head { "enum-head", {
                { enum_key, opt(attribute_specifier_seq), opt(identifier),
                        opt(enum_base) },
                {{ enum_key, opt(attribute_specifier_seq),
                        nested_name_specifier, identifier,
                        opt(enum_base) }, langCXX() }
        }},

        enum_key { "enum-key", {
                { TOK_KW_ENUM },

                // C++11 scoped enums
                {{ TOK_KW_ENUM, TOK_KW_CLASS }, stdCXX11() },
                {{ TOK_KW_ENUM, TOK_KW_STRUCT }, stdCXX11() }
        }},

        enumerator_list { "enumerator-list", {
                { enumerator_definition },
                { enumerator_list, TOK_COMMA, enumerator_definition }
        }, NonTerminal::TRANSPARENT },

        enumerator_definition { "enumerator-definition", {
                { enumerator },
                { enumerator, TOK_EQUAL, constant_expression }
        }},

        enumerator { "enumerator", {
                { identifier }
        }},

        // namespace-name:
        // original-namespace-name: see section A.1 Keywords [gram.key]

        namespace_definition { "namespace-definition", langCXX(), {
                { named_namespace_definition },
                { unnamed_namespace_definition }
        }},

        named_namespace_definition { "named-namespace-definition", langCXX(), {
                { original_namespace_definition },
                { extension_namespace_definition }
        }},

        original_namespace_definition { "original-namespace-definition",
                                          langCXX(), {
                { opt(TOK_KW_INLINE), TOK_KW_NAMESPACE, undeclared_name,
                        TOK_LBRACE, namespace_body, TOK_RBRACE }
        }},

        extension_namespace_definition { "extension-namespace-definition",
                                           langCXX(), {
                { opt(TOK_KW_INLINE), TOK_KW_NAMESPACE, original_namespace_name,
                        TOK_LBRACE, namespace_body, TOK_RBRACE }
        }},

        unnamed_namespace_definition { "unnamed-namespace-definition",
                                         langCXX(), {
                { opt(TOK_KW_INLINE), TOK_KW_NAMESPACE,
                        TOK_LBRACE, namespace_body, TOK_RBRACE }
        }},

        namespace_body { "namespace-body", langCXX(), {
                { opt(declaration_seq) }
        }},

        // namespace-alias: see section A.1 Keywords [gram.key]

        namespace_alias_definition { "namespace-alias-definition", langCXX(), {
                { TOK_KW_NAMESPACE, identifier, TOK_EQUAL,
                        qualified_namespace_specifier, TOK_SEMI }
        }},

        qualified_namespace_specifier { "qualified-namespace-specifier",
                                          langCXX(), {
                { opt(nested_name_specifier), namespace_name },
                { opt(nested_name_specifier), undeclared_name }
        }},

        using_declaration { "using-declaration", langCXX(), {
                { TOK_KW_USING, opt(TOK_KW_TYPENAME), nested_name_specifier,
                        unqualified_id, TOK_SEMI },
                { TOK_KW_USING, TOK_COLONCOLON, unqualified_id, TOK_SEMI }
        }},

        using_directive { "using-directive", langCXX(), {
                { opt(attribute_specifier_seq), TOK_KW_USING, TOK_KW_NAMESPACE,
                        opt(nested_name_specifier), namespace_name, TOK_SEMI },
                { opt(attribute_specifier_seq), TOK_KW_USING, TOK_KW_NAMESPACE,
                        opt(nested_name_specifier), undeclared_name, TOK_SEMI }
        }},

        linkage_specification { "linkage-specification", langCXX(), {
                { TOK_KW_EXTERN, string_literal, TOK_LBRACE,
                        opt(declaration_seq), TOK_RBRACE },
                { TOK_KW_EXTERN, string_literal, declaration }
        }},

        asm_definition { "asm-definition", {
                { TOK_KW_ASM, TOK_LPAREN, string_literal, TOK_RPAREN, TOK_SEMI }
        }},

        alignment_specifier { "alignment-specifier", stdC11() || stdCXX11(), {
                { TOK_KW_ALIGNAS, TOK_LPAREN, type_id, opt(TOK_ELLIPSIS),
                        TOK_RPAREN },
                { TOK_KW_ALIGNAS, TOK_LPAREN, assignment_expression,
                        opt(TOK_ELLIPSIS), TOK_RPAREN }
        }},

        decltype_specifier { "decltype-specifier", stdCXX11(), {
                { TOK_KW_DECLTYPE, TOK_LPAREN, expression, TOK_RPAREN },
                { TOK_KW_DECLTYPE, TOK_LPAREN, TOK_KW_AUTO, TOK_RPAREN }
        }},

        opaque_enum_declaration { "opaque-enum-declaration", stdCXX11(), {
                { enum_key, opt(attribute_specifier_seq), identifier,
                        opt(enum_base), TOK_SEMI }
        }},

        enum_base { "enum-base", stdCXX11(), {
                { TOK_COLON, type_specifier_seq }
        }},

        alias_declaration { "alias-declaration", stdCXX11(), {
                { TOK_KW_USING, identifier, opt(attribute_specifier_seq),
                        TOK_EQUAL, type_id, TOK_SEMI }
        }},

        attribute_declaration { "attribute-declaration", stdCXX11(), {
                { attribute_specifier_seq, TOK_SEMI }
        }},

        attribute_specifier_seq { "attribute-specifier-seq", stdCXX11(), {
                { attribute_specifier_seq, attribute_specifier },
                { attribute_specifier }
        }},

        attribute_specifier { "attribute-specifier", stdCXX11(), {
                { TOK_LSQUARE, TOK_LSQUARE, attribute_list, TOK_RSQUARE,
                        TOK_RSQUARE },
                { alignment_specifier }
        }},

        attribute_list { "attribute-list", stdCXX11(), {
                { opt(attribute) },
                { attribute_list, TOK_COMMA, opt(attribute) },
                { attribute, TOK_ELLIPSIS },
                { attribute_list, TOK_COMMA, attribute, TOK_ELLIPSIS }
        }},

        attribute { "attribute", stdCXX11(), {
                { attribute_token, opt(attribute_argument_clause) }
        }},

        attribute_token { "attribute-token", stdCXX11(), {
                { identifier },
                { attribute_scoped_token }
        }},

        attribute_scoped_token { "attribute-scoped-token", stdCXX11(), {
                { attribute_namespace, TOK_COLONCOLON, identifier }
        }},

        attribute_namespace { "attribute-namespace", stdCXX11(), {
                { identifier }
        }},

        attribute_argument_clause { "attribute_argument_clause", stdCXX11(), {
                { TOK_LPAREN, balanced_token_seq, TOK_RPAREN }
        }},

        balanced_token_seq { "balanced-token-seq", stdCXX11(), {
                { opt(balanced_token) },
                { balanced_token_seq, balanced_token }
        }, NonTerminal::TRANSPARENT },

        balanced_token { "balanced-token", stdCXX11(), {
                { TOK_LPAREN, balanced_token_seq, TOK_RPAREN },
                { TOK_LSQUARE, balanced_token_seq, TOK_RSQUARE },
                { TOK_LBRACE, balanced_token_seq, TOK_RBRACE },
                { &isBalancedToken }
        }},

        /*--------------------------------------
         * A.7 Declarators [gram.decl]
         */
        init_declarator_list { "init-declarator-list", {
                { init_declarator },
                { init_declarator_list, TOK_COMMA, init_declarator }
        }},

        init_declarator { "init-declarator", {
                { declarator, opt(initializer) }
        }},

        declarator { "declarator", {
                { ptr_declarator },
                /* deviation from original C++11 grammar:
                   parameters-and-qualifiers only parsed via noptr-declarator */
                {{ noptr_declarator, pred(trailing_return_type,
                                          &Declarator::isFunction) },
                        stdCXX11() }
        }},

        ptr_declarator { "ptr-declarator", {
                { noptr_declarator },
                { ptr_operator, ptr_declarator }
        }, NonTerminal::TRANSPARENT },

        noptr_declarator { "noptr-declarator", {
                { declarator_id, opt(attribute_specifier_seq) },
                { noptr_declarator, parameters_and_qualifiers },
                { noptr_declarator, array_declarator },
                { nested_declarator }
        }, NonTerminal::TRANSPARENT },

        nested_declarator { "nested-declarator", {
                { TOK_LPAREN, ptr_declarator, TOK_RPAREN }
        }},

        array_declarator { "array-declarator", {
                {{ TOK_LSQUARE, opt(constant_expression), TOK_RSQUARE,
                        opt(attribute_specifier_seq) }, !stdC99() },
                {{ TOK_LSQUARE, opt(type_qualifier_seq),
                        opt(assignment_expression), TOK_RSQUARE,
                        opt(attribute_specifier_seq) }, stdC99() },
                {{ TOK_LSQUARE, TOK_KW_STATIC, opt(type_qualifier_seq),
                        assignment_expression, TOK_RSQUARE,
                        opt(attribute_specifier_seq) }, stdC99() },
                {{ TOK_LSQUARE, type_qualifier_seq, TOK_KW_STATIC,
                        assignment_expression, TOK_RSQUARE,
                        opt(attribute_specifier_seq) }, stdC99() },
                {{ TOK_LSQUARE, opt(type_qualifier_seq), TOK_STAR,
                        TOK_RSQUARE, opt(attribute_specifier_seq) }, stdC99() },
        }},

        parameters_and_qualifiers { "parameters-and-qualifiers", {
                {{ TOK_LPAREN, parameter_declaration_clause, TOK_RPAREN,
                        opt(type_qualifier_seq), opt(ref_qualifier),
                        opt(exception_specification),
                        opt(attribute_specifier_seq) }, langCXX() },
                {{ TOK_LPAREN, parameter_declaration_clause,
                        TOK_RPAREN }, !langCXX() }
        }},

        ptr_operator { "ptr-operator", {
                { TOK_STAR, opt(attribute_specifier_seq),
                        opt(type_qualifier_seq) },
                {{ TOK_AMP, opt(attribute_specifier_seq) }, langCXX() },
                                                // C++ lvalue reference
                {{ TOK_AMPAMP, opt(attribute_specifier_seq) }, stdCXX11() },
                                                // C++11 rvalue reference
                {{ nested_name_specifier, TOK_STAR,
                        opt(attribute_specifier_seq),
                        opt(type_qualifier_seq) }, langCXX() }
                                                // C++ pointer-to-member
        }},

        type_qualifier_seq { "type-qualifier-seq", {  // C++: cv-qualifier-seq
                { type_qualifier, opt(type_qualifier_seq) }
        }},

        type_qualifier { "type-qualifier", {  // C++: cv-qualifier
                { TOK_KW_CONST },
                { TOK_KW_VOLATILE },
                {{ TOK_KW_RESTRICT }, stdC99() },
                {{ TOK_KW_ATOMIC }, stdC11() }
        }},

        declarator_id { "declarator-id", {
                { opt(TOK_ELLIPSIS), id_expression }
        }},

        type_id { "type-id", {
                { type_specifier_seq, opt(abstract_declarator) }
        }},

        abstract_declarator { "abstract-declarator", {
                { ptr_abstract_declarator },

                /* deviation from original C++11 grammar: split into two rules,
                   the first parsing parameters-and-qualifiers via
                   noptr-abstract-declarator */
                {{ noptr_abstract_declarator,
                        pred(trailing_return_type,
                             &Declarator::isFunction) }, stdCXX11() },
                {{ parameters_and_qualifiers, trailing_return_type },
                        stdCXX11() },

                {{ abstract_pack_declarator }, stdCXX11() }
        }},

        ptr_abstract_declarator { "ptr-abstract-declarator", {
                { noptr_abstract_declarator },
                { ptr_operator, opt(ptr_abstract_declarator) }
        }, NonTerminal::TRANSPARENT },

        noptr_abstract_declarator { "noptr-abstract-declarator", {
                { opt(noptr_abstract_declarator), parameters_and_qualifiers },
                { opt(noptr_abstract_declarator), array_declarator },
                { nested_abstract_declarator }
        }, NonTerminal::TRANSPARENT },

        nested_abstract_declarator { "nested-abstract-declarator", {
                { TOK_LPAREN, ptr_abstract_declarator, TOK_RPAREN }
        }},

        trailing_return_type { "trailing-return-type", stdCXX11(), {
                { TOK_ARROW, trailing_type_specifier_seq, 
                        opt(abstract_declarator) }
        }},

        ref_qualifier { "ref-qualifier", stdCXX11(), {
                { TOK_AMP },
                { TOK_AMPAMP }
        }},

        abstract_pack_declarator { "abstract-pack-declarator", stdCXX11(), {
                { noptr_abstract_pack_declarator },
                { ptr_operator, abstract_pack_declarator }
        }},

        noptr_abstract_pack_declarator { "noptr-abstract-pack-declarator",
                                           stdCXX11(), {
                { noptr_abstract_pack_declarator, parameters_and_qualifiers },
                { noptr_abstract_pack_declarator, array_declarator },
                { TOK_ELLIPSIS }
        }},

        parameter_declaration_clause { "parameter-declaration-clause", {
                { opt(parameter_declaration_list), opt(TOK_ELLIPSIS) },
                { parameter_declaration_list, TOK_COMMA, TOK_ELLIPSIS }
        }},

        parameter_declaration_list { "parameter-declaration-list", {
                { parameter_declaration },
                { parameter_declaration_list, TOK_COMMA, parameter_declaration }
        }, NonTerminal::TRANSPARENT },

        parameter_declaration { "parameter-declaration", {
                { opt(attribute_specifier_seq), decl_specifier_seq,
                       declarator },
                {{ opt(attribute_specifier_seq), decl_specifier_seq,
                        declarator, TOK_EQUAL,
                        initializer_clause }, langCXX() },
                                                // parameter with default value
                { opt(attribute_specifier_seq), decl_specifier_seq,
                        opt(abstract_declarator) },
                {{ opt(attribute_specifier_seq), decl_specifier_seq,
                        opt(abstract_declarator), TOK_EQUAL,
                        initializer_clause }, langCXX() }
                                                // parameter with default value
        }},

        function_definition { "function-definition", {
                /* deviation from original C++11 grammar: split into two rules,
                   making decl-specifier-seq mandatory in the first rule
                   (reason: the declarator-id's of constructor declarations
                   are mistaken for a decl-specifier-seq) */
                { opt(attribute_specifier_seq), decl_specifier_seq,
                        declarator, opt(virt_specifier_seq), function_body },
                { opt(attribute_specifier_seq), declarator,
                        opt(virt_specifier_seq), function_body }
        }},

        function_body { "function-body", {
                { opt(ctor_initializer), compound_statement },
                {{ function_try_block }, langCXX() },

                // C++11 defaulted/deleted functions
                {{ TOK_EQUAL, TOK_KW_DEFAULT, TOK_SEMI }, stdCXX11() },
                {{ TOK_EQUAL, TOK_KW_DELETE, TOK_SEMI }, stdCXX11() },
        }},

        initializer { "initializer", {  // C: see initializer-clause
                { brace_or_equal_initializer },
                {{ TOK_LPAREN, expression_list, TOK_RPAREN }, langCXX() }
        }},

        brace_or_equal_initializer { "brace-or-equal-initializer", {
                { TOK_EQUAL, initializer_clause },
                {{ braced_init_list }, stdCXX11() }  // uniform initialisation
        }},

        initializer_clause { "initializer-clause", {
                { assignment_expression },
                { braced_init_list }
        }},

        initializer_list { "initializer-list", {
                // designation not recognised if (options.c() < cxx::C99)
                { opt(designation), initializer_clause,
                        opt(TOK_ELLIPSIS) },
                { initializer_list, TOK_COMMA, opt(designation),
                        initializer_clause, opt(TOK_ELLIPSIS) }
        }},

        braced_init_list { "braced-init-list", {
                { TOK_LBRACE, initializer_list, opt(TOK_COMMA), TOK_RBRACE },
                { TOK_LBRACE, TOK_RBRACE }
        }},

        // C99 designated initializers
        designation { "designation", stdC99(), {
                { designator_list, TOK_EQUAL }
        }, NonTerminal::TRANSPARENT },

        designator_list { "designator-list", stdC99(), {
                { designator },
                { designator_list, designator }
        }},

        designator { "designator", stdC99(), {
                { TOK_LSQUARE, constant_expression, TOK_RSQUARE },
                { TOK_DOT, identifier }
        }},

        /*--------------------------------------
         * A.8 Classes [gram.class]
         */
        // class-name: see section A.1 Keywords [gram.key]

        class_specifier { "class-specifier", {
                { class_head, TOK_LBRACE, opt(member_specification),
                        TOK_RBRACE }
        }},  // = struct-or-union-specifier in C grammar

        class_head { "class-head", {
                { class_key, opt(attribute_specifier_seq), class_head_name,
                        opt(class_virt_specifier), opt(base_clause) },
                { class_key, opt(attribute_specifier_seq), opt(base_clause) }
        }},

        class_head_name { "class-head-name", {
                { opt(nested_name_specifier), class_name },
                { opt(nested_name_specifier), undeclared_name },
                { opt(nested_name_specifier), simple_template_id }
        }},

        class_virt_specifier { "class-virt-specifier", stdCXX11(), {
                { pred(TOK_IDENTIFIER, &isFinalSpecifier) }
        }},

        class_key { "class-key", {
                { TOK_KW_STRUCT },
                { TOK_KW_UNION },
                {{ TOK_KW_CLASS }, langCXX() }
        }},

        member_declaration { "member-declaration", {
                {{ decl_specifier_seq, opt(member_declarator_list),
                        TOK_SEMI }, !langCXX() },
                /* deviation from original C++11 grammar: split the following
                   into two rules making decl-specifier-seq mandatory in the
                   first (reason: the declarator-id's of constructor
                   declarations are otherwise mistaken for
                   decl-specifier-seq) */
                {{ opt(attribute_specifier_seq), decl_specifier_seq,
                        opt(member_declarator_list), TOK_SEMI }, langCXX() },
                {{ opt(attribute_specifier_seq),
                        opt(member_declarator_list), TOK_SEMI }, langCXX() },

                {{ function_definition, opt(TOK_SEMI) }, langCXX() },
                {{ using_declaration }, langCXX() },
                {{ template_declaration }, langCXX() },
                {{ alias_declaration }, langCXX() },
                {{ static_assert_declaration }, stdC11() || stdCXX11() }
        }},

        member_specification { "member-specification", {
                { member_declaration, opt(member_specification) },
                {{ access_specifier, TOK_COLON,
                        opt(member_specification) }, langCXX() }
        }, NonTerminal::TRANSPARENT }, // = struct-declaration-list in C grammar

        member_declarator_list { "member-declarator-list", {
                { member_declarator },
                { member_declarator_list, TOK_COMMA, member_declarator }
        }},

        member_declarator { "member-declarator", {
                { declarator, opt(virt_specifier_seq), opt(pure_specifier) },
                {{ declarator, opt(brace_or_equal_initializer) }, stdCXX11() },
                                                // member with default value
                {{ opt(identifier), opt(attribute_specifier_seq),
                        TOK_COLON, constant_expression }, langCXX() },
                                                // C++ bitfield
                {{ opt(declarator), TOK_COLON,
                        constant_expression }, !langCXX() }
                                                // C bitfield
        }},

        virt_specifier_seq { "virt-specifier-seq", stdCXX11(), {
                { virt_specifier },
                { virt_specifier_seq, virt_specifier }
        }},

        virt_specifier { "virt-specifier", stdCXX11(), {
                { pred(TOK_IDENTIFIER,
                        [](ParseState &state) {
                                return *state.input() == u8"override";
                        }) },
                { pred(TOK_IDENTIFIER, &isFinalSpecifier) }
        }},

        pure_specifier { "pure-specifier", langCXX(), {
                { TOK_EQUAL,
                        pred(TOK_DEC_INT_LITERAL, [](ParseState &state) {
                                return *state.input() == u8"0";
                        }) }
        }},

        /*--------------------------------------
         * A.9 Derived classes [gram.derived]
         */
        base_clause { "base-clause", langCXX(), {
                { TOK_COLON, base_specifier_list }
        }},

        base_specifier_list { "base-specifier-list", langCXX(), {
                { base_specifier, opt(TOK_ELLIPSIS) },
                { base_specifier_list, TOK_COMMA, base_specifier,
                        opt(TOK_ELLIPSIS) }
        }, NonTerminal::TRANSPARENT },

        base_specifier { "base-specifier", langCXX(), {
                { opt(attribute_specifier_seq), base_type_specifier },
                { opt(attribute_specifier_seq), TOK_KW_VIRTUAL,
                        opt(access_specifier), base_type_specifier },
                { opt(attribute_specifier_seq), access_specifier,
                        opt(TOK_KW_VIRTUAL), base_type_specifier }
        }},

        class_or_decltype { "class-or-decltype", langCXX(), {
                { opt(nested_name_specifier), class_name },
                { opt(nested_name_specifier), undeclared_name },
                {{ decltype_specifier }, stdCXX11() }
        }},

        base_type_specifier { "base-type-specifier", langCXX(), {
                { class_or_decltype }
        }},

        access_specifier { "access-specifier", langCXX(), {
                { TOK_KW_PRIVATE },
                { TOK_KW_PROTECTED },
                { TOK_KW_PUBLIC }
        }},

        /*--------------------------------------
         * A.10 Special member functions [gram.special]
         */
        conversion_function_id { "conversion-function-id", langCXX(), {
                { TOK_KW_OPERATOR, conversion_type_id }
        }},

        conversion_type_id { "conversion-type-id", langCXX(), {
                { type_specifier_seq, opt(conversion_declarator) }
        }},

        conversion_declarator { "conversion-declarator", langCXX(), {
                { ptr_operator, opt(conversion_declarator) }
        }},

        ctor_initializer { "ctor-initializer", langCXX(), {
                { TOK_COLON, mem_initializer_list }
        }},

        mem_initializer_list { "mem-initializer-list", langCXX(), {
                { mem_initializer, opt(TOK_ELLIPSIS) },
                { mem_initializer, opt(TOK_ELLIPSIS), TOK_COMMA,
                        mem_initializer_list }
        }, NonTerminal::TRANSPARENT },

        mem_initializer { "mem-initializer", langCXX(), {
                { mem_initializer_id, TOK_LPAREN, opt(expression_list),
                        TOK_RPAREN },
                {{ mem_initializer_id, braced_init_list }, stdCXX11() }
        }},

        mem_initializer_id { "mem-initializer-id", langCXX(), {
                { class_or_decltype },
                { identifier }
        }},

        destructor_id { "destructor-id", langCXX(), {
                { TOK_TILDE, class_name },
                { TOK_TILDE, undeclared_name },
                {{ TOK_TILDE, decltype_specifier }, stdCXX11() }
        }},

        /*--------------------------------------
         * A.11 Overloading [gram.over]
         */
        operator_function_id { "operator-function-id", langCXX(), {
                { TOK_KW_OPERATOR, overloadable_operator }
        }},

        overloadable_operator { "overloadable-operator", langCXX(), {
                { TOK_KW_NEW }, { TOK_KW_DELETE },
                { TOK_KW_NEW, TOK_LSQUARE, TOK_RSQUARE },
                { TOK_KW_DELETE, TOK_LSQUARE, TOK_RSQUARE },
                { TOK_PLUS }, { TOK_MINUS }, { TOK_STAR }, { TOK_SLASH },
                { TOK_PERCENT }, { TOK_CARET }, { TOK_AMP }, { TOK_PIPE },
                { TOK_TILDE }, { TOK_EXCLAIM }, { TOK_EQUAL }, { TOK_LESS },
                { TOK_GREATER }, { TOK_PLUSEQUAL }, { TOK_MINUSEQUAL },
                { TOK_STAREQUAL }, { TOK_SLASHEQUAL }, { TOK_PERCENTEQUAL },
                { TOK_CARETEQUAL }, { TOK_AMPEQUAL }, { TOK_PIPEEQUAL },
                { TOK_LSHIFT }, { TOK_RSHIFT }, { TOK_LSHIFTEQUAL },
                { TOK_RSHIFTEQUAL }, { TOK_EQUALEQUAL }, { TOK_EXCLAIMEQUAL },
                { TOK_LESSEQUAL }, { TOK_GREATEREQUAL }, { TOK_AMPAMP },
                { TOK_PIPEPIPE }, { TOK_PLUSPLUS }, { TOK_MINUSMINUS },
                { TOK_COMMA }, { TOK_ARROWSTAR }, { TOK_ARROW },
                { TOK_LPAREN, TOK_RPAREN },
                { TOK_LSQUARE, TOK_RSQUARE }
        }},

        literal_operator_id { "literal-operator-id", stdCXX11(), {
                { TOK_KW_OPERATOR, string_literal, identifier }
        }},

        /*--------------------------------------
         * A.12 Templates [gram.temp]
         */
        template_declaration { "template-declaration", langCXX(), {
                {{ opt(TOK_KW_EXPORT), TOK_KW_TEMPLATE, TOK_LESS,
                        template_parameter_list, TOK_GREATER,
                        declaration }, !stdCXX11() },
                {{ TOK_KW_TEMPLATE, TOK_LESS, template_parameter_list,
                        pred(TOK_GREATER, &processTemplParmArgListEndToken),
                        declaration }, stdCXX11() }
        }},

        template_parameter_list { "template-parameter-list", langCXX(), {
                { template_parameter },
                { template_parameter_list, TOK_COMMA, template_parameter }
        }},

        template_parameter { "template-parameter", langCXX(), {
                { type_parameter },        // template type/template parameter
                { parameter_declaration }  // non-type template parameter
        }},

        type_parameter { "type-parameter", langCXX(), {
                // template type parameters
                { TOK_KW_CLASS, opt(TOK_ELLIPSIS), opt(identifier) },
                { TOK_KW_CLASS, opt(identifier), TOK_EQUAL, type_id },
                { TOK_KW_TYPENAME, opt(TOK_ELLIPSIS), opt(identifier) },
                { TOK_KW_TYPENAME, opt(identifier), TOK_EQUAL, type_id },

                // template template parameters
                { TOK_KW_TEMPLATE, TOK_LESS, template_parameter_list,
                        pred(TOK_GREATER, &processTemplParmArgListEndToken),
                        TOK_KW_CLASS, opt(TOK_ELLIPSIS), opt(identifier) },
                { TOK_KW_TEMPLATE, TOK_LESS, template_parameter_list,
                        pred(TOK_GREATER, &processTemplParmArgListEndToken),
                        TOK_KW_CLASS, opt(identifier), TOK_EQUAL,
                        id_expression }
        }},

        simple_template_id { "simple-template-id", langCXX(), {
                { template_name, TOK_LESS, opt(template_argument_list),
                      pred(TOK_GREATER, &processTemplParmArgListEndToken) },
                { undeclared_name, TOK_LESS, opt(template_argument_list),
                      pred(TOK_GREATER, &processTemplParmArgListEndToken) }
        }},

        template_id { "template-id", langCXX(), {
                { simple_template_id },
                { operator_function_id, TOK_LESS, opt(template_argument_list),
                      pred(TOK_GREATER, &processTemplParmArgListEndToken) },
                {{ literal_operator_id, TOK_LESS, opt(template_argument_list),
                      pred(TOK_GREATER, &processTemplParmArgListEndToken) },
                        stdCXX11() }
        }},

        // template-name: see section A.1 Keywords [gram.key]

        template_argument_list { "template-argument-list", langCXX(), {
                { template_argument, opt(TOK_ELLIPSIS) },
                { template_argument_list, TOK_COMMA, template_argument,
                                opt(TOK_ELLIPSIS) }
        }},

        template_argument { "template-argument", langCXX(), {
                { type_id },
                { constant_expression },
                { id_expression }
        }},

        typename_specifier { "typename-specifier", langCXX(), {
                { TOK_KW_TYPENAME, nested_name_specifier, identifier },
                { TOK_KW_TYPENAME, nested_name_specifier,
                        opt(TOK_KW_TEMPLATE), simple_template_id }
        }},

        explicit_instantiation { "explicit-instantiation", langCXX(), {
                {{ TOK_KW_TEMPLATE, declaration }, !stdCXX11() },
                {{ opt(TOK_KW_EXTERN), TOK_KW_TEMPLATE,
                        declaration }, stdCXX11() }
        }},

        explicit_specialization { "explicit-specialization", langCXX(), {
                { TOK_KW_TEMPLATE, TOK_LESS, TOK_GREATER, declaration }
        }},

        /*--------------------------------------
         * A.13 Exception handling [gram.except]
         */
        try_block { "try-block", langCXX(), {
                { TOK_KW_TRY, compound_statement, handler_seq }
        }},

        function_try_block { "function-try-block", langCXX(), {
                { TOK_KW_TRY, opt(ctor_initializer), compound_statement,
                        handler_seq }
        }},

        handler_seq { "handler-seq", langCXX(), {
                { handler, opt(handler_seq) }
        }},

        handler { "handler", langCXX(), {
                { TOK_KW_CATCH, TOK_LPAREN, exception_declaration, TOK_RPAREN,
                        compound_statement }
        }},

        exception_declaration { "exception-declaration", langCXX(), {
                { opt(attribute_specifier_seq), type_specifier_seq,
                        declarator },
                { opt(attribute_specifier_seq), type_specifier_seq,
                        opt(abstract_declarator) }
        }},

        throw_expression { "throw-expression", langCXX(), {
                { TOK_KW_THROW, opt(assignment_expression) }
        }},

        exception_specification { "exception-specification", langCXX(), {
                { dynamic_exception_specification },
                {{ noexcept_specification }, stdCXX11() }
        }},

        dynamic_exception_specification { "dynamic-exception-specification",
                                            langCXX(), {
                { TOK_KW_THROW, TOK_LPAREN, opt(type_id_list), TOK_RPAREN }
        }},

        type_id_list { "type-id-list", langCXX(), {
                { type_id, opt(TOK_ELLIPSIS) },
                { type_id_list, type_id, opt(TOK_ELLIPSIS) }
        }, NonTerminal::TRANSPARENT },

        noexcept_specification { "noexcept-specification", stdCXX11(), {
                { TOK_KW_NOEXCEPT, TOK_LPAREN, constant_expression,
                        TOK_RPAREN },
                { TOK_KW_NOEXCEPT }
        }}
{
        decl_specifier_seq.addPostParseAction(&DeclSpecifier::end);
        type_specifier_seq.addPostParseAction(&DeclSpecifier::end);
        trailing_type_specifier_seq.addPostParseAction(&DeclSpecifier::end);

        declarator.addPostParseAction(&Declarator::end);
        nested_declarator.addPostParseAction(&Declarator::end);
        abstract_declarator.addPostParseAction(&Declarator::end);
        nested_abstract_declarator.addPostParseAction(&Declarator::end);
        new_declarator.addPostParseAction(&Declarator::end);
        conversion_declarator.addPostParseAction(&Declarator::end);

        lambda_declarator.addPostParseAction(
                           &DeclaratorPart::endParametersAndQualifiers);
        parameters_and_qualifiers.addPostParseAction(
                           &DeclaratorPart::endParametersAndQualifiers);

        ptr_operator.addPostParseAction(&DeclaratorPart::endPtrOperator);
}

//--------------------------------------

WRPARSECXX_API
CXXParser::CXXParser(
        const CXXOptions &options,
        Lexer            &lexer
) :
        CXXParser(options)
{
        setLexer(lexer);
}

//--------------------------------------

WRPARSECXX_API
CXXParser::CXXParser(
        CXXLexer &lexer
) :
        CXXParser(lexer.options())
{
        setLexer(lexer);
}

//--------------------------------------

WRPARSECXX_API CXXParser::~CXXParser() = default;

//--------------------------------------

template <> WRPARSECXX_API CXXParser::DeclSpecifier *
CXXParser::get(
        const SPPFNode &decl_spec_seq
)
{
        return dynamic_cast<DeclSpecifier *>(decl_spec_seq.auxData().get());
}

//--------------------------------------

template <> WRPARSECXX_API CXXParser::Declarator *
CXXParser::get(
        const SPPFNode &declarator
)
{
        return dynamic_cast<Declarator *>(declarator.auxData().get());
}

//--------------------------------------

template <> WRPARSECXX_API CXXParser::DeclaratorPart *
CXXParser::get(
        const SPPFNode &part
)
{
        return dynamic_cast<DeclaratorPart *>(part.auxData().get());
}

//--------------------------------------

uint8_t
CXXParser::qualifierForToken(
        const Token &token
)
{
        switch (token.kind()) {
        case TOK_KW_CONST:
                return CONST;
        case TOK_KW_VOLATILE:
                return VOLATILE;
        case TOK_KW_RESTRICT:
                return RESTRICT;
        case TOK_KW_ATOMIC:
                return ATOMIC;
        case TOK_AMP:
                return LVAL_REF;
        case TOK_AMPAMP:
                return RVAL_REF;
        default:
                return 0;
        }
}

//--------------------------------------

uint8_t
CXXParser::typeQualifiersFromSeq(
        const SPPFNode &type_qualifier_seq
)
{
        uint8_t qualifiers = 0;

        for (const SPPFNode &qualifier: subProductions(type_qualifier_seq)) {
                qualifiers |= qualifierForToken(*qualifier.firstToken());
        }

        return qualifiers;
}

//--------------------------------------

bool
CXXParser::DeclSpecifier::end(
        ParseState &state  ///< the current parsing state
)
{
        SPPFNode::ConstPtr decl_spec_seq = state.parsedNode();
        bool               ok            = true;

        if (decl_spec_seq) {
                CXXParser &cxx = CXXParser::getFrom(state);
                Ptr        me  = new this_t;

                for (const SPPFNode &spec: subProductions(decl_spec_seq)) {
                        ok = me->addDeclSpecifier(cxx, spec) && ok;
                }

                if (ok) {
                        state.parsedNode()->setAuxData(me);
                }
        }

        return ok;
}

//--------------------------------------

bool
CXXParser::DeclSpecifier::addDeclSpecifier(
        CXXParser      &cxx,
        const SPPFNode &spec
)
{
        SPPFNode::ConstPtr match;
        bool               apply = true;

        if (spec.is(cxx.type_qualifier)) {
                type_qual |= qualifierForToken(*spec.firstToken());
        } else if (spec.is(cxx.simple_type_specifier, match)) {
                Type type = NO_TYPE;
                Size size = NO_SIZE;
                Sign sign = NO_SIGN;

                switch (spec.firstToken()->kind()) {
                        case TOK_KW_VOID:     type = VOID; break;
                        case TOK_KW_AUTO:     type = AUTO; break;
                        case TOK_KW_DECLTYPE: type = DECLTYPE; break;
                        case TOK_KW_BOOL:     type = BOOL; break;
                        case TOK_KW_CHAR:     type = CHAR; break;
                        case TOK_KW_CHAR16_T: type = CHAR16_T; break;
                        case TOK_KW_CHAR32_T: type = CHAR32_T; break;
                        case TOK_KW_WCHAR_T:  type = WCHAR_T; break;
                        case TOK_KW_INT:      type = INT; break;
                        case TOK_KW_FLOAT:    type = FLOAT; break;
                        case TOK_KW_DOUBLE:   type = DOUBLE; break;
                        case TOK_KW_SHORT:    size = SHORT; break;
                        case TOK_KW_LONG:
                                if (spec.is(TOK_KW_LONG)) {
                                        size = LONG;
                                } else {
                                        size = LONG_LONG;
                                }
                                break;
                        case TOK_KW_SIGNED:   sign = SIGNED; break;
                        case TOK_KW_UNSIGNED: sign = UNSIGNED; break;
                        case TOK_IDENTIFIER:
                                if (*spec.firstToken() == "nullptr_t") {
                                        type = NULLPTR_T;
                                        break;
                                } // else fall through
                        default:
                                type = OTHER;
                                break;
                }

                if (type) {
                        if (type_spec) {
                                if (type_spec_node == &spec) {
                                        return true;
                                } else if (type == OTHER) {
                                        /* if (type == OTHER) stop parsing
                                           as this is probably the beginning
                                           of a declarator */
                                        return false;
                                } else if (type_spec_node == &spec) {
                                        // same type already specified elsewhere
                                        cxx.emit(Diagnostic::ERROR, spec,
                                                 "redundant type specifier \"%s\"",
                                                 spec);
                                        return true;  // but carry on parsing
                                } else {
                                        cxx.emit(Diagnostic::ERROR, spec,
                                                 "\"%s\" conflicts with earlier type specifier \"%s\"",
                                                 spec, *type_spec_node);
                                        return true;
                                }
                        }

                        switch (type) {
                        default:
                                assert(false);
                                break;
                        case VOID: case AUTO: case DECLTYPE: case BOOL:
                        case CHAR16_T: case CHAR32_T: case WCHAR_T:
                        case FLOAT: case NULLPTR_T: case OTHER:
                                if (sign_spec) {
                                        cxx.emit(Diagnostic::ERROR,
                                                 *sign_spec_node,
                                                 "\"%s\" modifier cannot be used with type \"%s\"",
                                                 sign_spec, spec);
                                        apply = false;
                                }
                                if (size_spec) {
                                        cxx.emit(Diagnostic::ERROR,
                                                 *size_spec_node,
                                                 "\"%s\" modifier cannot be used with type \"%s\"",
                                                 size_spec, spec);
                                        apply = false;
                                }
                                break;
                        case CHAR:
                                if (size_spec) {
                                        cxx.emit(Diagnostic::ERROR,
                                                 *size_spec_node,
                                                 "\"%s\" modifier cannot be used with type \"char\"",
                                                 size_spec);
                                        return true;
                                }
                                break;
                        case INT:
                                break;
                        case DOUBLE:
                                if (sign_spec) {
                                        cxx.emit(Diagnostic::ERROR,
                                                 *sign_spec_node,
                                                 "\"%s\" modifier cannot be used with type \"double\"",
                                                 sign_spec);
                                        apply = false;
                                }
                                if (size_spec && (size_spec != LONG)) {
                                        cxx.emit(Diagnostic::ERROR,
                                                 *size_spec_node,
                                                 "\"%s\" modifier cannot be used with type \"double\"",
                                                 size_spec);
                                        apply = false;
                                }
                                break;
                        }

                        if (apply) {
                                type_spec = type;
                                type_spec_node = &spec;
                        }
                } else if (size) {
                        if (size_spec && (size != size_spec)) {
                                cxx.emit(Diagnostic::ERROR, spec,
                                         "\"%s\" conflicts with earlier \"%s\" modifier",
                                         size, size_spec);
                                return true;
                        }

                        switch (size) {
                        default:
                                assert(false);
                                break;
                        case SHORT: case LONG_LONG:
                                if (type_spec && type_spec != INT) {
                                        cxx.emit(Diagnostic::ERROR, spec,
                                                 "\"%s\" modifier cannot be used with type \"%s\"",
                                                 size, *type_spec_node);
                                        return true;
                                }
                                break;
                        case LONG:
                                if (type_spec && (type_spec != INT)
                                              && (type_spec != DOUBLE)) {
                                        cxx.emit(Diagnostic::ERROR, spec,
                                                 "\"%s\" modifier cannot be used with type \"%s\"",
                                                 size, *type_spec_node);
                                        return true;
                                }
                                break;
                        }

                        size_spec = size;
                        size_spec_node = &spec;
                } else if (sign) {
                        if (sign_spec && (sign != sign_spec)) {
                                cxx.emit(Diagnostic::ERROR, spec,
                                         "\"%s\" conflicts with earlier modifier \"%s\"",
                                         sign, sign_spec);
                                apply = false;
                        }
                        if (type_spec && (type_spec != INT)
                                      && (type_spec != CHAR)) {
                                cxx.emit(Diagnostic::ERROR, spec,
                                         "\"%s\" modifier cannot be used with type \"%s\"",
                                         sign, *type_spec_node);
                                apply = false;
                        }

                        if (apply) {
                                sign_spec = sign;
                                sign_spec_node = &spec;
                        }
                }
        } else if (spec.is(cxx.type_specifier)) {
                /* elaborated-type-specifier, typename-specifier,
                   enum-specifier or class-specifier */
                if (type_spec) {
                        return type_spec_node == &spec;
                }
                if (sign_spec) {
                        cxx.emit(Diagnostic::ERROR, spec,
                                 "\"%s\" modifier cannot be used with type \"%s\"",
                                 sign_spec, *type_spec_node);
                        apply = false;
                } else if (size_spec) {
                        cxx.emit(Diagnostic::ERROR, spec,
                                 "\"%s\" modifier cannot be used with type \"%s\"",
                                 size_spec, *type_spec_node);
                        apply = false;
                }

                if (apply) {
                        type_spec = OTHER;
                        type_spec_node = &spec;
                }
        }

        return true;
}

//--------------------------------------
/**
 * \brief find a declarator node's rightmost ptr-operator
 *
 * Looks for the rightmost ptr-operator (*, &amp;, &amp;&amp; or X::*) directly
 * under a declarator, abstract-declarator, new-declarator or
 * conversion-declarator node.
 *
 * \param [in] cxx
 *      parser object for referencing the C++ grammar nonterminals
 * \param [in] dcl_node
 *      declarator node to search under
 * \return
 *      pointer to rightmost ptr-operator node if it exists,
 *      \c nullptr otherwise
 * \note
 *      Nested declarators are not searched so this function will return a
 *      null pointer given a declarator node parsed from <code>int (*p)</code>,
 *      for example.
 */
WRPARSECXX_API const SPPFNode *
CXXParser::Declarator::lastPtrOperator(
        CXXParser      &cxx,
        const SPPFNode &dcl_node
) // static
{
        const SPPFNode *ptr_op = nullptr;

        // ptr_operators always come first
        for (const SPPFNode &i: subProductions(dcl_node)) {
                if (i.is(cxx.ptr_operator)) {
                        ptr_op = &i;
                } else {
                        break;
                }
        }

        return ptr_op;
}

//--------------------------------------
/**
 * \brief determine if a declarator node is a reference
 *
 * \param [in] cxx
 *      parser object for referencing the C++ grammar nonterminals
 * \param [in] dcl_node
 *      declarator node
 * \return
 *      \c true if \c dcl_node is a reference declarator, \c false otherwise
 * \note
 *      Nested declarators are not searched so this function will return
 *      \c false given a declarator node parsed from
 *      <code>int (&amp;p)[]</code>, for example.
 */
WRPARSECXX_API bool
CXXParser::Declarator::isReference(
        CXXParser      &cxx,
        const SPPFNode &dcl_node
) // static
{
        const SPPFNode *last_ptr_op = lastPtrOperator(cxx, dcl_node);

        if (last_ptr_op) {
                const Token *token = last_ptr_op->firstToken();
                if (token->is(TOK_AMP) || token->is(TOK_AMPAMP)) {
                        return true;
                }
        }

        return false;
}

//--------------------------------------

bool
CXXParser::Declarator::end(
        ParseState &state  ///< the current parsing state
)
{
        if (state.parsedNode()) {
                Ptr me = new this_t;
                if (!me->check(state, CXXParser::getFrom(state),
                               *state.parsedNode())) {
                        return false;
                }
                state.parsedNode()->setAuxData(me);
        }
        return true;
}

//--------------------------------------

bool
CXXParser::Declarator::check(
        ParseState     &state,
        CXXParser      &cxx,
        const SPPFNode &dcl_node
)
{
        SPPFNode::ConstPtr  nested_dcl     = nullptr;
        const Token        *ref_op         = nullptr;
        bool                ref_to_ref     = false,
                            ptr_to_ref     = false,
                            multi_fn_parms = false,
                            array_of_refs  = false;

        for (const SPPFNode &part: subProductions(dcl_node)) {
                if (part.is(cxx.ptr_operator)) {
                        if (part.firstToken()->is(TOK_AMP)
                                        || part.firstToken()->is(TOK_AMPAMP)) {
                                if (!ref_op) {
                                        ref_op = part.firstToken();
                                } else if (!ref_to_ref) {
                                        state.emit(Diagnostic::ERROR,
                                                   "reference to reference not permitted");
                                        ref_to_ref = true;
                                }
                        } else if (ref_op && !ptr_to_ref) {
                                state.emit(Diagnostic::ERROR,
                                           "pointer to reference not permitted");
                                ptr_to_ref = true;
                        }
                        last_ptr = part.firstToken();
                } else if (part.is(cxx.parameters_and_qualifiers)) {
                        if (!begin_parms) {
                                begin_parms = part.firstToken();
                                        /* = first token of
                                             parameter-declaration-clause */
                        } else if (!multi_fn_parms) {
                                state.emit(Diagnostic::ERROR, part,
                                           "multiple sets of function parameters/qualifiers");
                                multi_fn_parms = true;
                        }
                } else if (part.is(cxx.array_declarator)) {
                        if (ref_op && !array_of_refs) {
                                state.emit(Diagnostic::ERROR,
                                           "array of references not permitted");
                                array_of_refs = true;
                        }
                        array = true;
                } else if (part.is(cxx.nested_declarator)
                                || part.is(cxx.nested_abstract_declarator)) {
                        nested_dcl = &part;
                }
        }

        if (nested_dcl) {
                return check(state, cxx, *nested_dcl);
        }

        return true;
}

//--------------------------------------

bool
CXXParser::Declarator::isFunction(
        ParseState &state  ///< the current parsing state
)
{
        auto               &cxx      = CXXParser::getFrom(state);
        SPPFNode::ConstPtr  dcl_node = state.parsedNode();
        return dcl_node
                && (dcl_node->find(cxx.parameters_and_qualifiers) != nullptr);
}

//--------------------------------------

WRPARSECXX_API bool
CXXParser::DeclaratorPart::isParmPackOperator(
        CXXParser      &cxx,
        const SPPFNode &part
)
{
        return (part.is(cxx.declarator_id)
                                && part.firstToken()->is(TOK_ELLIPSIS))
               || part.is(cxx.abstract_pack_declarator);
}

//--------------------------------------

bool
CXXParser::DeclaratorPart::endParametersAndQualifiers(
        ParseState &state  ///< the current parsing state
)
{
        SPPFNode::ConstPtr result = state.parsedNode();

        if (!result) {
                return false;
        }

        Ptr                 me    = new this_t;
        const CXXParser    &cxx   = CXXParser::getFrom(state);
        SPPFNode::ConstPtr  parms = nonTerminals(result).node();

        if (parms && parms->is(cxx.parameter_declaration_clause)) {
                if (parms->empty()) {
                        me->count = 0;
                } else if (!parms->hasChildren()) {
                        me->count = 1;
                } else {
                        me->count = numeric_cast<unsigned short>(
                                                      countNonTerminals(parms));
                }
                me->variadic = !parms->empty()
                                && parms->lastToken()->is(TOK_ELLIPSIS);
        }

        for (const SPPFNode &quals: nonTerminals(result)) {
                if (quals.is(cxx.type_qualifier_seq)) {
                        me->qualifiers |= typeQualifiersFromSeq(quals);
                } else if (quals.is(cxx.ref_qualifier)) {
                        me->qualifiers
                                |= qualifierForToken(*quals.firstToken());
                }
        }

        result->setAuxData(me);
        return true;
}

//--------------------------------------

bool
CXXParser::DeclaratorPart::endPtrOperator(
        ParseState &state  ///< the current parsing state
)
{
        SPPFNode::ConstPtr result = state.parsedNode();

        if (!result) {  // didn't match
                return false;
        }

        Ptr              me         = new this_t;
        const CXXParser &cxx        = CXXParser::getFrom(state);
        auto             type_quals = result->find(cxx.type_qualifier_seq, 1);

        if (type_quals) {
                me->qualifiers = typeQualifiersFromSeq(*type_quals);
        }

        result->setAuxData(me);
        return true;
}

//--------------------------------------

bool
CXXParser::isTypedefName(
        ParseState &state
)
{
        (void) state;
        return false;
}

//--------------------------------------

bool
CXXParser::isClassName(
        ParseState &state
)
{
        (void) state;
        return false;
}

//--------------------------------------

bool
CXXParser::isEnumName(
        ParseState &state
)
{
        (void) state;
        return false;
}

//--------------------------------------

bool
CXXParser::isNamespaceName(
        ParseState &state
)
{
        (void) state;
        return false;
}

//--------------------------------------

bool
CXXParser::isNamespaceAliasName(
        ParseState &state
)
{
        (void) state;
        return false;
}

//--------------------------------------

bool
CXXParser::isTemplateName(
        ParseState &state
)
{
        (void) state;
        return false;
}

//--------------------------------------

bool
CXXParser::isUndeclaredName(
        ParseState &state
)
{
        (void) state;
        return true;
}

//--------------------------------------

bool
CXXParser::isFinalSpecifier(
        ParseState &state  ///< the current parsing state
)
{
        return *state.input() == u8"final";
}

//--------------------------------------

bool
CXXParser::isBalancedToken(
        ParseState &state  ///< the current parsing state
)
{
        switch (state.input()->kind()) {
        case TOK_LPAREN: case TOK_RPAREN: case TOK_LSQUARE: case TOK_RSQUARE:
        case TOK_LBRACE: case TOK_RBRACE:
                return false;
        default:
                return true;
        }
}

//--------------------------------------

bool
CXXParser::processTemplParmArgListEndToken(
        ParseState &state  ///< the current parsing state
)
{
        if (CXXParser::getFrom(state).options_.cxx() < cxx::CXX11) {
                return true;
        }

        auto token = state.parser().tokens().make_iterator(state.input());

        TokenKind   new_kind;
        const char *new_spelling;

        switch (token->kind()) {
        case TOK_GREATER:
                return true;
        case TOK_RSHIFT:
                new_kind = TOK_GREATER, new_spelling = u8">";
                break;
        case TOK_GREATEREQUAL:
                new_kind = TOK_EQUAL, new_spelling = u8"=";
                break;
        case TOK_RSHIFTEQUAL:
                new_kind = TOK_GREATEREQUAL, new_spelling = u8">=";
                break;
        default:
                return false;
        }

        if (!(token->flags() & cxx::TF_SPLITABLE)) {
                return false;
        }

        // split token in two
        token->setKind(TOK_GREATER).setSpelling(u8">");
        state.parser().tokens().emplace_after(token, *token)
                ->setKind(new_kind).setSpelling(new_spelling).adjustOffset(1);
        return true;
}


} // namespace parse

//--------------------------------------

namespace fmt {


WRPARSE_API void
fmt::TypeHandler<parse::CXXParser::DeclSpecifier::Sign>::set(
        Arg                                   &arg,
        parse::CXXParser::DeclSpecifier::Sign  val
)
{
        arg.type = Arg::STR_T;
        switch (val) {
        case parse::CXXParser::DeclSpecifier::NO_SIGN:
                arg.s = { "none", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::SIGNED:
                arg.s = { "signed", 6 };
                break;
        case parse::CXXParser::DeclSpecifier::UNSIGNED:
                arg.s = { "unsigned", 8 };
                break;
        default:
                arg.s = { "unknown", 7 };
                break;
        }
}

//--------------------------------------

WRPARSE_API void
fmt::TypeHandler<parse::CXXParser::DeclSpecifier::Size>::set(
        Arg                                   &arg,
        parse::CXXParser::DeclSpecifier::Size  val
)
{
        arg.type = Arg::STR_T;
        switch (val) {
        case parse::CXXParser::DeclSpecifier::NO_SIZE:
                arg.s = { "none", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::SHORT:
                arg.s = { "short", 5 };
                break;
        case parse::CXXParser::DeclSpecifier::LONG:
                arg.s = { "long", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::LONG_LONG:
                arg.s = { "long long", 9 };
                break;
        default:
                arg.s = { "unknown", 7 };
                break;
        }
}

//--------------------------------------

WRPARSE_API void
fmt::TypeHandler<parse::CXXParser::DeclSpecifier::Type>::set(
        Arg                                   &arg,
        parse::CXXParser::DeclSpecifier::Type  val
)
{
        arg.type = Arg::STR_T;
        switch (val) {
        case parse::CXXParser::DeclSpecifier::NO_TYPE:
                arg.s = { "none", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::VOID:
                arg.s = { "void", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::AUTO:
                arg.s = { "auto", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::DECLTYPE:
                arg.s = { "decltype(...)", 13 };
                break;
        case parse::CXXParser::DeclSpecifier::BOOL:
                arg.s = { "bool", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::CHAR:
                arg.s = { "char", 4 };
                break;
        case parse::CXXParser::DeclSpecifier::CHAR16_T:
                arg.s = { "char16_t", 8 };
                break;
        case parse::CXXParser::DeclSpecifier::CHAR32_T:
                arg.s = { "char32_t", 8 };
                break;
        case parse::CXXParser::DeclSpecifier::WCHAR_T:
                arg.s = { "wchar_t", 7 };
                break;
        case parse::CXXParser::DeclSpecifier::INT:
                arg.s = { "int", 3 };
                break;
        case parse::CXXParser::DeclSpecifier::FLOAT:
                arg.s = { "float", 5 };
                break;
        case parse::CXXParser::DeclSpecifier::DOUBLE:
                arg.s = { "double", 6 };
                break;
        case parse::CXXParser::DeclSpecifier::NULLPTR_T:
                arg.s = { "nullptr_t", 9 };
                break;
        case parse::CXXParser::DeclSpecifier::OTHER:
                arg.s = { "user-defined", 12 };
                break;
        default:
                arg.s = { "unknown", 7 };
                break;
        }
}


} // namespace fmt
} // namespace wr
