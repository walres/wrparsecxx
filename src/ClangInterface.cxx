/**
 * \file ClangInterface.cxx
 *
 * \brief Functions for interacting with Clang ASTs
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
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <clang/Sema/Lookup.h>
#include <clang/Sema/Sema.h>
#include <wrparse/Token.h>
#include <wrparse/cxx/CXXParser.h>
#include <wrparse/cxx/CXXTokenKinds.h>
#include <wrparse/cxx/ClangInterface.h>


using namespace std;


namespace wr {
namespace parse {


struct ClangTokenData
{
        const char * const spelling;
        TokenKind          to_kind;
};

//--------------------------------------

static const unordered_map<int, ClangTokenData>
TOKEN_CONV = {
        { clang::tok::unknown, { "", TOK_NULL }},
        { clang::tok::eof, { "", TOK_EOF }},

        { clang::tok::l_paren, { "(", cxx::TOK_LPAREN }},
        { clang::tok::r_paren, { ")", cxx::TOK_RPAREN }},
        { clang::tok::l_square, { "[", cxx::TOK_LSQUARE }},
        { clang::tok::r_square, { "]", cxx::TOK_RSQUARE }},
        { clang::tok::l_brace, { "{", cxx::TOK_LBRACE }},
        { clang::tok::r_brace, { "}", cxx::TOK_RBRACE }},
        { clang::tok::period, { ".", cxx::TOK_DOT }},
        { clang::tok::ellipsis, { "...", cxx::TOK_ELLIPSIS }},
        { clang::tok::amp, { "&", cxx::TOK_AMP }},
        { clang::tok::ampamp, { "&&", cxx::TOK_AMPAMP }},
        { clang::tok::ampequal, { "&=", cxx::TOK_AMPEQUAL }},
        { clang::tok::star, { "*", cxx::TOK_STAR }},
        { clang::tok::starequal, { "*=", cxx::TOK_STAREQUAL }},
        { clang::tok::plus, { "+", cxx::TOK_PLUS }},
        { clang::tok::plusplus, { "++", cxx::TOK_PLUSPLUS }},
        { clang::tok::plusequal, { "+=", cxx::TOK_PLUSEQUAL }},
        { clang::tok::minus, { "-", cxx::TOK_MINUS }},
        { clang::tok::arrow, { "->", cxx::TOK_ARROW }},
        { clang::tok::minusminus, { "--", cxx::TOK_MINUSMINUS }},
        { clang::tok::minusequal, { "-=", cxx::TOK_MINUSEQUAL }},
        { clang::tok::tilde, { "~", cxx::TOK_TILDE }},
        { clang::tok::exclaim, { "!", cxx::TOK_EXCLAIM }},
        { clang::tok::exclaimequal, { "!=", cxx::TOK_EXCLAIMEQUAL }},
        { clang::tok::slash, { "/", cxx::TOK_SLASH }},
        { clang::tok::slashequal, { "/=", cxx::TOK_SLASHEQUAL }},
        { clang::tok::percent, { "%", cxx::TOK_PERCENT }},
        { clang::tok::percentequal, { "%=", cxx::TOK_PERCENTEQUAL }},
        { clang::tok::less, { "<", cxx::TOK_LESS }},
        { clang::tok::lessequal, { "<=", cxx::TOK_LESSEQUAL }},
        { clang::tok::lessless, { "<<", cxx::TOK_LSHIFT }},
        { clang::tok::lesslessequal, { "<<=", cxx::TOK_LSHIFTEQUAL }},
        { clang::tok::greater, { ">", cxx::TOK_GREATER }},
        { clang::tok::greaterequal, { ">=", cxx::TOK_GREATEREQUAL }},
        { clang::tok::greatergreater, { ">>", cxx::TOK_RSHIFT }},
        { clang::tok::greatergreaterequal,
                                { ">>=", cxx::TOK_RSHIFTEQUAL }},
        { clang::tok::caret, { "^", cxx::TOK_CARET }},
        { clang::tok::caretequal, { "^=", cxx::TOK_CARETEQUAL }},
        { clang::tok::pipe, { "|", cxx::TOK_PIPE }},
        { clang::tok::pipepipe, { "||", cxx::TOK_PIPEPIPE }},
        { clang::tok::pipeequal, { "|=", cxx::TOK_PIPEEQUAL }},
        { clang::tok::question, { "?", cxx::TOK_QUESTION }},
        { clang::tok::colon, { ":", cxx::TOK_COLON }},
        { clang::tok::semi, { ";", cxx::TOK_SEMI }},
        { clang::tok::equal, { "=", cxx::TOK_EQUAL }},
        { clang::tok::equalequal, { "==", cxx::TOK_EQUALEQUAL }},
        { clang::tok::comma, { ",", cxx::TOK_COMMA }},
        { clang::tok::hash, { "#", cxx::TOK_HASH }},
        { clang::tok::hashhash, { "##", cxx::TOK_HASHHASH }},
        { clang::tok::periodstar, { ".*", cxx::TOK_DOTSTAR }},
        { clang::tok::arrowstar, { "->*", cxx::TOK_ARROWSTAR }},
        { clang::tok::coloncolon, { "::", cxx::TOK_COLONCOLON }},

        { clang::tok::kw_alignas, { "alignas", cxx::TOK_KW_ALIGNAS }},
        { clang::tok::kw_alignof, { "alignof", cxx::TOK_KW_ALIGNOF }},
        { clang::tok::kw_asm, { "asm", cxx::TOK_KW_ASM }},
        { clang::tok::kw__Atomic, { "_Atomic", cxx::TOK_KW_ATOMIC }},
        { clang::tok::kw_auto, { "auto", cxx::TOK_KW_AUTO }},
        { clang::tok::kw_bool, { "bool", cxx::TOK_KW_BOOL }},
        { clang::tok::kw_break, { "break", cxx::TOK_KW_BREAK }},
        { clang::tok::kw_case, { "case", cxx::TOK_KW_CASE }},
        { clang::tok::kw_catch, { "catch", cxx::TOK_KW_CATCH }},
        { clang::tok::kw_char, { "char", cxx::TOK_KW_CHAR }},
        { clang::tok::kw_char16_t, { "char16_t", cxx::TOK_KW_CHAR16_T }},
        { clang::tok::kw_char32_t, { "char32_t", cxx::TOK_KW_CHAR32_T }},
        { clang::tok::kw_class, { "class", cxx::TOK_KW_CLASS }},
        { clang::tok::kw__Complex, { "_Complex", cxx::TOK_KW_COMPLEX }},
        { clang::tok::kw_const, { "const", cxx::TOK_KW_CONST }},
        { clang::tok::kw_const_cast, { "const_cast", cxx::TOK_KW_CONST_CAST }},
        { clang::tok::kw_constexpr, { "constexpr", cxx::TOK_KW_CONSTEXPR }},
        { clang::tok::kw_continue, { "continue", cxx::TOK_KW_CONTINUE }},
        { clang::tok::kw_decltype, { "decltype", cxx::TOK_KW_DECLTYPE }},
        { clang::tok::kw_default, { "default", cxx::TOK_KW_DEFAULT }},
        { clang::tok::kw_delete, { "delete", cxx::TOK_KW_DELETE }},
        { clang::tok::kw_do, { "do", cxx::TOK_KW_DO }},
        { clang::tok::kw_double, { "double", cxx::TOK_KW_DOUBLE }},
        { clang::tok::kw_dynamic_cast,
                        { "dynamic_cast", cxx::TOK_KW_DYNAMIC_CAST }},
        { clang::tok::kw_else, { "else", cxx::TOK_KW_ELSE }},
        { clang::tok::kw_enum, { "enum", cxx::TOK_KW_ENUM }},
        { clang::tok::kw_explicit, { "explicit", cxx::TOK_KW_EXPLICIT }},
        { clang::tok::kw_extern, { "extern", cxx::TOK_KW_EXTERN }},
        { clang::tok::kw_float, { "float", cxx::TOK_KW_FLOAT }},
        { clang::tok::kw_for, { "for", cxx::TOK_KW_FOR }},
        { clang::tok::kw_friend, { "friend", cxx::TOK_KW_FRIEND }},
        { clang::tok::kw___func__, { "__func__", cxx::TOK_KW_FUNC }},
        { clang::tok::kw_goto, { "goto", cxx::TOK_KW_GOTO }},
        { clang::tok::kw_if, { "if", cxx::TOK_KW_IF }},
        { clang::tok::kw__Imaginary, { "_Imaginary", cxx::TOK_KW_IMAGINARY }},
        { clang::tok::kw_inline, { "inline", cxx::TOK_KW_INLINE }},
        { clang::tok::kw_int, { "int", cxx::TOK_KW_INT }},
        { clang::tok::kw_long, { "long", cxx::TOK_KW_LONG }},
        { clang::tok::kw_mutable, { "mutable", cxx::TOK_KW_MUTABLE }},
        { clang::tok::kw_new, { "new", cxx::TOK_KW_NEW }},
        { clang::tok::kw_namespace, { "namespace", cxx::TOK_KW_NAMESPACE }},
        { clang::tok::kw_noexcept, { "noexcept", cxx::TOK_KW_NOEXCEPT }},
        { clang::tok::kw__Noreturn, { "_Noreturn", cxx::TOK_KW_NORETURN }},
        { clang::tok::kw_nullptr, { "nullptr", cxx::TOK_KW_NULLPTR }},
        { clang::tok::kw_operator, { "operator", cxx::TOK_KW_OPERATOR }},
        { clang::tok::kw_private, { "private", cxx::TOK_KW_PRIVATE }},
        { clang::tok::kw_protected, { "protected", cxx::TOK_KW_PROTECTED }},
        { clang::tok::kw_public, { "public", cxx::TOK_KW_PUBLIC }},
        { clang::tok::kw_register, { "register", cxx::TOK_KW_REGISTER }},
        { clang::tok::kw_reinterpret_cast,
                { "reinterpret_cast", cxx::TOK_KW_REINTERPRET_CAST }},
        { clang::tok::kw_restrict, { "restrict", cxx::TOK_KW_RESTRICT }},
        { clang::tok::kw_return, { "return", cxx::TOK_KW_RETURN }},
        { clang::tok::kw_short, { "short", cxx::TOK_KW_SHORT }},
        { clang::tok::kw_signed, { "signed", cxx::TOK_KW_SIGNED }},
        { clang::tok::kw_sizeof, { "sizeof", cxx::TOK_KW_SIZEOF }},
        { clang::tok::kw_static, { "static", cxx::TOK_KW_STATIC }},
        { clang::tok::kw_static_assert,
                        { "static_assert", cxx::TOK_KW_STATIC_ASSERT }},
        { clang::tok::kw_static_cast,
                        { "static_cast", cxx::TOK_KW_STATIC_CAST }},
        { clang::tok::kw_struct, { "struct", cxx::TOK_KW_STRUCT }},
        { clang::tok::kw_switch, { "switch", cxx::TOK_KW_SWITCH }},
        { clang::tok::kw_template, { "template", cxx::TOK_KW_TEMPLATE }},
        { clang::tok::kw_this, { "this", cxx::TOK_KW_THIS }},
        { clang::tok::kw_thread_local,
                        { "thread_local", cxx::TOK_KW_THREAD_LOCAL }},
        { clang::tok::kw_throw, { "throw", cxx::TOK_KW_THROW }},
        { clang::tok::kw_try, { "try", cxx::TOK_KW_TRY }},
        { clang::tok::kw_typedef, { "typedef", cxx::TOK_KW_TYPEDEF }},
        { clang::tok::kw_typeid, { "typeid", cxx::TOK_KW_TYPEID }},
        { clang::tok::kw_typename, { "typename", cxx::TOK_KW_TYPENAME }},
        { clang::tok::kw_union, { "union", cxx::TOK_KW_UNION }},
        { clang::tok::kw_unsigned, { "unsigned", cxx::TOK_KW_UNSIGNED }},
        { clang::tok::kw_using, { "using", cxx::TOK_KW_USING }},
        { clang::tok::kw_virtual, { "virtual", cxx::TOK_KW_VIRTUAL }},
        { clang::tok::kw_void, { "void", cxx::TOK_KW_VOID }},
        { clang::tok::kw_volatile, { "volatile", cxx::TOK_KW_VOLATILE }},
        { clang::tok::kw_wchar_t, { "wchar_t", cxx::TOK_KW_WCHAR_T }},
        { clang::tok::kw_while, { "while", cxx::TOK_KW_WHILE }},

        { clang::tok::identifier, { "", cxx::TOK_IDENTIFIER }},
        { clang::tok::numeric_constant, { "", cxx::TOK_DEC_INT_LITERAL }},
        { clang::tok::char_constant, { "", cxx::TOK_CHAR_LITERAL }},
        { clang::tok::wide_char_constant, { "", cxx::TOK_WCHAR_LITERAL }},
        { clang::tok::utf16_char_constant, { "", cxx::TOK_U16_CHAR_LITERAL }},
        { clang::tok::utf32_char_constant, { "", cxx::TOK_U32_CHAR_LITERAL }},
        { clang::tok::string_literal, { "", cxx::TOK_STR_LITERAL }},
        { clang::tok::wide_string_literal, { "", cxx::TOK_WSTR_LITERAL }},
        { clang::tok::utf8_string_literal, { "", cxx::TOK_U8_STR_LITERAL }},
        { clang::tok::utf16_string_literal, { "", cxx::TOK_U16_STR_LITERAL }},
        { clang::tok::utf32_string_literal, { "", cxx::TOK_U32_STR_LITERAL }}
};

//--------------------------------------

Token &
ClangLexerAdaptor::lex(
        Token &out_token
)
{
        clang::Token tmp;
        pp_.Lex(tmp);
        convert(tmp, out_token);
        return out_token;
}

//--------------------------------------

const char *
ClangLexerAdaptor::tokenKindName(
        TokenKind kind
) const
{
        return cxx::tokenKindName(kind);
}

//--------------------------------------

void
ClangLexerAdaptor::convert(
        const clang::Token &from,
        Token              &to
)
{
        auto i = TOKEN_CONV.find(from.getKind());

        if (i != TOKEN_CONV.end()) {
                to.setKind(i->second.to_kind);
                to.setSpelling(i->second.spelling);
        } else {
                to.setKind(TOK_NULL);
                to.setSpelling("");
        }

        uint8_t flags = 0;

        if (from.hasLeadingSpace()) {
                flags |= Token::TF_SPACE_BEFORE;
        }
        if (from.isAtStartOfLine()) {
                flags |= Token::TF_STARTS_LINE;
        }

        to.setFlags(flags);
        to.setOffset(from.getLocation().getRawEncoding());

        if (to.spelling().empty()) {
                if (from.isLiteral()) {
                        to.setSpelling({ from.getLiteralData(),
                                         from.getLength() });
                } else if (from.isAnyIdentifier()) {
                        auto name = from.getIdentifierInfo()->getName();
                        to.setSpelling({ name.data(), name.size() });
                }
        }
}

//--------------------------------------

TokenKind
ClangIdentifierTable::intern(
        const u8string_view &text,
        u8string_view       &out_text
)
{
        const Keywords::value_type *keyword = lookUpKeyword(text);

        if (keyword) {
                out_text = keyword->first;
                return keyword->second;
        }

        auto name = clang_id_table_.get({ text.char_data(), text.bytes() })
                                   .getName();
        out_text = { name.data(), name.size() };
        return cxx::TOK_IDENTIFIER;
}

//--------------------------------------

clang::SourceLocation
tokenLoc(
        const Token                 &token,
        const clang::SourceLocation &anchor
)
{
        return anchor.getLocWithOffset(token.offset());
}

//--------------------------------------

static clang::NamedDecl &
adjustDecl(
        clang::NamedDecl &decl
)
{
        clang::NamedDecl *adjusted = &decl;

        if (clang::TemplateDecl::classof(adjusted)) {
                auto templ = static_cast<clang::TemplateDecl *>(adjusted);
                adjusted = templ->getTemplatedDecl();
        } else if (clang::TypedefNameDecl::classof(adjusted)) {
                clang::TypedefNameDecl *type_def;
                type_def = static_cast<clang::TypedefNameDecl*>(adjusted);

                clang::QualType type     = type_def->getUnderlyingType();
                auto            tag_type = type->getAs<clang::TagType>();

                if (tag_type) {
                        adjusted = tag_type->getDecl();
                }
        }

        return *adjusted;
}

//--------------------------------------

bool
lookupName(
        clang::LookupResult &lookup,
        clang::DeclContext  *scope,
        clang::Sema         &sema,
        bool                 qualified
)
{
        bool found = false;

        if (!scope) {
                scope = sema.Context.getTranslationUnitDecl();
        }

        while (true) {
                found = sema.LookupQualifiedName(lookup, scope, qualified);

                if (found || qualified) {
                        break;
                }

                do {
                        scope = scope->getLookupParent();
                } while (scope && scope->isTransparentContext());

                if (!scope) {
                        break;
                }
        }

        return found;
}

//--------------------------------------

clang::DeclContext *
resolveNestedNameSpecifier(
        ASTNode::iterator   nest,
        clang::DeclContext *scope,
        clang::Sema        &sema
)
{
        bool qualified = nest.firstToken()->is(parse::cxx::TOK_COLONCOLON);

        if (!scope || qualified) {
                scope = sema.Context.getTranslationUnitDecl();
        }

        clang::LookupResult lookup(sema, clang::DeclarationName(),
                                   clang::SourceLocation(),
                                   clang::Sema::LookupNestedNameSpecifierName);

        for (ASTNode::iterator name: nest.productions()) {
                lookup.clear();
                lookup.setLookupName(&sema.Context.Idents.get(name.content()));

                qualified = lookupName(lookup, scope, sema, qualified);

                if (!qualified || !lookup.isSingleResult()) {
                        return nullptr;
                }

                scope = clang::Decl::castToDeclContext(
                                        &adjustDecl(*lookup.getFoundDecl()));
                if (!scope) {
                        return nullptr;
                }

                auto data = new NestedNameClangDeclCtx;
                data->dctx = scope;
                name->setAuxData(data);  // takes ownership
        }

        return scope;
}

//--------------------------------------

clang::NamedDecl *
findDecl(
        CXXParser          &cxx,
        ASTNode::iterator   decl_spec_seq,
        ASTNode::iterator   declarator,
        clang::DeclContext *scope,
        clang::Sema        &sema
)
{
        CXXParser::DeclSpecifier *ds_data = nullptr;

        if (decl_spec_seq) {
                ds_data = cxx.get<CXXParser::DeclSpecifier>(decl_spec_seq);
                if (!ds_data) {
                        return nullptr;  // wrong production
                }
        }

        ASTNode::iterator decl_id;
        clang::QualType   decl_type;

        if (declarator) {
                /* don't accept, for example, abstract-declarators because a
                   declarator-id is required */
                if (!declarator->is(cxx.declarator)) {
                        return nullptr;
                }

                decl_id = declarator.find(cxx.declarator_id, 1);

                if (decl_spec_seq) {
                        decl_type = toQualType(cxx, decl_spec_seq, scope, sema);
                        decl_type = toQualType(cxx, decl_type, declarator,
                                               scope, sema);
                }
        } else if (decl_spec_seq) {
                /* treat the input as a standalone name;
                   it must resolve to a unique declaration */
                if (ds_data->type_spec == CXXParser::DeclSpecifier::OTHER) {
                        decl_id = decl_spec_seq;
                }
        }

        if (!decl_id) {
                return nullptr;
        }

        DeclFindResults found = findDeclsByName(cxx, decl_id, scope, sema);

        if (decl_type.isNull()) {
                /* decl-specifier-seq was unresolvable or unspecified,
                   e.g. bare name, constructor or destructor */
                if (found.decls.size() == 1) {
                        return found.decls.front();
                } else if (!decl_spec_seq && found.found_ctors) {
                        // constructor look-up
                        decl_type = toQualType(cxx, sema.Context.VoidTy,
                                               declarator, scope, sema);
                } else {
                        return nullptr;
                }
        }

        clang::NamedDecl *result = nullptr;

        for (clang::NamedDecl *candidate: found.decls) {
                clang::DeclaratorDecl *ddecl
                       = llvm::dyn_cast<clang::DeclaratorDecl>(candidate);

                if (ddecl) {
                        clang::QualType        actual_type;
                        clang::TypeSourceInfo *tsi = ddecl->getTypeSourceInfo();

                        if (tsi) {
                                // == nullptr for some decls, e.g. destructors
                                actual_type = tsi->getType();
                        }

                        if ((decl_type.isNull() && actual_type.isNull())
                            || (!decl_type.isNull() && !actual_type.isNull()
                                && decl_type.getCanonicalType()
                                           == actual_type.getCanonicalType())) {
                                result = candidate;
                                break;
                        }
                }
        }

        return result;
}

