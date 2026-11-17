//#include "fxpch.h"
//
//#ifdef FX_PLATFORM_WINDOWS
//
//#include "Frostnux/Core/FileDialog.h"
//#include <shlobj.h>
//#include <commdlg.h>
//
//namespace Frostnux {
//
//	namespace {
//		std::wstring Utf8ToWide(const std::string& s)
//		{
//			if (s.empty()) return {};
//			int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
//			std::wstring out(len - 1, L'\0');
//			MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
//			return out;
//		}
//
//		std::string WideToUtf8(const wchar_t* s)
//		{
//			if (!s) return {};
//			int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
//			if (len <= 1) return {};
//			std::string out(len - 1, '\0');
//			WideCharToMultiByte(CP_UTF8, 0, s, -1, out.data(), len, nullptr, nullptr);
//			return out;
//		}
//
//		// "C++ Files|*.cpp;*.h|All Files|*.*" → L"C++ Files\0*.cpp;*.h\0All Files\0*.*\0\0"
//		std::wstring BuildFilter(const std::string& filter)
//		{
//			std::wstring out;
//			for (char c : filter)
//				out += (c == '|') ? L'\0' : static_cast<wchar_t>(c);
//			out += L'\0';
//			out += L'\0';
//			return out;
//		}
//	}
//
//	std::string SaveFileDialog(const std::string& defaultName, const std::string& filter)
//	{
//		wchar_t buffer[MAX_PATH] = {};
//		const std::wstring wName = Utf8ToWide(defaultName);
//		if (!wName.empty())
//			wcsncpy_s(buffer, wName.c_str(), _TRUNCATE);
//
//		const std::wstring wFilter = BuildFilter(filter);
//
//		OPENFILENAMEW ofn = {};
//		ofn.lStructSize = sizeof(ofn);
//		ofn.hwndOwner = nullptr;
//		ofn.lpstrFilter = wFilter.c_str();
//		ofn.lpstrFile = buffer;
//		ofn.nMaxFile = MAX_PATH;
//		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
//
//		if (!GetSaveFileNameW(&ofn)) return {};
//		return WideToUtf8(buffer);
//	}
//
//	std::string OpenFileDialog(const std::string& filter)
//	{
//		wchar_t buffer[MAX_PATH] = {};
//		const std::wstring wFilter = BuildFilter(filter);
//
//		OPENFILENAMEW ofn = {};
//		ofn.lStructSize = sizeof(ofn);
//		ofn.hwndOwner = nullptr;
//		ofn.lpstrFilter = wFilter.c_str();
//		ofn.lpstrFile = buffer;
//		ofn.nMaxFile = MAX_PATH;
//		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
//
//		if (!GetOpenFileNameW(&ofn)) return {};
//		return WideToUtf8(buffer);
//	}
//
//	std::string SelectFolderDialog()
//	{
//		wchar_t buffer[MAX_PATH] = {};
//		BROWSEINFOW bi = { 0 };
//		bi.lpszTitle = L"Select Folder";
//		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
//
//		LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
//		if (pidl != 0)
//		{
//			SHGetPathFromIDListW(pidl, buffer);
//			IMalloc* imalloc = 0;
//			if (SUCCEEDED(SHGetMalloc(&imalloc)))
//			{
//				imalloc->Free(pidl);
//				imalloc->Release();
//			}
//			return WideToUtf8(buffer);
//		}
//		return {};
//	}
//
//}
//
//#endif


#include "fxpch.h"

#ifdef FX_PLATFORM_WINDOWS

#include "Frostnux/Core/FileDialog.h"
#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <vector>

namespace Frostnux {

	namespace {
		std::wstring Utf8ToWide(const std::string& s)
		{
			if (s.empty()) return {};
			int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
			std::wstring out(len - 1, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
			return out;
		}

		std::string WideToUtf8(const wchar_t* s)
		{
			if (!s) return {};
			int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
			if (len <= 1) return {};
			std::string out(len - 1, '\0');
			WideCharToMultiByte(CP_UTF8, 0, s, -1, out.data(), len, nullptr, nullptr);
			return out;
		}

		std::wstring BuildFilter(const std::string& filter)
		{
			std::wstring out;
			for (char c : filter)
				out += (c == '|') ? L'\0' : static_cast<wchar_t>(c);
			out += L'\0';
			out += L'\0';
			return out;
		}
	}

	std::string SaveFileDialog(const std::string& defaultName, const std::string& filter)
	{
		wchar_t buffer[MAX_PATH] = {};
		const std::wstring wName = Utf8ToWide(defaultName);
		if (!wName.empty())
			wcsncpy_s(buffer, wName.c_str(), _TRUNCATE);

		const std::wstring wFilter = BuildFilter(filter);

		OPENFILENAMEW ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFilter = wFilter.c_str();
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (!GetSaveFileNameW(&ofn)) return {};
		return WideToUtf8(buffer);
	}

	std::string OpenFileDialog(const std::string& filter)
	{
		wchar_t buffer[MAX_PATH] = {};
		const std::wstring wFilter = BuildFilter(filter);

		OPENFILENAMEW ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFilter = wFilter.c_str();
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (!GetOpenFileNameW(&ofn)) return {};
		return WideToUtf8(buffer);
	}

	std::string SelectFolderDialog()
	{
		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		bool bNeedUninitialize = SUCCEEDED(hr);

		std::string result = {};

		IFileOpenDialog* pFileOpen = nullptr;
		hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			DWORD dwOptions;
			if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
			{
				pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
			}

			pFileOpen->SetTitle(L"Select Folder");

			hr = pFileOpen->Show(nullptr);

			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					if (SUCCEEDED(hr))
					{
						result = WideToUtf8(pszFilePath);
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}

		if (bNeedUninitialize)
		{
			CoUninitialize();
		}

		return result;
	}

}

#endif