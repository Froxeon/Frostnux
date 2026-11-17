#pragma once
#include "uiWindow.h"
#include "PhotoRenderer.h"
#include "PropertiesWindow.h"
#include "Frostnux/ProjectSystem/Solution.h"

namespace Frostnux {

	class SolutionExplorer : public uiWindow
	{
	public:
		SolutionExplorer(const std::string& name, Solution* solution, PropertiesWindow* properties);
		virtual ~SolutionExplorer();

		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;
		virtual void DrawContent() override;

		void SetSolution(Solution* s);
		void SetRootFolder(const std::filesystem::path& folderPath);
		void Refresh();

		void SetFileOpenCallback(std::function<void(const std::string&)> callback)
		{
			m_FileOpenCallback = std::move(callback);
		}

		static float GetScrollY() { return m_ScrollY; }
		static void SetScrollY(float y) { m_ScrollY = y; }

		virtual void RefreshText() override;
	private:
		enum class NodeKind : uint8_t { SolutionRoot, Project, Folder, File };

		struct Node
		{
			std::string			name;
			std::string			fullPath;
			std::string			language = "cpp";
			std::vector<Node>	children;

			int					projectIndex = -1;

			NodeKind			kind = NodeKind::File;
			bool				expanded = false;
		};

		struct VisibleNode
		{
			const Node* node;
			int			depth;
			float		y;
		};

		struct LanguageConfig
		{
			std::string id;				// ID, For example "cpp", "cs"
			std::string displayName;	// Name, For example "C++", "C#"
			std::string projectIcon;	// Icon Path
		};

		Solution* m_Solution = nullptr;
		PropertiesWindow* m_Properties = nullptr;
		std::filesystem::path m_RootFolder;
		std::function<void(const std::string&)>	m_FileOpenCallback;

		Node					m_Root;
		mutable std::vector<VisibleNode> m_VisibleNodes;

		std::unique_ptr<PhotoRenderer> m_Folder_Close_Icon;
		std::unique_ptr<PhotoRenderer> m_Folder_Open_Icon;
		std::unique_ptr<PhotoRenderer> m_CPP_File_Icon;
		std::unique_ptr<PhotoRenderer> m_Header_File_Icon;
		std::unique_ptr<PhotoRenderer> m_Python_File_Icon;
		std::unique_ptr<PhotoRenderer> m_CS_File_Icon;
		std::unique_ptr<PhotoRenderer> m_File_Icon;
		std::unique_ptr<PhotoRenderer> m_Solution_Icon;

		std::vector<LanguageConfig> m_SupportedLanguages;
		std::unordered_map<std::string, std::unique_ptr<PhotoRenderer>> m_ProjectIcons;

		const Node* m_HoveredNode = nullptr;
		const Node* m_SelectNode = nullptr;
		const Node* m_LastClickedNode = nullptr;

		float			m_LineHeight = 0.0f;
		float			m_TotalHeight = 0.0f;
		float			m_DragStartY = 0.0f;
		float			m_DragStartScrollY = 0.0f;
		float			m_LastClickTime = 0.0f;

		bool m_IsDraggingScrollbar = false;
		bool m_CurrentIsDarkTheme = false;

		static float	m_ScrollY;

		void RefreshThemeIcons();

		void BuildTree();
		void InsertFileRecursive(Node& parent, const std::filesystem::path& relPath, const std::filesystem::path& absPath);
		void InsertFolderRecursive(Node& parent, const std::filesystem::path& relPath);

		void DrawNode(const Node& node, int depth, float& y, float x, float width);
		void CalcTreeHeight(const Node& node, float& total) const;
		void BuildVisibleList(const Node& node, int depth, float startY, float& yOffset) const;

		void DrawFolderIcon(float x, float y, bool expanded) const;
		void DrawFileIcon(float x, float y, const std::string& filepath) const;

		void ToggleNode(const Node* target);
		bool IsFolderLike(const Node& n) const { return n.kind != NodeKind::File; }

		void DeleteSelectedNode();
	};

}