//--------------------------------------

DeclFindResults
findDeclsByName(
        CXXParser           &cxx,
        ASTNode::iterator    name_parent,
        clang::DeclContext  *scope,
        clang::Sema         &sema
)
{
        clang::LookupResult lookup(sema, clang::DeclarationName(),
                                   clang::SourceLocation(),
                                   clang::Sema::LookupNestedNameSpecifierName);
        ASTNode::iterator   id;

        if (name_parent.node()->is(cxx.type_specifier)) {
                id = name_parent.find(cxx.type_name, 1);
        } else {
                id = name_parent.find(cxx.unqualified_id, 1);
        }

        DeclFindResults results;

        if (!id) {
                return results;
        }

        if (id->auxData()) {
                results.decls.push_back(
                        boost::static_pointer_cast<IdentifierClangDecl>(
                                id->auxData())->decl);
                return results;
        }

        ASTNode::iterator nest = name_parent.find(cxx.nested_name_specifier, 2);

        if (nest) {
                scope = resolveNestedNameSpecifier(nest, scope, sema);
                if (!scope) {
                        return results;
                }
        } else if (!scope) {
                scope = sema.Context.getTranslationUnitDecl();
        }

        if (clang::RecordDecl::classofKind(scope->getDeclKind())) {
                ASTNode::iterator identifier = id.find(cxx.identifier, 1);

                auto rdecl = static_cast<clang::CXXRecordDecl *>(
                                       clang::Decl::castFromDeclContext(scope));

                llvm::StringRef tmp  (rdecl->getIdentifier()->getName());
                u8string_view   rname(tmp.data(), tmp.size());

                if (identifier && (*identifier.firstToken() == rname)) {
                        /* resolve constructor / destructor names;
                           clang handles these specially */
                        if (*id.firstToken() == cxx::TOK_TILDE) {
                                clang::CXXDestructorDecl *d
                                        = sema.LookupDestructor(rdecl);
                                if (d) {
                                        results.found_dtor = true;
                                        results.decls.push_back(d);
                                }
                        } else {
                                results.found_ctors = true;

                                for (auto c: sema.LookupConstructors(rdecl)) {
                                        results.decls.push_back(c);
                                }
                        }

                        return results;
                }

                lookup.clear(clang::Sema::LookupMemberName);
        } else {
                lookup.clear(clang::Sema::LookupOrdinaryName);
        }

        clang::ASTContext           &ast   = sema.Context;
        clang::DeclarationNameTable &names = ast.DeclarationNames;
        clang::DeclarationName       name;

        if (id.node()->is(cxx.operator_function_id)) {
                name = names.getCXXOperatorName(
                                        getOverloadedOperatorKind(cxx, id));
        } else if (id.node()->is(cxx.conversion_function_id)) {
                auto conv_type_id = id.find(cxx.type_specifier_seq, 2);
                auto conv_type    = toQualType(cxx, conv_type_id, scope, sema);

                if (conv_type.isNull()) {
                        return results;
                }

                auto conv_declarator = id.find(cxx.conversion_declarator, 2);

                if (conv_declarator && !conv_type.isNull()) {
                        conv_type = toQualType(cxx, conv_type, conv_declarator,
                                               scope, sema);
                }
                if (conv_type.isNull()) {
                        return results;
                }

                name = names.getCXXConversionFunctionName(
                                        ast.getCanonicalType(conv_type));
        } else if (id.node()->is(cxx.literal_operator_id)) {
                ASTNode::iterator identifier = id.find(cxx.identifier, 1);
                name = names.getCXXLiteralOperatorName(
                                        &ast.Idents.get(identifier.content()));
        } else {
                name = &ast.Idents.get(id.content());
        }

        lookup.setLookupName(name);
        lookupName(lookup, scope, sema, static_cast<bool>(nest));

        if (lookup.isSingleResult()) {
                auto data = new IdentifierClangDecl;
                data->decl = lookup.getFoundDecl();
                id->setAuxData(data);
        }

        for (clang::NamedDecl *result: lookup) {
                results.decls.push_back(result);
        }

        return results;
}

