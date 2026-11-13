#include <fxpch.h>
#include "uiLayer.h"
#include "uiTools/ui.h"
#include "Frostnux/Core/Log.h"
#include "Frostnux/Core/Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Managers/SettingsManager.h"
#include "Frostnux/Core/FileDialog.h"
#include "uiTools/TextInputDialog.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/BuildSystem/ProcessRunner.h"
#include "uiTools/uiMessageBox.h"

namespace Frostnux
{

	GLRenderer uiLayer::m_EditorRenderer;
	CodeEditor uiLayer::m_Editor{ &m_EditorRenderer };

	uiLayer& uiLayer::Get()
	{
		static uiLayer instance;
		return instance;
	}

	CodeEditor& uiLayer::GetCodeEditor()
	{
		return m_Editor;
	}

	uiLayer::uiLayer() : Layer("UILayer")
	{}

	uiLayer::~uiLayer()
	{
		for (auto* win : m_Windows)
			delete win;
		m_Windows.clear();
		delete m_TitleBar;
		delete m_StatusBar;
		delete m_ShortcutBar;
	}

	void uiLayer::LoadSolution(const std::string& path)
	{
		namespace fs = std::filesystem;

		SettingsManager::Get().CloseProject();

		Solution newSol;
		if (!newSol.Load(path))
		{
			FX_CORE_ERROR("Failed to load solution: {}", path);
			return;
		}

		m_Solution = std::move(newSol);
		if (m_SolutionExplorer)
			m_SolutionExplorer->SetSolution(&m_Solution);

		FX_CORE_INFO("Loaded Solution: {}", m_Solution.name);
		m_ProjectIndex.SetRoot(path);
		m_ProjectIndex.Scan();
		m_Editor.SetProjectIndex(&m_ProjectIndex);
		FX_CORE_INFO("ProjectIndex: indexed {} files", m_ProjectIndex.FileCount());

		const std::string projectRoot = fs::path(path).parent_path().string();
		if (SettingsManager::Get().OpenProject(projectRoot))
			RestoreEditorState();
	}

	void uiLayer::CreateNewSolution(const std::string& path)
	{
		namespace fs = std::filesystem;

		SettingsManager::Get().CloseProject();

		m_Solution = Solution();
		m_Solution.path = path;
		m_Solution.name = fs::path(path).stem().string();

		auto* proj = m_Solution.AddProject(m_Solution.name, ProjectKind::ConsoleApp);

		auto baseDir = fs::path(path).parent_path() / m_Solution.name;
		std::error_code ec;
		fs::create_directories(baseDir / "src", ec);

		auto mainPath = baseDir / "src" / "main.cpp";

		m_Solution.Save(path);

		if (m_SolutionExplorer)
			m_SolutionExplorer->SetSolution(&m_Solution);

		if (auto* tab = m_Editor.openFile(mainPath.string()))
			tab->invalidateHighlight();

		FX_CORE_INFO("Created new Solution at: {}", path);

		m_ProjectIndex.SetRoot(path);
		m_ProjectIndex.Scan();
		m_Editor.SetProjectIndex(&m_ProjectIndex);
		FX_CORE_INFO("ProjectIndex: indexed {} files", m_ProjectIndex.FileCount());

		const std::string projectRoot = fs::path(path).parent_path().string();
		SettingsManager::Get().OpenProject(projectRoot);
	}

