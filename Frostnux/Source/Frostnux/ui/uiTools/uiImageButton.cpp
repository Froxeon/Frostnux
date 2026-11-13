#include <fxpch.h>
#include "uiImageButton.h"
#include "MouseCircle.h"
#include "PhotoRenderer.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/Core/MouseButtonCodes.h"

namespace Frostnux {

	uiImageButton::uiImageButton(const std::string& imagePath, float x, float y, float width, float height, ButtonStyles style)
		: m_ImagePath(imagePath), m_X(x), m_Y(y), m_Width(width), m_Height(height), m_ButtonStyle(style)
	{
		if (ThemeManager::IsDarkTheme())
		{
			m_Color[0] = 0.1f; m_Color[1] = 0.1f; m_Color[2] = 0.1f; m_Color[3] = 1.0f;
		}
		else
		{
			m_Color[0] = 0.9f; m_Color[1] = 0.9f; m_Color[2] = 0.9f; m_Color[3] = 1.0f;
		}

		m_Image.reset(new PhotoRenderer());
		m_Image->LoadFromFile(imagePath);
	}

	uiImageButton::~uiImageButton() {}

	static bool IsSegmentIntersectCircle(float x1, float y1, float x2, float y2, float cx, float cy, float radius)
	{
		float dx = x2 - x1;
		float dy = y2 - y1;
		float t = ((cx - x1) * dx + (cy - y1) * dy) / (dx * dx + dy * dy);
		t = std::max(0.0f, std::min(1.0f, t));
		float closestX = x1 + t * dx;
		float closestY = y1 + t * dy;
		float distSq = (closestX - cx) * (closestX - cx) + (closestY - cy) * (closestY - cy);
		return distSq <= radius * radius;
	}

	void uiImageButton::Draw(float parentX, float parentY) const
	{
		float absX = parentX + m_X;
		float absY = parentY + m_Y;

		float r = m_Color[0], g = m_Color[1], b = m_Color[2];

		if (m_UseThemeColor)
		{
			if (ThemeManager::IsDarkTheme())	r = g = b = 0.10f;
			else								r = g = b = 0.90f;
		}

		if (m_ButtonStyle != ButtonStyles::NoBackgroundOrLine)
		{
			if (m_ButtonStyle != ButtonStyles::NoBackground)
			{
				float bgR = r, bgG = g, bgB = b;
				if (m_Hovered)
				{
					float delta = ThemeManager::IsDarkTheme() ? 0.05f : -0.05f;
					bgR = std::clamp(bgR + delta, 0.0f, 1.0f);
					bgG = std::clamp(bgG + delta, 0.0f, 1.0f);
					bgB = std::clamp(bgB + delta, 0.0f, 1.0f);
				}
				Renderer2D::DrawQuad(absX, absY, m_Width, m_Height, bgR, bgG, bgB, m_Color[3]);
			}

			if (m_ButtonStyle != ButtonStyles::NoLine)
			{
				auto& circle = MouseCircle::Get();
				float cx = circle.GetX(), cy = circle.GetY();
				float radius = circle.GetRadius();

				const float highlightColor[3] = { 0.3f, 0.3f, 0.3f };
				const float minAlpha = 0.01f;
				const float maxAlpha = 0.3f;

				int segments = std::max(4, (int)(std::max(m_Width, m_Height) / 8.0f));

				auto drawSegment = [&](float x1, float y1, float x2, float y2)
					{
						float dx = x2 - x1, dy = y2 - y1;
						float lenSq = dx * dx + dy * dy;
						float t = 0.0f;
						if (lenSq > 0.0f)
						{
							t = ((cx - x1) * dx + (cy - y1) * dy) / lenSq;
							t = std::max(0.0f, std::min(1.0f, t));
						}
						float closestX = x1 + t * dx;
						float closestY = y1 + t * dy;
						float dist = std::sqrt((closestX - cx) * (closestX - cx) + (closestY - cy) * (closestY - cy));

						float alpha = (dist >= radius) ? minAlpha : minAlpha + (1.0f - dist / radius) * (maxAlpha - minAlpha);

						Renderer2D::DrawLine(x1, y1, x2, y2, 1.0f, highlightColor[0], highlightColor[1], highlightColor[2], alpha);
					};

				for (int i = 0; i < segments; ++i)
				{
					float t0 = (float)i / segments;
					float t1 = (float)(i + 1) / segments;
					drawSegment(absX + m_Width * t0, absY, absX + m_Width * t1, absY);
					drawSegment(absX + m_Width * t0, absY + m_Height, absX + m_Width * t1, absY + m_Height);
					drawSegment(absX, absY + m_Height * t0, absX, absY + m_Height * t1);
					drawSegment(absX + m_Width, absY + m_Height * t0, absX + m_Width, absY + m_Height * t1);
				}
			}
		}
		else
		{
			if (m_Hovered)
			{
				Renderer2D::DrawQuad(absX, absY, m_Width, m_Height, r, g, b, std::max(0.0f, m_Color[3] - 0.85f));
			}
		}

		if (!m_ImagePath.empty())
		{
			m_Image->Draw(absX, absY, m_Width, m_Height);
		}

		if (m_Hovered && m_HoverTintAlpha > 0.0f)
		{
			const bool isDark = ThemeManager::IsDarkTheme();
			if (isDark)
				Renderer2D::DrawQuad(absX, absY, m_Width, m_Height, 1, 1, 1, m_HoverTintAlpha);
			else
				Renderer2D::DrawQuad(absX, absY, m_Width, m_Height, 0, 0, 0, m_HoverTintAlpha);
		}
	}

