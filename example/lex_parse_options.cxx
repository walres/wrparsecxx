#include <errno.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <locale>
#include <memory>
#include <streambuf>
#include <wrutil/codecvt.h>
#include <wrutil/filesystem.h>
#include <wrutil/Format.h>
#include <wrutil/uiostream.h>
#include <wrutil/u8string_view.h>
#include <wrparse/cxx/CXXLexer.h>

#include "lex_parse_options.h"


static int process(std::istream &input,
              int (*action)(std::istream &, const wr::parse::CXXOptions &, int),
              int status);

//--------------------------------------

std::string                           prog_name;
std::unique_ptr<wr::u8buffer_convert> transcode_buf;
std::vector<wr::u8string_view>        input_files;
wr::parse::cxx::Language              language = 0;
wr::parse::cxx::Features              features = 0;

//--------------------------------------

static const wr::Option program_options[] = {
        { "-digraphs", []() { features |= wr::parse::cxx::DIGRAPHS; } },
        { "-trigraphs", []() { features |= wr::parse::cxx::TRIGRAPHS; } },
        { "-fbinary-literals",
                []() { features |= wr::parse::cxx::BINARY_LITERALS; } },
        { "-fdollars-in-identifiers",
                []() { features |= wr::parse::cxx::IDENTIFIER_DOLLARS; } },
        { "-finline-functions",
                []() { features |= wr::parse::cxx::INLINE_FUNCTIONS; } },
        { "-fline-comments",
                []() { features |= wr::parse::cxx::LINE_COMMENTS; } },
        { "-flong-long", []() { features |= wr::parse::cxx::LONG_LONG; } },
        { "-fucns", []() { features |= wr::parse::cxx::UCNS; } },

        { "-finput-locale", wr::Option::NON_EMPTY_ARG_REQUIRED,
                [](wr::u8string_view arg) {
                        transcode_buf.reset(
                                new wr::u8buffer_convert(
                                        nullptr,
                                        new wr::codecvt_utf8_narrow(
                                                std::locale(arg.char_data())
                                        )
                                ));
                } },

        { "-std=", wr::Option::NON_EMPTY_ARG_REQUIRED,
                [](wr::u8string_view opt, wr::u8string_view arg) {
                        auto selected = wr::parse::CXXOptions::standard(arg);
                        if (!selected.first) {
                                throw wr::Option::InvalidArgument(
                                              "unrecognised language standard");
                        } else {
                                language &= ~selected.second;
                                language |= selected.first;
                        }
                } },

        { "-x", wr::Option::NON_EMPTY_ARG_REQUIRED,
                [](wr::u8string_view arg) {
                        auto selected = wr::parse::CXXOptions::language(arg);
                        if (!selected.first) {
                                throw wr::Option::InvalidArgument(
                                                "unrecognised language");
                        } else if (!(language & selected.second)) {
                                language |= selected.first;
                        }
                } },

        { "", [](wr::u8string_view arg) { input_files.push_back(arg); } },
        { "-", []() { input_files.push_back("-"); } }
};

//--------------------------------------

int
run(
        int          argc,
        const char **argv,
        int        (*action)(std::istream &, const wr::parse::CXXOptions &, int)
)
try {
        prog_name = wr::to_u8string(wr::path(argv[0]).filename());

        auto converted_args = wr::Option::localToUTF8(argc, argv);
        argv = converted_args.first.data();

        wr::Option::parse(program_options, argc, argv, 1);

        if (!language) {
                language = wr::parse::cxx::C_LATEST
                                | wr::parse::cxx::CXX_LATEST;
        }

        if (input_files.empty()) {
                return process(std::cin, action, EXIT_SUCCESS);
        }

        int status = EXIT_SUCCESS;

        for (const wr::u8string_view &file_name: input_files) {
                if (file_name == "-") {
                        status = process(wr::uin, action, status);
                        continue;
                }

                auto              path           = wr::u8path(file_name);
                std::string       failure_reason;
                wr::fs_error_code error;

                if (wr::is_directory(path, error)) {
                        failure_reason = wr::u8strerror(EISDIR);
                } else if (error) {
                        failure_reason =
                                wr::utf8_narrow_cvt().to_utf8(error.message());
                } else {
                        std::ifstream input_file(path.native());
                        if (input_file.is_open()) {
                                status = process(input_file, action, status);
                        } else {
                                failure_reason = u8"reason unknown";
                        }
                }

                if (!failure_reason.empty()) {
                        wr::print(wr::uerr,
                                  u8"%s: cannot open file \"%s\": %s\n",
                                  argv[0], file_name, failure_reason);
                        status = EXIT_FAILURE;
                }
        }

        return status;
} catch (const std::exception &err) {
        wr::uerr << prog_name << ": "
                 << wr::utf8_narrow_cvt().to_utf8(err.what()) << '\n';
        return EXIT_FAILURE;
}

//--------------------------------------

static int
process(
        std::istream  &input,
        int          (*action)(std::istream &,
                               const wr::parse::CXXOptions &, int),
        int            status
)
{
        std::istream input2(input.rdbuf());

        if (transcode_buf) {
                transcode_buf->rdbuf(input.rdbuf());
                input2.rdbuf(transcode_buf.get());
        } // else assume straight UTF-8 input

        wr::parse::CXXOptions options(language, features);
        return (*action)(input2, options, status);
}
