# ImGui FileBrowser
A simple file-browser for ImGui

![[screenshots/0.png]]

## Usage
1. Add ``imgui_filebrowser.cpp`` to your source files. 
2. Include ``imgui_filebrowser.h``. 

There are two functions: 
```
bool ImGui::OpenFileBrowser(const std::string& InPath, FileBrowserOption InOption = FileBrowserOption::FILE, const std::set<std::string>& InExt = {});
bool ImGui::FetchFileBrowserResult(std::string& OutPath);
```

There are some QOL tricks in the browser: 
 - Write ".." in the navigation-bar to go up one directory
 - Press tab when editing the navigation-bar to auto-complete

## Example
```
if (ImGui::Button("Pick file"))
	ImGui::OpenFileBrowser(defaultPath);

std::string result;
if (ImGui::FetchFileBrowserResult(result))
{
	// Do something with the result
}
```

## Notes
 - Using the standard library, specifically ``std::string``, ``std::set`` and ``std::filesystem``.
 - The window size can be changed by changing ``Width`` and ``Height`` in the ``FileBrowser`` class. 

## TODO: 
 - Multi-file select
 - Search / filters