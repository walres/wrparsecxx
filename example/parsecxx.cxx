#include <memory>
#include <wrutil/uiostream.h>
#include <wrparse/cxx/CXXLexer.h>
#include <wrparse/cxx/CXXParser.h>
#include <wrparse/cxx/CXXTokenKinds.h>
#include <wrparse/SPPFOutput.h>


extern int run(int argc, const char **argv,
               int (*action)(std::istream &input,
                             const wr::parse::CXXOptions &options, int status));

static int
parseCXX(
        std::istream                &input,
        const wr::parse::CXXOptions &options,
        int                          status
)
{
        wr::parse::CXXLexer  lexer (options, input);
        wr::parse::CXXParser parser(lexer);

        parser.enableDebug(getenv("WR_DEBUG_PARSER") != nullptr);

        while (!input.bad() && !parser.nextToken()->is(wr::parse::TOK_EOF)) {
                wr::parse::SPPFNode::Ptr result
                                         = parser.parse(parser.declaration);
                if (result) {
                        wr::uout << *result << std::endl;
                        size_t token_count = 0;
                        for (auto *t = result->firstToken(); t; t = t->next()) {
                                ++token_count;
                        }
                } else {
                        wr::uerr << "parse failed\n";
                        status = EXIT_FAILURE;
                        parser.reset();
                }

                lexer.clearStorage();
        }

        if (input.bad()) {
                status = EXIT_FAILURE;
        }

        return status;
}

int main(int argc, const char **argv) { return run(argc, argv, &parseCXX); }
