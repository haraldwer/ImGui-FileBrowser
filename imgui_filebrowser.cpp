#include "imgui_filebrowser.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <filesystem>

#include "ImGui/imgui.h"
#include "ImGui/imgui_custom.h"
#include "ImGui/imgui_stdlib.h"

inline std::string ImGuiStdStringReplace(const std::string& InStr, const std::string& InSearch, const std::string& InRep)
{
    if(InSearch.empty())
        return InStr;
        
    std::string result = InStr;
    size_t start_pos = 0;
    while((start_pos = result.find(InSearch, start_pos)) != std::string::npos) {
        result.replace(start_pos, InSearch.length(), InRep);
        start_pos += InRep.length();
    }
    return result;
}

inline std::string ImGuiStdStringRemove(const std::string& InStr, const std::string& InSearch)
{
    return ImGuiStdStringReplace(InStr, InSearch, "");        
}

std::string ImGui::FileBrowser::GetLabel() const
{
    return "File Browser##" + OriginalPath;
}

std::string ImGui::FileBrowser::GetRelative(const std::string& InPath)
{
    return "..\\content\\" + InPath; 
}

std::string ImGui::FileBrowser::GetLocal(const std::string& InPath)
{
    const auto relativePath = std::filesystem::relative(InPath);
    return ImGuiStdStringRemove(relativePath.string(), "..\\content");
}

std::string ImGui::FileBrowser::GetEditedPath(const std::string& InPath)
{
    if (InPath.starts_with("..\\"))
        return InPath.substr(3);
    return InPath; 
}

bool ImGui::OpenFileBrowser(const std::string& InPath, FileBrowserOption InOption, const std::set<std::string>& InExt)
{
    // Create instance if it does not exist
    if (!FileBrowser::Instance)
        FileBrowser::Instance = new FileBrowser(); 
    return FileBrowser::Instance->OpenInternal(InPath, InOption, InExt);     
}

bool ImGui::FetchFileBrowserResult(const std::string& InPath, std::string& OutSelectedPath)
{
    if (FileBrowser::Instance)
    {
        if (FileBrowser::Instance->FetchInternal(InPath, OutSelectedPath))
        {
            // Delete instance when finished
            delete FileBrowser::Instance;
            FileBrowser::Instance = nullptr;
            return true; 
        }
    }
    return false;  
}

bool ImGui::FileBrowser::OpenInternal(const std::string& InPath, FileBrowserOption InOption, const std::set<std::string>& InExt)
{
    if (IsOpen)
        return OriginalPath == InPath;

    std::string directory = InPath;
    if (!std::filesystem::is_directory(GetRelative(directory)))
    {
        const auto dirPath = std::filesystem::path(directory);
        if (dirPath.has_parent_path())
            directory = dirPath.parent_path().string();
    } 
    
    // Setup 
    IsOpen = true;
    OriginalPath = InPath;
    Option = InOption;
    Ext = InExt;
    TryApplyPath(directory);
    
    OpenPopup(GetLabel().c_str());
    
    return true; 
}

bool ImGui::FileBrowser::FetchInternal(const std::string& InPath, std::string& OutSelectedPath)
{
    if (!IsOpen)
        return false;

    if (OriginalPath != InPath)
        return false; 
    
    bool result = false;
    ImGuiIO& io = GetIO();
    SetNextWindowSize({ Width, Height });
    SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f,0.5f));
    SetNextWindowFocus();
    if (BeginPopupModal(GetLabel().c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        EditNavigation();
        Spacing();
        Separator();
        Spacing(); 
        EditContent();
        Spacing();

        // Extension text in the bottom right
        std::string ext;
        switch (Option)
        {
        case FileBrowserOption::DIRECTORY:
            ext = "Directory"; 
            break;
        case FileBrowserOption::FILE:
            for (auto e : Ext)
            {
                if (!ext.empty())
                    ext += ", ";
                ext += e;
            }
            break;
        }
        if (!ext.empty())
        {
            Text((" Ext: " + ext).c_str());
            SameLine();
        }

        // Select / Cancel buttons
        const ImGuiStyle style = GetStyle();
        constexpr ImVec2 buttonSize(100.f, 0.f);
        const float widthNeeded = buttonSize.x + style.ItemSpacing.x + buttonSize.x;
        SetCursorPosX(GetCursorPosX() + GetContentRegionAvail().x - widthNeeded);
        if (Button("Cancel", buttonSize))
        {
            OutSelectedPath = OriginalPath;
            result = true; 
        }
        SameLine(); 
        if (Button("Select", buttonSize))
        {
            switch (Option)
            {
            case FileBrowserOption::DIRECTORY:
                OutSelectedPath = Path; 
                break;
            case FileBrowserOption::FILE:
                OutSelectedPath = Selected.empty() ?
                    OriginalPath : Path + "\\" + Selected;
                break;
            }
            while (OutSelectedPath.starts_with('\\'))
                OutSelectedPath = OutSelectedPath.substr(1);
            result = true;
        } 
        EndPopup();
    }
    
    return result;
}

