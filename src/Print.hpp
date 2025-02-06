#pragma once

#include <cstdio>
#include <print>
#include <format>

namespace std
{
#ifndef __cpp_lib_print //>= 202207L

    inline void vprint_nonunicode(FILE* __stream, string_view __fmt, format_args __args)
    {
        __format::_Str_sink<char> __buf;
        std::vformat_to(__buf.out(), __fmt, __args);
        auto __out = __buf.view();
        if (std::fwrite(__out.data(), 1, __out.size(), __stream) != __out.size())
        __throw_system_error(EIO);
    }

    inline void vprint_unicode(FILE* __stream, string_view __fmt, format_args __args)
    {
    #if !defined(_WIN32) || defined(__CYGWIN__)
        // For most targets we don't need to do anything special to write
        // Unicode to a terminal.
        std::vprint_nonunicode(__stream, __fmt, __args);
    #else
        __format::_Str_sink<char> __buf;
        std::vformat_to(__buf.out(), __fmt, __args);
        auto __out = __buf.view();

        void* __open_terminal(FILE*);
        error_code __write_to_terminal(void*, span<char>);
        // If stream refers to a terminal, write a native Unicode string to it.
        if (auto __term = __open_terminal(__stream))
        {
        string __out = std::vformat(__fmt, __args);
        error_code __e;
        if (!std::fflush(__stream))
        {
            __e = __write_to_terminal(__term, __out);
            if (!__e)
            return;
            if (__e == std::make_error_code(errc::illegal_byte_sequence))
            return;
        }
        else
        __e = error_code(errno, generic_category());
        _GLIBCXX_THROW_OR_ABORT(system_error(__e, "std::vprint_unicode"));
        }

        // Otherwise just write the string to the file as vprint_nonunicode does.
        if (std::fwrite(__out.data(), 1, __out.size(), __stream) != __out.size())
        __throw_system_error(EIO);
    #endif
    }

    template<typename... _Args>
    inline void print(FILE* __stream, format_string<_Args...> __fmt, _Args&&... __args)
    {
        auto __fmtargs = std::make_format_args(__args...);
        if constexpr (__unicode::__literal_encoding_is_utf8())
            std::vprint_unicode(__stream, __fmt.get(), __fmtargs);
        else
            std::vprint_nonunicode(__stream, __fmt.get(), __fmtargs);
    }

    template<typename... _Args>
    inline void print(format_string<_Args...> __fmt, _Args&&... __args)
    { 
        std::print(stdout, __fmt, std::forward<_Args>(__args)...); 
    }

    template<typename... _Args>
    inline void println(FILE* __stream, format_string<_Args...> __fmt, _Args&&... __args)
    {
        std::print(__stream, "{}\n",
		std::format(__fmt, std::forward<_Args>(__args)...));
    }

    template<typename... _Args>
    inline void println(format_string<_Args...> __fmt, _Args&&... __args)
    { 
        std::println(stdout, __fmt, std::forward<_Args>(__args)...); 
    }
#endif
}
