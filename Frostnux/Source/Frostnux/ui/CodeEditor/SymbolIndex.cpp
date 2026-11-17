//#include "fxpch.h"
//#include "SymbolIndex.h"
//
//namespace Frostnux {
//
//	namespace {
//
//		[[nodiscard]] bool isIdentStart(char32_t c)
//		{
//			return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'_' || c > 127;
//		}
//		[[nodiscard]] bool isIdentChar(char32_t c)
//		{
//			return isIdentStart(c) || (c >= U'0' && c <= U'9');
//		}
//
//		void tokenizeLine(std::u32string_view line, std::vector<SymbolToken>& out)
//		{
//			out.clear();
//			const int n = (int)line.size();
//			int i = 0;
//			while (i < n)
//			{
//				char32_t c = line[i];
//				if (c == U' ' || c == U'\t' || c == U'\r') { ++i; continue; }
//
//				if (c == U'/' && i + 1 < n && line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(i) + 1] == U'/') break;
//
//				if (c == U'/' && i + 1 < n && line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(i) + 1] == U'*')
//				{
//					i += 2;
//					while (i + 1 < n && !(line[i] == U'*' && line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(i) + 1] == U'/')) ++i;
//					i = (i + 1 < n) ? i + 2 : n;
//					continue;
//				}
//
//				if (c == U'"' || c == U'\'')
//				{
//					char32_t q = c;
//					int s = i++;
//					while (i < n)
//					{
//						if (line[i] == U'\\' && i + 1 < n) { i += 2; continue; }
//						if (line[i] == q) { ++i; break; }
//						++i;
//					}
//					out.push_back(SymbolToken(SymbolToken::Kind::String, line.substr(static_cast<std::size_t>(s), static_cast<std::size_t>(i - s)), s));
//					continue;
//				}
//
//				if (isIdentStart(c))
//				{
//					int s = i;
//					while (i < n && isIdentChar(line[i])) ++i;
//					out.push_back(SymbolToken(SymbolToken::Kind::Ident,
//						line.substr(static_cast<std::size_t>(s), static_cast<std::size_t>(i - s)), s));
//					continue;
//				}
//
//				if (c >= U'0' && c <= U'9')
//				{
//					int s = i;
//					while (i < n && (isIdentChar(line[i]) || line[i] == U'.')) ++i;
//					out.push_back(SymbolToken(SymbolToken::Kind::Number,
//						line.substr(static_cast<std::size_t>(s), static_cast<std::size_t>(i - s)), s));
//					continue;
//				}
//
//				out.push_back(SymbolToken(SymbolToken::Kind::Punct,
//					line.substr(static_cast<std::size_t>(i), 1), i));
//				++i;
//			}
//		}
//
//		[[nodiscard]] bool isTypeModifier(std::u32string_view w)
//		{
//			if (w.empty()) return false;
//			switch (w[0])
//			{
//			case U'c': return w == U"const" || w == U"volatile" || w == U"constexpr" || w == U"consteval" || w == U"constinit";
//			case U's': return w == U"static";
//			case U'e': return w == U"extern" || w == U"explicit";
//			case U'i': return w == U"inline";
//			case U'v': return w == U"virtual" || w == U"volatile";
//			case U'f': return w == U"friend";
//			case U'm': return w == U"mutable";
//			case U'r': return w == U"register";
//			case U't': return w == U"thread_local" || w == U"typename" || w == U"typedef" || w == U"template";
//			case U'u': return w == U"using";
//			default:  return false;
//			}
//		}
//
//		[[nodiscard]] bool isControlKeyword(std::u32string_view w)
//		{
//			if (w.empty()) return false;
//			switch (w[0])
//			{
//			case U'a': return w == U"alignof";
//			case U'b': return w == U"break";
//			case U'c': return w == U"case" || w == U"continue" || w == U"catch";
//			case U'd': return w == U"do" || w == U"default" || w == U"delete" || w == U"decltype";
//			case U'e': return w == U"else";
//			case U'f': return w == U"for";
//			case U'g': return w == U"goto";
//			case U'i': return w == U"if";
//			case U'n': return w == U"new";
//			case U'o': return w == U"operator";
//			case U'r': return w == U"return";
//			case U's': return w == U"switch" || w == U"sizeof" || w == U"static_assert";
//			case U't': return w == U"try" || w == U"throw" || w == U"this";
//			case U'w': return w == U"while";
//			default:  return false;
//			}
//		}
//
//	}
//
//	struct SymbolIndex::ParseContext
//	{
//		std::vector<int>	scopeStack = { 0 };
//		std::vector<bool>	braceIsScope;
//		bool				pendingScope = false;
//	};
//
//	void SymbolIndex::rebuild(const TextBuffer& buf)
//	{
//		scopes_.clear();
//		symbols_.clear();
//		scopeSymbols_.clear();
//		symbolsByName_.clear();
//
//		scopes_.emplace_back();
//		scopes_[0].parent = -1;
//		scopes_[0].kind = SymbolKind::Namespace;
//		scopeSymbols_.emplace_back();
//
//		scopes_.reserve(64);
//		scopeSymbols_.reserve(64);
//
//		std::vector<SymbolToken> lineTokens;
//		lineTokens.reserve(64);
//
//		ParseContext ctx;
//
//		const int n = buf.lineCount();
//		for (int i = 0; i < n; ++i)
//		{
//			tokenizeLine(buf.line(i), lineTokens);
//			if (!lineTokens.empty())
//				processTokens(lineTokens, i, ctx);
//		}
//
//		symbolsByName_.reserve(symbols_.size());
//		for (int i = 0; i < static_cast<int>(symbols_.size()); ++i)
//		{
//			symbolsByName_.emplace(std::u32string_view(symbols_[i].name), i);
//		}
//	}
//
//	void SymbolIndex::processTokens(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx)
//	{
//		const int n = (int)toks.size();
//		int i = 0;
//
//		while (i < n)
//		{
//			const auto& t = toks[i];
//
//			if (t.kind == SymbolToken::Kind::Punct)
//			{
//				if (t.text == U"{")
//				{
//					ctx.braceIsScope.push_back(ctx.pendingScope);
//					ctx.pendingScope = false;
//					++i;
//					continue;
//				}
//				if (t.text == U"}")
//				{
//					if (!ctx.braceIsScope.empty())
//					{
//						if (ctx.braceIsScope.back() && ctx.scopeStack.size() > 1)
//							ctx.scopeStack.pop_back();
//						ctx.braceIsScope.pop_back();
//					}
//					++i;
//					continue;
//				}
//				++i;
//				continue;
//			}
//
//			if (t.kind != SymbolToken::Kind::Ident) { ++i; continue; }
//			const auto& w = t.text;
//
//			if (w == U"namespace")
//			{
//				if (i + 1 < n && toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].kind == SymbolToken::Kind::Ident)
//				{
//					int braceIdx = -1;
//					for (int j = i + 2; j < n; ++j)
//					{
//						if (toks[j].kind != SymbolToken::Kind::Punct) continue;
//						if (toks[j].text == U"{") { braceIdx = j; break; }
//						if (toks[j].text == U"=") { braceIdx = -1; break; }
//					}
//					if (braceIdx >= 0)
//					{
//						registerSymbol(ctx, std::u32string(toks[i + 1].text), SymbolKind::Namespace, {}, line, toks[i + 1].col);
//						pushPendingScope(ctx, SymbolKind::Namespace, std::u32string(toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].text), toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].col);
//						i = braceIdx;
//						continue;
//					}
//					i += 2;
//					continue;
//				}
//				++i;
//				continue;
//			}
//
//			if (w == U"class" || w == U"struct" || w == U"union")
//			{
//				SymbolKind k = (w == U"class") ? SymbolKind::Class : (w == U"struct") ? SymbolKind::Struct : SymbolKind::Union;
//				if (i + 1 < n && toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].kind == SymbolToken::Kind::Ident)
//				{
//					int braceIdx = -1;
//					for (int j = i + 2; j < n; ++j)
//					{
//						if (toks[j].kind != SymbolToken::Kind::Punct) continue;
//						if (toks[j].text == U"{") { braceIdx = j; break; }
//						if (toks[j].text == U";") { braceIdx = -2; break; }
//					}
//					if (braceIdx >= 0)
//					{
//						registerSymbol(ctx, std::u32string(toks[i + 1].text), SymbolKind::Namespace, {}, line, toks[i + 1].col);
//						pushPendingScope(ctx, k, std::u32string(toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].text), toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].col);
//						i = braceIdx;
//						continue;
//					}
//					if (braceIdx == -2)
//					{
//						registerSymbol(ctx, std::u32string(toks[i + 1].text), k, {}, line, toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].col);
//					}
//					i += 2;
//					continue;
//				}
//				++i;
//				continue;
//			}
//
//			if (w == U"enum")
//			{
//				int nameIdx = i + 1;
//				if (nameIdx < n && (toks[nameIdx].text == U"class" || toks[nameIdx].text == U"struct"))
//					++nameIdx;
//				if (nameIdx < n && toks[nameIdx].kind == SymbolToken::Kind::Ident)
//				{
//					int braceIdx = -1;
//					for (int j = nameIdx + 1; j < n; ++j)
//					{
//						if (toks[j].kind == SymbolToken::Kind::Punct && toks[j].text == U"{")
//						{
//							braceIdx = j; break;
//						}
//					}
//					if (braceIdx >= 0)
//					{
//						registerSymbol(ctx, std::u32string(toks[i + 1].text), SymbolKind::Namespace, {}, line, toks[i + 1].col);
//						pushPendingScope(ctx, SymbolKind::Enum, std::u32string(toks[nameIdx].text), toks[nameIdx].col);
//
//						for (int j = braceIdx + 1; j < n; ++j)
//						{
//							if (toks[j].kind == SymbolToken::Kind::Ident && !isTypeModifier(toks[j].text))
//								registerSymbol(ctx, std::u32string(toks[j].text), SymbolKind::EnumValue, {}, line, toks[j].col);
//						}
//						i = braceIdx + 1;
//						continue;
//					}
//				}
//			}
//
//			++i;
//		}
//
//		if (!ctx.scopeStack.empty())
//		{
//			const int sid = ctx.scopeStack.back();
//			const auto k = scopes_[sid].kind;
//			if (k == SymbolKind::Namespace || k == SymbolKind::Class || k == SymbolKind::Struct || k == SymbolKind::Union)
//			{
//				tryParseDeclaration(toks, line, ctx);
//			}
//		}
//	}
//
//	void SymbolIndex::tryParseDeclaration(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx)
//	{
//		const int n = (int)toks.size();
//		if (n < 2) return;
//
//		int parenIdx = -1;
//		for (int i = 0; i < n; ++i)
//		{
//			if (toks[i].kind == SymbolToken::Kind::Punct && toks[i].text == U"(")
//			{
//				parenIdx = i; break;
//			}
//			if (toks[i].kind == SymbolToken::Kind::Punct && toks[i].text == U";")
//				break;
//		}
//
//		if (parenIdx >= 2)
//		{
//			int nameIdx = -1;
//			for (int i = parenIdx - 1; i >= 0; --i)
//			{
//				if (toks[i].kind != SymbolToken::Kind::Ident) continue;
//				if (isTypeModifier(toks[i].text) || isControlKeyword(toks[i].text))
//					continue;
//				nameIdx = i;
//				break;
//			}
//			if (nameIdx >= 1)
//			{
//				int retIdx = -1;
//				for (int i = nameIdx - 1; i >= 0; --i)
//				{
//					if (toks[i].kind != SymbolToken::Kind::Ident) continue;
//					if (isTypeModifier(toks[i].text)) continue;
//					retIdx = i;
//					break;
//				}
//				if (retIdx >= 0)
//				{
//					registerSymbol(ctx, std::u32string(toks[nameIdx].text), SymbolKind::Function,
//						std::u32string(toks[retIdx].text), line, toks[nameIdx].col);
//				}
//			}
//			return;
//		}
//
//		if (toks.back().kind != SymbolToken::Kind::Punct || toks.back().text != U";")
//			return;
//
//		int nameIdx = -1;
//		for (int i = n - 2; i >= 0; --i)
//		{
//			if (toks[i].kind == SymbolToken::Kind::Punct)
//			{
//				if (toks[i].text == U"=") continue;
//				if (toks[i].text == U",") break;
//				if (toks[i].text == U")" || toks[i].text == U"(") return;
//				if (toks[i].text == U">") return;
//				continue;
//			}
//			if (toks[i].kind != SymbolToken::Kind::Ident) continue;
//			if (isTypeModifier(toks[i].text) || isControlKeyword(toks[i].text))
//				continue;
//			nameIdx = i;
//			break;
//		}
//		if (nameIdx < 1) return;
//
//		int typeIdx = -1;
//		for (int i = nameIdx - 1; i >= 0; --i)
//		{
//			if (toks[i].kind != SymbolToken::Kind::Ident) continue;
//			if (isTypeModifier(toks[i].text)) continue;
//			typeIdx = i;
//			break;
//		}
//		if (typeIdx < 0) return;
//
//		registerSymbol(ctx, std::u32string(toks[nameIdx].text), SymbolKind::Variable,
//			std::u32string(toks[typeIdx].text), line, toks[nameIdx].col);
//	}
//
//	void SymbolIndex::pushPendingScope(ParseContext& ctx, SymbolKind kind,
//		const std::u32string& name, int col)
//	{
//		const int parent = ctx.scopeStack.back();
//
//		auto it = scopes_[parent].children.find(name);
//		int id;
//		if (it != scopes_[parent].children.end())
//		{
//			id = it->second;
//		}
//		else
//		{
//			id = (int)scopes_.size();
//			Scope s;
//			s.name = name;
//			s.kind = kind;
//			s.parent = parent;
//			s.col = col;
//			s.qualified = scopes_[parent].qualified.empty() ? name : scopes_[parent].qualified + U"::" + name;
//			scopes_.push_back(std::move(s));
//			scopes_[parent].children[name] = id;
//			scopeSymbols_.emplace_back();
//		}
//
//		ctx.scopeStack.push_back(id);
//		ctx.pendingScope = true;
//	}
//
//	void SymbolIndex::registerSymbol(ParseContext& ctx, std::u32string name, SymbolKind kind,
//		std::u32string typeName, int line, int col)
//	{
//		if (name.empty()) return;
//		const int sid = ctx.scopeStack.back();
//
//		auto& table = scopeSymbols_[sid];
//
//		SymbolKey probe{ std::u32string_view(name), kind };
//		if (table.byNameKind.find(probe) != table.byNameKind.end())
//			return;
//
//		Symbol s;
//		s.name = std::move(name);
//		s.typeName = std::move(typeName);
//		s.kind = kind;
//		s.scopeId = sid;
//		s.line = line;
//		s.col = col;
//
//		const int idx = static_cast<int>(symbols_.size());
//		symbols_.push_back(std::move(s));
//
//		table.indices.push_back(idx);
//
//		table.byNameKind.emplace(SymbolKey{ std::u32string_view(symbols_[idx].name), kind }, idx);
//	}
//
//	int SymbolIndex::findScope(std::u32string_view qualified) const
//	{
//		std::u32string q(qualified);
//		while (!q.empty() && q.back() == U':') q.pop_back();
//		if (q.empty()) return 0;
//
//		std::vector<std::u32string> parts;
//		std::u32string cur;
//		for (char32_t c : q)
//		{
//			if (c == U':')
//			{
//				if (!cur.empty()) { parts.push_back(std::move(cur)); cur.clear(); }
//			}
//			else cur.push_back(c);
//		}
//		if (!cur.empty()) parts.push_back(std::move(cur));
//
//		int s = 0;
//		for (const auto& p : parts)
//		{
//			auto it = scopes_[s].children.find(p);
//			if (it == scopes_[s].children.end()) return -1;
//			s = it->second;
//		}
//		return s;
//	}
//
//	const Symbol* SymbolIndex::findSymbol(int scopeId, std::u32string_view name) const
//	{
//		while (scopeId >= 0)
//		{
//			if (scopeId < static_cast<int>(scopeSymbols_.size()))
//			{
//				for (int idx : scopeSymbols_[scopeId].indices)
//				{
//					if (symbols_[idx].name == name)
//						return &symbols_[idx];
//				}
//			}
//			scopeId = scopes_[scopeId].parent;
//		}
//		return nullptr;
//	}
//
//	std::vector<const Symbol*> SymbolIndex::symbolsInScope(int scopeId) const
//	{
//		std::vector<const Symbol*> out;
//		if (scopeId < 0 || scopeId >= static_cast<int>(scopeSymbols_.size()))
//			return out;
//
//		const auto& indices = scopeSymbols_[scopeId].indices;
//		out.reserve(indices.size());
//		for (int idx : indices)
//			out.push_back(&symbols_[idx]);
//		return out;
//	}
//
//	std::vector<const Symbol*> SymbolIndex::allSymbols() const
//	{
//		std::vector<const Symbol*> out;
//		out.reserve(symbols_.size());
//		for (const auto& s : symbols_) out.push_back(&s);
//		return out;
//	}
//
//	const Symbol* SymbolIndex::findSymbolAnywhere(std::u32string_view name) const
//	{
//		auto it = symbolsByName_.find(name);
//		return it == symbolsByName_.end() ? nullptr : &symbols_[it->second];
//	}
//
//}