	void uiLayer::OnAttach()
	{
		auto& app = Application::Get();
		int width = app.GetWindow().GetWidth();
		int height = app.GetWindow().GetHeight();

		Renderer2D::Init();

		auto& settings = SettingsManager::Get().GetSettings();

		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		file >> j;
		m_StatusBar = new uiStatusBar();
		m_StatusBar->OnAttach();
		m_StatusBar->SetStatusText(j.value("Ready", "Ready"));

		m_TitleBar = new uiTitleBar();
		m_TitleBar->OnAttach();

		//std::string findBookmark = "";
		//std::string addBookmark = "";
		//std::string deleteBookmark = "";
		//std::string nextBookmark = "";
		//std::string clearBookmark = "";
		//if (ThemeManager::IsDarkTheme())
		//{
		//	findBookmark = "Resources/Images/FindBookmark_White.png";
		//	addBookmark = "Resources/Images/AddBookmark_White.png";
		//	deleteBookmark = "Resources/Images/DeleteBookmark_White.png";
		//	nextBookmark = "Resources/Images/NextBookmark_White.png";
		//	clearBookmark = "Resources/Images/ClearBookmarks_White.png";
		//}
		//else
		//{
		//	findBookmark = "Resources/Images/FindBookmark_Black.png";
		//	addBookmark = "Resources/Images/AddBookmark_Black.png";
		//	deleteBookmark = "Resources/Images/DeleteBookmark_Black.png";
		//	nextBookmark = "Resources/Images/NextBookmark_Black.png";
		//	clearBookmark = "Resources/Images/ClearBookmarks_Black.png";
		//}

		m_CurrentIsDarkTheme = ThemeManager::IsDarkTheme();
		m_ShortcutBar = new uiShortcutBar();
		m_ShortcutBar->OnAttach();
		InitShortcutBar();
		//std::vector<ShortcutItem> editGroup =
		//{
		//	{ "undo", "Resources/Images/Undo.png", j.value("Undo", "Undo"), []()
		//	 { FX_INFO("Undo"); } },
		//	{ "redo", "Resources/Images/Redo.png", j.value("Redo", "Redo"), []()
		//	 { FX_INFO("Redo"); } }
		//};
		//std::vector<ShortcutItem> buildGroup =
		//{
		//	{ "build", "Resources/Images/Build.png", j.value("Build", "Build"), [this]()
		//	 { BuildSolution(); } },
		//	{ "clean", "Resources/Images/Clean.png", j.value("Clean", "Clean"), []()
		//	 { FX_INFO("Clean"); } },
		//	{ "run", "Resources/Images/Run.png", j.value("Run", "Run"), [this]()
		//	 { RunLastBuild(); } },
		//	{ "debug", "Resources/Images/Debug.png", j.value("Debug", "Debug"), []()
		//	 { FX_INFO("Debug"); } }
		//};
		//std::vector<ShortcutItem> bookmarkGroup =
		//{
		//	{ "findbookmark", findBookmark, j.value("FindBookmark", "Find Bookmark"), []()
		//	 { FX_INFO("Find Bookmark"); } },
		//	{ "addbookmark", addBookmark, j.value("AddBookmark", "Add Bookmark"), []()
		//	 { FX_INFO("Add Bookmark"); } },
		//	{ "deletebookmark", deleteBookmark, j.value("DeleteBookmark", "Delete Bookmark"), []()
		//	 { FX_INFO("Delete Bookmark"); } },
		//	{ "nextbookmark", nextBookmark, j.value("NextBookmark", "Next Bookmark"), []()
		//	 { FX_INFO("Next Bookmark"); } },
		//	{ "clearbookmarks", clearBookmark, j.value("ClearBookmarks", "Clear Bookmarks"), []()
		//	 { FX_INFO("Clear Bookmarks"); } }
		//};
		//m_ShortcutBar->AddGroup(editGroup, true);
		//m_ShortcutBar->AddGroup(buildGroup, true);
		//m_ShortcutBar->AddGroup(bookmarkGroup, false);

		uiWindow::InitDockSystem(0.0f, 130.0f, width, height - 130.0f - 35.0f);

		m_Properties = new PropertiesWindow(j.value("Properties", "Properties"));

		m_SolutionExplorer = new SolutionExplorer(j.value("SolutionExplorer", "Solution Explorer"), &m_Solution, m_Properties);

		m_SolutionExplorer->SetFileOpenCallback([this](const std::string& path)
			{
				auto* tab = m_Editor.openFile(path);
				if (tab) tab->invalidateHighlight();
				FX_CORE_INFO("Open File: {}", path);
			});
		m_OutputPanel = new OutputPanel(j.value("Output", "Output"));
		m_Windows.push_back(m_OutputPanel);
		uiWindow::DockWindow(m_OutputPanel, DockRegion::Bottom);
		auto* notifications = new uiWindow(j.value("Notifications", "Notifications"));

		m_Windows.push_back(m_SolutionExplorer);
		m_Windows.push_back(m_Properties);

		uiWindow::DockWindow(m_SolutionExplorer, DockRegion::Left);
		uiWindow::DockWindow(m_Properties, DockRegion::Right);

		for (auto* win : m_Windows)
		{
			win->OnAttach();
		}

		auto& fm = FontManager::Get();
		const float codeSize = 20.0f;

		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-Regular.ttf", codeSize, FontStyle::Regular);
		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-Bold.ttf", codeSize, FontStyle::Bold);
		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-Italic.ttf", codeSize, FontStyle::Italic);
		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-BoldItalic.ttf", codeSize, FontStyle::BoldItalic);
		// Fallback
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Normal.otf", FontStyle::Regular);
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Bold.otf", FontStyle::Bold);
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Regular.otf", FontStyle::Italic);
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Heavy.otf", FontStyle::BoldItalic);

		m_Editor.SetClipboardFunctions([]() -> std::string
			{
				auto* win = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				if (!win) return {};
				const char* s = glfwGetClipboardString(win);
				return s ? std::string(s) : std::string{};
			},
			[](const std::string& s)
			{
				auto* win = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				if (!win)
					return;
				glfwSetClipboardString(win, s.c_str());
			});
		m_Editor.SetSaveAsDialog([]() -> std::string
			{
				return SaveFileDialog("untitled.cpp", "C++ Files|*.cpp;*.h;*.hpp;*.c|All Files|*.*");
			});
		m_Editor.SetCtrlStateGetter([]() -> bool
			{
				auto* win = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				if (!win) return false;
				return glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
			});
		m_Editor.SetOnTabListChanged([this]() { SaveEditorState(); });

		m_WelcomePage.OnAttach();
		m_WelcomePage.SetCallbacks(
			[this]()
			{
				const std::string path = SaveFileDialog("Untitled.frostnux", "Frostnux Solution|*.frostnux");
				if (!path.empty())
					CreateNewSolution(path);
			},
			[this]()
			{
				const std::string path = OpenFileDialog("Frostnux Solution|*.frostnux");
				if (!path.empty())
					LoadSolution(path);
			});
		m_CurrentLangCode = LanguageManager::GetLanguageCode();

		auto& args = Application::Get().GetCommandLineArgs();
		if (args.size() > 1)
		{
			std::string projectPath = args[1];

			namespace fs = std::filesystem;
			if (fs::exists(projectPath))
			{
				if (projectPath.ends_with(".frostnux"))
				{
					LoadSolution(projectPath);
				}
				else if (fs::is_directory(projectPath))
				{
					bool foundSolution = false;
					std::error_code ec;

					for (const auto& entry : fs::directory_iterator(projectPath, ec))
					{
						if (ec) break;

						if (entry.is_regular_file() && entry.path().extension() == ".frostnux")
						{
							LoadSolution(entry.path().string());
							foundSolution = true;
							break;
						}
					}

					if (!foundSolution)
					{
						FX_CORE_ERROR("No .frostnux file found in directory (or access denied): {}", projectPath);
					}
				}
			}
			else
			{
				FX_CORE_ERROR("Path passed from command line does not exist!");
			}
		}
	}

