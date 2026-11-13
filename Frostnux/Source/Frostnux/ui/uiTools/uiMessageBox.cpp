#include <fxpch.h>
#include "uiMessageBox.h"
#include "TextRenderer.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/Core/Application.h"
#include "Frostnux/Events/MouseEvent.h"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace Frostnux {

	uiMessageBox& uiMessageBox::Get()
	{
		static uiMessageBox instance;
		return instance;
	}

	void uiMessageBox::Open(const std::string& title, const std::string& message, std::function<void()> onConfirm)
	{
		m_Title = title;
		m_Message = message;
		m_OnConfirm = onConfirm;
		m_IsOpen = true;
	}

	void uiMessageBox::CalculateLayout(float winW, float winH)
	{
		if (!TextRenderer::Get().IsInitialized())
		{
			m_BoxW = 400.0f;
			m_BoxH = 200.0f;
			m_BoxX = (winW - m_BoxW) * 0.5f;
			m_BoxY = (winH - m_BoxH) * 0.5f;
			return;
		}

		float titleW = TextRenderer::Get().GetTextWidth(m_Title);
		float msgW = TextRenderer::Get().GetTextWidth(m_Message);
		float maxContentW = std::max(titleW, msgW);

		m_BoxW = std::max(300.0f, maxContentW + 60.0f);
		m_BoxW = std::min(m_BoxW, std::min(winW - 80.0f, 600.0f));

		int lines = 1;
		for (char c : m_Message) if (c == '\n') lines++;

		float lineH = TextRenderer::Get().GetLineHeight();
		m_BoxH = 40.0f + lineH + 20.0f + (lines * lineH) + 60.0f;

		m_BoxX = (winW - m_BoxW) * 0.5f;
		m_BoxY = (winH - m_BoxH) * 0.5f;
	}

	void uiMessageBox::Draw()
	{
		if (!m_IsOpen) return;

		auto& app = Application::Get();
		float winW = (float)app.GetWindow().GetWidth();
		float winH = (float)app.GetWindow().GetHeight();

		nlohmann::json j;
		std::ifstream file("Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json");
		file >> j;

		CalculateLayout(winW, winH);

		Renderer2D::DrawQuad(0, 0, winW, winH, 0.0f, 0.0f, 0.0f, 0.6f);

		Renderer2D::DrawQuad(m_BoxX, m_BoxY, m_BoxW, m_BoxH, 0.15f, 0.15f, 0.18f, 1.0f);
		Renderer2D::DrawLine(m_BoxX, m_BoxY, m_BoxX + m_BoxW, m_BoxY, 2.0f, 0.3f, 0.3f, 0.3f, 1.0f);
		Renderer2D::DrawLine(m_BoxX + m_BoxW, m_BoxY, m_BoxX + m_BoxW, m_BoxY + m_BoxH, 2.0f, 0.3f, 0.3f, 0.3f, 1.0f);
		Renderer2D::DrawLine(m_BoxX + m_BoxW, m_BoxY + m_BoxH, m_BoxX, m_BoxY + m_BoxH, 2.0f, 0.3f, 0.3f, 0.3f, 1.0f);
		Renderer2D::DrawLine(m_BoxX, m_BoxY + m_BoxH, m_BoxX, m_BoxY, 2.0f, 0.3f, 0.3f, 0.3f, 1.0f);

		if (TextRenderer::Get().IsInitialized())
		{
			TextRenderer::Get().DrawTextUTF8(m_Title, m_BoxX + 30, m_BoxY + 20, 1.0f, 1.0f, 1.0f, 1.0f);
			TextRenderer::Get().DrawTextUTF8(m_Message, m_BoxX + 30, m_BoxY + 60, 0.8f, 0.8f, 0.8f, 1.0f);
		}

		double mx, my;
		glfwGetCursorPos(static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow()), &mx, &my);
		float mouseX = (float)mx;
		float mouseY = (float)my;

		float btnW = 100.0f;
		float btnH = 36.0f;
		float btnY = m_BoxY + m_BoxH - 56.0f;

		float confirmX = m_BoxX + m_BoxW - 240.0f;
		float cancelX = m_BoxX + m_BoxW - 120.0f;

		m_HoverConfirm = (mouseX >= confirmX && mouseX <= confirmX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
		m_HoverCancel = (mouseX >= cancelX && mouseX <= cancelX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);

		Renderer2D::DrawQuad(confirmX, btnY, btnW, btnH, m_HoverConfirm ? 0.8f : 0.5f, 0.2f, 0.2f, 1.0f);
		if (TextRenderer::Get().IsInitialized())
			TextRenderer::Get().DrawTextUTF8(j.value("Confirm", "Confirm"), confirmX + 40, btnY + 10, 1.0f, 1.0f, 1.0f, 1.0f);

		Renderer2D::DrawQuad(cancelX, btnY, btnW, btnH, m_HoverCancel ? 0.4f : 0.3f, 0.3f, 0.3f, 1.0f);
		if (TextRenderer::Get().IsInitialized())
			TextRenderer::Get().DrawTextUTF8(j.value("Cancel", "Cancel"), cancelX + 25, btnY + 10, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	bool uiMessageBox::OnEvent(Event& event)
	{
		if (!m_IsOpen) return false;

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e)
			{
				if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
				{
					float mx = e.GetMouseX();
					float my = e.GetMouseY();

					auto& app = Application::Get();
					float winW = (float)app.GetWindow().GetWidth();
					float winH = (float)app.GetWindow().GetHeight();

					CalculateLayout(winW, winH);

					float btnW = 100.0f;
					float btnH = 36.0f;
					float btnY = m_BoxY + m_BoxH - 56.0f;

					float confirmX = m_BoxX + m_BoxW - 240.0f;
					float cancelX = m_BoxX + m_BoxW - 120.0f;

					if (mx >= confirmX && mx <= confirmX + btnW && my >= btnY && my <= btnY + btnH)
					{
						m_IsOpen = false;
						if (m_OnConfirm) m_OnConfirm();
						return true;
					}

					if (mx >= cancelX && mx <= cancelX + btnW && my >= btnY && my <= btnY + btnH)
					{
						m_IsOpen = false;
						return true;
					}
				}

				return true;
			});

		return true;
	}
}
