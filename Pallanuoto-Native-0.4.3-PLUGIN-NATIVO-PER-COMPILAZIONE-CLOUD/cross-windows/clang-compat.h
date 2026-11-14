#pragma once
// OBS 31.1.1 util_uint64.h expects this MSVC intrinsic, absent in Clang's intrin.h.
// This shim is only used when parsing the official OBS headers with Clang.
#if defined(__clang__) && defined(_WIN32) && defined(_M_X64)
static inline unsigned long long _udiv128(unsigned long long high, unsigned long long low,
 unsigned long long divisor, unsigned long long *remainder)
{
 const unsigned __int128 value=((unsigned __int128)high<<64)|low;
 *remainder=(unsigned long long)(value%divisor);
 return (unsigned long long)(value/divisor);
}
#endif