	void uiLayer::RestoreEditorState()
	{
		auto& sm = SettingsManager::Get();
		if (!sm.HasProject())
			return;

		const auto& projSettings = sm.GetProjectSettings();
		const auto& openFiles = projSettings.openFiles;
		const auto& recentFiles = sm.GetSettings().recentFiles;

		std::string activePath;
		if (projSettings.activeFileIndex >= 0 && projSettings.activeFileIndex < (int)openFiles.size())
		{
			activePath = openFiles[projSettings.activeFileIndex];
		}
		else if (!recentFiles.empty())
		{
			activePath = recentFiles.front();
		}

		while (m_Editor.tabCount() > 0) m_Editor.closeTab(m_Editor.tabCount() - 1);

		int activeIdx = -1;
		int openIdx = 0;
		for (const auto& path : openFiles)
		{
			Tab* tab = m_Editor.openFile(path);
			if (!tab)
				continue;
			if (path == activePath)
				activeIdx = openIdx;
			++openIdx;
		}

		if (activeIdx >= 0 && activeIdx < m_Editor.tabCount())
			m_Editor.switchTabTo(activeIdx);
	}

	void uiLayer::SaveEditorState()
	{
		auto& sm = SettingsManager::Get();
		auto& settings = sm.GetSettings();

		const std::string activePath = m_Editor.GetActiveFilePath();

		if (!activePath.empty())
		{
			auto& recent = settings.recentFiles;
			recent.erase(std::remove(recent.begin(), recent.end(), activePath), recent.end());
			recent.insert(recent.begin(), activePath);
		}

		if (sm.HasProject())
		{
			auto& projSettings = sm.GetProjectSettings();
			projSettings.openFiles = m_Editor.GetOpenFiles();

			int activeIdx = -1;
			const auto& openFiles = projSettings.openFiles;
			for (size_t i = 0; i < openFiles.size(); ++i)
			{
				if (openFiles[i] == activePath) { activeIdx = (int)i; break; }
			}
			projSettings.activeFileIndex = activeIdx;

			sm.SaveProject();
		}

		sm.Save();
	}

