#pragma once

#include <string>

namespace ImGui
{
    enum class FileBrowserOption
    {
        DIRECTORY,
        FILE
    };
    
    class FileBrowser
    {
    public:
        

        FileBrowser() = default; 
        inline static FileBrowser* Instance = nullptr;
        bool OpenInternal(const std::string& InPath, FileBrowserOption InOption, const std::set<std::string>& InExt);
        bool FetchInternal(const std::string& InPath, std::string& OutSelectedPath);
    
    private:

        std::string GetLabel() const;
        static std::string GetRelative(const std::string& InPath);
        static std::string GetLocal(const std::string& InPath); 
        static std::string GetEditedPath(const std::string& InPath); 
        
        void Refresh();
        void RefreshGuess();
        
        bool TryPopPath();
        bool TryApplyPath(const std::string& InString);
        
        void EditNavigation();
        void EditContent();

        bool ContentEntry(const std::string& InEntry, bool InIsDir);
        bool TryApplyRename(const std::string& InPreviousName, const std::string& InNewName) const;
        bool TryDelete(const std::string& InName) const;
        std::string TryDuplicate(const std::string& InName) const;
        
        bool IsOpen = false; 
        std::string OriginalPath;
        std::string Path;
        std::string NavigationGuess;
        std::string EditedPath;
        std::string Selected;
        std::string RenameResult; 
        bool newEntry = false;
        std::set<std::string> Ext;
        FileBrowserOption Option = FileBrowserOption::FILE;
        
        const float Width = 500.0f;
        const float Height = 400.0f;

        std::vector<std::string> Directories;
        std::vector<std::string> Files;
    };

    bool OpenFileBrowser(const std::string& InPath, FileBrowserOption InOption = FileBrowserOption::FILE, const std::set<std::string>& InExt = {});
    bool FetchFileBrowserResult(const std::string& InPath, std::string& OutSelectedPath);
}

