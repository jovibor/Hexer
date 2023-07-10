module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "HexCtrl.h"
#include "winioctl.h"
#include <cassert>
#include <filesystem>
#include <format>
#include <memory>
export module FileLoader;

import Utility;

export class CFileLoader : public HEXCTRL::IHexVirtData
{
public:
	~CFileLoader();
	void CloseFile();
	[[nodiscard]] auto GetCacheSize()const->DWORD;
	[[nodiscard]] auto GetFileSize()const->std::uint64_t;
	[[nodiscard]] auto GetFileData()const->std::byte*;
	[[nodiscard]] auto GetVirtualInterface() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsMutable()const;
	[[nodiscard]] bool OpenFile(const Ut::FILEOPEN& fos);
private:
	void FlushData();
	[[nodiscard]] bool IsModified()const;
	[[nodiscard]] bool IsVirtual()const;
	void OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)override;
	void OnHexSetData(const HEXCTRL::HEXDATAINFO& hdi)override;
	[[nodiscard]] bool OpenVirtual();
	void PrintLastError(std::wstring_view wsvSource)const;
	[[nodiscard]] auto ReadData(std::uint64_t ullOffset, std::uint64_t ullSize) -> HEXCTRL::SpanByte;
private:
	static constexpr auto m_uBuffSize { 1024UL * 512UL }; //512KB cache buffer size.
	std::unique_ptr < std::byte[], decltype([](auto p) { _aligned_free(p); }) > m_pCache { };
	std::wstring m_wstrPath; //File path to open.
	HANDLE m_hFile { };      //Returned by CreateFileW.
	HANDLE m_hMapObject { }; //Returned by CreateFileMappingW.
	LPVOID m_lpBase { };     //Returned by MapViewOfFile.
	LARGE_INTEGER m_stFileSize { };
	std::uint64_t m_ullOffsetCurr { }; //Offset of the current data that is in the buffer.
	std::uint64_t m_ullSizeCurr { };   //Size of the current data that is in the buffer.
	DWORD m_dwAlignment { };    //An alignment that the offset and the size must be aligned on, for the ReadFile.
	bool m_fMutable { false };  //Is file opened as RW or RO?
	bool m_fVirtual { false };  //Is file opened in HexCtrl Virtual mode.
	bool m_fModified { false }; //File was modified.
};

CFileLoader::~CFileLoader()
{
	CloseFile();
}

void CFileLoader::CloseFile()
{
	if (m_hFile != nullptr) {
		if (IsVirtual()) {
			FlushData();
		}

		FlushViewOfFile(m_lpBase, 0);
		UnmapViewOfFile(m_lpBase);
		CloseHandle(m_hMapObject);
		CloseHandle(m_hFile);
	}

	m_pCache.reset();
	m_wstrPath.clear();
	m_hFile = nullptr;
	m_hMapObject = nullptr;
	m_lpBase = nullptr;
	m_stFileSize = { };
	m_ullOffsetCurr = 0;
	m_ullSizeCurr = 0;
	m_dwAlignment = 0;
	m_fMutable = false;
	m_fVirtual = false;
	m_fModified = false;
}

auto CFileLoader::GetCacheSize()const->DWORD
{
	return m_uBuffSize;
}

auto CFileLoader::GetFileSize()const->std::uint64_t
{
	return static_cast<std::uint64_t>(m_stFileSize.QuadPart);
}

auto CFileLoader::GetFileData()const->std::byte*
{
	return IsVirtual() ? nullptr : static_cast<std::byte*>(m_lpBase);
}

auto CFileLoader::GetVirtualInterface()->HEXCTRL::IHexVirtData*
{
	return IsVirtual() ? this : nullptr;
}

bool CFileLoader::IsMutable()const
{
	return m_fMutable;
}

