#include "sgx_log.h"

#ifdef SGX_ENCLAVE_BUILD

#include <cstdarg>
#include <cstdio>

namespace
{
constexpr int kLogBufferSize = 1024;
}

SgxTrustedOStream sgx_cout;
SgxTrustedOStream sgx_cerr;

void sgx_print_string(const char *s)
{
    ocall_print(s ? s : "");
}

int sgx_vprintf(const char *fmt, va_list args)
{
#ifdef SGX_SILENCE_ALGORITHM_LOG
    (void)fmt;
    (void)args;
    return 0;
#else
    char buf[kLogBufferSize];
    int written = vsnprintf(buf, sizeof(buf), fmt, args);
    if (written < 0)
        return written;
    buf[sizeof(buf) - 1] = '\0';
    sgx_print_string(buf);
    return written;
#endif
}

int sgx_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = sgx_vprintf(fmt, args);
    va_end(args);
    return written;
}

int sgx_fprintf(void *, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = sgx_vprintf(fmt, args);
    va_end(args);
    return written;
}

extern "C" int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = sgx_vprintf(fmt, args);
    va_end(args);
    return written;
}

extern "C" int fprintf(void *, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = sgx_vprintf(fmt, args);
    va_end(args);
    return written;
}

void SgxTrustedOStream::append(const char *s)
{
    if (!s)
        return;

    for (const char *p = s; *p; ++p)
    {
        buffer_.push_back(*p);
        if (*p == '\n')
            flush();
    }
}

void SgxTrustedOStream::flush()
{
    if (buffer_.empty())
        return;
#ifndef SGX_SILENCE_ALGORITHM_LOG
    sgx_print_string(buffer_.c_str());
#endif
    buffer_.clear();
}

SgxTrustedOStream &sgx_endl(SgxTrustedOStream &stream)
{
    stream << '\n';
    stream.flush();
    return stream;
}

#endif