//--------------------------------------

clang::OverloadedOperatorKind
getOverloadedOperatorKind(
        CXXParser         &cxx,
        ASTNode::iterator  operator_func_id
)
{
        if (!operator_func_id.node()->is(cxx.operator_function_id)) {
                return clang::OO_None;
        }

        Token *last = operator_func_id.lastToken(),
              *t    = operator_func_id.firstToken()->next();
                                                // skip "operator" keyword
        switch (t->kind()) {
        case cxx::TOK_PLUS:         return clang::OO_Plus;
        case cxx::TOK_MINUS:        return clang::OO_Minus;
        case cxx::TOK_STAR:         return clang::OO_Star;
        case cxx::TOK_SLASH:        return clang::OO_Slash;
        case cxx::TOK_PERCENT:      return clang::OO_Percent;
        case cxx::TOK_CARET:        return clang::OO_Caret;
        case cxx::TOK_AMP:          return clang::OO_Amp;
        case cxx::TOK_PIPE:         return clang::OO_Pipe;
        case cxx::TOK_TILDE:        return clang::OO_Tilde;
        case cxx::TOK_EXCLAIM:      return clang::OO_Exclaim;
        case cxx::TOK_EQUAL:        return clang::OO_Equal;
        case cxx::TOK_LESS:         return clang::OO_Less;
        case cxx::TOK_GREATER:      return clang::OO_Greater;
        case cxx::TOK_PLUSEQUAL:    return clang::OO_PlusEqual;
        case cxx::TOK_MINUSEQUAL:   return clang::OO_MinusEqual;
        case cxx::TOK_STAREQUAL:    return clang::OO_StarEqual;
        case cxx::TOK_SLASHEQUAL:   return clang::OO_SlashEqual;
        case cxx::TOK_PERCENTEQUAL: return clang::OO_PercentEqual;
        case cxx::TOK_CARETEQUAL:   return clang::OO_CaretEqual;
        case cxx::TOK_AMPEQUAL:     return clang::OO_AmpEqual;
        case cxx::TOK_PIPEEQUAL:    return clang::OO_PipeEqual;
        case cxx::TOK_LSHIFT:       return clang::OO_LessLess;
        case cxx::TOK_RSHIFT:       return clang::OO_GreaterGreater;
        case cxx::TOK_LSHIFTEQUAL:  return clang::OO_LessLessEqual;
        case cxx::TOK_EQUALEQUAL:   return clang::OO_EqualEqual;
        case cxx::TOK_EXCLAIMEQUAL: return clang::OO_ExclaimEqual;
        case cxx::TOK_LESSEQUAL:    return clang::OO_LessEqual;
        case cxx::TOK_GREATEREQUAL: return clang::OO_GreaterEqual;
        case cxx::TOK_AMPAMP:       return clang::OO_AmpAmp;
        case cxx::TOK_PIPEPIPE:     return clang::OO_PipePipe;
        case cxx::TOK_PLUSPLUS:     return clang::OO_PlusPlus;
        case cxx::TOK_MINUSMINUS:   return clang::OO_MinusMinus;
        case cxx::TOK_COMMA:        return clang::OO_Comma;
        case cxx::TOK_ARROWSTAR:    return clang::OO_ArrowStar;
        case cxx::TOK_ARROW:        return clang::OO_Arrow;
        case cxx::TOK_LPAREN:       return clang::OO_Call;
        case cxx::TOK_RPAREN:       return clang::OO_Subscript;
        case cxx::TOK_RSHIFTEQUAL:  return clang::OO_GreaterGreaterEqual;
        case cxx::TOK_KW_NEW:
                return (t == last) ? clang::OO_New : clang::OO_Array_New;
        case cxx::TOK_KW_DELETE:
                return (t == last) ? clang::OO_Delete : clang::OO_Array_Delete;
        default:
                throw std::runtime_error("Invalid overloaded operator name "
                                         + operator_func_id.content());
        }
}

