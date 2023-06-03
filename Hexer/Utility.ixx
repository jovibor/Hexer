module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <span>
#include <string>
export module Utility;

export namespace Utility
{
	constexpr auto HEXER_VERSION_MAJOR = 0;
	constexpr auto HEXER_VERSION_MINOR = 9;
	constexpr auto HEXER_VERSION_PATCH = 0;
	constexpr wchar_t g_wstrAppName[] { L"Hexer" };

	struct FILEPROPS {
		std::wstring_view wsvFilePath { };
		std::wstring_view wsvFileName { };
		std::uint64_t     ullFileSize { };
		std::uint32_t     dwPageSize { };
		bool              fWritable { };
	};
}