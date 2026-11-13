#pragma once
#include "Frostnux/Core/Layer.h"
#include "CodeEditor/ProjectIndex.h"
#include "CodeEditor/CodeEditor.h"
#include "CodeEditor/GLRenderer.h"
#include "Frostnux/Managers/FontManager.h"
#include "Frostnux/ProjectSystem/Solution.h"
#include "uiTools/ui.h"

#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <string>

namespace Frostnux {

	class uiWindow;
	class uiTitleBar;
	class uiButton;
	class MouseCircle;

	class uiLayer : public Layer
	{
	public:
		static uiLayer& Get();

		uiLayer();
		virtual ~uiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;

		void AddWindow(uiWindow* window);

		static bool IsPointOverAnyWindow(float x, float y);

		static void SaveEditorState();
		static void RestoreEditorState();

		static CodeEditor& GetCodeEditor();

		void LoadSolution(const std::string& path);
		void CreateNewSolution(const std::string& path);

		void BuildSolution();
		void RunLastBuild();
	private:
		std::vector<uiWindow*> m_Windows;
		uiTitleBar* m_TitleBar = nullptr;
		uiStatusBar* m_StatusBar = nullptr;
		uiShortcutBar* m_ShortcutBar = nullptr;
		SolutionExplorer* m_SolutionExplorer = nullptr;
		PropertiesWindow* m_Properties = nullptr;

		static GLRenderer m_EditorRenderer;
		static CodeEditor m_Editor;
		ProjectIndex m_ProjectIndex;
		WelcomePage m_WelcomePage;
		Solution m_Solution;
		OutputPanel* m_OutputPanel = nullptr;

		std::string m_CurrentLangCode;
		std::string m_LastBuiltExe;
		bool m_CurrentIsDarkTheme = false;

		std::thread m_BuildThread;
		std::atomic<bool> m_IsBuilding{ false };
		std::mutex m_OutputMutex;
		std::queue<std::string> m_OutputQueue;

		void InitShortcutBar();

		void BuildThreadFunc();
		void QueueOutput(const std::string& line);
		void FlushOutputQueue();
		void RefreshUIText();
	};

}
