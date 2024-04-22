module;
/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "HexCtrl.h"
#include "psapi.h"
#include "winioctl.h"
#include <cassert>
#include <filesystem>
#include <format>
#include <memory>
export module DataLoader;

import Utility;

export class CDataLoader final : public HEXCTRL::IHexVirtData {
public:
	CDataLoader();
	~CDataLoader();
	void Close();
	[[nodiscard]] auto GetCacheSize()const->DWORD;
	[[nodiscard]] auto GetDataSize()const->std::uint64_t;
	[[nodiscard]] auto GetFileData()const->std::byte*;
	[[nodiscard]] auto GetMemPageSize()const->DWORD;
	[[nodiscard]] auto GetVirtualInterface() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsMutable()const;
	[[nodiscard]] bool IsProcess()const;
	[[nodiscard]] bool Open(const Ut::FILEOPEN& fos);
private:
	void FlushFileData();
	[[nodiscard]] bool IsModified()const;
	[[nodiscard]] bool IsVirtual()const;
	void OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)override;
	void OnHexSetData(const HEXCTRL::HEXDATAINFO& hdi)override;
	[[nodiscard]] bool OpenFileVirtual();
	[[nodiscard]] bool OpenFile(const Ut::FILEOPEN& fos);
	[[nodiscard]] bool OpenProcess(const Ut::FILEOPEN& fos);
	void PrintLastError(std::wstring_view wsvSource)const;
	[[nodiscard]] auto ReadFileData(std::uint64_t ullOffset, std::uint64_t ullSize) -> HEXCTRL::SpanByte;
	[[nodiscard]] auto ReadProcData(std::uint64_t ullOffset, std::uint64_t ullSize) -> HEXCTRL::SpanByte;
private:
	static constexpr auto m_dwCacheSize { 1024UL * 512UL }; //512KB cache size.
	std::unique_ptr < std::byte[], decltype([](auto p) { _aligned_free(p); }) > m_pCache { };
	std::wstring m_wstrFileName; //File name without path.
	std::wstring m_wstrFilePath; //Data path to open.
	HANDLE m_hHandle { };        //Handle of a file or process.
	HANDLE m_hMapObject { };     //Returned by CreateFileMappingW.
	LPVOID m_lpMapBase { };      //Returned by MapViewOfFile.
	LARGE_INTEGER m_stDataSize { };
	std::uint64_t m_ullOffsetCurr { }; //Offset of the data that is currently in the cache.
	std::uint64_t m_ullSizeCurr { };   //Size of the data that is currently in the cache.
	DWORD m_dwPageSize { };      //System Virtual page size.
	DWORD m_dwAlignment { };     //An alignment that the offset and the size must be aligned on, for the ReadFile.
	bool m_fMutable { false };   //Is data opened as RW or RO?
	bool m_fVirtual { false };   //Is data opened in HexCtrl Virtual mode.
	bool m_fProcess { false };   //Is it process or file?
	bool m_fModified { false };  //File was modified.
};

CDataLoader::CDataLoader()
{
	SYSTEM_INFO si { };
	GetSystemInfo(&si);
	m_dwPageSize = si.dwPageSize; //4KB.
}

CDataLoader::~CDataLoader()
{
	Close();
}

void CDataLoader::Close()
{
	if (m_hHandle != nullptr) {
		if (IsVirtual()) {
			FlushFileData();
		}

		FlushViewOfFile(m_lpMapBase, 0);
		UnmapViewOfFile(m_lpMapBase);
		CloseHandle(m_hMapObject);
		CloseHandle(m_hHandle);
	}

	m_pCache.reset();
	m_wstrFilePath.clear();
	m_hHandle = nullptr;
	m_hMapObject = nullptr;
	m_lpMapBase = nullptr;
	m_stDataSize = { };
	m_ullOffsetCurr = 0;
	m_ullSizeCurr = 0;
	m_dwAlignment = 0;
	m_fMutable = false;
	m_fVirtual = false;
	m_fProcess = false;
	m_fModified = false;
}

auto CDataLoader::GetCacheSize()const->DWORD
{
	return m_dwCacheSize;
}