//--------------------------------------

clang::TypeDecl *
findTypeByName(
        CXXParser          &cxx,
        ASTNode::iterator   type_name,
        clang::DeclContext *scope,
        clang::Sema        &sema
)
{
        DeclFindResults found = findDeclsByName(cxx, type_name, scope, sema);

        if (found.decls.size() == 1) {
                return llvm::dyn_cast<clang::TypeDecl>(found.decls.front());
        } else {
                return nullptr;
        }
}

//--------------------------------------

clang::QualType
applyCVRQualifiers(
        uint8_t         qualifiers,
        clang::QualType type
)
{
        if (qualifiers & CXXParser::CONST) {
                type.addConst();
        }
        if (qualifiers & CXXParser::VOLATILE) {
                type.addVolatile();
        }
        if (qualifiers & CXXParser::RESTRICT) {
                type.addRestrict();
        }
        return type;
}

//--------------------------------------

clang::QualType
toQualType(
        CXXParser          &cxx,
        ASTNode::iterator   decl_spec_seq,
        clang::DeclContext *scope,
        clang::Sema        &sema
)
{
        auto *data = cxx.get<CXXParser::DeclSpecifier>(decl_spec_seq);

        if (!data) {
                return {};
        }

        clang::ASTContext &ast        = sema.Context;
        clang::QualType    clang_type;

        switch (data->type_spec) {
        case CXXParser::DeclSpecifier::VOID:
                clang_type = ast.VoidTy;
                break;
        case CXXParser::DeclSpecifier::AUTO:
                clang_type = ast.getAutoDeductType();
                break;
        case CXXParser::DeclSpecifier::DECLTYPE:
                break;  // XXX work out how to support this
        case CXXParser::DeclSpecifier::BOOL:
                clang_type = ast.BoolTy;
                break;
        case CXXParser::DeclSpecifier::CHAR:
                switch (data->sign_spec) {
                case CXXParser::DeclSpecifier::SIGNED:
                        clang_type = ast.SignedCharTy;
                        break;
                case CXXParser::DeclSpecifier::UNSIGNED:
                        clang_type = ast.UnsignedCharTy;
                        break;
                case CXXParser::DeclSpecifier::NO_SIGN:
                        clang_type = ast.CharTy;
                        break;
                }
                break;
        case CXXParser::DeclSpecifier::CHAR16_T:
                clang_type = ast.Char16Ty;
                break;
        case CXXParser::DeclSpecifier::CHAR32_T:
                clang_type = ast.Char32Ty;
                break;
        case CXXParser::DeclSpecifier::WCHAR_T:
                clang_type = ast.WCharTy;
                break;
        case CXXParser::DeclSpecifier::INT:
        case CXXParser::DeclSpecifier::NO_TYPE:
                switch (data->sign_spec) {
                case CXXParser::DeclSpecifier::NO_SIGN:
                case CXXParser::DeclSpecifier::SIGNED:
                        switch (data->size_spec) {
                        case CXXParser::DeclSpecifier::NO_SIZE:
                                clang_type = ast.IntTy;
                                break;
                        case CXXParser::DeclSpecifier::SHORT:
                                clang_type = ast.ShortTy;
                                break;
                        case CXXParser::DeclSpecifier::LONG:
                                clang_type = ast.LongTy;
                                break;
                        case CXXParser::DeclSpecifier::LONG_LONG:
                                clang_type = ast.LongLongTy;
                                break;
                        }
                        break;
                case CXXParser::DeclSpecifier::UNSIGNED:
                        switch (data->size_spec) {
                        case CXXParser::DeclSpecifier::NO_SIZE:
                                clang_type = ast.UnsignedIntTy;
                                break;
                        case CXXParser::DeclSpecifier::SHORT:
                                clang_type = ast.UnsignedShortTy;
                                break;
                        case CXXParser::DeclSpecifier::LONG:
                                clang_type = ast.UnsignedLongTy;
                                break;
                        case CXXParser::DeclSpecifier::LONG_LONG:
                                clang_type = ast.UnsignedLongLongTy;
                                break;
                        }
                        break;
                }
                break;
        case CXXParser::DeclSpecifier::FLOAT:
                clang_type = ast.FloatTy;
                break;
        case CXXParser::DeclSpecifier::DOUBLE:
                switch (data->size_spec) {
                case CXXParser::DeclSpecifier::NO_SIZE:
                        clang_type = ast.DoubleTy;
                        break;
                case CXXParser::DeclSpecifier::LONG:
                        clang_type = ast.LongDoubleTy;
                        break;
                default:
                        break;
                }
                break;
        case CXXParser::DeclSpecifier::NULLPTR_T:
                clang_type = ast.NullPtrTy;
                break;
        case CXXParser::DeclSpecifier::OTHER:
                if (data->type_prod) {
                        ASTNode::iterator type_name
                                = decl_spec_seq.find(*data->type_prod, 1);

                        const clang::TypeDecl *type_decl = nullptr;

                        if (type_name) {
                                type_decl = findTypeByName(cxx, type_name,
                                                           scope, sema);
                        }
                        if (type_decl) {
                                clang_type = ast.getTypeDeclType(type_decl);
                        }
                }
                break;
        }

        if (!clang_type.isNull()) {
                clang_type = applyCVRQualifiers(data->cvr_qual, clang_type);
        }

        return clang_type;
}

