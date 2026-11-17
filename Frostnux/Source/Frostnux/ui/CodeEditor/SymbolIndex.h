//#pragma once
//#include "TextBuffer.h"
//#include <vector>
//#include <deque>
//#include <unordered_map>
//#include <unordered_set>
//#include <string>
//#include <string_view>
//
//namespace Frostnux {
//
//	enum class SymbolKind : uint8_t
//	{
//		Namespace, Class, Struct, Union, Enum, EnumValue,
//		Function, Variable, Member, Typedef, Unknown
//	};
//
//	struct Symbol
//	{
//		std::u32string	name;
//		std::u32string	typeName;
//		int				scopeId = -1;
//		int				line = 0;
//		int				col = 0;
//		SymbolKind		kind = SymbolKind::Unknown;
//
//		Symbol() = default;
//
//		Symbol(std::u32string n, std::u32string tn, SymbolKind k, int s = -1, int l = 0, int c = 0)
//			: name(std::move(n)), typeName(std::move(tn)), scopeId(s), line(l), col(c), kind(k) {}
//	};
//
//	struct Scope
//	{
//		std::u32string	name;
//		std::u32string	qualified;
//		std::unordered_map<std::u32string, int> children;
//		int				parent = -1;
//		int				col = 0;
//		SymbolKind		kind = SymbolKind::Namespace;
//
//		Scope() = default;
//
//		Scope(std::u32string n, std::u32string q, SymbolKind k, int p = -1, int c = 0, std::unordered_map<std::u32string, int> ch = {})
//			: name(std::move(n)), qualified(std::move(q)), children(std::move(ch)), parent(p), col(c), kind(k) {}
//	};
//
//	struct SymbolToken
//	{
//		enum class Kind : uint8_t { Ident, Punct, Number, String };
//
//		std::u32string_view	text;
//		int					col = 0;
//		Kind				kind = Kind::Ident;
//
//		SymbolToken() = default;
//
//		SymbolToken(Kind k, std::u32string_view t, int c = 0)
//			: text(t), col(c), kind(k) {}
//
//		SymbolToken(Kind, std::u32string&&, int = 0) = delete;
//	};
//
//	class SymbolIndex
//	{
//	public:
//		SymbolIndex() = default;
//		~SymbolIndex() = default;
//		SymbolIndex(SymbolIndex&& other) noexcept { swap(other); }
//
//		SymbolIndex& operator=(SymbolIndex&& other) noexcept
//		{
//			if (this != &other)
//				swap(other);
//			return *this;
//		}
//		SymbolIndex(const SymbolIndex&) = delete;
//		SymbolIndex& operator=(const SymbolIndex&) = delete;
//		void swap(SymbolIndex& other) noexcept
//		{
//			using std::swap;
//			swap(scopes_, other.scopes_);
//			swap(symbols_, other.symbols_);
//			swap(scopeSymbols_, other.scopeSymbols_);
//			swap(symbolsByName_, other.symbolsByName_);
//		}
//
//		void rebuild(const TextBuffer& buf);
//
//		[[nodiscard]] int rootScope() const { return 0; }
//		[[nodiscard]] int findScope(std::u32string_view qualified) const;
//		[[nodiscard]] const Symbol* findSymbol(int scopeId, std::u32string_view name) const;
//		[[nodiscard]] std::vector<const Symbol*> symbolsInScope(int scopeId) const;
//		[[nodiscard]] std::vector<const Symbol*> allSymbols() const;
//		[[nodiscard]] bool empty() const { return symbols_.empty(); }
//		[[nodiscard]] const Symbol* findSymbolAnywhere(std::u32string_view name) const;
//	private:
//		struct ParseContext;
//
//		struct SymbolKey
//		{
//			std::u32string_view name;
//			SymbolKind			kind;
//		};
//
//		struct SymbolKeyHash
//		{
//			std::size_t operator()(const SymbolKey& k) const noexcept
//			{
//				std::size_t h = 14695981039346656037ull;
//				for (char32_t c : k.name)
//				{
//					h ^= static_cast<std::size_t>(c);
//					h *= 1099511628211ull;
//				}
//				h ^= static_cast<std::size_t>(k.kind) * 0x9E3779B97F4A7C15ull;
//				return h;
//			}
//		};
//
//		struct SymbolKeyEq
//		{
//			bool operator()(const SymbolKey& a, const SymbolKey& b) const noexcept
//			{
//				return a.kind == b.kind && a.name == b.name;
//			}
//		};
//
//		struct SymbolTable
//		{
//			std::vector<int> indices;
//			std::unordered_map<SymbolKey, int, SymbolKeyHash, SymbolKeyEq> byNameKind;
//		};
//
//		struct U32ViewHash
//		{
//			std::size_t operator()(std::u32string_view s) const noexcept
//			{
//				std::size_t h = 14695981039346656037ull;
//				for (char32_t c : s)
//				{
//					h ^= static_cast<std::size_t>(c);
//					h *= 1099511628211ull;
//				}
//				return h;
//			}
//		};
//
//		void processTokens(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx);
//		void tryParseDeclaration(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx);
//
//		void pushPendingScope(ParseContext& ctx, SymbolKind kind, const std::u32string& name, int col);
//		void registerSymbol(ParseContext& ctx, std::u32string name, SymbolKind kind,
//			std::u32string typeName, int line, int col);
//
//		std::vector<Scope>		scopes_;
//		std::deque<Symbol>		symbols_;
//		std::vector<SymbolTable> scopeSymbols_;
//		std::unordered_map<std::u32string_view, int, U32ViewHash> symbolsByName_;
//	};
//
//}

