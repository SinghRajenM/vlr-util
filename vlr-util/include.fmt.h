#pragma once

#include "config.h"

#include <format>

#include "zstring_view.h"

// Note: Some using aliases to allow us to specify the types which the std::format library expects 
// for various method calls.

namespace lib_fmt {

using FormatStringA = std::basic_string_view<char>;
using FormatStringW = std::basic_string_view<wchar_t>;
using FormatStringT = std::basic_string_view<TCHAR>;

} // namespace lib_fmt

// Note: If you have "custom" types which you want to pass as format parameters, you need to 
// create template specializations which tell the library how to parse and format them.
// As of time of writing: https://en.cppreference.com/w/cpp/utility/format/formatter

// Note: The zstring_view types derive from std::basic_string_view rather than being it, and the 
// standard formatter specializations only match the exact type, so they need their own. (The fmt 
// library used to pick these up implicitly.) Both types are formatted via the underlying view.

template< typename TChar, typename TTraits >
struct std::formatter<vlr::basic_zstring_view<TChar, TTraits>, TChar>
	: std::formatter<std::basic_string_view<TChar, TTraits>, TChar>
{
	template< typename FormatContext >
	auto format( const vlr::basic_zstring_view<TChar, TTraits>& svzParam, FormatContext& ctx ) const
	{
		return std::formatter<std::basic_string_view<TChar, TTraits>, TChar>::format( svzParam.asStringView(), ctx );
	}
};

template< typename TChar, typename TTraits >
struct std::formatter<vlr::basic_zstring_view_param<TChar, TTraits>, TChar>
	: std::formatter<std::basic_string_view<TChar, TTraits>, TChar>
{
	template< typename FormatContext >
	auto format( const vlr::basic_zstring_view_param<TChar, TTraits>& svzParam, FormatContext& ctx ) const
	{
		return std::formatter<std::basic_string_view<TChar, TTraits>, TChar>::format( svzParam.asStringView(), ctx );
	}
};

#if VLR_CONFIG_INCLUDE_ATL_CString

// This code essentially just converts CString to string_view, which is supported directly.

#include <atlstr.h>

template<>
struct std::formatter<CStringA> : std::formatter<std::string_view>
{
	template< typename FormatContext >
	auto format( const CStringA& sParam, FormatContext& ctx ) const
	{
		return std::formatter<std::string_view>::format(
			std::string_view{ sParam.GetString(), static_cast<size_t>(sParam.GetLength()) }, ctx );
	}
};

template<>
struct std::formatter<CStringW> : std::formatter<std::wstring_view>
{
	template< typename FormatContext >
	auto format( const CStringW& sParam, FormatContext& ctx ) const
	{
		return std::formatter<std::wstring_view>::format(
			std::wstring_view{ sParam.GetString(), static_cast<size_t>(sParam.GetLength()) }, ctx );
	}
};

#endif
