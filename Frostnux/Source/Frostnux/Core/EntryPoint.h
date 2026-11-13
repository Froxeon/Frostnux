//#pragma once
//
//#if defined(FX_PLATFORM_WINDOWS) || defined(FX_PLATFORM_LINUX)
//
//extern Frostnux::Application* Frostnux::CreateApplication();
//
//int main(int argc, char** argv)
//{
//	auto app = Frostnux::CreateApplication();
//
//	app->Run();
//
//	delete app;
//	return 0;
//}
//
//#endif

#pragma once

#include <string>
#include <vector>

#if defined(FX_PLATFORM_WINDOWS)

#include <windows.h>

inline std::string WStringToString(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

extern Frostnux::Application* Frostnux::CreateApplication(const std::vector<std::string>& args);

int wmain(int argc, wchar_t** argv)
{
	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i)
	{
		args.push_back(WStringToString(argv[i]));
	}

	auto app = Frostnux::CreateApplication(args);
	app->Run();
	delete app;
	return 0;
}

#elif defined(FX_PLATFORM_LINUX)

extern Frostnux::Application* Frostnux::CreateApplication(const std::vector<std::string>& args);

int main(int argc, char** argv)
{
	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i)
	{
		args.push_back(std::string(argv[i]));
	}

	auto app = Frostnux::CreateApplication(args);
	app->Run();
	delete app;
	return 0;
}

#endif