	void uiLayer::OnDetach()
	{
		if (m_BuildThread.joinable())
			m_BuildThread.join();

		SettingsManager::Get().SaveProject();
		SettingsManager::Get().Save();

		for (auto* win : m_Windows)
			win->OnDetach();
		if (m_TitleBar)
			m_TitleBar->OnDetach();
		if (m_StatusBar)
			m_StatusBar->OnDetach();
		if (m_ShortcutBar)
			m_ShortcutBar->OnDetach();
		FontManager::Get().UnloadAll();
	}

	void uiLayer::OnUpdate(float deltaTime)
	{
		FlushOutputQueue();

		std::string langCode = LanguageManager::GetLanguageCode();
		bool isDark = ThemeManager::IsDarkTheme();
		bool needRebuildShortcuts = false;

		if (m_CurrentLangCode != langCode)
		{
			m_CurrentLangCode = langCode;
			RefreshUIText();
			needRebuildShortcuts = true;
		}
		if (m_CurrentIsDarkTheme != isDark)
		{
			m_CurrentIsDarkTheme = isDark;
			needRebuildShortcuts = true;
		}
		if (needRebuildShortcuts)
			InitShortcutBar();

		auto& app = Application::Get();
		int width = app.GetWindow().GetWidth();
		int height = app.GetWindow().GetHeight();
		if (width == 0 || height == 0)
			return;

		Renderer2D::Begin((float)width, (float)height);

		double mx, my;
		glfwGetCursorPos(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()), &mx, &my);

		float centerX = uiWindow::GetCenterX();
		float centerY = uiWindow::GetCenterY();
		float centerW = uiWindow::GetCenterW();
		float centerH = uiWindow::GetCenterH();

		float leftW = uiWindow::GetDynamicLeftWidth();
		float rightW = uiWindow::GetDynamicRightWidth();
		float bottomH = uiWindow::GetDynamicBottomHeight();

		m_Editor.update(deltaTime, glfwGetTime());
		if (m_Editor.tabCount() > 0)
		{
			m_Editor.render(centerX, centerY, centerW, centerH);
		}
		else
		{
			m_WelcomePage.Draw(centerX, centerY, centerW, centerH);
		}

		MouseCircle::Get().OnUpdate(deltaTime);

		uiWindow::DrawDockAreas();
		uiWindow::DrawSplitters();

		if (m_StatusBar)
		{
			m_StatusBar->OnUpdate(deltaTime);
		}

		DockRegion preview = uiWindow::GetPreviewRegion();
		if (preview != DockRegion::None)
		{
			float x = 0, y = 0, w = 0, h = 0;
			float mainW = uiWindow::GetMainW();
			float mainH = uiWindow::GetMainH();
			float centerW = mainW - leftW - rightW;
			float centerH = mainH - bottomH;

			switch (preview)
			{
			case DockRegion::Left:
				x = 0; y = 0; w = leftW; h = centerH;
				break;
			case DockRegion::Right:
				x = mainW - rightW; y = 0; w = rightW; h = centerH;
				break;
			case DockRegion::Bottom:
				x = 0; y = mainH - bottomH; w = mainW; h = bottomH;
				break;
			case DockRegion::Center:
				x = leftW; y = 0; w = centerW; h = centerH;
				break;
			default:
				break;
			}

			x += uiWindow::GetMainX();
			y += uiWindow::GetMainY();

			Renderer2D::DrawQuad(x, y, w, h, 0.2f, 0.5f, 0.8f, 0.5f);
		}

		if (m_TitleBar)
			m_TitleBar->OnUpdate(deltaTime);

		for (auto* win : m_Windows)
		{
			win->OnUpdate(deltaTime);
		}