	bool uiImageButton::IsPointInside(float px, float py, float parentX, float parentY) const
	{
		float absX = parentX + m_X;
		float absY = parentY + m_Y;
		return (px >= absX && px <= absX + m_Width && py >= absY && py <= absY + m_Height);
	}

	void uiImageButton::OnUpdate(float parentX, float parentY, bool parentVisible)
	{
		if (!parentVisible) return;
		Draw(parentX, parentY);
	}

	bool uiImageButton::OnEvent(Event& event, float parentX, float parentY, bool parentVisible)
	{
		if (!parentVisible) return false;

		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& e = (MouseMovedEvent&)event;
			bool inside = IsPointInside(e.GetX(), e.GetY(), parentX, parentY);
			if (inside != m_Hovered)
			{
				m_Hovered = inside;
				if (inside)
				{
					m_HoverTimer = 0.0f;
					m_ShowTooltip = false;
				}
				else
				{
					m_ShowTooltip = false;
				}
			}
			return false;
		}
		else if (event.GetEventType() == EventType::MouseButtonPressed)
		{
			MouseButtonPressedEvent& e = (MouseButtonPressedEvent&)event;
			if (e.GetMouseButton() == FX_MOUSE_BUTTON_LEFT && IsPointInside(e.GetMouseX(), e.GetMouseY(), parentX, parentY))
			{
				if (m_Callback) m_Callback();
				return true;
			}
		}
		return false;
	}

	void uiImageButton::OnTooltipUpdate(float deltaTime)
	{
		if (m_Hovered && !m_ShowTooltip)
		{
			m_HoverTimer += deltaTime;
			if (m_HoverTimer >= 0.5f)
				m_ShowTooltip = true;
		}
	}

	void uiImageButton::DrawTooltip(float parentX, float parentY) const
	{
		if (!m_ShowTooltip || m_Tooltip.empty()) return;

		float absX = parentX + m_X;
		float absY = parentY + m_Y;

		float textWidth = TextRenderer::Get().GetTextWidth(m_Tooltip);
		float textHeight = TextRenderer::Get().GetTextHeight();
		float padding = 8.0f;
		float tooltipW = textWidth + padding * 2;
		float tooltipH = textHeight + padding * 2;
		float tooltipX = absX + m_Width / 2 - tooltipW / 2;
		float tooltipY = absY + tooltipH - 5;

		if (tooltipY < 0) tooltipY = absY + m_Height + 5;

		const bool isDark = ThemeManager::IsDarkTheme();
		if (isDark)
			Renderer2D::DrawQuad(tooltipX, tooltipY, tooltipW, tooltipH, 0.10f, 0.10f, 0.10f, 0.85f);
		else
			Renderer2D::DrawQuad(tooltipX, tooltipY, tooltipW, tooltipH, 0.98f, 0.98f, 0.98f, 0.95f);

		float textCol = isDark ? 1.0f : 0.05f;
		float textX = tooltipX + padding;
		float textY = tooltipY + (tooltipH - textHeight) / 2;
		TextRenderer::Get().DrawTextUTF8(m_Tooltip, textX, textY, textCol, textCol, textCol, 0.9f);
	}

}
