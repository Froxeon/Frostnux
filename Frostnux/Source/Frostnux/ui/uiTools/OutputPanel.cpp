#include "fxpch.h"
#include "OutputPanel.h"
#include "TextRenderer.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/Core/Application.h"
#include "Frostnux/Events/MouseEvent.h"
#include "Frostnux/Events/KeyEvent.h"
#include "../CodeEditor/CodeEditor.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Frostnux {

	OutputPanel::OutputPanel(const std::string& name)
		: uiWindow(name)
	{}

	OutputPanel::~OutputPanel()
	{}

	void OutputPanel::AppendLine(const std::string& text)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Lines.push_back(text);

		if (m_Lines.size() > 5000)
			m_Lines.erase(m_Lines.begin(), m_Lines.begin() + 1000);

		if (m_ScrollY >= 0.0f)
			m_ScrollY += m_LineHeight;
	}

	void OutputPanel::Clear()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Lines.clear();
		m_ScrollY = -1.0f;
	}

	void OutputPanel::OnUpdate(float deltaTime)
	{
		DrawContent();
	}

	bool OutputPanel::OnEvent(Event& event)
	{
		if (uiWindow::OnEvent(event)) return true;

		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e)
			{
				if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return false;

				float contentX = m_RectX + 5;
				float contentY = m_RectY + 30;
				float contentW = m_RectWidth - 10;
				float contentH = m_RectHeight - 35;

				if (e.GetMouseX() >= contentX && e.GetMouseX() <= contentX + contentW && e.GetMouseY() >= contentY && e.GetMouseY() <= contentY + contentH)
				{
					m_IsSelecting = true;
					m_SelAnchor = GetPosAt(e.GetMouseX(), e.GetMouseY());
					m_SelActive = m_SelAnchor;
					return true;
				}
				return false;
			});

		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e)
			{
				if (m_IsSelecting)
				{
					m_SelActive = GetPosAt(e.GetX(), e.GetY());
					return true;
				}
				return false;
			});

		// 鼠标释放：结束选择
		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e)
			{
				if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && m_IsSelecting)
				{
					m_IsSelecting = false;
					return true;
				}
				return false;
			});

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				int mods = e.GetMods();
				if ((mods & FX_KEY_CONTROL) == 0) return false;

				if (e.GetKeyCode() == GLFW_KEY_C)
				{
					std::string sel = GetSelectedText();
					if (!sel.empty())
					{
						GLFWwindow* win = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
						glfwSetClipboardString(win, sel.c_str());
					}
					return true;
				}
				if (e.GetKeyCode() == GLFW_KEY_A)
				{
					std::lock_guard<std::mutex> lock(m_Mutex);
					m_SelAnchor = { 0, 0 };
					if (!m_Lines.empty())
						m_SelActive = { (int)m_Lines.size() - 1, (int)m_Lines.back().size() };
					return true;
				}
				return false;
			});

		return false;
	}

	OutPos OutputPanel::GetPosAt(float x, float y) const
	{
		float contentX = m_RectX + 5;
		float contentY = m_RectY + 30;
		float contentH = m_RectHeight - 35;

		float localY = y - contentY + m_ScrollY;
		int line = (int)(localY / m_LineHeight);
		line = std::clamp(line, std::min(0, (int)m_Lines.size() - 1), std::max(0, (int)m_Lines.size() - 1));

		std::lock_guard<std::mutex> lock(m_Mutex);
		if (line < 0 || line >= (int)m_Lines.size()) return { 0, 0 };

		std::string text = m_Lines[line];
		float localX = x - contentX;

		int col = 0;
		float acc = 0;
		for (size_t i = 0; i < text.size(); ++i)
		{
			float w = TextRenderer::Get().GetTextWidth(text.substr(i, 1));
			if (acc + w * 0.5f > localX)
				break;
			acc += w;
			col++;
		}

		return { line, col };
	}

	std::string OutputPanel::GetSelectedText() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_SelAnchor == m_SelActive || m_Lines.empty()) return "";

		OutPos start = std::min(m_SelAnchor, m_SelActive);
		OutPos end = std::max(m_SelAnchor, m_SelActive);

		std::string result;
		for (int i = start.line; i <= end.line; ++i)
		{
			if (i >= (int)m_Lines.size()) break;
			const std::string& line = m_Lines[i];
			int colStart = (i == start.line) ? start.col : 0;
			int colEnd = (i == end.line) ? end.col : (int)line.size();

			colStart = std::clamp(colStart, 0, (int)line.size());
			colEnd = std::clamp(colEnd, 0, (int)line.size());

			if (i > start.line) result += "\n";
			result += line.substr(colStart, colEnd - colStart);
		}
		return result;
	}

	void OutputPanel::DrawContent()
	{
		uiWindow::DrawContent();

		bool isDark = ThemeManager::IsDarkTheme();
		Renderer2D::DrawQuad(m_RectX + 1, m_RectY + 30, m_RectWidth - 2, m_RectHeight - 31, isDark ? 0.1f : 0.95f, isDark ? 0.1f : 0.95f, isDark ? 0.1f : 0.95f, 1.0f);

		float contentX = m_RectX + 5;
		float contentY = m_RectY + 30;
		float contentW = m_RectWidth - 10;
		float contentH = m_RectHeight - 35;

		Application& app = Application::Get();
		int winHeight = app.GetWindow().GetHeight();

		glEnable(GL_SCISSOR_TEST);
		glScissor((int)contentX, winHeight - (int)(contentY + contentH), (int)contentW, (int)contentH);

		std::lock_guard<std::mutex> lock(m_Mutex);

		m_TotalHeight = m_Lines.size() * m_LineHeight;
		float maxScroll = std::max(0.0f, m_TotalHeight - contentH);

		if (m_ScrollY < 0.0f)
			m_ScrollY = maxScroll;

		m_ScrollY = std::clamp(m_ScrollY, 0.0f, maxScroll);

		float startY = contentY - m_ScrollY;

		Color textColor = isDark ? Color{ 0.8f, 0.8f, 0.8f, 1.0f } : Color{ 0.2f, 0.2f, 0.2f, 1.0f };
		Color selColor = isDark ? Color{ 0.2f, 0.4f, 0.6f, 0.5f } : Color{ 0.6f, 0.8f, 1.0f, 0.5f };

		OutPos selStart = std::min(m_SelAnchor, m_SelActive);
		OutPos selEnd = std::max(m_SelAnchor, m_SelActive);
		bool hasSelection = !(m_SelAnchor == m_SelActive);

		for (size_t i = 0; i < m_Lines.size(); ++i)
		{
			float y = startY + i * m_LineHeight;
			if (y + m_LineHeight < contentY) continue;
			if (y > contentY + contentH) break;

			const std::string& line = m_Lines[i];

			if (hasSelection && (int)i >= selStart.line && (int)i <= selEnd.line)
			{
				int colStart = (i == (size_t)selStart.line) ? selStart.col : 0;
				int colEnd = (i == (size_t)selEnd.line) ? selEnd.col : (int)line.size();

				colStart = std::clamp(colStart, 0, (int)line.size());
				colEnd = std::clamp(colEnd, 0, (int)line.size());

				if (colEnd > colStart)
				{
					float x0 = contentX + TextRenderer::Get().GetTextWidth(line.substr(0, colStart));
					float x1 = contentX + TextRenderer::Get().GetTextWidth(line.substr(0, colEnd));

					if (x1 <= x0) x1 = x0 + 4.0f;

					Renderer2D::DrawQuad(x0, y, x1 - x0, m_LineHeight, selColor.r, selColor.g, selColor.b, selColor.a);
				}
			}

			TextRenderer::Get().DrawTextUTF8(line, contentX + 2, y + 2, textColor.r, textColor.g, textColor.b, textColor.a);
		}

		glDisable(GL_SCISSOR_TEST);
	}

}
