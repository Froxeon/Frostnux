//#pragma once
//#include <string>
//#include <vector>
//#include <filesystem>
//#include <nlohmann/json.hpp>
//
//namespace Frostnux {
//
//	enum class ProjectKind : uint8_t
//	{
//		ConsoleApp,
//		StaticLib,
//		DynamicLib
//	};
//
//	struct ProjectConfig
//	{
//		std::string name = "Debug";
//		std::string outputDir = "bin/Debug";
//		std::string optimization = "/Od";
//		std::vector<std::string> flags;
//		std::vector<std::string> includeDirs;
//		std::vector<std::string> defines;
//		std::vector<std::string> libraryDirs;	// -L
//		std::vector<std::string> libraries;		// -l
//
//		bool debugInfo = true;
//	};
//
//	class Project
//	{
//	public:
//		std::string name = "Project";
//		std::string language = "cpp";
//		std::string standard = "c++20";
//		std::filesystem::path path;
//		std::string outputDir = "bin";
//		std::string outputName;
//		std::string activeConfig = "Debug";
//
//		std::vector<std::string> sources;
//		std::vector<std::string> headers;
//		std::vector<std::string> dependencies;
//		std::vector<ProjectConfig> configurations;
//
//		ProjectKind kind = ProjectKind::ConsoleApp;
//
//		bool Load(const std::filesystem::path& filepath);
//		bool Save(const std::filesystem::path& filepath = {});
//
//		void ScanSources();
//
//		[[nodiscard]] std::filesystem::path Directory() const { return path.parent_path(); }
//		[[nodiscard]] const ProjectConfig* ActiveConfig() const;
//
//		void AddDependency(const std::string& depName)
//		{
//			if (depName == name) return;
//			for (const auto& d : dependencies)
//			{
//				if (d == depName) return;
//			}
//			dependencies.push_back(depName);
//			Save();
//		}
//
//		void RemoveDependency(const std::string& depName)
//		{
//			auto it = std::remove(dependencies.begin(), dependencies.end(), depName);
//			if (it != dependencies.end())
//			{
//				dependencies.erase(it, dependencies.end());
//				Save();
//			}
//		}
//	};
//
//}

#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <nlohmann/json.hpp>

namespace Frostnux {

	enum class ProjectKind : uint8_t
	{
		ConsoleApp,
		StaticLib,
		DynamicLib
	};

	struct ProjectConfig
	{
		std::string name = "Debug";
		std::string outputDir = "bin/Debug";
		std::string optimization = "/Od";
		std::vector<std::string> flags;
		std::vector<std::string> includeDirs;
		std::vector<std::string> defines;
		std::vector<std::string> libraryDirs;	// -L
		std::vector<std::string> libraries;		// -l
		bool debugInfo = true;
	};

	class Project {
	public:
		std::string name = "Project";
		std::string language = "cpp";
		std::string standard = "c++20";
		std::filesystem::path path;
		std::string outputDir = "bin";
		std::string outputName;
		std::string activeConfig = "Debug";

		std::vector<std::string> sources;
		std::vector<std::string> headers;
		std::vector<std::string> dependencies;
		std::vector<ProjectConfig> configurations;

		ProjectKind kind = ProjectKind::ConsoleApp;

		bool Load(const std::filesystem::path& filepath);
		bool Save(const std::filesystem::path& filepath = {});

		void ScanSources(bool force = false);

		[[nodiscard]] std::filesystem::path Directory() const { return path.parent_path(); }
		[[nodiscard]] const ProjectConfig* ActiveConfig() const;

		void AddDependency(const std::string& depName)
		{
			if (depName == name) return;
			for (const auto& d : dependencies)
				if (d == depName) return;
			dependencies.push_back(depName);
			Save();
		}

		void RemoveDependency(const std::string& depName)
		{
			auto it = std::remove(dependencies.begin(), dependencies.end(), depName);
			if (it != dependencies.end())
			{
				dependencies.erase(it, dependencies.end());
				Save();
			}
		}
	private:
		std::chrono::steady_clock::time_point m_LastScanTime{};
	};

}
