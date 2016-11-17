/**
 * \file ClangInterface.h
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
#ifndef WRPARSECXX_CLANG_INTERFACE_H
#define WRPARSECXX_CLANG_INTERFACE_H

#include <list>
#include <wrparse/AST.h>
#include <wrparse/Lexer.h>
#include <wrparse/cxx/CXXLexer.h>


namespace clang {


class ASTContext;       // clang/AST/ASTContext.h
class DeclContext;      // clang/AST/DeclBase.h
class IdentifierTable;  // clang/Basic/IdentifierTable.h
class Lexer;            // clang/Lex/Lexer.h
class LookupResult;     // clang/Sema/Lookup.h
class NamedDecl;        // clang/AST/Decl.h
class Preprocessor;     // clang/Lex/Preprocessor.h
class QualType;         // clang/AST/Type.h
class Sema;             // clang/Sema/Sema.h
class SourceLocation;   // clang/Basic/SourceLocation.h
class TypeDecl;         // clang/AST/Decl.h


} // namespace clang


namespace wr {
namespace parse {
namespace cxx {


class CXXParser;
class ASTNode;


class ClangLexerAdaptor :
        public Lexer
{
public:
        ClangLexerAdaptor(clang::Preprocessor &pp) : pp_(pp) {}

        virtual Token &lex(Token &out_token) override;
        virtual const char *tokenKindName(TokenKind kind) const override;

        static void convert(const clang::Token &from, Token &to);

private:
        clang::Preprocessor &pp_;
};

//--------------------------------------

class ClangIdentifierTable :
        public cxx::IdentifierTable
{
public:
        ClangIdentifierTable(clang::IdentifierTable &clang_id_table) :
                clang_id_table_(clang_id_table) {}

        virtual TokenKind intern(const u8string_view &text,
                                 u8string_view &out_text) override;

private:
        clang::IdentifierTable &clang_id_table_;
};

//--------------------------------------

struct NestedNameClangDeclCtx : AuxData
{
        clang::DeclContext *dctx;
};

//--------------------------------------

struct IdentifierClangDecl : AuxData
{
        clang::NamedDecl *decl;
};

//--------------------------------------

clang::SourceLocation tokenLoc(const Token &token,
                               const clang::SourceLocation &anchor);

bool lookupName(clang::LookupResult &lookup, clang::DeclContext *scope,
                clang::Sema &sema, bool qualified);

clang::DeclContext *resolveNestedNameSpecifier(ASTNode::iterator nest,
                                               clang::DeclContext *scope,
                                               clang::Sema &sema);

clang::NamedDecl *findDecl(CXXParser &cxx, ASTNode::iterator decl_spec_seq,
                           ASTNode::iterator declarator,
                           clang::DeclContext *scope, clang::Sema &sema);


typedef std::list<clang::NamedDecl *> DeclList;

struct DeclFindResults
{
        DeclList decls;

        union
        {
                uint8_t all_flags = 0;

                struct
                {
                        bool found_ctors      : 1,  /**< if \c true then one or
                                                         more class constructors
                                                         were found */
                             found_dtor       : 1;  /**< if \c true then a class
                                                         destructor was found */
                };
        };
};


DeclFindResults findDeclsByName(CXXParser &cxx, ASTNode::iterator name_parent,
                                clang::DeclContext *scope, clang::Sema &sema);

clang::OverloadedOperatorKind getOverloadedOperatorKind(
                        CXXParser &cxx, ASTNode::iterator operator_func_id);

clang::TypeDecl *findTypeByName(CXXParser &cxx, ASTNode::iterator type_name,
                                clang::DeclContext *scope, clang::Sema &sema);

clang::QualType applyCVRQualifiers(uint8_t qualifiers, clang::QualType type);

clang::QualType toQualType(CXXParser &cxx, ASTNode::iterator decl_spec_seq,
                           clang::DeclContext *scope, clang::Sema &sema);

clang::QualType toQualType(CXXParser &cxx, clang::QualType decl_spec_type,
                           ASTNode::iterator declarator,
                           clang::DeclContext *scope, clang::Sema &sema);

clang::QualType buildPtrOrRefType(CXXParser &cxx,
                                  ASTNode::iterator ptr_operator,
                                  clang::QualType target_type,
                                  clang::DeclContext *scope, clang::Sema &sema);

clang::QualType buildFunctionType(CXXParser &cxx, ASTNode::iterator declarator,
                                  ASTNode::iterator parameters,
                                  clang::QualType return_type,
                                  clang::DeclContext *scope, clang::Sema &sema);

clang::QualType buildArrayType(ASTNode::iterator array_declarator,
                               clang::QualType element_type,
                               clang::DeclContext *scope, clang::Sema &sema);


} // namespace cxx
} // namespace parse
} // namespace wr


#endif // !WRPARSECXX_CLANG_INTERFACE_H
