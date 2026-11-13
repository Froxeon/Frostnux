#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Frostnux {

	struct AppSettings
	{
		std::vector<std::string> recentFiles;
		std::vector<int> tokenIndex;

		int languageIndex = 1;
		int themeIndex = 0;
		int fontSize = 24;
		unsigned int WindowWidth = 1920, WindowHeight = 1080;
		bool IsMaximize = false;
	};

	struct ProjectSettings
	{
		std::vector<std::string>	openFiles;
		float						solutionExplorerScrollY = 0.0f;
		int							activeFileIndex = -1;
	};

	class SettingsManager
	{
	public:
		static SettingsManager& Get();

		void Load();
		void Save();
		AppSettings& GetSettings() { return m_Settings; }
		const AppSettings& GetSettings() const { return m_Settings; }

		bool OpenProject(const std::string& projectRootPath);

		void CloseProject();
		void SaveProject();
		void LoadProject();

		bool HasProject() const { return m_bProjectLoaded; }
		const std::string& GetProjectRoot()       const { return m_ProjectRootPath; }
		const std::string& GetProjectConfigPath() const { return m_ProjectPath; }
		ProjectSettings& GetProjectSettings() { return m_ProjectSettings; }
		const ProjectSettings& GetProjectSettings() const { return m_ProjectSettings; }

		void AddRecentFile(const std::string& filepath);
		void SetOpenFiles(const std::vector<std::string>& files);
	private:
		SettingsManager() = default;
		~SettingsManager() = default;

		void EnsureGlobalFolder();

		AppSettings		m_Settings;
		ProjectSettings	m_ProjectSettings;

		bool		m_bProjectLoaded = false;
		std::string	m_ProjectRootPath;
		std::string	m_ProjectPath;

		std::string m_Path = "config/config.json";
		std::string m_RootPath = "config";
	};

}