//--------------------------------------

clang::QualType
toQualType(
        CXXParser          &cxx,
        clang::QualType     decl_spec_type,
        ASTNode::iterator   declarator,
        clang::DeclContext *scope,
        clang::Sema        &sema
)
{
        auto *data = cxx.get<CXXParser::Declarator>(declarator);

        if (!data) {
                return {};
        }

        ASTNode::iterator declarator_id = declarator.find(cxx.declarator_id, 1);

        if (declarator_id) {
                ASTNode::iterator nest
                        = declarator_id.find(cxx.nested_name_specifier, 2);

                if (nest) {
                        scope = resolveNestedNameSpecifier(nest, scope, sema);
                        if (!scope) {
                                return {};
                        }
                }
        }

        clang::QualType type = decl_spec_type;

        if (data->begin_parms) {
                ASTNode::iterator trail_ret
                        = declarator.find(cxx.trailing_return_type, 1);

                if (trail_ret) {
                        type = {};

                        ASTNode::iterator trail_type_spec =
                             trail_ret.find(cxx.trailing_type_specifier_seq, 1);

                        if (trail_type_spec) {
                                type = toQualType(cxx, trail_type_spec,
                                                  scope, sema);
                        }
                        if (type.isNull()) {
                                return type;
                        }

                        ASTNode::iterator abstract_declarator =
                               trail_ret.findChild(cxx.abstract_declarator, 1);

                        if (abstract_declarator) {
                                type = toQualType(cxx, type,
                                              abstract_declarator, scope, sema);
                        }
                }
        }

        if (type.isNull()) {
                return type;
        }

        ASTNode::iterator inner;
        bool              is_parm_pack = false;

        for (ASTNode::iterator part: declarator.productions()) {
                if (part->is(cxx.ptr_operator)) {
                        type = buildPtrOrRefType(cxx, part, type, scope, sema);
                } else if (part->is(cxx.parameters_and_qualifiers)) {
                        type = buildFunctionType(cxx, declarator,
                                                 part, type, scope, sema);
                } else if (part->is(cxx.array_declarator)) {
                        type = buildArrayType(part, type, scope, sema);
                } else if (part->is(cxx.nested_declarator)
                                || part->is(cxx.nested_abstract_declarator)) {
                        inner = part;
                } else if (part->is(cxx.declarator_id)
                                && part.firstToken()->is(cxx::TOK_ELLIPSIS)) {
                        is_parm_pack = true;
                } else if (CXXParser::DeclaratorPart::isParmPackOperator(cxx,
                                                                *part.node())) {
                        is_parm_pack = true;
                }

                if (type.isNull()) {
                        return type;
                }
        }

        if (is_parm_pack) {
                type = sema.Context.getPackExpansionType(
                                        type, llvm::Optional<unsigned int>());
        }
        if (inner) {  // handle nested-declarator if any
                type = toQualType(cxx, type, inner, scope, sema);
        }

        return type;
}