#pragma once
#include "TextBuffer.h"
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>

namespace Frostnux {

	enum class SymbolKind : uint8_t
	{
		Namespace, Class, Struct, Union, Enum, EnumValue,
		Function, Variable, Member, Typedef, Unknown
	};

	struct Symbol
	{
		std::u32string	name;
		int				typeNameId = -1;
		int				scopeId = -1;
		int				line = 0;
		int				col = 0;
		SymbolKind		kind = SymbolKind::Unknown;

		Symbol() = default;

		Symbol(std::u32string n, int tnId, SymbolKind k, int s = -1, int l = 0, int c = 0)
			: name(std::move(n)), typeNameId(tnId), scopeId(s), line(l), col(c), kind(k) {}
	};

	struct Scope
	{
		std::u32string	name;
		std::u32string	qualified;
		std::unordered_map<std::u32string, int> children;
		int				parent = -1;
		int				col = 0;
		SymbolKind		kind = SymbolKind::Namespace;

		Scope() = default;

		Scope(std::u32string n, std::u32string q, SymbolKind k, int p = -1, int c = 0, std::unordered_map<std::u32string, int> ch = {})
			: name(std::move(n)), qualified(std::move(q)), children(std::move(ch)), parent(p), col(c), kind(k) {}
	};

	struct SymbolToken
	{
		enum class Kind : uint8_t { Ident, Punct, Number, String };

		std::u32string_view	text;
		int					col = 0;
		Kind				kind = Kind::Ident;

		SymbolToken() = default;

		SymbolToken(Kind k, std::u32string_view t, int c = 0)
			: text(t), col(c), kind(k) {}

		SymbolToken(Kind, std::u32string&&, int = 0) = delete;
	};

	class SymbolIndex
	{
	public:
		SymbolIndex() = default;
		~SymbolIndex() = default;

		SymbolIndex(SymbolIndex&& other) noexcept { swap(other); }
		SymbolIndex& operator=(SymbolIndex&& other) noexcept
		{
			if (this != &other) swap(other);
			return *this;
		}
		SymbolIndex(const SymbolIndex&) = delete;
		SymbolIndex& operator=(const SymbolIndex&) = delete;