auto CDataLoader::GetDataSize()const->std::uint64_t
{
	return static_cast<std::uint64_t>(m_stDataSize.QuadPart);
}

auto CDataLoader::GetFileData()const->std::byte*
{
	return IsVirtual() ? nullptr : static_cast<std::byte*>(m_lpMapBase);
}

auto CDataLoader::GetMemPageSize()const->DWORD
{
	return m_dwPageSize;
}

auto CDataLoader::GetVirtualInterface()->HEXCTRL::IHexVirtData*
{
	return IsVirtual() ? this : nullptr;
}

bool CDataLoader::IsMutable()const
{
	return m_fMutable;
}

bool CDataLoader::IsProcess()const
{
	return m_fProcess;
}

bool CDataLoader::Open(const Ut::FILEOPEN& fos)
{
	assert(m_hHandle == nullptr);
	if (m_hHandle != nullptr) { //Already opened.
		return false;
	}

	if (fos.eMode == Ut::EOpenMode::OPEN_PROC) {
		return OpenProcess(fos);
	}

	return OpenFile(fos);
}


//Private methods.

void CDataLoader::FlushFileData()
{
	if (!IsModified())
		return;

	OVERLAPPED ol { .Offset { LODWORD(m_ullOffsetCurr) }, .OffsetHigh { HIDWORD(m_ullOffsetCurr) } };
	DWORD dwBytesWritten { };
	if (WriteFile(m_hHandle, m_pCache.get(), static_cast<DWORD>(m_ullSizeCurr), &dwBytesWritten, &ol) == FALSE) {
		PrintLastError(L"WriteFile");
	}

	m_fModified = false;
}

bool CDataLoader::IsModified()const
{
	return m_fModified;
}

bool CDataLoader::IsVirtual()const
{
	return m_fVirtual;
}

void CDataLoader::OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)
{
	if (m_fProcess) {
		hdi.spnData = ReadProcData(hdi.stHexSpan.ullOffset, hdi.stHexSpan.ullSize);
	}
	else {
		hdi.spnData = ReadFileData(hdi.stHexSpan.ullOffset, hdi.stHexSpan.ullSize);
	}
}

void CDataLoader::OnHexSetData(const HEXCTRL::HEXDATAINFO& /*hdi*/)
{
	m_fModified = true;
}

