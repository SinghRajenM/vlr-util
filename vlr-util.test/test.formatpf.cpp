#include "pch.h"

#include "vlr-util/formatpf.h"
#include "vlr-util/zstring_view.h"
#include "vlr-util/util.std_aliases.h"

TEST( formatpf, general )
{
	constexpr auto svzFormatString = vlr::tzstring_view{ _T( "The answer is %d" ) };

	{
		auto sResult = vlr::formatpf_to<CStringA>( "The answer is %d", 42 );
		EXPECT_STREQ( sResult, "The answer is 42" );
	}
	{
		auto sResult = vlr::formatpf_to<CStringW>( L"The answer is %d", 42 );
		EXPECT_STREQ( sResult, L"The answer is 42" );
	}
	{
		auto sResult = vlr::formatpf_to<CString>( _T( "The answer is %d" ), 42 );
		EXPECT_STREQ( sResult, _T( "The answer is 42" ) );
	}
	{
		auto sResult = vlr::formatpf_to<std::string>( "The answer is %d", 42 );
		EXPECT_STREQ( sResult.c_str(), "The answer is 42" );
	}
	{
		auto sResult = vlr::formatpf_to<std::wstring>( L"The answer is %d", 42 );
		EXPECT_STREQ( sResult.c_str(), L"The answer is 42" );
	}
	{
		auto sResult = vlr::formatpf_to<vlr::tstring>( _T( "The answer is %d" ), 42 );
		EXPECT_STREQ( sResult.c_str(), _T( "The answer is 42" ) );
	}
	{
		auto sResult = vlr::formatpf_to<CString>( svzFormatString, 42 );
		EXPECT_STREQ( sResult, _T( "The answer is 42" ) );
	}
	{
		auto sResult = vlr::formatpf_to<vlr::tstring>( svzFormatString, 42 );
		EXPECT_STREQ( sResult.c_str(), _T( "The answer is 42" ) );
	}
}

// Note: String arguments are the interesting case, because a printf-style call passes them through a
// C variadic and so they have to be reduced to a raw pointer first (the fmt library used to do this
// for us via its typed argument store).

TEST( formatpf, string_arguments )
{
	{
		auto sResult = vlr::formatpf_to<std::string>( "[%s]", "literal" );
		EXPECT_STREQ( sResult.c_str(), "[literal]" );
	}
	{
		auto sResult = vlr::formatpf_to<std::wstring>( L"[%s]", L"literal" );
		EXPECT_STREQ( sResult.c_str(), L"[literal]" );
	}
	{
		const auto sValue = std::string{ "std::string" };
		auto sResult = vlr::formatpf_to<std::string>( "[%s]", sValue );
		EXPECT_STREQ( sResult.c_str(), "[std::string]" );
	}
	{
		const auto sValue = std::wstring{ L"std::wstring" };
		auto sResult = vlr::formatpf_to<std::wstring>( L"[%s]", sValue );
		EXPECT_STREQ( sResult.c_str(), L"[std::wstring]" );
	}
	{
		const auto svzValue = vlr::tzstring_view{ _T( "zstring_view" ) };
		auto sResult = vlr::formatpf_to<vlr::tstring>( _T( "[%s]" ), svzValue );
		EXPECT_STREQ( sResult.c_str(), _T( "[zstring_view]" ) );
	}
	{
		// Note: This mirrors the usage in Convert::ToDisplay_ApproxDataSize
		const auto svzValue = vlr::tzstring_view_param{ _T( "KB" ) };
		auto sResult = vlr::formatpf( _T( "%.02f %s" ), 1.5, svzValue );
		EXPECT_STREQ( sResult.c_str(), _T( "1.50 KB" ) );
	}
	{
		const auto sValue = CString{ _T( "CString" ) };
		auto sResult = vlr::formatpf_to<vlr::tstring>( _T( "[%s]" ), sValue );
		EXPECT_STREQ( sResult.c_str(), _T( "[CString]" ) );
	}
	{
		// Note: Mixed argument types, and a result longer than any small-buffer optimization
		auto sResult = vlr::formatpf_to<std::string>( "%s=%d/%s", "name", 7, std::string( 200, 'x' ) );
		EXPECT_EQ( sResult, std::string{ "name=7/" } + std::string( 200, 'x' ) );
	}
}