void ImGui::FileBrowser::Refresh()
{
    // 1. Find current directory
    const std::filesystem::path path(GetRelative(Path));
    if (!std::filesystem::exists(path))
    {
        TryPopPath(); 
        return; 
    }
    
    // 2. Store directory content
    Directories.clear(); 
    Files.clear();
    for (auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_directory())
        {
            Directories.push_back(entry.path().filename().string());
        }
        else if (Option != FileBrowserOption::DIRECTORY)
        {
            // Filter extensions
            if (!Ext.empty())
            {
                if (!entry.path().has_extension())
                    continue;
                const std::string ext = entry.path().extension().string();
                if (!Ext.contains(ext))
                    continue;
            }
            Files.push_back(entry.path().filename().string());
        }
    }
    
    // 3. Refresh hint
    RefreshGuess();
}

void ImGui::FileBrowser::RefreshGuess()
{
    std::string editedPath = GetEditedPath(EditedPath);
    const std::filesystem::path path(GetRelative(editedPath));
    if (!path.has_parent_path())
        return;
    if (!exists(path.parent_path()))
        return;

    int matchC = 0;
    std::string matchPath;
    for (auto& entry : std::filesystem::directory_iterator(path.parent_path()))
    {
        // Find content of parent path
        if (!entry.is_directory())
            continue;
        
        // Compare to EditedPath
        // Find the one with greatest match
        std::string entryPath = GetLocal(entry.path().string());
        int i = 0; 
        for (i = 0; i < MIN(entryPath.length(), editedPath.length()); i++)
            if (entryPath[i] != editedPath[i])
                break; 
        if (i > matchC)
        {
            matchPath = entryPath;
            matchC = i;
        }
    }

    if (matchC > 0)
        NavigationGuess = matchPath;
}

bool ImGui::FileBrowser::TryPopPath()
{
    // Pop until exists or at root
    std::filesystem::path path(GetRelative(Path));
    while (!std::filesystem::exists(path) && path.has_parent_path())
        path = path.parent_path(); 
    return TryApplyPath(GetLocal(path.parent_path().string()));
}

bool ImGui::FileBrowser::TryApplyPath(const std::string& InString)
{
    const std::filesystem::path path(GetRelative(InString));
    if (!std::filesystem::exists(path))
        return false;

    Path = GetLocal(path.string()); 
    EditedPath = "..\\" + Path;
    Refresh();
    return true; 
}

void ImGui::FileBrowser::EditNavigation()
{
    constexpr ImGuiInputTextFlags flags = 
        ImGuiInputTextFlags_CharsNoBlank |
        ImGuiInputTextFlags_AutoSelectAll |
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory;
    if (InputText("##Path", &EditedPath, flags))
    {
        EditedPath == ".." ?
            TryPopPath() :
            TryApplyPath(GetEditedPath(EditedPath));
    }
    if (IsItemEdited())
        RefreshGuess();
    if (IsItemActive())
    {
        if (IsKeyDown(ImGuiKey_Tab))
        {
            EditedPath == ".." ?
               TryPopPath() : TryApplyPath(NavigationGuess);
            ClearActiveID(); 
        }
    }

    SameLine();
    if (Button("^"))
        TryPopPath();
    SameLine();
    if (Button("Refresh"))
        Refresh();  
}

void ImGui::FileBrowser::EditContent()
{
    const ImGuiStyle style = GetStyle(); 
    const ImVec2 size = {
        Width - style.WindowPadding.x * 2.0f,
        Height - style.WindowPadding.y * 2.0f - 90.0f 
    };
    if (BeginListBox("##FileBrowserContent", size))
    {
        Spacing();
        Spacing();
        
        static std::string potentialDir;
        std::string newDir;
        for (auto& dir : Directories)
        {
            if (Selectable(dir.c_str(), potentialDir == dir))
            {
                if (potentialDir == dir)
                {
                    newDir = dir;
                    potentialDir = ""; 
                }
                Selected = ""; 
                potentialDir = dir;
            }
        }

        for (auto& file : Files)
        {
            if (Selectable(file.c_str(), file == Selected))
            {
                Selected = file;
                potentialDir = ""; 
            }
        }

        if (!newDir.empty())
            TryApplyPath(Path + "\\" + newDir);

        Spacing();
        Spacing();

        EndListBox();
    }
}
