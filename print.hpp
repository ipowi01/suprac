//
// Created by ádám on 2025. 12. 29..
//

#ifndef SUPRAC_PRINT_HPP
#define SUPRAC_PRINT_HPP

#ifdef NO_PRINT_H
namespace std {

    template<typename... Args>
    void print(std::format_string<Args...> fmt, Args&&... args) {
        std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stdout);
    }
    template<typename... Args>
    void eprint(std::format_string<Args...> fmt, Args&&... args) {
        std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stderr);
    }

    template<typename... Args>
    void print(FILE* stream, std::format_string<Args...> fmt, Args&&... args) {
        std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stream);
    }

    template<typename... Args>
    void println(std::format_string<Args...> fmt, Args&&... args) {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        msg += '\n';
        std::fputs(msg.c_str(), stdout);
    }

    template<typename... Args>
    void println(FILE* stream, std::format_string<Args...> fmt, Args&&... args) {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        msg += '\n';
        std::fputs(msg.c_str(), stream);
    }
    template<typename... Args>
    void eprintln(std::format_string<Args...> fmt, Args&&... args) {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        msg += '\n';
        std::fputs(msg.c_str(), stderr);
    }

}

#endif
#endif //SUPRAC_PRINT_HPP
