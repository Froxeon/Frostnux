#include <fxpch.h>
#include "SettingsManager.h"
#include "Frostnux/Core/Log.h"
#include "Frostnux/Core/Application.h"
#include "Frostnux/ui/uiTools/SolutionExplorer.h"
#include "Frostnux/ui/CodeEditor/Highlighter.h"

namespace Frostnux {

	namespace {
		constexpr std::size_t kMaxRecentFiles = 20;
		constexpr const char* kProjectFolderName = ".fx";
		constexpr const char* kProjectConfigName = "config.json";
	}

	SettingsManager& SettingsManager::Get()
	{
		static SettingsManager instance;
		return instance;
	}

	void SettingsManager::EnsureGlobalFolder()
	{
		std::error_code ec;
		std::filesystem::create_directories(m_RootPath, ec);
		if (ec)
			FX_CORE_ERROR("Failed to create global config folder '{}': {}", m_RootPath, ec.message());
	}

	void SettingsManager::Load()
	{
		EnsureGlobalFolder();

		std::ifstream file(m_Path);
		if (!file.is_open())
		{
			FX_CORE_WARN("Global settings not found at '{}', using defaults.", m_Path);
			//Save();
		}
		else
		{
			try
			{
				nlohmann::json j;
				file >> j;
				m_Settings.languageIndex = j.value("languageIndex", 1);
				m_Settings.themeIndex = j.value("themeIndex", 0);
				m_Settings.fontSize = j.value("fontSize", 24);
				s_FontSize = m_Settings.fontSize;
				m_Settings.recentFiles = j.value("recentFiles", std::vector<std::string>{});
				m_Settings.tokenIndex = j.value("tokenIndex", std::vector<int>{});
				m_Settings.WindowWidth = j.value("WindowWidth", 1920);
				m_Settings.WindowHeight = j.value("WindowHeight", 1080);
				m_Settings.IsMaximize = j.value("IsMaximize", false);
			}
			catch (const std::exception& e)
			{
				FX_CORE_ERROR("Failed to parse global settings JSON: {}", e.what());
			}
		}

		if (m_Settings.themeIndex < 0 || m_Settings.themeIndex >= (int)ThemeManager::GetThemeCount())
		{
			FX_CORE_WARN("Invalid theme index in settings: {}. Defaulting to Dark theme.", m_Settings.themeIndex);
			m_Settings.themeIndex = (int)Theme::Dark;
			ThemeManager::SetTheme(Theme::Dark);
			Save();
		}
		else
		{
			ThemeManager::SetTheme((Theme)m_Settings.themeIndex);
		}

		if (m_Settings.languageIndex < 0 || m_Settings.languageIndex >= (int)LanguageManager::GetLanguageCount())
		{
			FX_CORE_WARN("Invalid language index in settings: {}. Defaulting to English (US).", m_Settings.languageIndex);
			m_Settings.languageIndex = (int)LanguageType::en_US;
			LanguageManager::SetLanguage(LanguageType::en_US);
			Save();
		}
		else
		{
			LanguageManager::SetLanguage((LanguageType)m_Settings.languageIndex);
		}
	}

	void SettingsManager::Save()
	{
		m_Settings.fontSize = s_FontSize;
		m_Settings.languageIndex = (int)LanguageManager::GetCurrentLanguage();
		m_Settings.themeIndex = (int)ThemeManager::GetCurrentTheme();
		m_Settings.tokenIndex = EditorTheme::GetTokenCount();

		if (Application::HasInstance())
		{
			auto& window = Application::Get().GetWindow();
			m_Settings.IsMaximize = window.IsMaximized();
			m_Settings.WindowWidth = window.GetWidth();
			m_Settings.WindowHeight = window.GetHeight();
		}

		nlohmann::json j;
		j["themeIndex"] = m_Settings.themeIndex;
		j["languageIndex"] = m_Settings.languageIndex;
		j["fontSize"] = m_Settings.fontSize;
		j["recentFiles"] = m_Settings.recentFiles;
		j["tokenIndex"] = m_Settings.tokenIndex;
		j["WindowWidth"] = m_Settings.WindowWidth;
		j["WindowHeight"] = m_Settings.WindowHeight;
		j["IsMaximize"] = m_Settings.IsMaximize;

		std::ofstream file(m_Path);
		if (!file)
		{
			FX_CORE_ERROR("Failed to save global settings to {}", m_Path);
			return;
		}
		file << j.dump(4);
		FX_CORE_INFO("Global settings saved.");
	}

