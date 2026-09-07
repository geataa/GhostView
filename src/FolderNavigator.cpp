#include "FolderNavigator.h"
#include <algorithm>
#include <filesystem>
#include <cwctype>

#if defined(_WIN32)
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif

namespace fs = std::filesystem;

FolderNavigator::FolderNavigator() = default;
FolderNavigator::~FolderNavigator() = default;

static bool NaturalCompare(const std::wstring& a, const std::wstring& b) {
#if defined(_WIN32)
    return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
#else
    size_t ia = 0, ib = 0;
    while (ia < a.size() && ib < b.size()) {
        if (std::iswdigit(a[ia]) && std::iswdigit(b[ib])) {
            size_t startA = ia, startB = ib;
            while (ia < a.size() && std::iswdigit(a[ia])) ++ia;
            while (ib < b.size() && std::iswdigit(b[ib])) ++ib;
            while (startA < ia - 1 && a[startA] == L'0') ++startA;
            while (startB < ib - 1 && b[startB] == L'0') ++startB;
            size_t lenA = ia - startA, lenB = ib - startB;
            if (lenA != lenB) return lenA < lenB;
            for (size_t k = 0; k < lenA; ++k) {
                if (a[startA + k] != b[startB + k]) {
                    return a[startA + k] < b[startB + k];
                }
            }
        } else {
            wchar_t ca = std::towlower(a[ia]);
            wchar_t cb = std::towlower(b[ib]);
            if (ca != cb) return ca < cb;
            ++ia;
            ++ib;
        }
    }
    return a.size() < b.size();
#endif
}

bool FolderNavigator::IsImageFile(const std::wstring& path) {
    size_t dotPos = path.find_last_of(L'.');
    if (dotPos == std::wstring::npos) return false;

    std::wstring ext = path.substr(dotPos);
    for (auto& c : ext) {
        if (c >= L'A' && c <= L'Z') {
            c += (L'a' - L'A'); // Locale-independent lowercase
        }
    }

    return (ext == L".jpg" || ext == L".jpeg" || ext == L".png" ||
            ext == L".bmp" || ext == L".gif"  || ext == L".webp" ||
            ext == L".ico" || ext == L".tif"  || ext == L".tiff" ||
            ext == L".jfif" || ext == L".jpe"  || ext == L".dib");
}

void FolderNavigator::ScanFolder(const std::wstring& folder) {
    m_files.clear();
    m_currentIndex = 0;
    m_currentFolder = folder;

    try {
#if defined(_WIN32)
        fs::path p(folder);
#else
        fs::path p(WideToUtf8(folder));
#endif
        if (fs::exists(p) && fs::is_directory(p)) {
            for (const auto& entry : fs::directory_iterator(p, fs::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file()) {
#if defined(_WIN32)
                    std::wstring fullPath = entry.path().wstring();
#else
                    std::wstring fullPath = Utf8ToWide(entry.path().string());
#endif
                    if (IsImageFile(fullPath)) {
                        m_files.push_back(fullPath);
                    }
                }
            }
        }
    } catch (...) {
    }

    // Natural sort (Windows Explorer style / Natural numeric comparator)
    std::sort(m_files.begin(), m_files.end(), NaturalCompare);
}

bool FolderNavigator::LoadFromInitialFile(const std::wstring& filePath) {
    if (filePath.empty()) return false;

    std::wstring absPath;
    try {
#if defined(_WIN32)
        fs::path p = fs::absolute(fs::path(filePath));
        absPath = p.wstring();
#else
        fs::path p = fs::absolute(fs::path(WideToUtf8(filePath)));
        absPath = Utf8ToWide(p.string());
#endif
    } catch (...) {
        absPath = filePath;
    }

    size_t slashPos = absPath.find_last_of(L"\\/");
    std::wstring parentDir = (slashPos != std::wstring::npos) ? absPath.substr(0, slashPos) : L".";

    ScanFolder(parentDir);

    // Locate initial file in list
    bool found = false;
    for (size_t i = 0; i < m_files.size(); ++i) {
        if (_wcsicmp(m_files[i].c_str(), absPath.c_str()) == 0) {
            m_currentIndex = i;
            found = true;
            break;
        }
    }

    // If initial file is not in list (e.g. extension not matched in scan), add it
    if (!found) {
        m_files.push_back(absPath);
        std::sort(m_files.begin(), m_files.end(), NaturalCompare);
        for (size_t i = 0; i < m_files.size(); ++i) {
            if (_wcsicmp(m_files[i].c_str(), absPath.c_str()) == 0) {
                m_currentIndex = i;
                break;
            }
        }
    }
    return true;
}

bool FolderNavigator::SetCurrentPath(const std::wstring& filePath) {
    return LoadFromInitialFile(filePath);
}

bool FolderNavigator::Next() {
    if (m_files.empty()) return false;
    if (m_currentIndex + 1 < m_files.size()) {
        m_currentIndex++;
    } else {
        m_currentIndex = 0; // Wrap around
    }
    return true;
}

bool FolderNavigator::Prev() {
    if (m_files.empty()) return false;
    if (m_currentIndex > 0) {
        m_currentIndex--;
    } else {
        m_currentIndex = m_files.size() - 1; // Wrap around
    }
    return true;
}

bool FolderNavigator::First() {
    if (m_files.empty()) return false;
    m_currentIndex = 0;
    return true;
}

bool FolderNavigator::Last() {
    if (m_files.empty()) return false;
    m_currentIndex = m_files.size() - 1;
    return true;
}

bool FolderNavigator::SetIndex(size_t index) {
    if (index >= m_files.size()) return false;
    m_currentIndex = index;
    return true;
}

std::wstring FolderNavigator::GetCurrentPath() const {
    if (m_files.empty() || m_currentIndex >= m_files.size()) return L"";
    return m_files[m_currentIndex];
}

std::wstring FolderNavigator::GetCurrentFileName() const {
    std::wstring path = GetCurrentPath();
    if (path.empty()) return L"";
    size_t slashPos = path.find_last_of(L"\\/");
    if (slashPos != std::wstring::npos) {
        return path.substr(slashPos + 1);
    }
    return path;
}
