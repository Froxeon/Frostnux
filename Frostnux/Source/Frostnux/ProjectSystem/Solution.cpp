#include "fxpch.h"
#include "Solution.h"
#include <fstream>

using json = nlohmann::json;

namespace Frostnux {

	bool Solution::Load(const std::filesystem::path& filepath)
	{
		std::ifstream f(filepath);
		if (!f) return false;

		json j;
		try { f >> j; }
		catch (...) { return false; }

		path = filepath;
		name = j.value("name", "MySolution");
		startupProjectName = j.value("startupProject", "");
		projects.clear();

		auto base = filepath.parent_path();
		for (const auto& rel : j.value("projects", std::vector<std::string>{}))
		{
			auto projPath = base / rel;
			auto p = std::make_unique<Project>();
			if (p->Load(projPath)) projects.push_back(std::move(p));
		}

		if (startupProjectName.empty() && !projects.empty())
		{
			startupProjectName = projects[0]->name;
		}
		return true;
	}

	bool Solution::Save(const std::filesystem::path& filepath)
	{
		auto target = filepath.empty() ? path : filepath;
		if (target.empty()) return false;
		path = target;
		auto base = path.parent_path();

		for (auto& p : projects)
		{
			if (p->path.empty())
			{
				auto projDir = base / p->name;
				std::error_code ec;
				std::filesystem::create_directories(projDir, ec);
				p->path = projDir / (p->name + ".fxproject");
			}
			p->Save();
		}

		json j;
		j["name"] = name;
		j["startupProject"] = startupProjectName;

		std::vector<std::string> rels;
		for (auto& p : projects)
			rels.push_back(std::filesystem::relative(p->path, base).generic_string());
		j["projects"] = rels;

		std::error_code ec;
		std::filesystem::create_directories(base, ec);
		std::ofstream f(path);
		if (!f) return false;
		f << j.dump(4);
		return true;
	}

	Project* Solution::AddProject(const std::string& projName, ProjectKind kind)
	{
		auto p = std::make_unique<Project>();
		p->name = projName;
		p->kind = kind;
		p->outputName = projName;

		ProjectConfig d; d.name = "Debug";   d.debugInfo = true;
		ProjectConfig r; r.name = "Release"; r.optimization = "/O2";
		p->configurations = { std::move(d), std::move(r) };

		Project* raw = p.get();
		projects.push_back(std::move(p));

		if (startupProjectName.empty()) startupProjectName = projName;
		return raw;
	}

	void Solution::RemoveProject(int idx)
	{
		if (idx < 0 || idx >= (int)projects.size()) return;
		projects.erase(projects.begin() + idx);
	}

	Project* Solution::FindProject(const std::string& n)
	{
		for (auto& p : projects)
			if (p->name == n) return p.get();
		return nullptr;
	}

	Project* Solution::GetStartupProject()
	{
		if (startupProjectName.empty() && !projects.empty()) return projects[0].get();
		for (auto& p : projects)
			if (p->name == startupProjectName) return p.get();
		return projects.empty() ? nullptr : projects[0].get();
	}

	std::vector<Project*> Solution::GetBuildOrder(Project* target)
	{
		std::vector<Project*> order;
		std::unordered_set<Project*> visited;
		std::unordered_set<Project*> visiting;

		std::function<bool(Project*)> visit = [&](Project* p) -> bool
			{
			if (!p) return true;
			if (visiting.count(p))
			{
				FX_CORE_ERROR("Circular dependency detected involving project: {}", p->name);
				return false;
			}
			if (visited.count(p)) return true;

			visiting.insert(p);

			for (const auto& depName : p->dependencies)
			{
				Project* depProj = FindProject(depName);
				if (depProj)
				{
					if (!visit(depProj)) return false;
				}
				else
				{
					FX_CORE_WARN("Project {} depends on {}, but it was not found!", p->name, depName);
				}
			}

			visiting.erase(p);
			visited.insert(p);
			order.push_back(p);
			return true;
			};

		visit(target);
		return order;
	}

	bool Solution::WouldCreateCycle(Project* targetProj, const std::string& depName)
	{
		if (targetProj->name == depName) return true;
		Project* depProj = FindProject(depName);
		if (!depProj) return false;

		std::unordered_set<std::string> visited;

		std::function<bool(Project*)> hasTarget = [&](Project* p) -> bool
			{
			if (p->name == targetProj->name) return true;
			if (visited.count(p->name)) return false;
			visited.insert(p->name);

			for (const auto& child : p->dependencies)
			{
				Project* childProj = FindProject(child);
				if (childProj && hasTarget(childProj)) return true;
			}
			return false;
			};

		return hasTarget(depProj);
	}

}
