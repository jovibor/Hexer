#include "stdafx.h"
#include "CFileLoader.h"
#include "winioctl.h"
#include <cassert>
#include <filesystem>
#include <format>

CFileLoader::~CFileLoader()
{
	CloseFile();
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
	return IsOpenedVirtual() ? nullptr : static_cast<std::byte*>(m_lpBase);
}

auto CFileLoader::GetVirtualInterface()->HEXCTRL::IHexVirtData*
{
	return IsOpenedVirtual() ? this : nullptr;
}

bool CFileLoader::IsOpenedVirtual()const
{
	return m_fVirtual;
}

bool CFileLoader::IsMutable()const
{
	return m_fWritable;
}

bool CFileLoader::OpenFile(std::wstring_view wsvPath)
{
	assert(m_hFile == nullptr);
	if (m_hFile != nullptr) { //Already opened.
		return false;
	}

	if (wsvPath.starts_with(L"\\\\")) { //Special path.
		m_fVirtual = true;
	}

	m_wstrPath = std::wstring { wsvPath }; //To make sure the path is null-terminated.
	m_hFile = CreateFileW(m_wstrPath.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_hFile = CreateFileW(m_wstrPath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (m_hFile == INVALID_HANDLE_VALUE) {
			const auto dwError = GetLastError();
			wchar_t buffErr[MAX_PATH];
			FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffErr, MAX_PATH, nullptr);
			const auto wstrMsg = std::format(L"CreateFileW failed: 0x{:08X}\r\n{}", dwError, buffErr);
			::MessageBoxW(nullptr, wstrMsg.data(), m_wstrPath.data(), MB_ICONERROR);
			return false;
		}
	}
	else {
		m_fWritable = true;
	}

	if (IsOpenedVirtual()) {
		return OpenVirtual();
	}

	GetFileSizeEx(m_hFile, &m_stFileSize);
	if (m_stFileSize.QuadPart == 0) { //Zero size.
		MessageBoxW(nullptr, L"File is zero size.", m_wstrPath.data(), MB_ICONERROR);
		return false;
	}

	m_hMapObject = CreateFileMappingW(m_hFile, nullptr, m_fWritable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
	if (!m_hMapObject) {
		CloseHandle(m_hFile);
		MessageBoxW(nullptr, L"CreateFileMappingW failed.", m_wstrPath.data(), MB_ICONERROR);
		return false;
	}

	m_lpBase = MapViewOfFile(m_hMapObject, m_fWritable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);

	return true;
}

void CFileLoader::CloseFile()
{
	if (m_hFile != nullptr) {
		if (IsOpenedVirtual()) {
			FlushData();
		}

		FlushViewOfFile(m_lpBase, 0);
		UnmapViewOfFile(m_lpBase);
		CloseHandle(m_hMapObject);
		CloseHandle(m_hFile);
	}

	m_upCache.reset();
	m_wstrPath.clear();
	m_hFile = nullptr;
	m_hMapObject = nullptr;
	m_lpBase = nullptr;
	m_stFileSize = { };
	m_ullOffsetCurr = 0;
	m_ullSizeCurr = 0;
	m_dwAlignment = 0;
	m_fWritable = false;
	m_fVirtual = false;
	m_fModified = false;
}


//Private methods.

bool CFileLoader::IsModified()const
{
	return m_fModified;
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
	m_upCache.reset(static_cast<std::byte*>(_aligned_malloc(m_uBuffSize, m_dwAlignment))); //Initialize the data cache.

	return true;
}

auto CFileLoader::ReadData(std::uint64_t ullOffset, std::uint64_t ullSize)->HEXCTRL::SpanByte
{
	assert(ullOffset + ullSize <= static_cast<std::uint64_t>(m_stFileSize.QuadPart));
	if (ullOffset + ullSize > static_cast<std::uint64_t>(m_stFileSize.QuadPart)) { //Overflow check.
		return { };
	}

	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) { //Data is already in the cache.
		return HEXCTRL::SpanByte { m_upCache.get() + (ullOffset - m_ullOffsetCurr), ullSize};
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
	if (ReadFile(m_hFile, m_upCache.get(), static_cast<DWORD>(ullSizeAligned), &dwBytesRead, &ol) == FALSE) {
		PrintLastError(L"ReadFile");
		return { };
	}

	m_ullOffsetCurr = ullOffsetAligned;
	m_ullSizeCurr = ullSizeAligned;

	return { m_upCache.get() + ullOffsetRemainder, ullSizeAligned - ullOffsetRemainder };
}

void CFileLoader::FlushData()
{
	if (!IsModified())
		return;

	OVERLAPPED ol { };
	ol.Offset = LODWORD(m_ullOffsetCurr);
	ol.OffsetHigh = HIDWORD(m_ullOffsetCurr);
	DWORD dwBytesWritten { };

	if (WriteFile(m_hFile, m_upCache.get(), static_cast<DWORD>(m_ullSizeCurr), &dwBytesWritten, &ol) == FALSE) {
		PrintLastError(L"WriteFile");
	}

	m_fModified = false;
}

void CFileLoader::PrintLastError(std::wstring_view wsvSource)const
{
	const auto dwError = GetLastError();
	wchar_t buffErr[MAX_PATH];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffErr, MAX_PATH, nullptr);
	const auto wstrMsg = std::format(L"{} failed: 0x{:08X}\r\n{}", wsvSource, dwError, buffErr);
	::MessageBoxW(nullptr, wstrMsg.data(), m_wstrPath.data(), MB_ICONERROR);
}