bool CDataLoader::OpenFile(const Ut::FILEOPEN & fos)
{
	m_wstrFilePath = fos.wstrFullPath;
	m_wstrFileName = m_wstrFilePath.substr(m_wstrFilePath.find_last_of(L'\\') + 1);

	if (fos.eMode == Ut::EOpenMode::OPEN_DEVICE && m_wstrFilePath.starts_with(L"\\\\")) { //Special path.
		m_fVirtual = true;
	}

	const auto fNewFile = fos.eMode == Ut::EOpenMode::NEW_FILE;
	m_hHandle = CreateFileW(m_wstrFilePath.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		fNewFile ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (fNewFile) { //Setting the size of the new file.
		if (SetFilePointerEx(m_hHandle, { .QuadPart { static_cast<LONGLONG>(fos.ullNewFileSize) } }, nullptr, FILE_BEGIN) == FALSE) {
			PrintLastError(L"SetFilePointerEx");
			return false;
		}
		SetEndOfFile(m_hHandle);
	}

	if (m_hHandle == INVALID_HANDLE_VALUE) {
		if (!fNewFile) { //Trying to open in ReadOnly mode.
			m_hHandle = CreateFileW(m_wstrFilePath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		}

		if (m_hHandle == INVALID_HANDLE_VALUE) {
			PrintLastError(L"CreateFileW");
			return false;
		}
	}
	else {
		m_fMutable = true;
	}

	if (IsVirtual()) {
		return OpenFileVirtual();
	}

	if (GetFileSizeEx(m_hHandle, &m_stDataSize); m_stDataSize.QuadPart == 0) { //Zero size.
		MessageBoxW(nullptr, L"File is zero size.", m_wstrFilePath.data(), MB_ICONERROR);
		return false;
	}

	if (m_hMapObject = CreateFileMappingW(m_hHandle, nullptr, m_fMutable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
		m_hMapObject == nullptr) {
		PrintLastError(L"CreateFileMappingW");
		CloseHandle(m_hHandle);
		return false;
	}

	m_lpMapBase = MapViewOfFile(m_hMapObject, m_fMutable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);

	return true;
}

bool CDataLoader::OpenFileVirtual()
{
	if (m_hHandle == nullptr)
		return false;

	DISK_GEOMETRY stGeometry { };
	DWORD dwBytesRet { };
	if (!DeviceIoControl(m_hHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0, &stGeometry, sizeof(stGeometry), &dwBytesRet, nullptr)) {
		PrintLastError(L"DeviceIoControl(IOCTL_DISK_GET_DRIVE_GEOMETRY)");
		return false;
	}

	m_dwAlignment = stGeometry.BytesPerSector;

	GET_LENGTH_INFORMATION stLengthInfo { };
	switch (stGeometry.MediaType) {
	case MEDIA_TYPE::Unknown:
	case MEDIA_TYPE::RemovableMedia:
	case MEDIA_TYPE::FixedMedia:
		if (!DeviceIoControl(m_hHandle, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &stLengthInfo, sizeof(stLengthInfo), &dwBytesRet, nullptr)) {
			PrintLastError(L"DeviceIoControl(IOCTL_DISK_GET_LENGTH_INFO)");
			return false;
		}
		break;
	default:
		stLengthInfo.Length.QuadPart = stGeometry.Cylinders.QuadPart * stGeometry.TracksPerCylinder *
			stGeometry.SectorsPerTrack * stGeometry.BytesPerSector;
		break;
	}

	m_stDataSize.QuadPart = stLengthInfo.Length.QuadPart;
	m_pCache.reset(static_cast<std::byte*>(_aligned_malloc(GetCacheSize(), m_dwAlignment))); //Initialize the data cache.

	return true;
}

bool CDataLoader::OpenProcess(const Ut::FILEOPEN& fos)
{
	m_wstrFileName = fos.wstrFullPath; //Process name.
	m_hHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, fos.dwProcID);
	if (m_hHandle == nullptr) {
		PrintLastError(L"OpenProcess");
		return false;
	}

	PROCESS_MEMORY_COUNTERS_EX pmc { .cb { sizeof(PROCESS_MEMORY_COUNTERS_EX) } };
	K32GetProcessMemoryInfo(m_hHandle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(PROCESS_MEMORY_COUNTERS_EX));
	m_stDataSize.QuadPart = pmc.PrivateUsage;

	m_dwAlignment = 32;
	m_pCache.reset(static_cast<std::byte*>(_aligned_malloc(GetCacheSize(), m_dwAlignment)));
	m_fVirtual = true;
	m_fProcess = true;
	m_fMutable = false;

	return true;
}

void CDataLoader::PrintLastError(std::wstring_view wsvSource)const
{
	const auto dwError = GetLastError();
	wchar_t buffErr[MAX_PATH];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffErr, MAX_PATH, nullptr);
	Ut::Log::AddLogEntryError(std::format(L"{}: {} failed: 0x{:08X} {}", m_wstrFileName, wsvSource, dwError, buffErr));
}

auto CDataLoader::ReadFileData(std::uint64_t ullOffset, std::uint64_t ullSize)->HEXCTRL::SpanByte
{
	assert(ullOffset + ullSize <= static_cast<std::uint64_t>(m_stDataSize.QuadPart));
	if (ullOffset + ullSize > static_cast<std::uint64_t>(m_stDataSize.QuadPart)) { //Overflow check.
		return { };
	}

	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) { //Data is already in the cache.
		return { m_pCache.get() + (ullOffset - m_ullOffsetCurr), ullSize };
	}

	FlushFileData(); //Flush current cache data if it was modified, before the ReadFile.

	const auto ullOffsetRemainder = ullOffset % m_dwAlignment;
	const auto ullOffsetAligned = ullOffset - ullOffsetRemainder;
	const auto ullSizeAligned = (ullOffsetAligned + GetCacheSize()) <= static_cast<std::uint64_t>(m_stDataSize.QuadPart) ?
		GetCacheSize() : m_stDataSize.QuadPart - ullOffsetAligned; //Size at the end of a file can be not aligned.
	assert(ullSizeAligned >= ullSize);

	OVERLAPPED ol { .Offset { LODWORD(ullOffsetAligned) }, .OffsetHigh { HIDWORD(ullOffsetAligned) } };
	DWORD dwBytesRead { };
	if (ReadFile(m_hHandle, m_pCache.get(), static_cast<DWORD>(ullSizeAligned), &dwBytesRead, &ol) == FALSE) {
		PrintLastError(L"ReadFile");
		return { };
	}

	m_ullOffsetCurr = ullOffsetAligned;
	m_ullSizeCurr = ullSizeAligned;

	return { m_pCache.get() + ullOffsetRemainder, ullSizeAligned - ullOffsetRemainder };
}

