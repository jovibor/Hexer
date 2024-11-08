module;
/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "HexCtrl.h"
#include <cassert>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <numeric>
#include <winioctl.h>
export module DataLoader;

import Utility;

export class CDataLoader final : public HEXCTRL::IHexVirtData {
public:
	CDataLoader();
	~CDataLoader();
	void Close();
	[[nodiscard]] auto GetCacheSize()const->DWORD; //Cache size that is reported to the outside, for IHexCtrl.
	[[nodiscard]] auto GetDataSize()const->std::uint64_t;
	[[nodiscard]] auto GetFileMapData()const->std::byte*;
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetMaxVirtOffset()const->std::uint64_t;
	[[nodiscard]] auto GetMemPageSize()const->DWORD;
	[[nodiscard]] auto GetProcID()const->DWORD;
	[[nodiscard]] auto GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&;
	[[nodiscard]] auto GetVirtualInterface() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsMutable()const;
	[[nodiscard]] bool IsProcess()const;
	[[nodiscard]] bool Open(const Ut::DATAOPEN& dos);
private:
	[[nodiscard]] auto GetInternalCacheSize()const->DWORD; //The real cache size used internally.
	[[nodiscard]] bool IsModified()const;
	[[nodiscard]] bool IsVirtual()const;
	void LogLastError(std::wstring_view wsvSource, DWORD dwErr = 0)const;
	void OnHexGetOffset(HEXCTRL::HEXDATAINFO& hdi, bool fGetVirt)override;
	void OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)override;
	void OnHexSetData(const HEXCTRL::HEXDATAINFO& hdi)override;
	[[nodiscard]] auto OpenFileVirtual() -> std::expected<void, DWORD>;
	[[nodiscard]] auto OpenFile(const Ut::DATAOPEN& dos) -> std::expected<void, DWORD>;
	[[nodiscard]] auto OpenProcess(const Ut::DATAOPEN& dos) -> std::expected<void, DWORD>;
	[[nodiscard]] auto ReadFileData(std::uint64_t ullOffset, std::uint64_t ullSize) -> HEXCTRL::SpanByte;
	[[nodiscard]] auto ReadProcData(std::uint64_t ullOffset, std::uint64_t ullSize) -> HEXCTRL::SpanByte;
	void WriteFileData();
	void WriteProcData(const HEXCTRL::HEXDATAINFO& hdi);
private:
	static constexpr auto m_dwCacheSize { 1024UL * 1024UL }; //1MB cache size.
	std::unique_ptr < std::byte[], decltype([](auto p) { _aligned_free(p); }) > m_pCache { };
	std::wstring m_wstrDataPath; //Data path to open.
	std::wstring m_wstrFileName; //File name without path, or Process name.
	std::vector<MEMORY_BASIC_INFORMATION> m_vecProcMemory; //Process memory regions info.
	HANDLE m_hHandle { };        //Handle of a file or process.
	HANDLE m_hMapObject { };     //Returned by CreateFileMappingW.
	LPVOID m_lpMapBase { };      //Returned by MapViewOfFile.
	LARGE_INTEGER m_stDataSize { };
	std::uint64_t m_ullOffsetCurr { };    //Offset of the data that is currently in the cache.
	std::uint64_t m_ullSizeCurr { };      //Size of the data that is currently in the cache.
	std::uint64_t m_ullMaxVirtOffset { }; //Maximum virtual address of a process.
	DWORD m_dwPageSize { };     //System Virtual page size.
	DWORD m_dwAlignment { };    //An alignment that the offset and the size must be aligned on, for the ReadFile.
	DWORD m_dwProcID { };       //Process ID.
	bool m_fMutable { false };  //Is data opened as RW or RO?
	bool m_fVirtual { false };  //Is data opened in HexCtrl Virtual mode.
	bool m_fProcess { false };  //Is it process or file?
	bool m_fModified { false }; //File was modified.
	bool m_fCacheZeroed { false }; //If cache set with zeros or not.
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
		if (IsVirtual() && !IsProcess()) {
			WriteFileData();
		}

		FlushViewOfFile(m_lpMapBase, 0);
		UnmapViewOfFile(m_lpMapBase);
		CloseHandle(m_hMapObject);
		CloseHandle(m_hHandle);
	}

	m_pCache.reset();
	m_wstrDataPath.clear();
	m_vecProcMemory.clear();
	m_hHandle = nullptr;
	m_hMapObject = nullptr;
	m_lpMapBase = nullptr;
	m_stDataSize = { };
	m_ullOffsetCurr = 0;
	m_ullSizeCurr = 0;
	m_dwAlignment = 0;
	m_ullMaxVirtOffset = 0;
	m_fMutable = false;
	m_fVirtual = false;
	m_fProcess = false;
	m_fModified = false;
}

