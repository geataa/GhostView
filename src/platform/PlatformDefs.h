#pragma once

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
// Linux / POSIX definitions
#include <cstdint>
#include <cstddef>
#include <cwchar>
#include <cwctype>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint32_t UINT;
typedef uint32_t UINT32;
typedef int32_t INT;
typedef int32_t LONG;
typedef uint64_t UINT64;
typedef int64_t INT64;
typedef intptr_t LONG_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t INT_PTR;
typedef intptr_t LRESULT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef int BOOL;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

typedef int32_t HRESULT;
#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define E_FAIL ((HRESULT)0x80004005L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)

typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HANDLE;

struct POINT {
    long x;
    long y;
};

struct RECT {
    long left;
    long top;
    long right;
    long bottom;
};

#define swprintf_s(buf, ...) swprintf(buf, sizeof(buf)/sizeof(buf[0]), __VA_ARGS__)
#define _wcsicmp wcscasecmp

// Wide string <-> UTF-8 std::string helpers
inline std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    std::string out;
    out.reserve(wstr.size() * 3);
    for (wchar_t wc : wstr) {
        if (wc < 0x80) {
            out.push_back((char)wc);
        } else if (wc < 0x800) {
            out.push_back((char)(0xC0 | ((wc >> 6) & 0x1F)));
            out.push_back((char)(0x80 | (wc & 0x3F)));
        } else if (wc < 0x10000) {
            out.push_back((char)(0xE0 | ((wc >> 12) & 0x0F)));
            out.push_back((char)(0x80 | ((wc >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (wc & 0x3F)));
        } else {
            out.push_back((char)(0xF0 | ((wc >> 18) & 0x07)));
            out.push_back((char)(0x80 | ((wc >> 12) & 0x3F)));
            out.push_back((char)(0x80 | ((wc >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (wc & 0x3F)));
        }
    }
    return out;
}

inline std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    std::wstring out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size();) {
        unsigned char c = str[i];
        if (c < 0x80) {
            out.push_back((wchar_t)c);
            ++i;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < str.size()) {
            wchar_t wc = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F);
            out.push_back(wc);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < str.size()) {
            wchar_t wc = ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
            out.push_back(wc);
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < str.size()) {
            wchar_t wc = ((c & 0x07) << 18) | ((str[i + 1] & 0x3F) << 12) | ((str[i + 2] & 0x3F) << 6) | (str[i + 3] & 0x3F);
            out.push_back(wc);
            i += 4;
        } else {
            ++i;
        }
    }
    return out;
}

#endif
