#pragma once
#include <fxpch.h>
#include "uiImageButton.h"
#include <vector>
#include <functional>

namespace Frostnux {

	struct ShortcutItem
	{
		std::string id;
		std::string path;
		std::string tooltip;
		std::function<void()> callback;
		bool enabled = true;

		ShortcutItem() = default;
		ShortcutItem(const std::string& id_, const std::string& path_, const std::string& tooltip_, std::function<void()> callback_, bool enabled_ = true)
			: id(id_), path(path_), tooltip(tooltip_), callback(callback_), enabled(enabled_)
		{
		}
	};

	struct ShortcutGroup
	{
		std::vector<ShortcutItem> items;
		std::vector<uiImageButton*> buttons;
		bool hasSeparatorAfter = false;
	};

	class uiShortcutBar
	{
	public:
		uiShortcutBar();
		~uiShortcutBar();

		void OnAttach();
		void OnDetach();
		void OnUpdate(float deltaTime);
		bool OnEvent(Event& event);
		void Draw();

		void AddGroup(const std::vector<ShortcutItem>& items, bool separatorAfter = false);
		void Clear();
	private:
		std::vector<std::unique_ptr<uiImageButton>> m_Buttons;
		std::vector<ShortcutGroup> m_Groups;
		std::string m_CurrentLangCode;
		float m_BarHeight = 45.0f;
		float m_ButtonWidth = 35.0f;
		float m_ButtonHeight = 35.0f;
		float m_Padding = 5.0f;
		float m_SeparatorMargin = 5.0f;
		float m_OffsetY = 80.0f;

		std::vector<float> m_GroupEndX;
		void RebuildButtons();
	};

}