#include "fxpch.h"
#include "SymbolIndex.h"

namespace Frostnux {

	namespace {

		[[nodiscard]] bool isIdentStart(char32_t c)
		{
			return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'_' || c > 127;
		}
		[[nodiscard]] bool isIdentChar(char32_t c)
		{
			return isIdentStart(c) || (c >= U'0' && c <= U'9');
		}

		void tokenizeLine(std::u32string_view line, std::vector<SymbolToken>& out)
		{
			out.clear();
			const int n = (int)line.size();
			int i = 0;
			while (i < n)
			{
				char32_t c = line[i];
				if (c == U' ' || c == U'\t' || c == U'\r') { ++i; continue; }

				if (c == U'/' && i + 1 < n && line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(i) + 1] == U'/') break;

				if (c == U'/' && i + 1 < n && line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(i) + 1] == U'*')
				{
					i += 2;
					while (i + 1 < n && !(line[i] == U'*' && line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(i) + 1] == U'/')) ++i;
					i = (i + 1 < n) ? i + 2 : n;
					continue;
				}

				if (c == U'"' || c == U'\'')
				{
					char32_t q = c;
					int s = i++;
					while (i < n)
					{
						if (line[i] == U'\\' && i + 1 < n) { i += 2; continue; }
						if (line[i] == q) { ++i; break; }
						++i;
					}
					out.push_back(SymbolToken(SymbolToken::Kind::String, line.substr(static_cast<std::size_t>(s), static_cast<std::size_t>(i - s)), s));
					continue;
				}

