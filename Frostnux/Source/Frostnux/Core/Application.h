#pragma once

#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/ApplicationEvent.h"
#include "Window.h"
#include "LayerStack.h"

namespace Frostnux {

	class Application
	{
	public:
		Application(const std::vector<std::string>& args = {});
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(class Layer* layer);
		void PushOverlay(class Layer* overlay);

		Window* CreateGLFWWindow(const WindowProps& props);

		inline static Application* GetPointer() { return s_Instance; }
		inline static Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_MainWindow; }
		inline Window* GetWindowPointer() { return m_MainWindow.get(); }

		static bool HasInstance() { return s_Instance != nullptr; }

		const std::vector<std::string>& GetCommandLineArgs() const { return m_CommandLineArgs; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<class Window>			m_MainWindow;
		std::vector<std::unique_ptr<Window>>	m_Windows;
		std::vector<std::string>				m_CommandLineArgs;
		LayerStack								m_LayerStack;
		double									unsemi_transparency = 0.99;

		bool m_Running = true;

		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Frostnux::Application* CreateApplication(const std::vector<std::string>& args = {});

}
