#include <fxpch.h>
#include "SolutionExplorer.h"
#include "TextRenderer.h"
#include "Frostnux/Core/Log.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/Core/Application.h"
#include "../uiLayer.h"
#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/MouseEvent.h"
#include "Frostnux/Events/KeyEvent.h"
#include "PopupMenu.h"
#include "TextInputDialog.h"
#include "uiMessageBox.h"
#include "Frostnux/Core/FileDialog.h"

namespace Frostnux {

	float SolutionExplorer::m_ScrollY = 0.0f;

	namespace {
		bool ShouldIgnoreFolder(const std::string& name)
		{
			if (name.empty() || name[0] == '.') return true;
			static const char* kIgnored[] =
			{
				"build", "Build", "out", "Out", "bin", "Bin",
				"obj", "Obj", "x64", "x86", "Debug", "Release",
				"CMakeFiles", "node_modules"
			};
			for (auto* s : kIgnored)
				if (name == s) return true;
			return false;
		}
	}

	SolutionExplorer::SolutionExplorer(const std::string& name, Solution* solution, PropertiesWindow* properties)
		: uiWindow(name), m_Solution(solution), m_Properties(properties)
	{
		m_LineHeight = s_FontSize * 1.5f;
		SetSize(0, 110, 250, 600);

		const bool dark = ThemeManager::IsDarkTheme();

		m_Folder_Close_Icon.reset(new PhotoRenderer());
		m_Folder_Open_Icon.reset(new PhotoRenderer());
		m_CPP_File_Icon.reset(new PhotoRenderer());
		m_Header_File_Icon.reset(new PhotoRenderer());
		m_Python_File_Icon.reset(new PhotoRenderer());
		m_CS_File_Icon.reset(new PhotoRenderer());
		m_File_Icon.reset(new PhotoRenderer());
		m_Solution_Icon.reset(new PhotoRenderer());

		m_Folder_Close_Icon->LoadFromFile("Resources/Images/Folder_Close.png");
		m_Folder_Open_Icon->LoadFromFile("Resources/Images/Folder_Open.png");
		m_CPP_File_Icon->LoadFromFile("Resources/Images/CPP_File.png");
		if (dark)
			m_Header_File_Icon->LoadFromFile("Resources/Images/Header_File_White.png");
		else
			m_Header_File_Icon->LoadFromFile("Resources/Images/Header_File_Black.png");
		m_Python_File_Icon->LoadFromFile("Resources/Images/Python_File.png");
		m_CS_File_Icon->LoadFromFile("Resources/Images/CS_File.png");
		if (dark)
			m_File_Icon->LoadFromFile("Resources/Images/File_White.png");
		else
			m_File_Icon->LoadFromFile("Resources/Images/File_Black.png");
		m_Solution_Icon->LoadFromFile("Resources/Images/Solution.png");

		m_SupportedLanguages = {
			{ "cpp",		"C++ Project",		"Resources/Images/CppProject.png" }
			/*
			{ "cs",			"C# Project",		"Resources/Images/CSProject.png" },
			{ "py",			"PythonProject",		"Resources/Images/PythonProject.png" },
			{ "java",		"Java Project",		"Resources/Images/JavaProject.png" },
			{ "go",			"Go Project",		"Resources/Images/GoProject.png" },
			{ "rust",		"Rust Project",		"Resources/Images/RustProject.png" },
			{ "kotlin",		"Kotlin Project",	"Resources/Images/KotlinProject.png" }
			*/
		};

		for (const auto& lang : m_SupportedLanguages)
		{
			auto icon = std::make_unique<PhotoRenderer>();
			icon->LoadFromFile(lang.projectIcon);
			m_ProjectIcons[lang.id] = std::move(icon);
		}

		RefreshThemeIcons();

		BuildTree();
	}

	SolutionExplorer::~SolutionExplorer()
	{
		if (m_Folder_Close_Icon)	m_Folder_Close_Icon->Unload();
		if (m_Folder_Open_Icon)		m_Folder_Open_Icon->Unload();
		if (m_CPP_File_Icon)		m_CPP_File_Icon->Unload();
		if (m_Header_File_Icon)		m_Header_File_Icon->Unload();
		if (m_File_Icon)			m_File_Icon->Unload();
		if (m_Solution_Icon)		m_Solution_Icon->Unload();

		m_ProjectIcons.clear();
	}

	void SolutionExplorer::SetSolution(Solution* s)
	{
		m_Solution = s;
		m_RootFolder.clear();
		BuildTree();
	}