auto CDataLoader::GetCacheSize()const->DWORD
{
	return m_dwCacheSize / 2;
}

auto CDataLoader::GetDataSize()const->std::uint64_t
{
	return static_cast<std::uint64_t>(m_stDataSize.QuadPart);
}

auto CDataLoader::GetFileMapData()const->std::byte*
{
	return IsVirtual() ? nullptr : static_cast<std::byte*>(m_lpMapBase);
}

auto CDataLoader::GetFileName()const->const std::wstring&
{
	return m_wstrFileName;
}

auto CDataLoader::GetMaxVirtOffset()const->std::uint64_t
{
	return m_ullMaxVirtOffset;
}

auto CDataLoader::GetMemPageSize()const->DWORD
{
	return m_dwPageSize;
}

auto CDataLoader::GetProcID()const->DWORD
{
	return m_dwProcID;
}

auto CDataLoader::GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&
{
	return m_vecProcMemory;
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

bool CDataLoader::Open(const Ut::DATAOPEN& dos)
{
	assert(m_hHandle == nullptr);
	if (m_hHandle != nullptr) { //Already opened.
		return false;
	}

	using enum Ut::EOpenMode;
	std::wstring wstrLog;
	std::expected<void, DWORD> expOpen;
	switch (dos.eMode) {
	case OPEN_PROC:
		expOpen = OpenProcess(dos);
		if (expOpen) {
			wstrLog = std::format(L"Process opened: {} (ID: {}) ({})", GetFileName(), GetProcID(), Ut::GetRWWstr(IsMutable()));
		}
		else {
			wstrLog = std::format(L"Process open failed: {} (ID: {}). \r\n{}", GetFileName(), GetProcID(),
				Ut::GetLastErrorWstr(expOpen.error()));
			MessageBoxW(AfxGetMainWnd()->m_hWnd, wstrLog.data(), L"Opening error", MB_ICONERROR);
		}
		break;
	default:
		expOpen = OpenFile(dos);
		if (expOpen) {
			wstrLog = std::format(L"{} opened: {} ({})", Ut::GetNameFromEOpenMode(dos.eMode), GetFileName(),
				Ut::GetRWWstr(IsMutable()));
		}
		else {
			wstrLog = std::format(L"{} open failed: {} \r\n{}", Ut::GetNameFromEOpenMode(dos.eMode), GetFileName(),
				Ut::GetLastErrorWstr(expOpen.error()));
			MessageBoxW(AfxGetMainWnd()->m_hWnd, wstrLog.data(), L"Opening error", MB_ICONERROR);
		}
		break;
	}

	expOpen ? Ut::Log::AddLogEntryInfo(wstrLog) : Ut::Log::AddLogEntryError(wstrLog);

	return expOpen.has_value();
}


//Private methods.

auto CDataLoader::GetInternalCacheSize()const->DWORD
{
	//Internal working cache size is two times bigger than the size reported to the IHexCtrl.
	//This will ensure that the size returned after OnHexGetData will be not less than the size requested.
	//Even after offset and size aligning, either during File or Process data reading.
	return m_dwCacheSize;
}

bool CDataLoader::IsModified()const
{
	return m_fModified;
}

bool CDataLoader::IsVirtual()const
{
	return m_fVirtual;
}

void CDataLoader::LogLastError(std::wstring_view wsvSource, DWORD dwErr)const
{
	Ut::Log::AddLogEntryError(std::format(L"{}: {} failed: 0x{:08X} {}",
		GetFileName(), wsvSource, dwErr > 0 ? dwErr : GetLastError(), Ut::GetLastErrorWstr(dwErr)));
}

void CDataLoader::OnHexGetOffset(HEXCTRL::HEXDATAINFO& hdi, bool fGetVirt)
{
	if (!IsProcess()) //Virtual address space only for processes.
		return;

	const auto ullOffset = hdi.stHexSpan.ullOffset;
	std::uint64_t u64RegsTotalSize { }; //Regions total size.
	if (fGetVirt) {
		const auto it = std::find_if(m_vecProcMemory.begin(), m_vecProcMemory.end(),
		[ullOffset, &u64RegsTotalSize](const MEMORY_BASIC_INFORMATION& ref) mutable {
			u64RegsTotalSize += ref.RegionSize;
			return ullOffset < u64RegsTotalSize; });

		if (it != m_vecProcMemory.end()) {
			const auto ullOffsetFromRegionBegin = ullOffset - (u64RegsTotalSize - it->RegionSize);
			hdi.stHexSpan.ullOffset = reinterpret_cast<ULONGLONG>(it->BaseAddress) + ullOffsetFromRegionBegin;
		}
	}
	else {
		const auto it = std::find_if(m_vecProcMemory.begin(), m_vecProcMemory.end(),
			[ullOffset, &u64RegsTotalSize](const MEMORY_BASIC_INFORMATION& ref) mutable {
				if (ullOffset >= reinterpret_cast<ULONGLONG>(ref.BaseAddress)
					&& ullOffset < (reinterpret_cast<ULONGLONG>(ref.BaseAddress) + ref.RegionSize)) {
					return true;
				}

				u64RegsTotalSize += ref.RegionSize;
				return false; });

		if (it != m_vecProcMemory.end()) {
			hdi.stHexSpan.ullOffset = u64RegsTotalSize + (ullOffset - reinterpret_cast<ULONGLONG>(it->BaseAddress));
		}
	}
}

void CDataLoader::OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)
{
	if (IsProcess()) {
		hdi.spnData = ReadProcData(hdi.stHexSpan.ullOffset, hdi.stHexSpan.ullSize);
	}
	else {
		hdi.spnData = ReadFileData(hdi.stHexSpan.ullOffset, hdi.stHexSpan.ullSize);
	}
}

