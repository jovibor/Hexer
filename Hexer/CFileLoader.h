#pragma once
#include "HexCtrl.h"
#include <memory>
import Utility;

class CFileLoader : public HEXCTRL::IHexVirtData
{
public:
	~CFileLoader();
	[[nodiscard]] auto GetCacheSize()const->DWORD;
	[[nodiscard]] auto GetFileSize()const->std::uint64_t;
	[[nodiscard]] auto GetFileData()const->std::byte*;
	[[nodiscard]] auto GetVirtualInterface() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsOpenedVirtual()const;
	[[nodiscard]] bool IsMutable()const;
	[[nodiscard]] bool OpenFile(const Utility::FILEOPEN& fos);
	void CloseFile();
private:
	void OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)override;
	void OnHexSetData(const HEXCTRL::HEXDATAINFO& hdi)override;
	[[nodiscard]] bool OpenVirtual();
	[[nodiscard]] auto ReadData(std::uint64_t ullOffset, std::uint64_t ullSize) -> HEXCTRL::SpanByte;
	[[nodiscard]] bool IsModified()const;
	void FlushData();
	void PrintLastError(std::wstring_view wsvSource)const;
private:
	static constexpr auto m_uBuffSize { 1024UL * 512UL }; //512KB cache buffer size.
	std::unique_ptr < std::byte[], decltype([](std::byte* p) { _aligned_free(p); }) > m_upCache { };
	std::wstring m_wstrPath; //File path to open.
	HANDLE m_hFile { };      //Returned by CreateFileW.
	HANDLE m_hMapObject { }; //Returned by CreateFileMappingW.
	LPVOID m_lpBase { };     //Returned by MapViewOfFile.
	LARGE_INTEGER m_stFileSize { };
	std::uint64_t m_ullOffsetCurr { }; //Offset of the current data that is in the buffer.
	std::uint64_t m_ullSizeCurr { };   //Size of the current data that is in the buffer.
	DWORD m_dwAlignment { };    //An alignment, the offset and the size must be aligned on, for the ReadFile.
	bool m_fWritable { false }; //Is file opened as RW or RO?
	bool m_fVirtual { false };
	bool m_fModified { false }; //File was modified.
};