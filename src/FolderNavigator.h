#pragma once
#include "platform/PlatformDefs.h"
#include <string>
#include <vector>

class FolderNavigator {
public:
    FolderNavigator();
    ~FolderNavigator();

    bool LoadFromInitialFile(const std::wstring& filePath);
    bool SetCurrentPath(const std::wstring& filePath);

    bool Next();
    bool Prev();
    bool First();
    bool Last();
    bool SetIndex(size_t index);

    bool HasImages() const { return !m_files.empty(); }
    size_t GetCurrentIndex() const { return m_currentIndex; }
    size_t GetTotalCount() const { return m_files.size(); }
    const std::vector<std::wstring>& GetAllFiles() const { return m_files; }
    std::wstring GetCurrentPath() const;
    std::wstring GetCurrentFileName() const;

    static bool IsImageFile(const std::wstring& path);

    void ScanFolder(const std::wstring& folder);

private:
    std::wstring m_currentFolder;
    std::vector<std::wstring> m_files;
    size_t m_currentIndex = 0;
};