//--------------------------------------

clang::QualType
buildPtrOrRefType(
        CXXParser           &cxx,
        ASTNode::iterator    ptr_operator,
        clang::QualType      target_type,
        clang::DeclContext  *scope,
        clang::Sema         &sema
)
{
        auto *data = cxx.get<CXXParser::DeclaratorPart>(ptr_operator);

        if (!data) {
                return {};
        }

        clang::DeclContext *mem_ptr_class;
        clang::QualType     type;

        switch (ptr_operator.firstToken()->kind()) {
        case cxx::TOK_STAR:
                type = sema.Context.getPointerType(target_type);
                type = applyCVRQualifiers(data->qualifiers, type);
                break;
        case cxx::TOK_AMP:
                type = sema.Context.getLValueReferenceType(target_type);
                break;
        case cxx::TOK_AMPAMP:
                type = sema.Context.getRValueReferenceType(target_type);
                break;
        default:
                mem_ptr_class = resolveNestedNameSpecifier(
                        ptr_operator.find(cxx.nested_name_specifier),
                        scope, sema);

                if (!mem_ptr_class) {
                        break;
                }
                if (!clang::TypeDecl
                          ::classofKind(mem_ptr_class->getDeclKind())) {
                        break;
                }

                type = sema.Context.getMemberPointerType(target_type,
                        static_cast<clang::TypeDecl *>(
                                clang::Decl::castFromDeclContext(mem_ptr_class))
                                                        ->getTypeForDecl());

                type = applyCVRQualifiers(data->qualifiers, type);
                break;
        }

        return type;
}