	void SettingsManager::AddRecentFile(const std::string& filepath)
	{
		auto& recent = m_Settings.recentFiles;

		recent.erase(std::remove(recent.begin(), recent.end(), filepath), recent.end());
		recent.insert(recent.begin(), filepath);

		if (recent.size() > kMaxRecentFiles)
			recent.resize(kMaxRecentFiles);

		Save();
	}

	bool SettingsManager::OpenProject(const std::string& projectRootPath)
	{
		if (projectRootPath.empty())
		{
			FX_CORE_ERROR("SettingsManager::OpenProject called with empty path.");
			return false;
		}

		if (m_bProjectLoaded && m_ProjectRootPath != projectRootPath)
			CloseProject();

		std::filesystem::path root(projectRootPath);
		std::filesystem::path fxDir = root / kProjectFolderName;

		std::error_code ec;
		std::filesystem::create_directories(fxDir, ec);
		if (ec)
		{
			FX_CORE_ERROR("Failed to create project settings folder '{}': {}", fxDir.string(), ec.message());
			return false;
		}

		m_ProjectRootPath = root.string();
		m_ProjectPath = (fxDir / kProjectConfigName).string();
		m_bProjectLoaded = false;

		LoadProject();
		return true;
	}

	void SettingsManager::CloseProject()
	{
		if (!m_bProjectLoaded)
			return;

		SaveProject();

		m_bProjectLoaded = false;
		m_ProjectRootPath.clear();
		m_ProjectPath.clear();
		m_ProjectSettings = ProjectSettings{};
	}

	void SettingsManager::LoadProject()
	{
		m_ProjectSettings = ProjectSettings{};

		std::ifstream file(m_ProjectPath);
		if (!file.is_open())
		{
			FX_CORE_WARN("Project settings not found at '{}', using defaults.", m_ProjectPath);

			nlohmann::json j;
			j["openFiles"] = m_ProjectSettings.openFiles;
			j["solutionExplorerScrollY"] = m_ProjectSettings.solutionExplorerScrollY;
			j["activeFileIndex"] = m_ProjectSettings.activeFileIndex;

			std::ofstream out(m_ProjectPath);
			if (out) out << j.dump(4);
			else     FX_CORE_ERROR("Failed to create project settings '{}'", m_ProjectPath);

			m_bProjectLoaded = true;
			return;
		}

		try
		{
			nlohmann::json j;
			file >> j;
			m_ProjectSettings.openFiles = j.value("openFiles", std::vector<std::string>{});
			m_ProjectSettings.solutionExplorerScrollY = j.value("solutionExplorerScrollY", 0.0f);
			m_ProjectSettings.activeFileIndex = j.value("activeFileIndex", -1);
		}
		catch (const std::exception& e)
		{
			FX_CORE_ERROR("Failed to parse project settings JSON: {}", e.what());
			m_ProjectSettings = ProjectSettings{};
		}

		m_bProjectLoaded = true;

		SolutionExplorer::SetScrollY(m_ProjectSettings.solutionExplorerScrollY);
	}

	void SettingsManager::SaveProject()
	{
		if (!m_bProjectLoaded)
			return;

		m_ProjectSettings.solutionExplorerScrollY = SolutionExplorer::GetScrollY();

		nlohmann::json j;
		j["openFiles"] = m_ProjectSettings.openFiles;
		j["solutionExplorerScrollY"] = m_ProjectSettings.solutionExplorerScrollY;
		j["activeFileIndex"] = m_ProjectSettings.activeFileIndex;

		std::ofstream file(m_ProjectPath);
		if (!file)
		{
			FX_CORE_ERROR("Failed to save project settings to '{}'", m_ProjectPath);
			return;
		}
		file << j.dump(4);
		FX_CORE_INFO("Project settings saved: {}", m_ProjectPath);
	}

	void SettingsManager::SetOpenFiles(const std::vector<std::string>& files)
	{
		if (!m_bProjectLoaded)
		{
			FX_CORE_WARN("SetOpenFiles called without an open project, ignored.");
			return;
		}
		m_ProjectSettings.openFiles = files;
		SaveProject();
	}

}