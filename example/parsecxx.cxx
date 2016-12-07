#include <memory>
#include <wrutil/uiostream.h>
#include <wrparse/cxx/CXXLexer.h>
#include <wrparse/cxx/CXXParser.h>
#include <wrparse/cxx/CXXTokenKinds.h>
#include <wrparse/SPPFOutput.h>


extern int run(int argc, const char **argv,
               int (*action)(std::istream &input,
                             const wr::parse::CXXOptions &options, int status));

//--------------------------------------

struct DiagnosticPrinter : public wr::parse::DiagnosticHandler
{
        virtual void onDiagnostic(const wr::parse::Diagnostic &d) override
        {
                wr::print(wr::uerr, "%u:%u: %s: %s\n",
                          d.line(), d.column(), d.describeCategory(), d.text());
        }
};

//--------------------------------------

static int
parseCXX(
        std::istream                &input,
        const wr::parse::CXXOptions &options,
        int                          status
)
{
        wr::parse::CXXLexer  lexer (options, input);
        wr::parse::CXXParser parser(lexer);
        DiagnosticPrinter    diag_out;

        parser.addDiagnosticHandler(diag_out);
        parser.enableDebug(getenv("WR_DEBUG_PARSER") != nullptr);

        while (input.good()) {
                wr::parse::SPPFNode::Ptr result
                                         = parser.parse(parser.declaration);
                if (result) {
                        wr::uout << *result << std::endl;
                        size_t token_count = 0;
                        for (auto *t = result->firstToken(); t; t = t->next()) {
                                ++token_count;
                        }
                }

                if (parser.errorCount()) {
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