	void SolutionExplorer::Refresh()
	{
		if (m_Solution)
		{
			for (auto& p : m_Solution->projects)
				p->ScanSources(true);
		}
		BuildTree();
	}

	void SolutionExplorer::BuildTree()
	{
		m_HoveredNode = nullptr;
		m_SelectNode = nullptr;
		m_LastClickedNode = nullptr;
		m_ScrollY = 0.0f;

		m_Root = Node{};
		m_Root.kind = NodeKind::SolutionRoot;
		m_Root.expanded = true;

		// ==========================================
		// Folder Mode
		// ==========================================
		if (!m_RootFolder.empty())
		{
			m_Root.name = m_RootFolder.filename().string();
			if (m_Root.name.empty()) m_Root.name = m_RootFolder.string();
			m_Root.fullPath = m_RootFolder.generic_string();

			std::error_code ec;
			for (auto it = std::filesystem::recursive_directory_iterator(m_RootFolder, std::filesystem::directory_options::skip_permission_denied, ec); it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
			{
				if (ec) { ec.clear(); continue; }

				auto rel = it->path().lexically_relative(m_RootFolder);
				if (rel.empty()) continue;

				bool skip = false;
				for (auto& part : rel)
				{
					if (ShouldIgnoreFolder(part.string())) { skip = true; break; }
				}

				if (skip)
				{
					if (it->is_directory()) it.disable_recursion_pending();
					continue;
				}

				if (it->is_directory())
				{
					InsertFolderRecursive(m_Root, rel);
				}
				else if (it->is_regular_file())
				{
					InsertFileRecursive(m_Root, rel, it->path());
				}
			}
		}
		// ==========================================
		// Solution Mode
		// ==========================================
		else if (m_Solution)
		{
			m_Root.name = m_Solution->name.empty() ? "Solution" : m_Solution->name;

			for (int pi = 0; pi < static_cast<int>(m_Solution->projects.size()); ++pi)
			{
				auto* proj = m_Solution->projects[pi].get();

				Node pNode;
				pNode.kind = NodeKind::Project;
				pNode.name = (proj->name == m_Solution->startupProjectName) ? ("[Startup] " + proj->name) : proj->name;
				pNode.fullPath = proj->Directory().generic_string();
				pNode.projectIndex = pi;
				pNode.expanded = true;
				pNode.language = proj->language;

				proj->ScanSources();
				auto base = proj->Directory();

				std::error_code ec;
				for (auto it = std::filesystem::recursive_directory_iterator(base, std::filesystem::directory_options::skip_permission_denied, ec); it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
				{
					if (ec) { ec.clear(); continue; }

					auto rel = it->path().lexically_relative(base);
					if (rel.empty()) continue;

					bool skip = false;
					for (auto& part : rel)
					{
						if (ShouldIgnoreFolder(part.string())) { skip = true; break; }
					}

					if (skip)
					{
						if (it->is_directory()) it.disable_recursion_pending();
						continue;
					}

					if (it->is_directory())
					{
						InsertFolderRecursive(pNode, rel);
					}
					else if (it->is_regular_file())
					{
						if (it->path().extension() == ".fxproject" || it->path().extension() == ".frostnux")
							continue;

						InsertFileRecursive(pNode, rel, it->path());
					}
				}

				m_Root.children.push_back(std::move(pNode));
			}
		}
		else
		{
			m_Root.name = "(no solution)";
			return;
		}

		auto sortTreeRecursive = [](auto&& self, Node& node) -> void
			{
			for (auto& child : node.children)
			{
				self(self, child);
			}

			std::sort(node.children.begin(), node.children.end(), [](const Node& a, const Node& b)
				{
				if (a.kind != b.kind)
				{
					return static_cast<int>(a.kind) < static_cast<int>(b.kind);
				}

				std::string aName = a.name;
				std::string bName = b.name;
				std::transform(aName.begin(), aName.end(), aName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				std::transform(bName.begin(), bName.end(), bName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				return aName < bName;
				});
			};

		sortTreeRecursive(sortTreeRecursive, m_Root);
	}

	void SolutionExplorer::InsertFileRecursive(Node& parent, const std::filesystem::path& relPath, const std::filesystem::path& absPath)
	{
		auto it = relPath.begin();
		auto end = relPath.end();
		Node* cur = &parent;
		std::filesystem::path curAbs = parent.fullPath;

		while (it != end)
		{
			auto next = it;
			++next;

			if (next == end)
			{
				Node leaf;
				leaf.kind = NodeKind::File;
				leaf.name = it->string();
				leaf.fullPath = absPath.generic_string();
				leaf.projectIndex = cur->projectIndex;
				cur->children.push_back(std::move(leaf));
				return;
			}

			std::string folderName = it->string();
			curAbs /= folderName;
			Node* found = nullptr;
			for (auto& c : cur->children)
			{
				if (c.kind == NodeKind::Folder && c.name == folderName)
				{
					found = &c;
					break;
				}
			}
			if (!found)
			{
				Node f;
				f.kind = NodeKind::Folder;
				f.name = folderName;
				f.fullPath = curAbs.generic_string();
				f.projectIndex = cur->projectIndex;
				f.expanded = true;
				cur->children.push_back(std::move(f));
				found = &cur->children.back();
			}
			cur = found;
			it = next;
		}
	}

	void SolutionExplorer::InsertFolderRecursive(Node& parent, const std::filesystem::path& relPath)
	{
		Node* cur = &parent;
		std::filesystem::path curAbs = parent.fullPath;

		for (auto& part : relPath)
		{
			std::string folderName = part.string();
			if (folderName.empty() || folderName == ".") continue;

			curAbs /= folderName;

			Node* found = nullptr;
			for (auto& c : cur->children)
			{
				if (c.kind == NodeKind::Folder && c.name == folderName)
				{
					found = &c;
					break;
				}
			}
			if (!found)
			{
				Node f;
				f.kind = NodeKind::Folder;
				f.name = folderName;
				f.fullPath = curAbs.generic_string();
				f.projectIndex = cur->projectIndex;
				f.expanded = true;
				cur->children.push_back(std::move(f));
				found = &cur->children.back();
			}
			cur = found;
		}
	}

	void SolutionExplorer::OnUpdate(float deltaTime)
	{
		(void)deltaTime;
		if (!IsVisible()) return;

		if (m_CurrentIsDarkTheme != ThemeManager::IsDarkTheme())
			RefreshThemeIcons();

		DrawContent();
	}

	bool SolutionExplorer::OnEvent(Event& event)
	{
		if (!IsVisible()) return false;
		if (uiWindow::OnEvent(event)) return true;

		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				if (e.GetKeyCode() == FX_KEY_DELETE)
				{
					auto* win = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
					double mx, my;
					glfwGetCursorPos(win, &mx, &my);

					if (m_SelectNode)
					{
						DeleteSelectedNode();
						return true;
					}
				}
				return false;
			});


		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e)
			{
				float mx = e.GetMouseX(), my = e.GetMouseY();
				float contentX = GetX();
				float contentY = GetY() + 30;
				float contentW = GetWidth();
				float contentH = GetHeight() - 30;

				if (!(mx >= contentX && mx <= contentX + contentW &&
					my >= contentY && my <= contentY + contentH))
					return false;

				float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
				if (maxScroll <= 0) return false;

				m_ScrollY -= e.GetYOffset() * 50.0f;
				if (m_ScrollY < 0) m_ScrollY = 0;
				if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;
				return true;
			});

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e)
			{
				if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_RIGHT)
				{
					float mx = e.GetMouseX(), my = e.GetMouseY();
					float contentX = GetX();
					float contentY = GetY() + 30;
					float contentW = GetWidth();
					float contentH = GetHeight() - 30;

					if (mx < contentX || mx > contentX + contentW || my < contentY || my > contentY + contentH)
						return false;

					nlohmann::json j;
					std::ifstream file("Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json");
					file >> j;

					m_VisibleNodes.clear();
					float yOffset = 0.0f;
					BuildVisibleList(m_Root, 0, contentY, yOffset);

					const Node* clickedNode = nullptr;
					for (const auto& vn : m_VisibleNodes)
					{
						if (my >= vn.y && my <= vn.y + m_LineHeight)
						{
							clickedNode = vn.node;
							break;
						}
					}
					if (!clickedNode) return false;

					std::vector<PopupMenu::Item> menuItems;

					if (clickedNode->kind == NodeKind::File)
					{
						PopupMenu::Item openItem; openItem.label = j.value("Open", "Open");
						openItem.onClick = [this, path = clickedNode->fullPath]()
							{
								if (m_FileOpenCallback) m_FileOpenCallback(path);
							};
						menuItems.push_back(openItem);

						PopupMenu::Item deleteItem; deleteItem.label = j.value("Delete", "Delete");
						deleteItem.onClick = [this, path = clickedNode->fullPath]()
							{
								std::error_code ec;
								std::filesystem::remove(path, ec);
								if (!ec) { Refresh(); }
							};
						menuItems.push_back(deleteItem);
					}
					else if (clickedNode->kind == NodeKind::Project || clickedNode->kind == NodeKind::Folder)
					{
						bool isProject = (clickedNode->kind == NodeKind::Project);
						std::string dirPath = clickedNode->fullPath;
						int projIdx = clickedNode->projectIndex;

						PopupMenu::Item newFileItem;
						newFileItem.label = j.value("New File...", "New File...");
						newFileItem.onClick = [this, dirPath, isProject, projIdx, j]()
							{
								TextInputDialog::Get().Open(j.value("Create New File", "Create New File"), "main.cpp",
									[this, dirPath, isProject, projIdx](const std::string& name)
									{
										if (name.empty()) return;

										std::filesystem::path target = std::filesystem::path(dirPath) / name;
										std::error_code ec;
										if (std::filesystem::exists(target, ec)) return;

										std::ofstream out(target, std::ios::binary);
										if (!out) return;
										out.close();

										if (isProject && m_Solution && projIdx >= 0)
										{
											auto* proj = m_Solution->projects[projIdx].get();
											proj->ScanSources();
											m_Solution->Save();
										}
										Refresh();
									});
							};
						menuItems.push_back(newFileItem);

						PopupMenu::Item newFolderItem;
						newFolderItem.label = j.value("New Folder...", "New Folder...");
						newFolderItem.onClick = [this, dirPath, isProject, projIdx, j]()
							{
								TextInputDialog::Get().Open(j.value("Create New Folder", "Create New Folder"), j.value("NewFolder", "NewFolder"),
									[this, dirPath, isProject, projIdx](const std::string& name)
									{
										if (name.empty()) return;

										std::filesystem::path target = std::filesystem::path(dirPath) / name;
										std::error_code ec;
										if (std::filesystem::exists(target, ec)) return;

										if (!std::filesystem::create_directories(target, ec) || ec) return;

										if (isProject && m_Solution && projIdx >= 0)
										{
											auto* proj = m_Solution->projects[projIdx].get();
											proj->ScanSources();
											m_Solution->Save();
										}
										Refresh();
									});
							};
						menuItems.push_back(newFolderItem);

						if (isProject)
						{
							PopupMenu::Item addExisting;
							addExisting.label = j.value("Add Existing File...", "Add Existing File...");
							addExisting.onClick = [this, dirPath, projIdx]()
								{
									const std::string path = OpenFileDialog("C++ Files|*.cpp;*.h;*.hpp|All Files|*.*");
									if (!path.empty() && m_Solution && projIdx >= 0)
									{
										auto* proj = m_Solution->projects[projIdx].get();
										proj->ScanSources();
										m_Solution->Save();
										Refresh();
									}
								};
							menuItems.push_back(addExisting);

							PopupMenu::Item setStartup;
							setStartup.label = j.value("Set as Startup Project", "Set as Startup Project");
							setStartup.onClick = [this, projIdx]()
								{
									if (m_Solution && projIdx >= 0)
									{
										std::string realName = m_Solution->projects[projIdx]->name;
										m_Solution->startupProjectName = realName;
										m_Solution->Save();
										Refresh();
									}
								};
							menuItems.push_back(setStartup);

							if (m_Solution && projIdx >= 0)
							{
								Project* targetProj = m_Solution->projects[projIdx].get();

								for (int i = 0; i < m_Solution->projects.size(); ++i)
								{
									if (i == projIdx) continue;
									Project* otherProj = m_Solution->projects[i].get();

									bool isAlreadyDep = false;
									for (const auto& dep : targetProj->dependencies)
									{
										if (dep == otherProj->name) { isAlreadyDep = true; break; }
									}
									if (isAlreadyDep) continue;

									PopupMenu::Item addDepItem;
									addDepItem.label = j.value("Add dependency", "Add dependency") + ": " + otherProj->name;
									addDepItem.onClick = [this, targetProj, otherProj]()
										{
											if (m_Solution->WouldCreateCycle(targetProj, otherProj->name))
											{
												FX_CORE_ERROR("Can not add dependency: Cycle detected!");
											}
											else
											{
												targetProj->AddDependency(otherProj->name);
												FX_CORE_INFO("Added dependency: {} -> {}", targetProj->name, otherProj->name);
												Refresh();
											}
										};
									menuItems.push_back(addDepItem);
								}

								for (const auto& depName : targetProj->dependencies)
								{
									PopupMenu::Item removeDepItem;
									removeDepItem.label = j.value("Remove dependency", "Remove dependency") + ": " + depName;
									removeDepItem.onClick = [this, targetProj, depName]()
										{
											targetProj->RemoveDependency(depName);
											FX_CORE_INFO("Remove dependency: {} -> {}", targetProj->name, depName);
											Refresh();
										};
									menuItems.push_back(removeDepItem);
								}
							}

						}
						else
						{
							PopupMenu::Item deleteFolderItem;
							deleteFolderItem.label = j.value("Delete Folder", "Delete Folder");
							deleteFolderItem.onClick = [this, dirPath, j]()
								{
									uiMessageBox::Get().Open(
										j.value("Delete Folder", "Delete Folder"),
										j.value("Delete Folder Desc", "Are you sure you want to delete this folder and all its contents? This action cannot be undone."),
										[this, dirPath]()
										{
											std::error_code ec;
											std::filesystem::remove_all(dirPath, ec);

											if (!ec)
											{
												FX_CORE_INFO("Successfully deleted folder: {}", dirPath);
												Refresh();
											}
											else
											{
												FX_CORE_ERROR("Failed to delete folder: {}", ec.message());
											}
										}
									);
								};
							menuItems.push_back(deleteFolderItem);
						}
					}
					else if (clickedNode->kind == NodeKind::SolutionRoot)
					{
						if (m_Solution)
						{
							for (const auto& lang : m_SupportedLanguages)
							{
								PopupMenu::Item newProjItem;
								newProjItem.label = "New " + lang.displayName;
								std::string langId = lang.id;
								std::string displayName = lang.displayName;

								newProjItem.onClick = [this, langId, displayName]()
									{
										TextInputDialog::Get().Open("New " + displayName, "MyProject", [this, langId](const std::string& name)
											{
												if (m_Solution && !name.empty())
												{
													Project* newProj = m_Solution->AddProject(name, ProjectKind::ConsoleApp);
													newProj->language = langId;
													m_Solution->Save();
													Refresh();
												}
											});
									};
								menuItems.push_back(newProjItem);
							}
						}
						else
						{
							PopupMenu::Item refreshItem;
							refreshItem.label = "Refresh";
							refreshItem.onClick = [this]() { Refresh(); };
							menuItems.push_back(refreshItem);
						}
					}

					if (!menuItems.empty())
					{
						PopupMenu::Get().Open(mx, my, std::move(menuItems));
					}
					return true;
				}

