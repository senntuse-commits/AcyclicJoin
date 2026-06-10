#pragma once

#ifdef SGX_ENCLAVE_BUILD

#include "Enclave_t.h"

#include <cstdarg>
#include <cstdio>
#include <string>
#include <type_traits>

int sgx_vprintf(const char *fmt, va_list args);
int sgx_printf(const char *fmt, ...);
int sgx_fprintf(void *stream, const char *fmt, ...);
void sgx_print_string(const char *s);

class SgxTrustedOStream
{
public:
    SgxTrustedOStream &operator<<(const char *s)
    {
        append(s ? s : "(null)");
        return *this;
    }

    SgxTrustedOStream &operator<<(char c)
    {
        char buf[2] = {c, 0};
        append(buf);
        return *this;
    }

    SgxTrustedOStream &operator<<(const std::string &s)
    {
        append(s.c_str());
        return *this;
    }

    SgxTrustedOStream &operator<<(SgxTrustedOStream &(*manip)(SgxTrustedOStream &))
    {
        return manip(*this);
    }

    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, SgxTrustedOStream &>::type operator<<(T value)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%lld", (long long)value);
        append(buf);
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value, SgxTrustedOStream &>::type operator<<(T value)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.6f", (double)value);
        append(buf);
        return *this;
    }

    template <typename T>
    typename std::enable_if<!std::is_arithmetic<T>::value, SgxTrustedOStream &>::type operator<<(const T &)
    {
        return *this;
    }

    void flush();

private:
    void append(const char *s);

    std::string buffer_;
};

SgxTrustedOStream &sgx_endl(SgxTrustedOStream &stream);

extern SgxTrustedOStream sgx_cout;
extern SgxTrustedOStream sgx_cerr;

#endif