bool CFileLoader::OpenFile(const Ut::FILEOPEN& fos)
{
	assert(m_hFile == nullptr);
	if (m_hFile != nullptr) { //Already opened.
		return false;
	}

	if (fos.wstrFilePath.starts_with(L"\\\\")) { //Special path.
		m_fVirtual = true;
	}

	m_wstrPath = fos.wstrFilePath;
	m_hFile = CreateFileW(m_wstrPath.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		fos.fNewFile ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (fos.fNewFile) { //Setting the size of the new file.
		if (SetFilePointerEx(m_hFile, { .QuadPart { static_cast<LONGLONG>(fos.ullFileSize)} }, nullptr, FILE_BEGIN) == FALSE) {
			PrintLastError(L"SetFilePointerEx");
			return false;
		}
		SetEndOfFile(m_hFile);
	}

	if (m_hFile == INVALID_HANDLE_VALUE) {
		if (!fos.fNewFile) { //Trying to open in ReadOnly mode.
			m_hFile = CreateFileW(m_wstrPath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		}

		if (m_hFile == INVALID_HANDLE_VALUE) {
			PrintLastError(L"CreateFileW");
			return false;
		}
	}
	else {
		m_fMutable = true;
	}

	if (IsVirtual()) {
		return OpenVirtual();
	}

	if (GetFileSizeEx(m_hFile, &m_stFileSize); m_stFileSize.QuadPart == 0) { //Zero size.
		MessageBoxW(nullptr, L"File is zero size.", m_wstrPath.data(), MB_ICONERROR);
		return false;
	}

	if (m_hMapObject = CreateFileMappingW(m_hFile, nullptr, m_fMutable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
		m_hMapObject == nullptr) {
		PrintLastError(L"CreateFileMappingW");
		CloseHandle(m_hFile);
		return false;
	}

	m_lpBase = MapViewOfFile(m_hMapObject, m_fMutable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);

	return true;
}


//Private methods.

void CFileLoader::FlushData()
{
	if (!IsModified())
		return;

	OVERLAPPED ol { };
	ol.Offset = LODWORD(m_ullOffsetCurr);
	ol.OffsetHigh = HIDWORD(m_ullOffsetCurr);
	DWORD dwBytesWritten { };

	if (WriteFile(m_hFile, m_pCache.get(), static_cast<DWORD>(m_ullSizeCurr), &dwBytesWritten, &ol) == FALSE) {
		PrintLastError(L"WriteFile");
	}

	m_fModified = false;
}

bool CFileLoader::IsModified()const
{
	return m_fModified;
}

bool CFileLoader::IsVirtual()const
{
	return m_fVirtual;
}

void CFileLoader::OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)
{
	hdi.spnData = ReadData(hdi.stHexSpan.ullOffset, hdi.stHexSpan.ullSize);
}

void CFileLoader::OnHexSetData(const HEXCTRL::HEXDATAINFO& /*hdi*/)
{
	m_fModified = true;
}

bool CFileLoader::OpenVirtual()
{
	if (m_hFile == nullptr)
		return false;

	DISK_GEOMETRY stGeometry { };
	DWORD dwBytesRet { };
	if (!DeviceIoControl(m_hFile, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0, &stGeometry, sizeof(stGeometry), &dwBytesRet, nullptr)) {
		PrintLastError(L"DeviceIoControl(IOCTL_DISK_GET_DRIVE_GEOMETRY)");
		return false;
	}

	m_dwAlignment = stGeometry.BytesPerSector;

	GET_LENGTH_INFORMATION stLengthInfo { };
	switch (stGeometry.MediaType) {
	case MEDIA_TYPE::Unknown:
	case MEDIA_TYPE::RemovableMedia:
	case MEDIA_TYPE::FixedMedia:
		if (!DeviceIoControl(m_hFile, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &stLengthInfo, sizeof(stLengthInfo), &dwBytesRet, nullptr)) {
			PrintLastError(L"DeviceIoControl(IOCTL_DISK_GET_LENGTH_INFO)");
			return false;
		}
		break;
	default:
		stLengthInfo.Length.QuadPart = stGeometry.Cylinders.QuadPart * stGeometry.TracksPerCylinder *
			stGeometry.SectorsPerTrack * stGeometry.BytesPerSector;
		break;
	}

	m_stFileSize.QuadPart = stLengthInfo.Length.QuadPart;
	m_pCache.reset(static_cast<std::byte*>(_aligned_malloc(m_uBuffSize, m_dwAlignment))); //Initialize the data cache.

	return true;
}

void CFileLoader::PrintLastError(std::wstring_view wsvSource)const
{
	const auto dwError = GetLastError();
	wchar_t buffErr[MAX_PATH];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffErr, MAX_PATH, nullptr);
	Ut::Log::AddLogEntryError(std::format(L"{} failed: 0x{:08X}\r\n{}", wsvSource, dwError, buffErr));
}

auto CFileLoader::ReadData(std::uint64_t ullOffset, std::uint64_t ullSize)->HEXCTRL::SpanByte
{
	assert(ullOffset + ullSize <= static_cast<std::uint64_t>(m_stFileSize.QuadPart));
	if (ullOffset + ullSize > static_cast<std::uint64_t>(m_stFileSize.QuadPart)) { //Overflow check.
		return { };
	}

	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) { //Data is already in the cache.
		return HEXCTRL::SpanByte { m_pCache.get() + (ullOffset - m_ullOffsetCurr), ullSize};
	}

	FlushData(); //Flush current cache data if it was modified, before the ReadFile.

	const auto ullOffsetRemainder = ullOffset % m_dwAlignment;
	const auto ullOffsetAligned = ullOffset - ullOffsetRemainder;
	const auto ullSizeAligned = (ullOffsetAligned + m_uBuffSize) <= static_cast<std::uint64_t>(m_stFileSize.QuadPart) ?
		m_uBuffSize : m_stFileSize.QuadPart - ullOffsetAligned; //Size at the end of a file can be not aligned.
	assert(ullSizeAligned >= ullSize);

	OVERLAPPED ol { };
	ol.Offset = LODWORD(ullOffsetAligned);
	ol.OffsetHigh = HIDWORD(ullOffsetAligned);
	DWORD dwBytesRead { };
	if (ReadFile(m_hFile, m_pCache.get(), static_cast<DWORD>(ullSizeAligned), &dwBytesRead, &ol) == FALSE) {
		PrintLastError(L"ReadFile");
		return { };
	}

	m_ullOffsetCurr = ullOffsetAligned;
	m_ullSizeCurr = ullSizeAligned;

	return { m_pCache.get() + ullOffsetRemainder, ullSizeAligned - ullOffsetRemainder };
}