		uiWindow::DrawDockPanel(mx, my);

		if (m_ShortcutBar)
		{
			m_ShortcutBar->OnUpdate(deltaTime);
			m_ShortcutBar->Draw();
		}

		PopupMenu::Get().Draw();
		TextInputDialog::Get().Draw();

		uiMessageBox::Get().Draw();

		Renderer2D::End();
	}

	bool uiLayer::OnEvent(Event& event)
	{
		if (uiMessageBox::Get().IsOpen())
		{
			if (uiMessageBox::Get().OnEvent(event)) return true;
		}

		if (event.GetEventType() == EventType::KeyPressed)
		{
			KeyPressedEvent& e = (KeyPressedEvent&)event;

			const int  key = e.GetKeyCode();
			const int  mods = e.GetMods();
			const bool ctrl = (mods & FX_KEY_CONTROL) != 0;
			const bool shift = (mods & FX_KEY_SHIFT) != 0;

			if (key == FX_KEY_F7) // Compile
			{
				BuildSolution();
				return true;
			}
			if (key == FX_KEY_F5) // Run
			{
				RunLastBuild();
				return true;
			}

			if (ctrl)
			{
				if (key == FX_KEY_S)
				{
					m_Editor.SaveActiveTab(false);
				}
				if (shift)
				{
					if (key == FX_KEY_S)
					{
						m_Editor.SaveActiveTab(true);
					}
				}
			}
		}

		if (PopupMenu::Get().HandleEvent(&event))
			return true;
		if (TextInputDialog::Get().OnEvent(event))
			return true;

		MouseCircle::Get().OnEvent(event);

		if (uiWindow::HandleSplitterEvent(event))
			return true;

		if (m_TitleBar && m_TitleBar->OnEvent(event))
			return true;

		if (m_ShortcutBar && m_ShortcutBar->OnEvent(event))
			return true;

		for (auto* win : m_Windows)
		{
			if (win->OnEvent(event))
				return true;
		}

		if (m_Editor.tabCount() > 0)
		{
			if (m_Editor.OnEvent(event))
				return true;
		}
		else
		{
			if (m_WelcomePage.OnEvent(event))
				return true;
		}

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e)
			{
				int width = e.GetWidth(), height = e.GetHeight();

				uiWindow::InitDockSystem(0.0f, 130.0f, width, height - 160.0f);

				return false;
			});

		return false;
	}

	void uiLayer::AddWindow(uiWindow* window)
	{
		if (window)
		{
			m_Windows.push_back(window);
			window->OnAttach();
		}
	}

	bool uiLayer::IsPointOverAnyWindow(float x, float y)
	{
		auto& instance = Get();
		for (auto* win : instance.m_Windows)
		{
			if (win->IsVisible())
			{
				float wx = win->GetX();
				float wy = win->GetY();
				float ww = win->GetWidth();
				float wh = win->GetHeight();

				if (x >= wx && x <= wx + ww && y >= wy && y <= wy + wh)
				{
					return true;
				}
			}
		}
		return false;
	}

	void uiLayer::RefreshUIText()
	{
		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		if (file)
			file >> j;

		if (m_TitleBar)
		{
			m_TitleBar->RefreshText();
		}

		for (auto* win : m_Windows)
		{
			if (!win) continue;

			if (win)
				win->RefreshText();
		}

		if (m_StatusBar)
		{
			m_StatusBar->SetStatusText(j.value("Ready", "Ready"));
		}
	}

	void uiLayer::QueueOutput(const std::string& line)
	{
		std::lock_guard<std::mutex> lock(m_OutputMutex);
		m_OutputQueue.push(line);
	}

	void uiLayer::FlushOutputQueue()
	{
		if (!m_OutputPanel) return;

		std::lock_guard<std::mutex> lock(m_OutputMutex);
		while (!m_OutputQueue.empty())
		{
			m_OutputPanel->AppendLine(m_OutputQueue.front());
			m_OutputQueue.pop();
		}
	}

	void uiLayer::BuildSolution()
	{
		if (m_IsBuilding) return;

		Project* startupProj = m_Solution.GetStartupProject();
		if (!startupProj)
		{
			if (m_OutputPanel) m_OutputPanel->AppendLine("[Build] Error: No startup project found.");
			return;
		}

		if (m_OutputPanel) m_OutputPanel->Clear();
		m_IsBuilding = true;

		if (m_BuildThread.joinable())
			m_BuildThread.join();

		m_BuildThread = std::thread(&uiLayer::BuildThreadFunc, this);
	}

	void uiLayer::BuildThreadFunc()
	{
		Project* startupProj = m_Solution.GetStartupProject();
		QueueOutput("[Build] Starting build for " + startupProj->name + "...");

		auto buildOrder = m_Solution.GetBuildOrder(startupProj);

		struct BuildArtifact
		{
			std::string libPath;
			std::string includeDir;
		};
		std::unordered_map<std::string, BuildArtifact> artifacts;

		std::filesystem::path rootDir = std::filesystem::current_path();
		std::filesystem::path gppPath = rootDir / "Resources" / "Compilers" / "bin" / "g++.exe";
		std::filesystem::path libexecPath = rootDir / "Resources" / "Compilers" / "libexec" / "gcc" / "x86_64-w64-mingw32" / "15.2.0";
		std::filesystem::path arPath = rootDir / "Resources" / "Compilers" / "bin" / "ar.exe";

		bool buildSuccess = true;

		for (Project* proj : buildOrder)
		{
			proj->ScanSources();
			std::string sourceFiles;
			for (const auto& src : proj->sources)
			{
				auto fullPath = proj->Directory() / src;
				sourceFiles += "\"" + fullPath.string() + "\" ";
			}

			if (sourceFiles.empty())
			{
				QueueOutput("[Build] Skipping " + proj->name + ": No sources.");
				continue;
			}

			std::filesystem::path outDir = proj->Directory() / proj->outputDir / proj->activeConfig;
			std::filesystem::create_directories(outDir);

			std::string cmd;
			std::string exePath;

			std::string depFlags = "";
			for (const auto& depName : proj->dependencies) 
			{
				if (artifacts.count(depName))
				{
					depFlags += " -I\"" + artifacts[depName].includeDir + "\" ";
					depFlags += " \"" + artifacts[depName].libPath + "\" ";
				}
			}

			QueueOutput("[Build] Building Project: " + proj->name);

			if (proj->kind == ProjectKind::StaticLib)
			{
				std::string objPath = (outDir / (proj->name + ".o")).string();
				std::string libPath = (outDir / ("lib" + proj->name + ".a")).string();
				cmd = "\"" + gppPath.string() + "\" -B \"" + libexecPath.string() + "\" -std=" + proj->standard + " -c " + sourceFiles + " -o \"" + objPath + "\" " + depFlags;
				cmd += " && \"" + arPath.string() + "\" rcs \"" + libPath + "\" \"" + objPath + "\"";
				artifacts[proj->name] = { libPath, proj->Directory().string() };
			}
			else if (proj->kind == ProjectKind::DynamicLib)
			{
				exePath = (outDir / (proj->name + ".dll")).string();
				cmd = "\"" + gppPath.string() + "\" -B \"" + libexecPath.string() + "\" -std=" + proj->standard + " -shared " + sourceFiles + " -o \"" + exePath + "\" " + depFlags;
				artifacts[proj->name] = { exePath, proj->Directory().string() };
			}
			else
			{
				exePath = (outDir / (proj->name + ".exe")).string();
				cmd = "\"" + gppPath.string() + "\" -B \"" + libexecPath.string() + "\" -std=" + proj->standard + " -g -Wall " + sourceFiles + " -o \"" + exePath + "\" " + depFlags;
				m_LastBuiltExe = exePath;
			}

			const ProjectConfig* cfg = proj->ActiveConfig();
			if (cfg)
			{
				for (const auto& inc : cfg->includeDirs) cmd += " -I\"" + (proj->Directory() / inc).string() + "\"";
				for (const auto& def : cfg->defines) cmd += " -D" + def;
				for (const auto& libDir : cfg->libraryDirs) cmd += " -L\"" + (proj->Directory() / libDir).string() + "\"";
				for (const auto& lib : cfg->libraries) cmd += " -l" + lib;
			}

			QueueOutput("[Build] Command: " + cmd);

			int exitCode = ProcessRunner::RunSync(cmd, proj->Directory().string(), [this](const std::string& line) { this->QueueOutput(line); });

			if (exitCode != 0)
			{
				QueueOutput("[Build] Failed building " + proj->name + ". Aborting.");
				buildSuccess = false;
				break;
			}
		}

		if (buildSuccess)
		{
			QueueOutput("[Build] All projects built successfully!");
		}

		m_IsBuilding = false;
	}

	void uiLayer::RunLastBuild()
	{
		if (m_LastBuiltExe.empty() || !std::filesystem::exists(m_LastBuiltExe))
		{
			if (m_OutputPanel) m_OutputPanel->AppendLine("[Run] Error: No executable found. Build first (F7).");
			return;
		}

		if (m_OutputPanel) m_OutputPanel->AppendLine("[Run] Starting " + m_LastBuiltExe + "...\n");

		ProcessRunner::Run("\"" + m_LastBuiltExe + "\"", "", [this](const std::string& line)
			{
			if (m_OutputPanel) m_OutputPanel->AppendLine(line);
			},
			[this](int exitCode)
			{
				if (m_OutputPanel) m_OutputPanel->AppendLine("\n[Run] Program exited with code: " + std::to_string(exitCode));
			}
		);
	}

	void uiLayer::InitShortcutBar()
	{
		if (!m_ShortcutBar)
			return;

		m_ShortcutBar->Clear();
		std::string langPath = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(langPath);
		nlohmann::json j;
		if (file)
			file >> j;

		const bool dark = ThemeManager::IsDarkTheme();
		const std::string suffix = dark ? "_White" : "_Black";

		auto themedPath = [&](const std::string& base) -> std::string
			{
				namespace fs = std::filesystem;
				fs::path p(base);
				std::string candidate = (p.parent_path()
					/ (p.stem().string() + suffix + p.extension().string())).string();
				return fs::exists(candidate) ? candidate : base;
			};

		const std::string findBookmark = themedPath("Resources/Images/FindBookmark.png");
		const std::string addBookmark = themedPath("Resources/Images/AddBookmark.png");
		const std::string deleteBookmark = themedPath("Resources/Images/DeleteBookmark.png");
		const std::string nextBookmark = themedPath("Resources/Images/NextBookmark.png");
		const std::string clearBookmark = themedPath("Resources/Images/ClearBookmarks.png");

		std::vector<ShortcutItem> editGroup =
		{
			{ "undo", themedPath("Resources/Images/Undo.png"), j.value("Undo", "Undo"), []()
				{ FX_INFO("Undo"); } },
			{ "redo", themedPath("Resources/Images/Redo.png"), j.value("Redo", "Redo"), []()
				{ FX_INFO("Redo"); } }
		};
		std::vector<ShortcutItem> buildGroup =
		{
			{ "build", themedPath("Resources/Images/Build.png"), j.value("Build", "Build"), [this]()
				{ BuildSolution(); } },
			{ "clean", themedPath("Resources/Images/Clean.png"), j.value("Clean", "Clean"), []()
				{ FX_INFO("Clean"); } },
			{ "run", themedPath("Resources/Images/Run.png"), j.value("Run", "Run"), [this]()
				{ RunLastBuild(); } },
			{ "debug", themedPath("Resources/Images/Debug.png"), j.value("Debug", "Debug"), []()
				{ FX_INFO("Debug"); } }
		};
		std::vector<ShortcutItem> bookmarkGroup =
		{
			{ "findbookmark", findBookmark, j.value("FindBookmark", "Find Bookmark"), []()
				{ FX_INFO("Find Bookmark"); } },
			{ "addbookmark", addBookmark, j.value("AddBookmark", "Add Bookmark"), []()
				{ FX_INFO("Add Bookmark"); } },
			{ "deletebookmark", deleteBookmark, j.value("DeleteBookmark", "Delete Bookmark"), []()
				{ FX_INFO("Delete Bookmark"); } },
			{ "nextbookmark", nextBookmark, j.value("NextBookmark", "Next Bookmark"), []()
				{ FX_INFO("Next Bookmark"); } },
			{ "clearbookmarks", clearBookmark, j.value("ClearBookmarks", "Clear Bookmarks"), []()
				{ FX_INFO("Clear Bookmarks"); } }
		};

		m_ShortcutBar->AddGroup(editGroup, true);
		m_ShortcutBar->AddGroup(buildGroup, true);
		m_ShortcutBar->AddGroup(bookmarkGroup, false);
	}

}