				if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return false;

				float mx = e.GetMouseX(), my = e.GetMouseY();
				float contentX = GetX();
				float contentY = GetY() + 30;
				float contentW = GetWidth();
				float contentH = GetHeight() - 30;

				if (mx < contentX || mx > contentX + contentW || my < contentY || my > contentY + contentH)
					return false;

				float currentTime = (float)glfwGetTime();

				m_VisibleNodes.clear();
				float yOffset = 0.0f;
				BuildVisibleList(m_Root, 0, contentY, yOffset);

				const Node* clickedNode = nullptr;
				int clickedDepth = 0;
				float clickedY = 0.0f;
				for (const auto& vn : m_VisibleNodes)
				{
					if (my >= vn.y && my <= vn.y + m_LineHeight)
					{
						clickedNode = vn.node;
						clickedDepth = vn.depth;
						clickedY = vn.y;
						break;
					}
				}
				if (!clickedNode) return false;

				float indent = clickedDepth * 16.0f;
				float iconX = contentX + indent;
				float iconSize = 16.0f;
				bool isOnTriangle = (mx >= iconX && mx <= iconX + iconSize + 10 && my >= clickedY && my <= clickedY + m_LineHeight);

				if (IsFolderLike(*clickedNode))
				{
					bool doubleClick = (m_LastClickedNode == clickedNode &&
						(currentTime - m_LastClickTime) < 0.5f);

					if (isOnTriangle || doubleClick)
					{
						ToggleNode(clickedNode);
						m_LastClickedNode = nullptr;
						m_LastClickTime = 0.0f;
						return true;
					}

					m_LastClickedNode = clickedNode;
					m_LastClickTime = currentTime;
					m_SelectNode = clickedNode;
					return true;
				}
				else
				{
					bool doubleClick = (m_LastClickedNode == clickedNode && (currentTime - m_LastClickTime) < 0.5f);

					if (doubleClick)
					{
						if (m_FileOpenCallback && !clickedNode->fullPath.empty())
							m_FileOpenCallback(clickedNode->fullPath);
						m_LastClickedNode = nullptr;
						m_LastClickTime = 0.0f;
						return true;
					}

					m_LastClickedNode = clickedNode;
					m_LastClickTime = currentTime;
					m_SelectNode = clickedNode;

					if (m_Properties && !clickedNode->fullPath.empty())
						m_Properties->SetFileProperties(clickedNode->fullPath);
					return true;
				}
			});

		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e)
			{
				if (m_IsDraggingScrollbar)
				{
					float contentY = GetY() + 30;
					float contentH = GetHeight() - 30;
					float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
					float deltaY = e.GetY() - m_DragStartY;
					float ratio = deltaY / contentH;
					m_ScrollY = m_DragStartScrollY + ratio * maxScroll;
					if (m_ScrollY < 0) m_ScrollY = 0;
					if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;
					return true;
				}

				float mx = e.GetX(), my = e.GetY();
				float contentX = GetX();
				float contentY = GetY() + 30;
				float contentW = GetWidth();
				float contentH = GetHeight() - 30;

				if (mx < contentX || mx > contentX + contentW || my < contentY || my > contentY + contentH)
				{
					m_HoveredNode = nullptr;
					return false;
				}

				m_VisibleNodes.clear();
				float yOffset = 0.0f;
				BuildVisibleList(m_Root, 0, contentY, yOffset);

				m_HoveredNode = nullptr;
				for (const auto& vn : m_VisibleNodes)
				{
					if (my >= vn.y && my <= vn.y + m_LineHeight)
					{
						m_HoveredNode = vn.node;
						break;
					}
				}
				return false;
			});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e)
			{
				if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
					m_IsDraggingScrollbar = false;
				return false;
			});

		return false;
	}

	void SolutionExplorer::DrawContent()
	{
		uiWindow::DrawContent();

		float contentX = GetX() + 10;
		float contentY = GetY() + 40;
		float contentW = GetWidth() - 10;
		float contentH = GetHeight() - 40;

		Application& app = Application::Get();
		int winHeight = app.GetWindow().GetHeight();

		glEnable(GL_SCISSOR_TEST);
		glScissor((int)contentX, winHeight - (int)(contentY + contentH), (int)contentW, (int)contentH);

		m_TotalHeight = 0.0f;
		CalcTreeHeight(m_Root, m_TotalHeight);

		float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
		if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;
		if (m_ScrollY < 0) m_ScrollY = 0;

		float y = contentY - m_ScrollY;
		DrawNode(m_Root, 0, y, contentX, contentW);

		if (m_TotalHeight > contentH)
		{
			float thumbH = contentH * (contentH / m_TotalHeight);
			float thumbY = contentY + (m_ScrollY / (m_TotalHeight - contentH)) * (contentH - thumbH);
			Renderer2D::DrawQuad(contentX + contentW - 8, thumbY, 6.0f, thumbH, 0.5f, 0.5f, 0.5f, 0.8f);
		}

		glDisable(GL_SCISSOR_TEST);
	}

	void SolutionExplorer::CalcTreeHeight(const Node& node, float& total) const
	{
		total += m_LineHeight;
		if (IsFolderLike(node) && node.expanded)
			for (const auto& c : node.children)
				CalcTreeHeight(c, total);
	}

	//void SolutionExplorer::DrawNode(const Node& node, int depth, float& y, float x, float width)
	//{
	//	float indent = depth * 16.0f;
	//	float iconSize = 16.0f;
	//	float textX = x + indent + iconSize + 4;

	//	if (y + m_LineHeight < GetY() + 30 || y > GetY() + GetHeight())
	//	{
	//		y += m_LineHeight;
	//		if (IsFolderLike(node) && node.expanded)
	//			for (const auto& c : node.children)
	//				DrawNode(c, depth + 1, y, x, width);
	//		return;
	//	}

	//	if (&node == m_HoveredNode)
	//	{
	//		if (ThemeManager::IsDarkTheme())
	//			Renderer2D::DrawQuad(x, y, width, m_LineHeight, 0.2f, 0.2f, 0.2f, 0.5f);
	//		else
	//			Renderer2D::DrawQuad(x, y, width, m_LineHeight, 0.8f, 0.8f, 0.8f, 0.5f);
	//	}

	//	if (&node == m_SelectNode)
	//	{
	//		Renderer2D::DrawQuad(x, y, width, m_LineHeight, 0.5f, 0.5f, 0.5f, 0.5f);

	//		float r = 0.7f, g = 0.7f, b = 0.7f, a = 1.0f;
	//		float thickness = 1.0f;

	//		Renderer2D::DrawLine(x, y, x + width, y, thickness, r, g, b, a);
	//		Renderer2D::DrawLine(x + width, y, x + width, y + m_LineHeight, thickness, r, g, b, a);
	//		Renderer2D::DrawLine(x + width, y + m_LineHeight, x, y + m_LineHeight, thickness, r, g, b, a);
	//		Renderer2D::DrawLine(x, y + m_LineHeight, x, y, thickness, r, g, b, a);
	//	}

	//	float iconX = x + indent;
	//	float iconY = y + (m_LineHeight - iconSize) / 2;

	//	if (IsFolderLike(node))
	//		DrawFolderIcon(iconX, iconY, node.expanded);
	//	else
	//		DrawFileIcon(iconX, iconY, node.fullPath);

	//	if (TextRenderer::Get().IsInitialized())
	//	{
	//		if (ThemeManager::IsDarkTheme())
	//			TextRenderer::Get().DrawTextUTF8(node.name, textX + m_LineHeight, y + 2, 0.9f, 0.9f, 0.9f, 1.0f);
	//		else
	//			TextRenderer::Get().DrawTextUTF8(node.name, textX + m_LineHeight, y + 2, 0.1f, 0.1f, 0.1f, 1.0f);
	//	}

	//	y += m_LineHeight;

	//	if (IsFolderLike(node) && node.expanded)
	//		for (const auto& c : node.children)
	//			DrawNode(c, depth + 1, y, x, width);
	//}
	void SolutionExplorer::DrawNode(const Node& node, int depth, float& y, float x, float width)
	{
		float indent = depth * 16.0f;
		float iconSize = 16.0f;
		float textX = x + indent + iconSize + 4;

		if (y + m_LineHeight < GetY() + 30 || y > GetY() + GetHeight())
		{
			y += m_LineHeight;
			if (IsFolderLike(node) && node.expanded)
				for (const auto& c : node.children)
					DrawNode(c, depth + 1, y, x, width);
			return;
		}

		if (&node == m_HoveredNode)
		{
			if (ThemeManager::IsDarkTheme())
				Renderer2D::DrawQuad(x, y, width, m_LineHeight, 0.2f, 0.2f, 0.2f, 0.5f);
			else
				Renderer2D::DrawQuad(x, y, width, m_LineHeight, 0.8f, 0.8f, 0.8f, 0.5f);
		}

		if (&node == m_SelectNode)
		{
			Renderer2D::DrawQuad(x, y, width, m_LineHeight, 0.5f, 0.5f, 0.5f, 0.5f);

			float r = 0.7f, g = 0.7f, b = 0.7f, a = 1.0f;
			float thickness = 1.0f;

			Renderer2D::DrawLine(x, y, x + width, y, thickness, r, g, b, a);
			Renderer2D::DrawLine(x + width, y, x + width, y + m_LineHeight, thickness, r, g, b, a);
			Renderer2D::DrawLine(x + width, y + m_LineHeight, x, y + m_LineHeight, thickness, r, g, b, a);
			Renderer2D::DrawLine(x, y + m_LineHeight, x, y, thickness, r, g, b, a);
		}

		float iconX = x + indent;
		float iconY = y + (m_LineHeight - iconSize) / 2;

		if (IsFolderLike(node))
		{
			if (ThemeManager::IsDarkTheme()) glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
			else glColor4f(0.2f, 0.2f, 0.2f, 1.0f);

			if (node.expanded)
				Renderer2D::DrawTriangle(iconX, iconY, iconX + 10.0f, iconY, iconX + 5.0f, iconY + 10.0f, 0.8f, 0.8f, 0.8f, 1.0f);
			else
			{
				Renderer2D::DrawLine(iconX, iconY, iconX, iconY + 10.0f, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f);
				Renderer2D::DrawLine(iconX, iconY + 10.0f, iconX + 7.0f, iconY + 5.0f, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f);
				Renderer2D::DrawLine(iconX + 7.0f, iconY + 5.0f, iconX, iconY, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f);
			}
			if (node.kind == NodeKind::SolutionRoot)
			{
				if (m_Solution_Icon) m_Solution_Icon->Draw(iconX + 20, iconY - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
			}
			else if (node.kind == NodeKind::Project)
			{
				auto it = m_ProjectIcons.find(node.language);
				if (it != m_ProjectIcons.end() && it->second)
				{
					it->second->Draw(iconX + 20, iconY - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
				}
				else
				{
					if (m_File_Icon) m_File_Icon->Draw(iconX + 20, iconY - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
				}
			}
			else
			{
				if (node.expanded)
					m_Folder_Open_Icon->Draw(iconX + 20, iconY - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
				else
					m_Folder_Close_Icon->Draw(iconX + 20, iconY - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
			}
		}
		else
		{
			DrawFileIcon(iconX, iconY, node.fullPath);
		}

		if (TextRenderer::Get().IsInitialized())
		{
			if (ThemeManager::IsDarkTheme())
				TextRenderer::Get().DrawTextUTF8(node.name, textX + m_LineHeight, y + 2, 0.9f, 0.9f, 0.9f, 1.0f);
			else
				TextRenderer::Get().DrawTextUTF8(node.name, textX + m_LineHeight, y + 2, 0.1f, 0.1f, 0.1f, 1.0f);
		}

		y += m_LineHeight;

		if (IsFolderLike(node) && node.expanded)
			for (const auto& c : node.children)
				DrawNode(c, depth + 1, y, x, width);
	}

	void SolutionExplorer::DrawFolderIcon(float x, float y, bool expanded) const
	{
		if (ThemeManager::IsDarkTheme())
			glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
		else
			glColor4f(0.2f, 0.2f, 0.2f, 1.0f);

		if (expanded)
		{
			Renderer2D::DrawTriangle(x, y, x + 10.0f, y, x + 5.0f, y + 10.0f, 0.8f, 0.8f, 0.8f, 1.0f);
			m_Folder_Open_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
		else
		{
			Renderer2D::DrawLine(x, y, x, y + 10.0f, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f);
			Renderer2D::DrawLine(x, y + 10.0f, x + 7.0f, y + 5.0f, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f);
			Renderer2D::DrawLine(x + 7.0f, y + 5.0f, x, y, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f);
			m_Folder_Close_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
	}

	void SolutionExplorer::DrawFileIcon(float x, float y, const std::string& filepath) const
	{
		auto dot = filepath.find_last_of('.');
		std::string ext = (dot == std::string::npos) ? "" : filepath.substr(dot + 1);

		if (ext == "cpp" || ext == "c" || ext == "cc" || ext == "cxx")
			m_CPP_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		else if (ext == "h" || ext == "hpp" || ext == "hxx")
			m_Header_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		else if (ext == "py" || ext == "pyc" || ext == "pyd")
			m_Python_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		else if (ext == "cs")
			m_CS_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		else
			m_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
	}

	void SolutionExplorer::BuildVisibleList(const Node& node, int depth, float startY, float& yOffset) const
	{
		float screenY = startY + yOffset - m_ScrollY;
		m_VisibleNodes.push_back({ &node, depth, screenY });
		yOffset += m_LineHeight;

		if (IsFolderLike(node) && node.expanded)
			for (const auto& c : node.children)
				BuildVisibleList(c, depth + 1, startY, yOffset);
	}

	void SolutionExplorer::ToggleNode(const Node* target)
	{
		if (!target) return;

		std::function<bool(Node&)> walk = [&](Node& n) -> bool
			{
				if (&n == target)
				{
					n.expanded = !n.expanded;
					return true;
				}
				for (auto& c : n.children)
					if (walk(c)) return true;
				return false;
			};
		walk(m_Root);
	}

	void SolutionExplorer::RefreshText()
	{}

	void SolutionExplorer::RefreshThemeIcons()
	{
		m_CurrentIsDarkTheme = ThemeManager::IsDarkTheme();

		if (m_File_Icon)
		{
			m_File_Icon->Unload();
			m_File_Icon->LoadFromFile(ThemeManager::IsDarkTheme() ? "Resources/Images/File_White.png"
				: "Resources/Images/File_Black.png");
		}
		if (m_Header_File_Icon)
		{
			m_Header_File_Icon->Unload();
			m_Header_File_Icon->LoadFromFile(ThemeManager::IsDarkTheme() ? "Resources/Images/Header_File_White.png"
				: "Resources/Images/Header_File_Black.png");
		}
	}

	void SolutionExplorer::DeleteSelectedNode()
	{
		if (!m_SelectNode || m_SelectNode->kind == NodeKind::SolutionRoot || m_SelectNode->fullPath.empty())
			return;

		std::string path = m_SelectNode->fullPath;
		bool isFolder = (m_SelectNode->kind == NodeKind::Folder);

		uiMessageBox::Get().Open(
			isFolder ? "Delete Folder" : "Delete File",
			isFolder ? "Are you sure you want to delete this folder and all its contents?" : "Are you sure you want to delete this file?",
			[this, path, isFolder]()
			{
				std::error_code ec;
				if (isFolder)
					std::filesystem::remove_all(path, ec);
				else
					std::filesystem::remove(path, ec);

				if (!ec)
				{
					FX_CORE_INFO("Deleted: {}", path);
					m_SelectNode = nullptr;
					m_HoveredNode = nullptr;
					Refresh();
				}
				else
				{
					FX_CORE_ERROR("Failed to delete: {}", ec.message());
				}
			}
		);
	}

	void SolutionExplorer::SetRootFolder(const std::filesystem::path& folderPath)
	{
		m_Solution = nullptr;
		m_RootFolder = folderPath;
		BuildTree();
	}

}