				if (isIdentStart(c))
				{
					int s = i;
					while (i < n && isIdentChar(line[i])) ++i;
					out.push_back(SymbolToken(SymbolToken::Kind::Ident,
						line.substr(static_cast<std::size_t>(s), static_cast<std::size_t>(i - s)), s));
					continue;
				}

				if (c >= U'0' && c <= U'9')
				{
					int s = i;
					while (i < n && (isIdentChar(line[i]) || line[i] == U'.')) ++i;
					out.push_back(SymbolToken(SymbolToken::Kind::Number,
						line.substr(static_cast<std::size_t>(s), static_cast<std::size_t>(i - s)), s));
					continue;
				}

				out.push_back(SymbolToken(SymbolToken::Kind::Punct,
					line.substr(static_cast<std::size_t>(i), 1), i));
				++i;
			}
		}

		[[nodiscard]] bool isTypeModifier(std::u32string_view w)
		{
			if (w.empty()) return false;
			switch (w[0])
			{
			case U'c': return w == U"const" || w == U"volatile" || w == U"constexpr" || w == U"consteval" || w == U"constinit";
			case U's': return w == U"static";
			case U'e': return w == U"extern" || w == U"explicit";
			case U'i': return w == U"inline";
			case U'v': return w == U"virtual" || w == U"volatile";
			case U'f': return w == U"friend";
			case U'm': return w == U"mutable";
			case U'r': return w == U"register";
			case U't': return w == U"thread_local" || w == U"typename" || w == U"typedef" || w == U"template";
			case U'u': return w == U"using";
			default:  return false;
			}
		}

		[[nodiscard]] bool isControlKeyword(std::u32string_view w)
		{
			if (w.empty()) return false;
			switch (w[0])
			{
			case U'a': return w == U"alignof";
			case U'b': return w == U"break";
			case U'c': return w == U"case" || w == U"continue" || w == U"catch";
			case U'd': return w == U"do" || w == U"default" || w == U"delete" || w == U"decltype";
			case U'e': return w == U"else";
			case U'f': return w == U"for";
			case U'g': return w == U"goto";
			case U'i': return w == U"if";
			case U'n': return w == U"new";
			case U'o': return w == U"operator";
			case U'r': return w == U"return";
			case U's': return w == U"switch" || w == U"sizeof" || w == U"static_assert";
			case U't': return w == U"try" || w == U"throw" || w == U"this";
			case U'w': return w == U"while";
			default:  return false;
			}
		}

	}

	struct SymbolIndex::ParseContext
	{
		std::vector<int>	scopeStack = { 0 };
		std::vector<bool>	braceIsScope;
		bool				pendingScope = false;
	};

	void SymbolIndex::rebuild(const TextBuffer& buf)
	{
		scopes_.clear();
		symbols_.clear();
		scopeSymbols_.clear();
		symbolsByName_.clear();
		typeNamePool_.clear();
		typeNameLookup_.clear();

		scopes_.emplace_back();
		scopes_[0].parent = -1;
		scopes_[0].kind = SymbolKind::Namespace;
		scopeSymbols_.emplace_back();

		scopes_.reserve(64);
		scopeSymbols_.reserve(64);

		std::vector<SymbolToken> lineTokens;
		lineTokens.reserve(64);

		ParseContext ctx;

		const int n = buf.lineCount();
		for (int i = 0; i < n; ++i)
		{
			tokenizeLine(buf.line(i), lineTokens);
			if (!lineTokens.empty())
				processTokens(lineTokens, i, ctx);
		}

		symbolsByName_.reserve(symbols_.size());
		for (int i = 0; i < static_cast<int>(symbols_.size()); ++i)
		{
			symbolsByName_.emplace(std::u32string_view(symbols_[i].name), i);
		}
	}

	void SymbolIndex::processTokens(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx)
	{
		const int n = (int)toks.size();
		int i = 0;

		while (i < n)
		{
			const auto& t = toks[i];

			if (t.kind == SymbolToken::Kind::Punct)
			{
				if (t.text == U"{")
				{
					ctx.braceIsScope.push_back(ctx.pendingScope);
					ctx.pendingScope = false;
					++i;
					continue;
				}
				if (t.text == U"}")
				{
					if (!ctx.braceIsScope.empty())
					{
						if (ctx.braceIsScope.back() && ctx.scopeStack.size() > 1)
							ctx.scopeStack.pop_back();
						ctx.braceIsScope.pop_back();
					}
					++i;
					continue;
				}
				++i;
				continue;
			}

			if (t.kind != SymbolToken::Kind::Ident) { ++i; continue; }
			const auto& w = t.text;

			if (w == U"namespace")
			{
				if (i + 1 < n && toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].kind == SymbolToken::Kind::Ident)
				{
					int braceIdx = -1;
					for (int j = i + 2; j < n; ++j)
					{
						if (toks[j].kind != SymbolToken::Kind::Punct) continue;
						if (toks[j].text == U"{") { braceIdx = j; break; }
						if (toks[j].text == U"=") { braceIdx = -1; break; }
					}
					if (braceIdx >= 0)
					{
						registerSymbol(ctx, std::u32string(toks[i + 1].text), SymbolKind::Namespace, {}, line, toks[i + 1].col);
						pushPendingScope(ctx, SymbolKind::Namespace, std::u32string(toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].text), toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].col);
						i = braceIdx;
						continue;
					}
					i += 2;
					continue;
				}
				++i;
				continue;
			}

			if (w == U"class" || w == U"struct" || w == U"union")
			{
				SymbolKind k = (w == U"class") ? SymbolKind::Class : (w == U"struct") ? SymbolKind::Struct : SymbolKind::Union;
				if (i + 1 < n && toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].kind == SymbolToken::Kind::Ident)
				{
					int braceIdx = -1;
					for (int j = i + 2; j < n; ++j)
					{
						if (toks[j].kind != SymbolToken::Kind::Punct) continue;
						if (toks[j].text == U"{") { braceIdx = j; break; }
						if (toks[j].text == U";") { braceIdx = -2; break; }
					}
					if (braceIdx >= 0)
					{
						registerSymbol(ctx, std::u32string(toks[i + 1].text), SymbolKind::Namespace, {}, line, toks[i + 1].col);
						pushPendingScope(ctx, k, std::u32string(toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].text), toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].col);
						i = braceIdx;
						continue;
					}
					if (braceIdx == -2)
					{
						registerSymbol(ctx, std::u32string(toks[i + 1].text), k, {}, line, toks[static_cast<std::vector<Frostnux::SymbolToken, std::allocator<Frostnux::SymbolToken>>::size_type>(i) + 1].col);
					}
					i += 2;
					continue;
				}
				++i;
				continue;
			}

			if (w == U"enum")
			{
				int nameIdx = i + 1;
				if (nameIdx < n && (toks[nameIdx].text == U"class" || toks[nameIdx].text == U"struct"))
					++nameIdx;
				if (nameIdx < n && toks[nameIdx].kind == SymbolToken::Kind::Ident)
				{
					int braceIdx = -1;
					for (int j = nameIdx + 1; j < n; ++j)
					{
						if (toks[j].kind == SymbolToken::Kind::Punct && toks[j].text == U"{")
						{
							braceIdx = j; break;
						}
					}
					if (braceIdx >= 0)
					{
						registerSymbol(ctx, std::u32string(toks[i + 1].text), SymbolKind::Namespace, {}, line, toks[i + 1].col);
						pushPendingScope(ctx, SymbolKind::Enum, std::u32string(toks[nameIdx].text), toks[nameIdx].col);

						for (int j = braceIdx + 1; j < n; ++j)
						{
							if (toks[j].kind == SymbolToken::Kind::Ident && !isTypeModifier(toks[j].text))
								registerSymbol(ctx, std::u32string(toks[j].text), SymbolKind::EnumValue, {}, line, toks[j].col);
						}
						i = braceIdx + 1;
						continue;
					}
				}
			}

			++i;
		}

		if (!ctx.scopeStack.empty())
		{
			const int sid = ctx.scopeStack.back();
			const auto k = scopes_[sid].kind;
			if (k == SymbolKind::Namespace || k == SymbolKind::Class || k == SymbolKind::Struct || k == SymbolKind::Union)
			{
				tryParseDeclaration(toks, line, ctx);
			}
		}
	}

	void SymbolIndex::tryParseDeclaration(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx)
	{
		const int n = (int)toks.size();
		if (n < 2) return;

		int parenIdx = -1;
		for (int i = 0; i < n; ++i)
		{
			if (toks[i].kind == SymbolToken::Kind::Punct && toks[i].text == U"(")
			{
				parenIdx = i; break;
			}
			if (toks[i].kind == SymbolToken::Kind::Punct && toks[i].text == U";")
				break;
		}

		if (parenIdx >= 2)
		{
			int nameIdx = -1;
			for (int i = parenIdx - 1; i >= 0; --i)
			{
				if (toks[i].kind != SymbolToken::Kind::Ident) continue;
				if (isTypeModifier(toks[i].text) || isControlKeyword(toks[i].text))
					continue;
				nameIdx = i;
				break;
			}
			if (nameIdx >= 1)
			{
				int retIdx = -1;
				for (int i = nameIdx - 1; i >= 0; --i)
				{
					if (toks[i].kind != SymbolToken::Kind::Ident) continue;
					if (isTypeModifier(toks[i].text)) continue;
					retIdx = i;
					break;
				}
				if (retIdx >= 0)
				{
					registerSymbol(ctx, std::u32string(toks[nameIdx].text), SymbolKind::Function,
						std::u32string(toks[retIdx].text), line, toks[nameIdx].col);
				}
			}
			return;
		}

		if (toks.back().kind != SymbolToken::Kind::Punct || toks.back().text != U";")
			return;

		int nameIdx = -1;
		for (int i = n - 2; i >= 0; --i)
		{
			if (toks[i].kind == SymbolToken::Kind::Punct)
			{
				if (toks[i].text == U"=") continue;
				if (toks[i].text == U",") break;
				if (toks[i].text == U")" || toks[i].text == U"(") return;
				if (toks[i].text == U">") return;
				continue;
			}
			if (toks[i].kind != SymbolToken::Kind::Ident) continue;
			if (isTypeModifier(toks[i].text) || isControlKeyword(toks[i].text))
				continue;
			nameIdx = i;
			break;
		}
		if (nameIdx < 1) return;

		int typeIdx = -1;
		for (int i = nameIdx - 1; i >= 0; --i)
		{
			if (toks[i].kind != SymbolToken::Kind::Ident) continue;
			if (isTypeModifier(toks[i].text)) continue;
			typeIdx = i;
			break;
		}
		if (typeIdx < 0) return;

		registerSymbol(ctx, std::u32string(toks[nameIdx].text), SymbolKind::Variable,
			std::u32string(toks[typeIdx].text), line, toks[nameIdx].col);
	}

	void SymbolIndex::pushPendingScope(ParseContext& ctx, SymbolKind kind,
		const std::u32string& name, int col)
	{
		const int parent = ctx.scopeStack.back();

		auto it = scopes_[parent].children.find(name);
		int id;
		if (it != scopes_[parent].children.end())
		{
			id = it->second;
		}
		else
		{
			id = (int)scopes_.size();
			Scope s;
			s.name = name;
			s.kind = kind;
			s.parent = parent;
			s.col = col;
			s.qualified = scopes_[parent].qualified.empty() ? name : scopes_[parent].qualified + U"::" + name;
			scopes_.push_back(std::move(s));
			scopes_[parent].children[name] = id;
			scopeSymbols_.emplace_back();
		}

		ctx.scopeStack.push_back(id);
		ctx.pendingScope = true;
	}

	void SymbolIndex::registerSymbol(ParseContext& ctx, std::u32string name, SymbolKind kind,
		std::u32string typeName, int line, int col)
	{
		if (name.empty()) return;
		const int sid = ctx.scopeStack.back();

		auto& table = scopeSymbols_[sid];

		SymbolKey probe{ std::u32string_view(name), kind };
		if (table.byNameKind.find(probe) != table.byNameKind.end())
			return;

		const int typeNameId = internTypeName(typeName);

		Symbol s;
		s.name = std::move(name);
		s.typeNameId = typeNameId;
		s.kind = kind;
		s.scopeId = sid;
		s.line = line;
		s.col = col;

		const int idx = static_cast<int>(symbols_.size());
		symbols_.push_back(std::move(s));

		table.indices.push_back(idx);

		table.byNameKind.emplace(SymbolKey{ std::u32string_view(symbols_[idx].name), kind }, idx);
	}

	int SymbolIndex::internTypeName(const std::u32string& tn)
	{
		if (tn.empty()) return -1;

		auto it = typeNameLookup_.find(tn);
		if (it != typeNameLookup_.end())
			return it->second;

		const int id = static_cast<int>(typeNamePool_.size());
		typeNamePool_.push_back(tn);
		typeNameLookup_.emplace(std::u32string_view(typeNamePool_.back()), id);
		return id;
	}

	int SymbolIndex::findScope(std::u32string_view qualified) const
	{
		std::u32string q(qualified);
		while (!q.empty() && q.back() == U':') q.pop_back();
		if (q.empty()) return 0;

		std::vector<std::u32string> parts;
		std::u32string cur;
		for (char32_t c : q)
		{
			if (c == U':')
			{
				if (!cur.empty()) { parts.push_back(std::move(cur)); cur.clear(); }
			}
			else cur.push_back(c);
		}
		if (!cur.empty()) parts.push_back(std::move(cur));

		int s = 0;
		for (const auto& p : parts)
		{
			auto it = scopes_[s].children.find(p);
			if (it == scopes_[s].children.end()) return -1;
			s = it->second;
		}
		return s;
	}

	const Symbol* SymbolIndex::findSymbol(int scopeId, std::u32string_view name) const
	{
		while (scopeId >= 0)
		{
			if (scopeId < static_cast<int>(scopeSymbols_.size()))
			{
				for (int idx : scopeSymbols_[scopeId].indices)
				{
					if (symbols_[idx].name == name)
						return &symbols_[idx];
				}
			}
			scopeId = scopes_[scopeId].parent;
		}
		return nullptr;
	}

	std::vector<const Symbol*> SymbolIndex::symbolsInScope(int scopeId) const
	{
		std::vector<const Symbol*> out;
		if (scopeId < 0 || scopeId >= static_cast<int>(scopeSymbols_.size()))
			return out;

		const auto& indices = scopeSymbols_[scopeId].indices;
		out.reserve(indices.size());
		for (int idx : indices)
			out.push_back(&symbols_[idx]);
		return out;
	}

	std::vector<const Symbol*> SymbolIndex::allSymbols() const
	{
		std::vector<const Symbol*> out;
		out.reserve(symbols_.size());
		for (const auto& s : symbols_) out.push_back(&s);
		return out;
	}

	const Symbol* SymbolIndex::findSymbolAnywhere(std::u32string_view name) const
	{
		auto it = symbolsByName_.find(name);
		return it == symbolsByName_.end() ? nullptr : &symbols_[it->second];
	}

}