auto CDataLoader::ReadProcData(std::uint64_t ullOffset, std::uint64_t ullSize)->HEXCTRL::SpanByte
{
	assert(ullOffset + ullSize <= static_cast<std::uint64_t>(GetDataSize()));
	if (ullOffset + ullSize > static_cast<std::uint64_t>(GetDataSize())) { //Overflow check.
		return { };
	}

	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) { //Data is already in the cache.
		return { m_pCache.get() + (ullOffset - m_ullOffsetCurr), ullSize };
	}

	const std::byte* pMemCurr { nullptr };
	const std::uint64_t u64SizeCached = (ullOffset + GetCacheSize()) <= GetDataSize() ? GetCacheSize()
		: GetDataSize() - ullOffset;
	std::uint64_t u64SizeRemain { u64SizeCached };
	std::uint64_t u64CacheOffset { 0ULL };   //Offset in the cache to write data to.
	std::uint64_t u64RegsTotalSize { 0ULL }; //Total size of the commited pages.
	bool fVestige { false }; //Is there any remainder memory to acquire?

	while (true) {
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQueryEx(m_hHandle, pMemCurr, &mbi, sizeof(mbi)) == 0) {
			break;
		}
		pMemCurr = reinterpret_cast<const std::byte*>(mbi.BaseAddress) + mbi.RegionSize;

		constexpr auto dwFlags = PAGE_NOACCESS | PAGE_GUARD;
		if (mbi.State == MEM_COMMIT && !(mbi.AllocationProtect & dwFlags) && !(mbi.Protect & dwFlags)) {
			std::byte* pAddrToRead { };
			std::uint64_t u64SizeToRead { };

			if (fVestige) {
				const auto u64AvailMemToRead = mbi.RegionSize;
				u64SizeToRead = u64AvailMemToRead >= u64SizeRemain ? u64SizeRemain : u64AvailMemToRead;
				pAddrToRead = reinterpret_cast<std::byte*>(mbi.BaseAddress);
			}
			else if (ullOffset >= u64RegsTotalSize && ullOffset < (u64RegsTotalSize + mbi.RegionSize)) {
				const auto u64OffsetFromRangeBegin = ullOffset - u64RegsTotalSize;
				const auto u64AvailMemToRead = mbi.RegionSize - u64OffsetFromRangeBegin;
				u64SizeToRead = u64AvailMemToRead >= u64SizeRemain ? u64SizeRemain : u64AvailMemToRead;
				pAddrToRead = reinterpret_cast<std::byte*>(mbi.BaseAddress) + u64OffsetFromRangeBegin;
			}

			if (u64SizeToRead > 0) {
				if (ReadProcessMemory(m_hHandle, pAddrToRead, m_pCache.get() + u64CacheOffset, u64SizeToRead,
					nullptr) == FALSE) {
					PrintLastError(L"ReadProcessMemory");
					return { };
				}
			}
			if (u64SizeToRead == u64SizeRemain) { //We got required size.
				break;
			}

			fVestige = true;
			u64SizeRemain -= u64SizeToRead;
			u64CacheOffset += u64SizeToRead;
			u64RegsTotalSize += mbi.RegionSize;
		}
	}

	m_ullOffsetCurr = ullOffset;
	m_ullSizeCurr = u64SizeCached;

	return { m_pCache.get(), m_ullSizeCurr };
}