void CDataLoader::OnHexSetData(const HEXCTRL::HEXDATAINFO& hdi)
{
	m_fModified = true;

	if (IsProcess()) {
		WriteProcData(hdi); //Writing to the process address space immediately.
	}
}

auto CDataLoader::OpenFile(const Ut::DATAOPEN& dos)->std::expected<void, DWORD>
{
	m_wstrDataPath = dos.wstrDataPath;
	m_wstrFileName = m_wstrDataPath.substr(m_wstrDataPath.find_last_of(L'\\') + 1); //File name.

	if (dos.eMode == Ut::EOpenMode::OPEN_DEVICE || m_wstrDataPath.starts_with(L"\\\\")) { //Special path.
		m_fVirtual = true;
	}

	const auto fNewFile = dos.eMode == Ut::EOpenMode::NEW_FILE;
	m_hHandle = CreateFileW(m_wstrDataPath.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		fNewFile ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (fNewFile) { //Setting the size of the new file.
		if (SetFilePointerEx(m_hHandle, { .QuadPart { static_cast<LONGLONG>(dos.ullNewFileSize) } }, nullptr, FILE_BEGIN) == FALSE) {
			const auto err = GetLastError();
			LogLastError(L"SetFilePointerEx", err);
			return std::unexpected(err);
		}
		SetEndOfFile(m_hHandle);
	}

	if (m_hHandle == INVALID_HANDLE_VALUE) {
		if (!fNewFile) { //Trying to open in ReadOnly mode.
			m_hHandle = CreateFileW(m_wstrDataPath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		}

		if (m_hHandle == INVALID_HANDLE_VALUE) {
			const auto err = GetLastError();
			LogLastError(L"CreateFileW", err);
			return std::unexpected(err);
		}
	}
	else {
		m_fMutable = true;
	}

	if (IsVirtual()) {
		return OpenFileVirtual();
	}

	if (GetFileSizeEx(m_hHandle, &m_stDataSize); m_stDataSize.QuadPart == 0) { //Zero size.
		MessageBoxW(nullptr, L"File is zero size.", m_wstrDataPath.data(), MB_ICONERROR);
		return std::unexpected(0);
	}

	if (m_hMapObject = CreateFileMappingW(m_hHandle, nullptr, m_fMutable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
		m_hMapObject == nullptr) {
		const auto err = GetLastError();
		LogLastError(L"CreateFileMappingW", err);
		CloseHandle(m_hHandle);
		return std::unexpected(err);
	}

	m_lpMapBase = MapViewOfFile(m_hMapObject, m_fMutable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);

	return { };
}

auto CDataLoader::OpenFileVirtual()->std::expected<void, DWORD>
{
	if (m_hHandle == nullptr)
		return std::unexpected(0);

	DISK_GEOMETRY stGeometry { };
	DWORD dwBytesRet { };
	if (!DeviceIoControl(m_hHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0, &stGeometry, sizeof(stGeometry), &dwBytesRet, nullptr)) {
		const auto err = GetLastError();
		LogLastError(L"DeviceIoControl(IOCTL_DISK_GET_DRIVE_GEOMETRY)", err);
		return std::unexpected(err);
	}

	m_dwAlignment = stGeometry.BytesPerSector;

	GET_LENGTH_INFORMATION stLengthInfo { };
	switch (stGeometry.MediaType) {
	case MEDIA_TYPE::Unknown:
	case MEDIA_TYPE::RemovableMedia:
	case MEDIA_TYPE::FixedMedia:
		if (!DeviceIoControl(m_hHandle, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &stLengthInfo, sizeof(stLengthInfo), &dwBytesRet, nullptr)) {
			const auto err = GetLastError();
			LogLastError(L"DeviceIoControl(IOCTL_DISK_GET_LENGTH_INFO)", err);
			return std::unexpected(err);
		}
		break;
	default:
		stLengthInfo.Length.QuadPart = stGeometry.Cylinders.QuadPart * stGeometry.TracksPerCylinder *
			stGeometry.SectorsPerTrack * stGeometry.BytesPerSector;
		break;
	}

	m_stDataSize.QuadPart = stLengthInfo.Length.QuadPart;
	m_pCache.reset(static_cast<std::byte*>(_aligned_malloc(GetInternalCacheSize(), m_dwAlignment))); //Initialize the data cache.

	return { };
}

auto CDataLoader::OpenProcess(const Ut::DATAOPEN& dos)->std::expected<void, DWORD>
{
	m_fProcess = true;
	m_wstrFileName = dos.wstrDataPath; //Process name.
	m_dwProcID = dos.dwProcID;
	if (m_hHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcID()); m_hHandle == nullptr) {
		return std::unexpected(GetLastError());
	}

	m_vecProcMemory.clear();
	m_vecProcMemory.reserve(16);
	const std::byte* pMemCurr { nullptr };
	while (true) {
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQueryEx(m_hHandle, pMemCurr, &mbi, sizeof(mbi)) == 0) {
			break;
		}
		pMemCurr = reinterpret_cast<const std::byte*>(mbi.BaseAddress) + mbi.RegionSize;

		if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
			m_ullMaxVirtOffset = reinterpret_cast<std::uint64_t>(pMemCurr) - 1;
			m_vecProcMemory.emplace_back(mbi);
		}
	}

	m_stDataSize.QuadPart = std::reduce(m_vecProcMemory.begin(), m_vecProcMemory.end(), 0ULL,
		[](ULONGLONG ullSumm, const MEMORY_BASIC_INFORMATION& ref) { return ullSumm + ref.RegionSize; });
	m_dwAlignment = 64;
	m_pCache.reset(static_cast<std::byte*>(_aligned_malloc(GetInternalCacheSize(), m_dwAlignment)));
	m_fVirtual = true;
	m_fMutable = true;

	return { };
}

