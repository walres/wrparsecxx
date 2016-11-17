#include <wrutil/uiostream.h>
#include <wrparse/cxx/CXXTokenKinds.h>
#include <wrparse/cxx/CXXLexer.h>


extern int run(int argc, const char **argv,
               int (*action)(std::istream &input,
                             const wr::parse::CXXOptions &options, int status));


static int
lex(
        std::istream                &input,
        const wr::parse::CXXOptions &options,
        int                          status
)
{
        wr::parse::CXXLexer lexer(options, input);
        wr::parse::Token    token;

        do {
                lexer.lex(token);

                switch (token.kind()) {
                case wr::parse::cxx::TOK_WHITESPACE:
                        if (token.spelling() == "\n") {
                                wr::uout << '\n';
                        }
                        // fall through
                case wr::parse::TOK_EOF:
                case wr::parse::cxx::TOK_COMMENT:
                        break;
                default:
                        if (token.flags() & wr::parse::TF_SPACE_BEFORE) {
                                wr::uout << '_';
                        }

                        wr::uout << lexer.tokenKindName(token.kind());

                        if (token.kind() >= wr::parse::cxx::TOK_IDENTIFIER) {
                                wr::uout << '(' << token.spelling() << ')';
                        }

                        wr::uout << ' ';
                }
        } while (!input.bad() && (token.kind() != wr::parse::TOK_EOF));

        if (input.bad()) {
                status = EXIT_FAILURE;
        }

        wr::uout << std::endl;
        return status;
}

int main(int argc, const char **argv) { return run(argc, argv, &lex); }
