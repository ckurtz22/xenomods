#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

// ghidra typedefs
using undefined = uint8_t;
using undefined1 = uint8_t;
using undefined4 = uint32_t;
using undefined8 = uint64_t;

using byte = uint8_t;
using uint = uint32_t;
using ulong = uint64_t;

#ifndef CONCATENATE
	#define _CONCATENATE(x1, x2) x1##x2
	#define CONCATENATE(x1, x2) _CONCATENATE(x1, x2)
#endif

#ifndef STRINGIFY
	#define _STRINGIFY(x) #x
	#define STRINGIFY(x) _STRINGIFY(x)
#endif

#ifndef PACKED
	#define PACKED __attribute__((__packed__))
#endif

#define INSERT_PADDING_BYTES(num_bytes) \
	uint8_t CONCATENATE(pad, __LINE__)[(num_bytes)];

#define STATIC_ASSERT_SIZE(type, size)  \
	static_assert(sizeof(type) == size, \
				  STRINGIFY(type) " should have size " STRINGIFY(size));

// Technically, we could also use trait classes which get EBCO'd
// to do this. That would work perfectly well, and not bloat up classes either.
// I don't think there's a way to attach a reason though,
// short of some cNTTP fuckery (which also means the "traits" become a bunch of templates with giant cnttp string templates, which. yeah...).
// I don't really want to forego the trouble of doing that,
// so this will do the trick for now.
//
// While it's true that comments could be used to communicate reasoning,
// generally speaking reasoning right where it's being disabled should be more useful,
// especially since it'll be the line the diagnostics will end up showing.
#define XENOMODS_DISALLOW_MOVE(T, reason) T(T&&) = delete;
#define XENOMODS_DISALLOW_COPY(T, reason) T(const T&) = delete;

// Preprocessor macro beautifier for testing
// what codename we are compiling for.
//
// This can be used to gate off code
// for one particular codename.
// Intended to be used in expressions.
#define XENOMODS_CODENAME(cn) defined __XENOMODS_CODENAME_##cn
#define XENOMODS_CODENAME_STR _XENOMODS_CODENAME_STR


namespace xenomods {
	// FIXME: const/by-value versions?

	template<class Enum>
		requires(std::is_enum_v<std::remove_cvref_t<Enum>>)
	constexpr std::underlying_type_t<Enum> underlying_value(Enum e) {
		return static_cast<std::underlying_type_t<Enum>>(e);
	}

#if 0 // this is my take at a const overload, i tried :(
	template<class Enum>
		requires(std::is_enum_v<std::remove_cvref_t<Enum>>)
	constexpr std::underlying_type_t<std::remove_cvref_t<Enum>>& underlyingValue(const std::remove_cvref_t<Enum>& e) {
		return reinterpret_cast<const std::underlying_type_t<std::remove_cvref_t<Enum>>&>(e);
	}


	template<class Enum>
		requires(std::is_enum_v<std::remove_cvref_t<Enum>>)
	constexpr std::underlying_type_t<std::remove_cvref_t<Enum>> underlying_value(std::remove_cvref_t<Enum> e) {
		return reinterpret_cast<std::underlying_type_t<std::remove_cvref_t<Enum>>>(e);
	}
#endif

	namespace detail {

		template <typename T>
		constexpr auto operator |(T lhs, T rhs) requires(std::is_enum_v<std::remove_cvref_t<T>>) {
			return (xenomods::underlying_value(lhs) | xenomods::underlying_value(rhs));
		}

		template <typename T>
		constexpr auto operator &(T lhs, T rhs) requires(std::is_enum_v<std::remove_cvref_t<T>>)  {
			return (xenomods::underlying_value(lhs) & xenomods::underlying_value(rhs));
		}

		template <typename T>
		constexpr auto operator ^(T lhs, T rhs) requires(std::is_enum_v<std::remove_cvref_t<T>>) {
			return (xenomods::underlying_value(lhs) ^ xenomods::underlying_value(rhs));
		}

		template <typename T>
		constexpr auto operator ~(T lhs) requires(std::is_enum_v<std::remove_cvref_t<T>>)  {
			return ~xenomods::underlying_value(lhs);
		}

		template<typename T>
		constexpr bool bitMask(T lhs, T mask) {
			return (lhs & mask) == xenomods::underlying_value(mask);
		}

		template<typename T>
		constexpr bool bitMaskStrict(T lhs, T mask) {
			return xenomods::underlying_value(lhs) == xenomods::underlying_value(mask);
		}
	}

	template<class TConv, std::size_t Size>
	struct ConvertTo {
		static_assert(sizeof(TConv) == Size, "Converted type size and true object size must be the same");

		// default (so we're literal)
		constexpr ConvertTo() = default;

		// init with type to copy into
		constexpr ConvertTo(const TConv& tc) {
			memcpy(&__byteRep[0], &tc, sizeof(TConv));
		}

		// convert into writable version
		operator TConv&() {
			return *reinterpret_cast<TConv*>(&__byteRep[0]);
		}

		// convert read only version
		operator const TConv&() const {
			return *reinterpret_cast<const TConv*>(&__byteRep[0]);
		}

		uint8_t __byteRep[Size];
	};

	using xenomods::detail::bitMask;
	using xenomods::detail::bitMaskStrict;

#define XENOMODS_CONVERTTO_TYPE(T, U, size) \
    struct T : public xenomods::ConvertTo<U, size> { \
        using ConvertTo<U, size>::ConvertTo;  \
    }

} // namespace xenomods

// pull detail operators into global namespace
using xenomods::detail::operator|;
using xenomods::detail::operator&;
using xenomods::detail::operator^;
using xenomods::detail::operator~;
