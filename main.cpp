#include <iostream>
#include <fstream>
#include <filesystem>
#include <format>
#include <optional>
#include <vector>
#include <functional>

#define NO_PRINT_H
#include "print.hpp"

#define TOKEN_KIND_LIST(X) \
    X(Comment)             \
    X(String)              \
    X(Char)                \
    X(MultiCharLiteral)    \
    X(Number)              \
    X(Id)                  \
    X(Op)                  \
    X(Punct)               \
    X(Unknown)             \



enum class token_kind {
#define X(name) name,
    TOKEN_KIND_LIST(X)
#undef X
};

struct Token {
    token_kind kind;
    std::string_view text;
    [[nodiscard]]
    std::string_view name() const noexcept;
    int line = 0;
    int col = 0;
};
[[nodiscard]]
std::string_view Token::name() const noexcept {
    switch (kind) {
#define X(name) case token_kind::name: return #name;
        TOKEN_KIND_LIST(X)
#undef X
        default: return "Unknown";
    }
}

struct Cursor {
    const std::string& code;
    std::size_t index = 0;
    int line = 1;
    int col = 1;

    [[nodiscard]]
    char peek(std::size_t offset = 0) const noexcept;
    char next() noexcept;
    char skip(std::size_t n = 1) noexcept;
    [[nodiscard]] bool eof() const noexcept;
    explicit operator bool() const noexcept {return !eof();}

};

bool Cursor::eof() const noexcept {
    return index >= code.size();
}

char Cursor::next() noexcept {
    char c = peek();
    if (c == '\0') return c;
    if (c == '\n') {
        line++;
        col = 1;
    } else col++;
    index++;
    return c;
}
char Cursor::skip(std::size_t n) noexcept {
    char last = '\0';
    while (n-- && peek() != '\0') {
        last = next();
    }
    return last;
}
char Cursor::peek(std::size_t offset) const noexcept {
    std::size_t i = index+offset;
    return i < code.size() ? code[i] : '\0';
}


std::string slurp_file(const std::string& filename) {
    std::ifstream f(filename);
    std::string s;

    // memory reservation for big files
    f.seekg(0, std::ios::end);
    s.reserve(f.tellg());
    f.seekg(0, std::ios::beg);

    s.assign((std::istreambuf_iterator<char>(f)),
             std::istreambuf_iterator<char>());
    f.close();
    return s;
}

void print_usage(FILE* stream, std::string_view program){
    std::println(stream,R"(
Usage: {} [OPTIONS] <input.sc>

OPTIONS:

-o <output>     Provide output path
-help           Print help to stdout
    )", program);
}

int main(int argc, const char **argv){
    using std::size_t;
#define DEBUG 1
#if !DEBUG
    namespace fs = std::filesystem;
    std::optional<fs::path> input_file_path{}, output_file_path{};
    std::string_view program = argv[0];
    for(int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];
        if (arg == "-o") {
            if (i == argc-1) {
                print_usage(stderr, program);
                std::eprintln("ERROR: no value is provided or flag {}", arg);
                exit(1);
            }
            output_file_path = argv[i+1];
        } else if (arg == "-help") {
            print_usage(stdout, program);
            exit(0);
        } else {
                input_file_path = arg;
        }
    }
    if(!input_file_path) {
        print_usage(stderr, program);
        std::eprintln("ERROR: no input file path was provided");
        exit(1);
    }
    if(!output_file_path) {
        //Might not need this branch
        if (input_file_path->extension() == ".sc")
            output_file_path = input_file_path->filename().replace_extension(".c");
        else {
            std::eprintln("ERROR: input file not having the extension .sc");
            exit(2);
        }

    }
#endif

#if !DEBUG
    std::string src = slurp_file(input_file_path->string());
#else
    std::string src =
R"(
//Hello world
/* commment */
"This is a string"
'chars'
'c'
'\13'

)";
#endif
    Cursor cur = {src};
    std::vector<Token> tokens;
    while (cur) {
        int line = cur.line;
        int col = cur.col;
        char c = cur.peek();
        if (isblank(c)) {
            while (cur && isblank(cur.peek())) cur.next();
            continue;
        }
        if (c == '/' && cur.peek(1) == '/') {
            size_t start = cur.index;
            while (cur && cur.peek() != '\n') cur.next();
            tokens.emplace_back(token_kind::Comment, std::string_view(&src[start], cur.index - start), line, col);
            continue;
        }
        if (c == '/' && cur.peek(1) == '*') {
            size_t start = cur.index;
            cur.skip(2);
            while (cur && !(cur.peek() == '*' && cur.peek(1) == '/')) cur.next();
            if (cur) cur.skip(2);
            tokens.emplace_back(token_kind::Comment, std::string_view(&src[start], cur.index - start), line, col);
            continue;
        }
        if(c == '"') {
            size_t start = cur.index;
            cur.next();
            while (cur) {
                if(cur.peek() == '\\') cur.skip(2);
                else if (cur.peek() == '"') {
                    cur.next();
                    break;
                } else cur.next();
            }
            tokens.emplace_back(token_kind::String, std::string_view(&src[start], cur.index - start), line, col);
            continue;
        }
        if(c == '\'') {
            size_t start = cur.index;
            cur.next();
            bool escape = false;
            int i = 0;
            while (cur) {
                if(cur.peek() == '\\') {
                    cur.skip(2);
                    i++;
                    escape = true;
                }
                else if (cur.peek() == '\'') {
                    cur.next();
                    break;
                } else {
                    cur.next();
                    i++;
                }
            }
            tokens.emplace_back((i <= 1) || escape ? token_kind::Char : token_kind::MultiCharLiteral,
                                    std::string_view(&src[start], cur.index - start), line, col);
            continue;
        }
        if (isdigit(c)) {
            size_t start = cur.index;
            auto isodigit = [](int c){return (c >= '0' && c <= '7');};
            bool hexoct = cur.peek() == '0';
            cur.next();
            if (hexoct && tolower(cur.peek()) == 'x') {
                cur.next();
                while (cur && isxdigit(cur.peek())) cur.next();
                if(!isblank(cur.peek())) {
                    while (cur && !isblank(cur.peek())) cur.next();
                    tokens.emplace_back(token_kind::Unknown, std::string_view(&src[start], cur.index - start), line, col);
                    continue;
                }
                tokens.emplace_back(token_kind::Number, std::string_view(&src[start], cur.index - start), line, col);
                continue;
            }
            else if (hexoct) {
                while (cur && isodigit(cur.peek())) cur.next();
                if (!isblank(cur.peek())) {
                    while (cur && !isblank(cur.peek())) cur.next();
                    tokens.emplace_back(token_kind::Unknown, std::string_view(&src[start], cur.index - start), line, col);
                    continue;
                }
                tokens.emplace_back(token_kind::Number, std::string_view(&src[start], cur.index - start), line, col);
                continue;
            } else {
                while (cur && isdigit(cur.peek())) cur.next();
                if(!isblank(cur.peek())) {
                    while(cur && !isblank(cur.peek())) cur.next();
                    tokens.emplace_back(token_kind::Unknown, std::string_view(&src[start], cur.index - start), line, col);
                    continue;
                }
                tokens.emplace_back(token_kind::Number, std::string_view(&src[start], cur.index - start), line, col);
            }
            //TODO: handle floats, hexadecimal floats, binary numbers(? - C23 stuff)
        }
        //TODO: Id, Op, Punct, Custom, Keywords



    }
#if DEBUG
    for (const auto& t : tokens) {
        std::println("{} \" {} \" (line {}, col {})", t.name(), t.text, t.line, t.col);
    }
#endif
}