auto CDataLoader::ReadFileData(std::uint64_t ullOffset, std::uint64_t ullSize)->HEXCTRL::SpanByte
{
	assert(ullOffset + ullSize <= GetDataSize());
	assert(ullSize <= GetCacheSize());
	if (ullOffset + ullSize > GetDataSize()) { //Overflow check.
		return { };
	}

	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) { //Data is already in the cache.
		return { m_pCache.get() + (ullOffset - m_ullOffsetCurr), ullSize };
	}

	WriteFileData(); //Flush current cache data if it was modified, before the ReadFile.

	const auto ullOffsetRemainder = ullOffset % m_dwAlignment;
	const auto ullOffsetAligned = ullOffset - ullOffsetRemainder;
	const auto ullSizeAligned = (ullOffsetAligned + GetInternalCacheSize()) <= GetDataSize() ?
		GetInternalCacheSize() : GetDataSize() - ullOffsetAligned; //Size at the end of a file might be non aligned.
	assert(ullSizeAligned >= ullSize);
	assert((ullSizeAligned - ullOffsetRemainder) >= ullSize);

	if (SetFilePointerEx(m_hHandle, { .QuadPart { static_cast<LONGLONG>(ullOffsetAligned) } }, nullptr, FILE_BEGIN) == FALSE) {
		LogLastError(L"SetFilePointerEx");
		assert(false);
		return { };
	}

	if (ReadFile(m_hHandle, m_pCache.get(), static_cast<DWORD>(ullSizeAligned), nullptr, nullptr) == FALSE) {
		LogLastError(L"ReadFile");
		assert(false);
		return { };
	}

	m_ullOffsetCurr = ullOffsetAligned;
	m_ullSizeCurr = ullSizeAligned;

	return { m_pCache.get() + ullOffsetRemainder, ullSizeAligned - ullOffsetRemainder };
}

