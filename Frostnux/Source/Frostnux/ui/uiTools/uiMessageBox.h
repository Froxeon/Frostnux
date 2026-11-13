#pragma once
#include <string>
#include <functional>
#include "Frostnux/Events/Event.h"

namespace Frostnux {

	class uiMessageBox
	{
	public:
		static uiMessageBox& Get();

		void Open(const std::string& title, const std::string& message, std::function<void()> onConfirm);

		void Draw();
		bool OnEvent(Event& event);

		bool IsOpen() const { return m_IsOpen; }
	private:
		uiMessageBox() = default;
		~uiMessageBox() = default;

		void CalculateLayout(float winW, float winH);

		bool m_IsOpen = false;
		std::string m_Title;
		std::string m_Message;

		std::function<void()> m_OnConfirm;

		bool m_HoverConfirm = false;
		bool m_HoverCancel = false;

		float m_BoxW = 0.0f;
		float m_BoxH = 0.0f;
		float m_BoxX = 0.0f;
		float m_BoxY = 0.0f;
	};
}