#include "fxpch.h"
#include "Project.h"
#include "Frostnux/Core/Log.h"
#include <fstream>

using json = nlohmann::json;

namespace Frostnux {

	namespace {
		std::string KindToString(ProjectKind k)
		{
			switch (k) {
			case ProjectKind::StaticLib:	return "StaticLib";
			case ProjectKind::DynamicLib:	return "DynamicLib";
			default:						return "ConsoleApp";
			}
		}
		ProjectKind StringToKind(const std::string& s)
		{
			if (s == "StaticLib")  return ProjectKind::StaticLib;
			if (s == "DynamicLib") return ProjectKind::DynamicLib;
			return ProjectKind::ConsoleApp;
		}
	}

	bool Project::Load(const std::filesystem::path& filepath)
	{
		std::ifstream f(filepath);
		if (!f) { FX_CORE_ERROR("Project::Load: cannot open {}", filepath.string()); return false; }

		json j;
		try { f >> j; }
		catch (const std::exception& e)
		{
			FX_CORE_ERROR("Project::Load: JSON error: {}", e.what());
			return false;
		}

		path = filepath;
		name = j.value("name", "MyProject");
		kind = StringToKind(j.value("kind", "ConsoleApp"));
		language = j.value("language", "cpp");
		standard = j.value("standard", "c++20");
		outputDir = j.value("outputDir", "bin");
		outputName = j.value("outputName", "");
		sources = j.value("sources", std::vector<std::string>{});
		headers = j.value("headers", std::vector<std::string>{});
		dependencies = j.value("dependencies", std::vector<std::string>{});

		configurations.clear();
		if (j.contains("configurations") && j["configurations"].is_object())
		{
			for (auto& [key, val] : j["configurations"].items())
			{
				ProjectConfig c;
				c.name = key;
				c.debugInfo = val.value("debugInfo", false);
				c.optimization = val.value("optimization", "/Od");
				c.flags = val.value("flags", std::vector<std::string>{});
				c.includeDirs = val.value("includeDirs", std::vector<std::string>{});
				c.defines = val.value("defines", std::vector<std::string>{});
				c.libraryDirs = val.value("libraryDirs", std::vector<std::string>{});
				c.libraries = val.value("libraries", std::vector<std::string>{});
				configurations.push_back(std::move(c));
			}
		}
		if (configurations.empty())
		{
			ProjectConfig d; d.name = "Debug";   d.debugInfo = true;
			ProjectConfig r; r.name = "Release"; r.optimization = "/O2";
			configurations = { std::move(d), std::move(r) };
		}
		activeConfig = j.value("activeConfig", "Debug");

		if (outputName.empty()) outputName = name;

		if (sources.empty())
		{
			ScanSources();
		}
		return true;
	}

	bool Project::Save(const std::filesystem::path& filepath)
	{
		auto target = filepath.empty() ? path : filepath;
		if (target.empty()) return false;
		path = target;
		if (outputName.empty()) outputName = name;

		json j;
		j["name"] = name;
		j["kind"] = KindToString(kind);
		j["language"] = language;
		j["standard"] = standard;
		j["sources"] = sources;
		j["headers"] = headers;
		j["outputDir"] = outputDir;
		j["outputName"] = outputName;
		j["activeConfig"] = activeConfig;
		j["dependencies"] = dependencies;

		json cfgs = json::object();
		for (const auto& c : configurations)
		{
			cfgs[c.name] = {
				{ "debugInfo", c.debugInfo },
				{ "optimization", c.optimization },
				{ "flags", c.flags },
				{ "includeDirs", c.includeDirs },
				{ "defines", c.defines },
				{ "libraryDirs", c.libraryDirs },
				{ "libraries", c.libraries }
			};
		}
		j["configurations"] = cfgs;

		std::error_code ec;
		std::filesystem::create_directories(target.parent_path(), ec);

		std::ofstream f(target);
		if (!f) return false;
		f << j.dump(4);
		return true;
	}

	void Project::ScanSources()
	{
		sources.clear();
		headers.clear();
		auto base = Directory();
		std::error_code ec;

		if (!std::filesystem::is_directory(base, ec)) return;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(base, ec))
		{
			if (entry.is_regular_file())
			{
				auto ext = entry.path().extension().string();
				auto rel = std::filesystem::relative(entry.path(), base, ec);

				if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx")
				{
					sources.push_back(rel.generic_string());
				}
				else if (ext == ".h" || ext == ".hpp" || ext == ".hxx")
				{
					headers.push_back(rel.generic_string());
				}
			}
		}
	}

	const ProjectConfig* Project::ActiveConfig() const
	{
		for (const auto& c : configurations)
			if (c.name == activeConfig) return &c;
		return configurations.empty() ? nullptr : &configurations.front();
	}

}
