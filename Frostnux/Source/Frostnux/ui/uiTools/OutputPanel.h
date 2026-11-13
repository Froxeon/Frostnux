#pragma once
#include "uiWindow.h"
#include <vector>
#include <mutex>

namespace Frostnux {

	struct OutPos
	{
		int line = 0;
		int col = 0;
		bool operator==(const OutPos& o) const { return line == o.line && col == o.col; }
		bool operator<(const OutPos& o) const { return line != o.line ? line < o.line : col < o.col; }
	};

	class OutputPanel : public uiWindow
	{
	public:
		OutputPanel(const std::string& name);
		virtual ~OutputPanel();

		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;
		virtual void DrawContent() override;

		void AppendLine(const std::string& text);
		void Clear();

		void SetScrollToBottom() { m_ScrollY = -1.0f; }

	private:
		std::vector<std::string> m_Lines;
		mutable std::mutex m_Mutex;
		float m_ScrollY = -1.0f;
		float m_LineHeight = 18.0f;
		float m_TotalHeight = 0.0f;

		OutPos m_SelAnchor;
		OutPos m_SelActive;
		bool m_IsSelecting = false;

		[[nodiscard]] OutPos GetPosAt(float x, float y) const;
		[[nodiscard]] std::string GetSelectedText() const;
	};
}