auto CDataLoader::ReadProcData(std::uint64_t ullOffset, std::uint64_t ullSize)->HEXCTRL::SpanByte
{
	assert(ullOffset + ullSize <= GetDataSize());
	assert(ullSize <= GetCacheSize());
	if (ullOffset + ullSize > GetDataSize()) { //Overflow check.
		return { };
	}

	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) { //Data is already in the cache.
		return { m_pCache.get() + (ullOffset - m_ullOffsetCurr), ullSize };
	}

	const auto ullOffsetRemainder = ullOffset % GetMemPageSize();
	const auto ullOffsetAligned = ullOffset - ullOffsetRemainder;
	const auto ullSizeAligned = (ullOffsetAligned + GetInternalCacheSize()) <= GetDataSize() ? GetInternalCacheSize()
		: GetDataSize() - ullOffsetAligned;
	assert(ullSizeAligned >= ullSize);
	assert((ullSizeAligned - ullOffsetRemainder) >= ullSize);

	std::uint64_t u64SizeRemain { ullSizeAligned };
	std::uint64_t u64CacheOffset { 0ULL };   //Offset in the cache to write data to.
	std::uint64_t u64RegsTotalSize { 0ULL }; //Total size of the commited regions of pages.
	bool fVestige { false }; //Is there any remainder memory to acquire?

	for (const auto& mbi : m_vecProcMemory) {
		u64RegsTotalSize += mbi.RegionSize;
		std::byte* pAddrToRead { };
		std::uint64_t u64SizeToRead { };

		if (fVestige) {
			const auto u64AvailSizeInRegion = mbi.RegionSize;
			u64SizeToRead = u64AvailSizeInRegion >= u64SizeRemain ? u64SizeRemain : u64AvailSizeInRegion;
			pAddrToRead = reinterpret_cast<std::byte*>(mbi.BaseAddress);
		}
		else if (ullOffsetAligned < u64RegsTotalSize) {
			const auto u64OffsetFromRegionBegin = ullOffsetAligned - (u64RegsTotalSize - mbi.RegionSize);
			const auto u64AvailSizeInRegion = mbi.RegionSize - u64OffsetFromRegionBegin;
			u64SizeToRead = u64AvailSizeInRegion >= u64SizeRemain ? u64SizeRemain : u64AvailSizeInRegion;
			pAddrToRead = reinterpret_cast<std::byte*>(mbi.BaseAddress) + u64OffsetFromRegionBegin;
		}

		if (u64SizeToRead > 0) {
			if (ReadProcessMemory(m_hHandle, pAddrToRead, m_pCache.get() + u64CacheOffset, u64SizeToRead, nullptr) == FALSE) {
				LogLastError(L"ReadProcessMemory");
				m_ullOffsetCurr = 0;
				m_ullSizeCurr = 0;
				if (!m_fCacheZeroed) {
					std::memset(m_pCache.get(), 0, GetInternalCacheSize());
					m_fCacheZeroed = true; //To avoid repetitive memset calls
				}
				return { m_pCache.get(), ullSize };
			}
			m_fCacheZeroed = false;

			if (u64SizeToRead == u64SizeRemain) { //We got required size.
				break;
			}

			fVestige = true;
			u64SizeRemain -= u64SizeToRead;
			u64CacheOffset += u64SizeToRead;
		}
	}

	m_ullOffsetCurr = ullOffsetAligned;
	m_ullSizeCurr = ullSizeAligned;

	return { m_pCache.get() + ullOffsetRemainder, ullSizeAligned - ullOffsetRemainder };
}