		void swap(SymbolIndex& other) noexcept
		{
			using std::swap;
			swap(scopes_, other.scopes_);
			swap(symbols_, other.symbols_);
			swap(scopeSymbols_, other.scopeSymbols_);
			swap(symbolsByName_, other.symbolsByName_);
			swap(typeNamePool_, other.typeNamePool_);
			swap(typeNameLookup_, other.typeNameLookup_);
		}

		void rebuild(const TextBuffer& buf);

		[[nodiscard]] int rootScope() const { return 0; }
		[[nodiscard]] int findScope(std::u32string_view qualified) const;
		[[nodiscard]] const Symbol* findSymbol(int scopeId, std::u32string_view name) const;
		[[nodiscard]] std::vector<const Symbol*> symbolsInScope(int scopeId) const;
		[[nodiscard]] std::vector<const Symbol*> allSymbols() const;
		[[nodiscard]] bool empty() const { return symbols_.empty(); }
		[[nodiscard]] const Symbol* findSymbolAnywhere(std::u32string_view name) const;

		[[nodiscard]] std::u32string_view typeNameOf(const Symbol& s) const
		{
			return s.typeNameId < 0 ? std::u32string_view{}
			: std::u32string_view(typeNamePool_[s.typeNameId]);
		}

		[[nodiscard]] std::u32string_view typeNameOfId(int id) const
		{
			return id < 0 ? std::u32string_view{}
			: std::u32string_view(typeNamePool_[id]);
		}

		[[nodiscard]] std::size_t typeNameCount() const { return typeNamePool_.size(); }
		[[nodiscard]] std::u32string_view typeNameAt(std::size_t i) const
		{
			return i < typeNamePool_.size() ? std::u32string_view(typeNamePool_[i]) : std::u32string_view{};
		}

		template<typename F>
		void forEachSymbol(F&& f) const
		{
			for (const auto& s : symbols_) f(s);
		}
	private:
		struct ParseContext;

		struct SymbolKey
		{
			std::u32string_view name;
			SymbolKind			kind;
		};

		struct SymbolKeyHash
		{
			std::size_t operator()(const SymbolKey& k) const noexcept
			{
				std::size_t h = 14695981039346656037ull;
				for (char32_t c : k.name)
				{
					h ^= static_cast<std::size_t>(c);
					h *= 1099511628211ull;
				}
				h ^= static_cast<std::size_t>(k.kind) * 0x9E3779B97F4A7C15ull;
				return h;
			}
		};

		struct SymbolKeyEq
		{
			bool operator()(const SymbolKey& a, const SymbolKey& b) const noexcept
			{
				return a.kind == b.kind && a.name == b.name;
			}
		};

		struct SymbolTable
		{
			std::vector<int> indices;
			std::unordered_map<SymbolKey, int, SymbolKeyHash, SymbolKeyEq> byNameKind;
		};

		struct U32ViewHash
		{
			std::size_t operator()(std::u32string_view s) const noexcept
			{
				std::size_t h = 14695981039346656037ull;
				for (char32_t c : s)
				{
					h ^= static_cast<std::size_t>(c);
					h *= 1099511628211ull;
				}
				return h;
			}
		};

		void processTokens(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx);
		void tryParseDeclaration(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx);

		void pushPendingScope(ParseContext& ctx, SymbolKind kind, const std::u32string& name, int col);
		void registerSymbol(ParseContext& ctx, std::u32string name, SymbolKind kind,
			std::u32string typeName, int line, int col);

		int internTypeName(const std::u32string& tn);

		std::vector<Scope>		scopes_;
		std::deque<Symbol>		symbols_;
		std::vector<SymbolTable> scopeSymbols_;
		std::unordered_map<std::u32string_view, int, U32ViewHash> symbolsByName_;

		std::deque<std::u32string> typeNamePool_;
		std::unordered_map<std::u32string_view, int, U32ViewHash> typeNameLookup_;
	};

}
