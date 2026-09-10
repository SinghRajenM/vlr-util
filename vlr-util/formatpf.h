#pragma once

#include "config.h"

#include "include.fmt.h"
#include <cstdio>
#include <cwchar>
#include "util.choice.h"
#include "util.convert.StringConversion.h"
#include "util.std_aliases.h"

namespace vlr {

namespace detail {

// The pass-through case

template< typename TResult, typename TSource, typename std::enable_if_t<std::is_same_v<std::decay_t<TResult>, std::decay_t<TSource>>>* = nullptr >
constexpr decltype(auto) ConvertTo_choice(const TSource& tSource, vlr::util::choice<0>&&)
{
	return (tSource);
}

// The direct convertible case

template< typename TResult, typename TSource, typename std::enable_if_t<std::is_convertible_v<TSource, TResult>>* = nullptr >
constexpr auto ConvertTo_choice(const TSource& tSource, vlr::util::choice<1>&&)
{
	return static_cast<TResult>(tSource);
}

// The ToStdString case(s)

template< typename TResult, typename TSource, typename std::enable_if_t<std::is_same_v<std::decay_t<TResult>, std::string>>* = nullptr >
constexpr auto ConvertTo_choice(const TSource& tSource, vlr::util::choice<2>&&)
{
	return util::Convert::ToStdStringA(tSource);
}

template< typename TResult, typename TSource, typename std::enable_if_t<std::is_same_v<std::decay_t<TResult>, std::wstring>>* = nullptr >
constexpr auto ConvertTo_choice(const TSource& tSource, vlr::util::choice<3>&&)
{
	return util::Convert::ToStdStringW(tSource);
}

#if VLR_CONFIG_INCLUDE_ATL_CString

// The ToCString case(s)

template< typename TResult, typename TSource, typename std::enable_if_t<std::is_same_v<std::decay_t<TResult>, CStringA>>* = nullptr >
constexpr auto ConvertTo_choice(const TSource& tSource, vlr::util::choice<4>&&)
{
	return util::Convert::ToCStringA(tSource);
}

template< typename TResult, typename TSource, typename std::enable_if_t<std::is_same_v<std::decay_t<TResult>, CStringW>>* = nullptr >
constexpr auto ConvertTo_choice(const TSource& tSource, vlr::util::choice<5>&&)
{
	return util::Convert::ToCStringW(tSource);
}

#endif

// The fallthrough case

template< typename TResult, typename TSource >
constexpr auto ConvertTo_choice(const TSource& tSource, vlr::util::choice<6>&&)
{
	return TResult{ tSource };
}

} // namespace detail

template< typename TResult, typename TSource >
constexpr auto ConvertTo(const TSource& tSource)
{
	return detail::ConvertTo_choice<TResult>(tSource, vlr::util::choice<0>{});
}

namespace detail {

// Note: Arguments to a C variadic (printf-style) call have to be trivially-copyable POD types, so
// string class types must be reduced to a raw pointer first. The fmt library used to do this for us
// via its typed argument store; this replaces that behaviour for the CRT-based implementation.
// Note: The result aliases the passed argument, so it is only valid for the duration of the full
// expression in which it is called (which is all we need for the formatting call below).

template< typename TChar, typename TArg >
constexpr decltype(auto) AsPrintfArg(const TArg& tArg)
{
	if constexpr (std::is_convertible_v<const TArg&, const TChar*>)
	{
		return static_cast<const TChar*>(tArg);
	}
	else if constexpr (requires { static_cast<const TChar*>(tArg.c_str()); })
	{
		return static_cast<const TChar*>(tArg.c_str());
	}
	else
	{
		return (tArg);
	}
}

template< typename TResult, typename... Arg >
inline auto formatpf_to_TResult(lib_fmt::FormatStringA svFormatString, const Arg&... args)
{
	// Note: A string_view is not necessarily null-terminated, and the CRT formatting functions
	// require a null-terminated format string.
	const auto saFormatString = std::string{ svFormatString };

	// Note: snprintf returns the number of characters which would have been written (excluding the
	// null terminator), or a negative value on error.
	const auto nRequired = std::snprintf(nullptr, 0, saFormatString.c_str(), AsPrintfArg<char>(args)...);
	if (nRequired <= 0)
	{
		return ConvertTo<TResult>(std::string{});
	}

	auto sResult = std::string(static_cast<size_t>(nRequired), '\0');
	std::snprintf(sResult.data(), static_cast<size_t>(nRequired) + 1, saFormatString.c_str(), AsPrintfArg<char>(args)...);

	return ConvertTo<TResult>(sResult);
}

template< typename TResult, typename... Arg >
inline auto formatpf_to_TResult(lib_fmt::FormatStringW svFormatString, const Arg&... args)
{
	const auto swFormatString = std::wstring{ svFormatString };

	// Note: There is no wide equivalent of the snprintf "measure into a null buffer" call; _scwprintf
	// returns the number of characters which would be written (excluding the null terminator).
	const auto nRequired = _scwprintf(swFormatString.c_str(), AsPrintfArg<wchar_t>(args)...);
	if (nRequired <= 0)
	{
		return ConvertTo<TResult>(std::wstring{});
	}

	auto sResult = std::wstring(static_cast<size_t>(nRequired), L'\0');
	_snwprintf_s(sResult.data(), static_cast<size_t>(nRequired) + 1, static_cast<size_t>(nRequired),
		swFormatString.c_str(), AsPrintfArg<wchar_t>(args)...);

	return ConvertTo<TResult>(sResult);
}

} // namespace detail

template< typename TResult, typename... Arg >
inline auto formatpf_to(lib_fmt::FormatStringA svFormatString, Arg&&... args)
-> TResult
{
	return detail::formatpf_to_TResult<TResult>(svFormatString, std::forward<Arg>(args)...);
}

template< typename TResult, typename... Arg >
inline auto formatpf_to(lib_fmt::FormatStringW svFormatString, Arg&&... args)
-> TResult
{
	return detail::formatpf_to_TResult<TResult>(svFormatString, std::forward<Arg>(args)...);
}

template< typename... Arg >
inline auto formatpf(lib_fmt::FormatStringA svFormatString, Arg&&... args)
{
	return detail::formatpf_to_TResult<vlr::string>(svFormatString, std::forward<Arg>(args)...);
}

template< typename... Arg >
inline auto formatpf(lib_fmt::FormatStringW svFormatString, Arg&&... args)
{
	return detail::formatpf_to_TResult<vlr::tstring>(svFormatString, std::forward<Arg>(args)...);
}

} // namespace vlr