void CDataLoader::WriteFileData()
{
	if (!IsModified())
		return;

	if (SetFilePointerEx(m_hHandle, { .QuadPart { static_cast<LONGLONG>(m_ullOffsetCurr) } }, nullptr, FILE_BEGIN) == FALSE) {
		LogLastError(L"SetFilePointerEx");
		assert(false);
		return;
	}

	if (WriteFile(m_hHandle, m_pCache.get(), static_cast<DWORD>(m_ullSizeCurr), nullptr, nullptr) == FALSE) {
		LogLastError(L"WriteFile");
	}

	m_fModified = false;
}

void CDataLoader::WriteProcData(const HEXCTRL::HEXDATAINFO& hdi)
{
	const std::byte* pMemCurr { nullptr };
	const auto pData = hdi.spnData.data();
	const auto ullOffset = hdi.stHexSpan.ullOffset;
	std::uint64_t u64SizeRemain { hdi.stHexSpan.ullSize };
	assert(ullOffset >= m_ullOffsetCurr);
	assert(u64SizeRemain <= m_ullSizeCurr);

	std::uint64_t u64CacheOffset { 0ULL };   //Offset in the pData to get data from.
	std::uint64_t u64RegsTotalSize { 0ULL }; //Total size of the commited regions of pages.
	bool fVestige { false }; //Is there any remainder data to write?

	while (true) {
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQueryEx(m_hHandle, pMemCurr, &mbi, sizeof(mbi)) == 0) {
			break;
		}
		pMemCurr = reinterpret_cast<const std::byte*>(mbi.BaseAddress) + mbi.RegionSize;

		constexpr auto dwFlags = PAGE_NOACCESS | PAGE_GUARD;
		if (mbi.State == MEM_COMMIT && !(mbi.AllocationProtect & dwFlags) && !(mbi.Protect & dwFlags)) {
			u64RegsTotalSize += mbi.RegionSize;
			std::byte* pAddrToWrite { };
			std::uint64_t u64SizeToWrite { };

			if (fVestige) {
				const auto u64AvailSizeInRegion = mbi.RegionSize;
				u64SizeToWrite = u64AvailSizeInRegion >= u64SizeRemain ? u64SizeRemain : u64AvailSizeInRegion;
				pAddrToWrite = reinterpret_cast<std::byte*>(mbi.BaseAddress);
			}
			else if (ullOffset < u64RegsTotalSize) {
				const auto u64OffsetFromRegionBegin = ullOffset - (u64RegsTotalSize - mbi.RegionSize);
				const auto u64AvailSizeInRegion = mbi.RegionSize - u64OffsetFromRegionBegin;
				u64SizeToWrite = u64AvailSizeInRegion >= u64SizeRemain ? u64SizeRemain : u64AvailSizeInRegion;
				pAddrToWrite = reinterpret_cast<std::byte*>(mbi.BaseAddress) + u64OffsetFromRegionBegin;
			}

			if (u64SizeToWrite > 0) {
				if (WriteProcessMemory(m_hHandle, pAddrToWrite, pData + u64CacheOffset, u64SizeToWrite, nullptr) == FALSE) {
					LogLastError(L"WriteProcessMemory");
					return;
				}

				if (u64SizeToWrite == u64SizeRemain) { //We have written required size.
					m_fModified = false;
					break;
				}

				fVestige = true;
				u64SizeRemain -= u64SizeToWrite;
				u64CacheOffset += u64SizeToWrite;
			}
		}
	}
}