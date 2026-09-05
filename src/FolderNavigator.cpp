#include "FolderNavigator.h"
#include <algorithm>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

FolderNavigator::FolderNavigator() = default;
FolderNavigator::~FolderNavigator() = default;

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

    std::wstring searchPattern = folder;
    if (!searchPattern.empty() && searchPattern.back() != L'\\' && searchPattern.back() != L'/') {
        searchPattern += L"\\";
    }
    std::wstring dirPrefix = searchPattern;
    searchPattern += L"*.*";

    // Use robust Win32 FindFirstFileW / FindNextFileW (never throws, handles all files & Unicode)
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring fileName = ffd.cFileName;
                if (IsImageFile(fileName)) {
                    m_files.push_back(dirPrefix + fileName);
                }
            }
        } while (FindNextFileW(hFind, &ffd) != 0);
        FindClose(hFind);
    }

    // Natural sort (Windows Explorer style)
    std::sort(m_files.begin(), m_files.end(), [](const std::wstring& a, const std::wstring& b) {
        return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
    });
}

bool FolderNavigator::LoadFromInitialFile(const std::wstring& filePath) {
    if (filePath.empty()) return false;

    wchar_t fullPath[MAX_PATH] = { 0 };
    if (!GetFullPathNameW(filePath.c_str(), MAX_PATH, fullPath, nullptr)) {
        return false;
    }

    // Resolve short 8.3 path (e.g. C:\PROGRA~1) to long path
    wchar_t longPath[MAX_PATH] = { 0 };
    if (GetLongPathNameW(fullPath, longPath, MAX_PATH) > 0) {
        wcscpy_s(fullPath, longPath);
    }

    std::wstring absPath = fullPath;
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
        std::sort(m_files.begin(), m_files.end(), [](const std::wstring& a, const std::wstring& b) {
            return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
        });
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