//--------------------------------------

clang::QualType
buildFunctionType(
        CXXParser           &cxx,
        ASTNode::iterator    declarator,
        ASTNode::iterator    parameters,
        clang::QualType      return_type,
        clang::DeclContext  *scope,
        clang::Sema         &sema
)
{
        clang::FunctionProtoType::ExtProtoInfo extra;

        CXXParser::DeclaratorPart *data = nullptr;

        ASTNode::iterator parm_clause
                = parameters.find(cxx.parameter_declaration_clause, 1);

        if (parm_clause) {
                data = cxx.get<CXXParser::DeclaratorPart>(parm_clause);
        }
        if (!data) {
                return {};
        }

        if (data->qualifiers & CXXParser::CONST) {
                extra.TypeQuals |= clang::Qualifiers::Const;
        }
        if (data->qualifiers & CXXParser::VOLATILE) {
                extra.TypeQuals |= clang::Qualifiers::Volatile;
        }
        if (data->qualifiers & CXXParser::RESTRICT) {
                extra.TypeQuals |= clang::Qualifiers::Restrict;
        }

        if (data->qualifiers & CXXParser::LVAL_REF) {
                extra.RefQualifier = clang::RQ_LValue;
        } else if (data->qualifiers & CXXParser::RVAL_REF) {
                extra.RefQualifier = clang::RQ_RValue;
        }
        if (data->variadic) {
                extra.Variadic = true;
        }
        if (declarator && declarator.find(cxx.trailing_return_type, 1)) {
                extra.HasTrailingReturn = true;
        }
        vector<clang::QualType> parm_types;

        parm_types.reserve(data->count);

        for (ASTNode::iterator &parm: parm_clause.productions()) {
                auto decl_spec_seq = parm.find(cxx.decl_specifier_seq, 1);
                assert(static_cast<bool>(decl_spec_seq));

                clang::QualType type = toQualType(cxx, decl_spec_seq,
                                                  scope, sema);
                if (type.isNull()) {
                        return type;
                }
                auto parm_declarator = parm.find(cxx.declarator, 1);
                if (!parm_declarator) {
                        parm_declarator = parm.find(cxx.abstract_declarator, 1);
                }
                if (parm_declarator) {
                        type = toQualType(cxx, type, parm_declarator,
                                          scope, sema);
                        if (type.isNull()) {
                                return type;
                        }
                }

                parm_types.push_back(type);
        }

        return sema.Context.getFunctionType(return_type, parm_types, extra);
}

//--------------------------------------

clang::QualType
buildArrayType(
        ASTNode::iterator   /* array_declarator */,
        clang::QualType     element_type,
        clang::DeclContext */* scope */,
        clang::Sema        &sema
)
{
        return sema.Context.getIncompleteArrayType(element_type,
                                                   clang::ArrayType::Normal, 0);
}

} // namespace parse
} // namespace wr
