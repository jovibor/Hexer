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
	void ChangeDataAccessMode(Ut::EDataAccessMode eDataAccessMode);
	void ChangeDataIOMode(Ut::EDataIOMode eDataIOMode);
	void Close();
	[[nodiscard]] auto GetCacheSize()const->DWORD; //Cache size that is reported to the outside, for IHexCtrl.
	[[nodiscard]] auto GetDataSize()const->std::uint64_t;
	[[nodiscard]] auto GetDataAccessMode()const->Ut::EDataAccessMode;
	[[nodiscard]] auto GetDataIOMode()const->Ut::EDataIOMode;
	[[nodiscard]] auto GetFileMMAPData()const->std::byte*;
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetMaxVirtOffset()const->std::uint64_t;
	[[nodiscard]] auto GetMemPageSize()const->DWORD;
	[[nodiscard]] auto GetOpenMode()const->Ut::EOpenMode;
	[[nodiscard]] auto GetProcID()const->DWORD;
	[[nodiscard]] auto GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&;
	[[nodiscard]] auto GetIHexVirtData() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsDataWritable()const;
	[[nodiscard]] bool IsDevice()const;
	[[nodiscard]] bool IsFile()const;
	[[nodiscard]] bool IsProcess()const;
	[[nodiscard]] bool Open(const Ut::DATAOPEN& dos, Ut::EDataAccessMode eDataAccessMode, Ut::EDataIOMode eDataIOMode);
private:
	void FlushCache();
	[[nodiscard]] auto GetDeviceAlign()const->DWORD;
	[[nodiscard]] auto GetInternalCacheSize()const->DWORD; //The real cache size used internally.
	void InitIOMMAP(bool fInit);
	void InitInternalCache(DWORD dwAlign = 32U);
	[[nodiscard]] bool IsDataIOImmediate()const;
	[[nodiscard]] bool IsDataIOMMAP()const;
	[[nodiscard]] bool IsModified()const;
	void LogLastError(std::wstring_view wsvSource, DWORD dwErr = 0)const;
	[[nodiscard]] auto NewFile() -> std::expected<void, DWORD>;
	void OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)override;
	void OnHexGetOffset(HEXCTRL::HEXDATAINFO& hdi, bool fGetVirt)override;
	void OnHexSetData(const HEXCTRL::HEXDATAINFO& hdi)override;
	[[nodiscard]] auto OpenDevice() -> std::expected<void, DWORD>;
	[[nodiscard]] auto OpenFile() -> std::expected<void, DWORD>;
	[[nodiscard]] auto OpenProcess() -> std::expected<void, DWORD>;
	[[nodiscard]] auto ReadFileData(const HEXCTRL::HEXDATAINFO& hdi) -> HEXCTRL::SpanByte;
	[[nodiscard]] auto ReadProcData(const HEXCTRL::HEXDATAINFO& hdi) -> HEXCTRL::SpanByte;
	void WriteDeviceData(const HEXCTRL::HEXDATAINFO& hdi);
	void WriteFileData(const HEXCTRL::HEXDATAINFO& hdi);
	void WriteProcData(const HEXCTRL::HEXDATAINFO& hdi);
private:
	std::unique_ptr < std::byte[], decltype([](auto p) { _aligned_free(p); }) > m_pCache { };
	Ut::DATAOPEN m_dos;            //Copy struct from the Open() method.
	std::wstring m_wstrFileName;   //File name without path, or Process name.
	std::vector<MEMORY_BASIC_INFORMATION> m_vecProcMemory; //Process memory regions info.
	HANDLE m_hHandle { };          //Handle of a file or process.
	HANDLE m_hMapObject { };       //Returned by CreateFileMappingW.
	LPVOID m_lpMapBase { };        //Returned by MapViewOfFile.
	LARGE_INTEGER m_stDataSize { };
	std::uint64_t m_ullOffsetCurr { };    //Offset of the data that is currently in the cache.
	std::uint64_t m_ullSizeCurr { };      //Size of the data that is currently in the cache.
	std::uint64_t m_ullMaxVirtOffset { }; //Maximum virtual address of a process.
	DWORD m_dwPageSize { };        //System Virtual page size.
	DWORD m_dwDeviceAlign { 1 };   //Alignment that the offset and the size must be aligned on, for the ReadFile on Device.
	Ut::EDataAccessMode m_eDataAccessMode { }; //Current data access mode.
	Ut::EDataIOMode m_eDataIOMode { };         //Current data IO mode.
	bool m_fMutable { false };     //Is data opened as RW or RO?
	bool m_fModified { false };    //Data was modified.
	bool m_fCacheZeroed { false }; //If cache is set with zeros or not.
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

void CDataLoader::ChangeDataAccessMode(Ut::EDataAccessMode eDataAccessMode)
{
	if (eDataAccessMode == GetDataAccessMode())
		return;

	m_eDataAccessMode = eDataAccessMode;
}

void CDataLoader::ChangeDataIOMode(Ut::EDataIOMode eDataIOMode)
{
	//Processes and Devices only work in DATA_IOIMMEDIATE mode, so changing mode 
	//for them is pointless. Also, if setting the currently used mode we do nothing.
	if (!IsFile() || eDataIOMode == GetDataIOMode())
		return;

	using enum Ut::EDataIOMode;
	m_eDataIOMode = eDataIOMode;
	switch (eDataIOMode) {
	case DATA_MMAP:
		InitIOMMAP(true);
		break;
	case DATA_IOBUFF:
	case DATA_IOIMMEDIATE:
		InitIOMMAP(false);
		InitInternalCache();
		break;
	default:
		break;
	}
}

void CDataLoader::Close()
{
	if (IsFile()) {
		if (IsDataIOMMAP()) {
			InitIOMMAP(false);
		}
		else {
			FlushCache();
		}
	}

	CloseHandle(m_hHandle);
	m_hHandle = nullptr;
	m_hMapObject = nullptr;
	m_lpMapBase = nullptr;
	m_pCache.reset();
	m_vecProcMemory.clear();
	m_dos = { };
	m_stDataSize = { };
	m_ullOffsetCurr = 0;
	m_ullSizeCurr = 0;
	m_dwDeviceAlign = 1;
	m_ullMaxVirtOffset = 0;
	m_fMutable = false;
	m_fModified = false;
	m_fCacheZeroed = false;
}

auto CDataLoader::GetCacheSize()const->DWORD
{
	return GetInternalCacheSize() / 2;
}

auto CDataLoader::GetDataSize()const->std::uint64_t
{
	return static_cast<std::uint64_t>(m_stDataSize.QuadPart);
}

auto CDataLoader::GetDataAccessMode()const->Ut::EDataAccessMode
{
	return m_eDataAccessMode;
}

auto CDataLoader::GetDataIOMode()const->Ut::EDataIOMode
{
	return m_eDataIOMode;
}

auto CDataLoader::GetFileMMAPData()const->std::byte*
{
	return static_cast<std::byte*>(m_lpMapBase);
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

auto CDataLoader::GetOpenMode()const->Ut::EOpenMode
{
	return m_dos.eOpenMode;
}

auto CDataLoader::GetProcID()const->DWORD
{
	return m_dos.dwProcID;
}

auto CDataLoader::GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&
{
	return m_vecProcMemory;
}

auto CDataLoader::GetIHexVirtData()->HEXCTRL::IHexVirtData*
{
	return (IsFile() && IsDataIOMMAP()) ? nullptr : this;
}

bool CDataLoader::IsDevice()const
{
	return m_dos.eOpenMode == Ut::EOpenMode::OPEN_DEVICE;
}

bool CDataLoader::IsFile()const
{
	using enum Ut::EOpenMode;
	return m_dos.eOpenMode == OPEN_FILE || m_dos.eOpenMode == NEW_FILE;
}

bool CDataLoader::IsDataWritable()const
{
	return m_fMutable;
}

bool CDataLoader::IsProcess()const
{
	return m_dos.eOpenMode == Ut::EOpenMode::OPEN_PROC;
}

bool CDataLoader::Open(const Ut::DATAOPEN& dos, Ut::EDataAccessMode eDataAccessMode, Ut::EDataIOMode eDataIOMode)
{
	assert(m_hHandle == nullptr);
	if (m_hHandle != nullptr) { //Already opened.
		return false;
	}

	m_dos = dos;
	m_eDataAccessMode = eDataAccessMode;
	m_eDataIOMode = eDataIOMode;

	using enum Ut::EOpenMode;
	std::expected<void, DWORD> expOpen;
	switch (dos.eOpenMode) {
	case OPEN_FILE:
		expOpen = OpenFile();
		break;
	case OPEN_DEVICE:
		expOpen = OpenDevice();
		break;
	case OPEN_PROC:
		expOpen = OpenProcess();
		break;
	case NEW_FILE:
		expOpen = NewFile();
		break;
	default:
		break;
	}

	if (!expOpen) {
		const auto wstrLog = std::format(L"{} open failed: {} \r\n{}", Ut::GetWstrEOpenMode(dos.eOpenMode), GetFileName(),
			Ut::GetLastErrorWstr(expOpen.error()));
		MessageBoxW(AfxGetMainWnd()->m_hWnd, wstrLog.data(), L"Opening error", MB_ICONERROR);
		Ut::Log::AddLogEntryError(wstrLog);
		return false;
	}

	const auto wstrLog = std::format(L"{} opened: {} ({})", Ut::GetWstrEOpenMode(dos.eOpenMode), GetFileName(),
		Ut::GetWstrEDataAccessMode(Ut::EDataAccessMode::DA_RO));
	Ut::Log::AddLogEntryInfo(wstrLog);

	return true;
}


//Private methods.

void CDataLoader::FlushCache()
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

auto CDataLoader::GetDeviceAlign()const->DWORD
{
	return m_dwDeviceAlign;
}

auto CDataLoader::GetInternalCacheSize()const->DWORD
{
	//Internal working cache size is two times bigger than the size reported to the IHexCtrl.
	//This will ensure that the data size returned after the OnHexGetData call will not be smaller than
	//the size requested, even after offset and size aligning, either during File or Process data reading.
	return 1024UL * 1024UL; //1MB cache size.
}

void CDataLoader::InitIOMMAP(bool fInit)
{
	if (fInit) {
		if (m_hMapObject != nullptr || m_lpMapBase != nullptr) {
			Ut::Log::AddLogEntryError(L"File already mapped.");
			return;
		}

		if (m_hMapObject = CreateFileMappingW(m_hHandle, nullptr, IsDataWritable() ? PAGE_READWRITE : PAGE_READONLY,
			0, 0, nullptr);	m_hMapObject == nullptr) {
			const auto err = GetLastError();
			LogLastError(L"CreateFileMappingW", err);
			CloseHandle(m_hHandle);
			return;
		}

		if (m_lpMapBase = MapViewOfFile(m_hMapObject, IsDataWritable() ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);
			m_lpMapBase == nullptr) {
			const auto err = GetLastError();
			LogLastError(L"MapViewOfFile", err);
			CloseHandle(m_hMapObject);
			CloseHandle(m_hHandle);
			return;
		}

		m_pCache.reset();
	}
	else {
		FlushViewOfFile(m_lpMapBase, 0);
		UnmapViewOfFile(m_lpMapBase);
		CloseHandle(m_hMapObject);
		m_lpMapBase = nullptr;
		m_hMapObject = nullptr;
	}
}

void CDataLoader::InitInternalCache(DWORD dwAlign)
{
	m_pCache.reset(static_cast<std::byte*>(_aligned_malloc(GetInternalCacheSize(), dwAlign)));
	m_ullOffsetCurr = 0;
	m_ullSizeCurr = 0;
}

bool CDataLoader::IsDataIOImmediate()const
{
	//If true then any data change will be written back to the file immediately.
	//Otherwise, the whole cache will be written, but only when it needs to be refilled.
	return GetDataIOMode() == Ut::EDataIOMode::DATA_IOIMMEDIATE;
}

bool CDataLoader::IsDataIOMMAP()const
{
	return GetDataIOMode() == Ut::EDataIOMode::DATA_MMAP;
}

bool CDataLoader::IsModified()const
{
	return m_fModified;
}

void CDataLoader::LogLastError(std::wstring_view wsvSource, DWORD dwErr)const
{
	Ut::Log::AddLogEntryError(std::format(L"{}: {} failed: 0x{:08X} {}",
		GetFileName(), wsvSource, dwErr > 0 ? dwErr : GetLastError(), Ut::GetLastErrorWstr(dwErr)));
}

auto CDataLoader::NewFile()->std::expected<void, DWORD>
{
	assert(m_dos.eOpenMode == Ut::EOpenMode::NEW_FILE);
	assert(m_dos.ullSizeNewFile > 0);
	if (m_dos.ullSizeNewFile == 0) {
		return std::unexpected(0);
	}

	m_wstrFileName = m_dos.wstrDataPath.substr(m_dos.wstrDataPath.find_last_of(L'\\') + 1); //File name.

	if (m_hHandle = CreateFileW(m_dos.wstrDataPath.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr); m_hHandle == INVALID_HANDLE_VALUE) {
		return std::unexpected(GetLastError());
	}

	if (SetFilePointerEx(m_hHandle, { .QuadPart { static_cast<LONGLONG>(m_dos.ullSizeNewFile) } }, nullptr, FILE_BEGIN) == FALSE) {
		const auto err = GetLastError();
		LogLastError(L"SetFilePointerEx", err);
		return std::unexpected(err);
	}

	SetEndOfFile(m_hHandle);

	if (GetFileSizeEx(m_hHandle, &m_stDataSize); m_stDataSize.QuadPart == 0) { //Check zero size.
		assert(false);
		return std::unexpected(0);
	}

	InitInternalCache();
	m_fMutable = true;

	return { };
}

void CDataLoader::OnHexGetData(HEXCTRL::HEXDATAINFO& hdi)
{
	using enum Ut::EOpenMode;
	switch (m_dos.eOpenMode) {
	case OPEN_PROC:
		hdi.spnData = ReadProcData(hdi);
		break;
	default:
		hdi.spnData = ReadFileData(hdi);
		break;
	}
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

void CDataLoader::OnHexSetData(const HEXCTRL::HEXDATAINFO& hdi)
{
	using enum Ut::EOpenMode;
	switch (m_dos.eOpenMode) {
	case OPEN_DEVICE:
		WriteDeviceData(hdi);
		break;
	case OPEN_FILE:
	case NEW_FILE:
		WriteFileData(hdi);
		break;
	case OPEN_PROC:
		WriteProcData(hdi);
		break;
	default:
		break;
	}
}

auto CDataLoader::OpenFile()->std::expected<void, DWORD>
{
	m_wstrFileName = m_dos.wstrDataPath.substr(m_dos.wstrDataPath.find_last_of(L'\\') + 1); //File name.
	m_hHandle = CreateFileW(m_dos.wstrDataPath.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (m_hHandle == INVALID_HANDLE_VALUE) {
		if (m_hHandle = CreateFileW(m_dos.wstrDataPath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);	m_hHandle == INVALID_HANDLE_VALUE) {
			return std::unexpected(GetLastError());
		}

		m_eDataAccessMode = Ut::EDataAccessMode::DA_RO;
	}
	else {
		m_fMutable = true;
	}

	if (GetFileSizeEx(m_hHandle, &m_stDataSize); m_stDataSize.QuadPart == 0) { //Check zero size.
		assert(false);
		return std::unexpected(0);
	}

	if (IsDataIOMMAP()) {
		InitIOMMAP(true);
	}
	else {
		InitInternalCache();
	}

	return { };
}

auto CDataLoader::OpenDevice()->std::expected<void, DWORD>
{
	m_wstrFileName = m_dos.wstrDataPath.substr(m_dos.wstrDataPath.find_last_of(L'\\') + 1); //File name.
	m_hHandle = CreateFileW(m_dos.wstrDataPath.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (m_hHandle == INVALID_HANDLE_VALUE) {
		if (m_hHandle = CreateFileW(m_dos.wstrDataPath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);	m_hHandle == INVALID_HANDLE_VALUE) {
			return std::unexpected(GetLastError());
		}
	}
	else {
		m_fMutable = true;
	}

	DISK_GEOMETRY stGeometry { };
	DWORD dwBytesRet { };
	if (!DeviceIoControl(m_hHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0, &stGeometry, sizeof(stGeometry), &dwBytesRet, nullptr)) {
		const auto err = GetLastError();
		LogLastError(L"DeviceIoControl(IOCTL_DISK_GET_DRIVE_GEOMETRY)", err);
		return std::unexpected(err);
	}

	m_dwDeviceAlign = stGeometry.BytesPerSector;

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
	InitInternalCache(GetDeviceAlign());
	m_eDataIOMode = Ut::EDataIOMode::DATA_IOIMMEDIATE;

	return { };
}

auto CDataLoader::OpenProcess()->std::expected<void, DWORD>
{
	m_wstrFileName = m_dos.wstrDataPath; //Process name.
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
	InitInternalCache();
	m_eDataIOMode = Ut::EDataIOMode::DATA_IOIMMEDIATE;
	m_fMutable = true;

	return { };
}

auto CDataLoader::ReadFileData(const HEXCTRL::HEXDATAINFO& hdi)->HEXCTRL::SpanByte
{
	const auto ullOffset { hdi.stHexSpan.ullOffset };
	const auto ullSize { hdi.stHexSpan.ullSize };
	assert(ullOffset + ullSize <= GetDataSize());
	assert(ullSize <= GetCacheSize());
	if (ullOffset + ullSize > GetDataSize()) { //Overflow check.
		return { };
	}

	//Data is already in the cache?
	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) {
		return { m_pCache.get() + (ullOffset - m_ullOffsetCurr), ullSize };
	}

	FlushCache();

	const auto ullOffsetRemainder = ullOffset % GetDeviceAlign();
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

auto CDataLoader::ReadProcData(const HEXCTRL::HEXDATAINFO& hdi)->HEXCTRL::SpanByte
{
	const auto ullOffset { hdi.stHexSpan.ullOffset };
	const auto ullSize { hdi.stHexSpan.ullSize };
	assert(ullOffset + ullSize <= GetDataSize());
	assert(ullSize <= GetCacheSize());
	if (ullOffset + ullSize > GetDataSize()) { //Overflow check.
		return { };
	}

	//Data is already in the cache?
	if (ullOffset >= m_ullOffsetCurr && (ullOffset + ullSize) <= (m_ullOffsetCurr + m_ullSizeCurr)) {
		return { m_pCache.get() + (ullOffset - m_ullOffsetCurr), ullSize };
	}

	const auto ullOffsetRemainder = ullOffset % GetMemPageSize();
	const auto ullOffsetAligned = ullOffset - ullOffsetRemainder;
	const auto ullSizeAligned = (ullOffsetAligned + GetInternalCacheSize()) <= GetDataSize() ? GetInternalCacheSize()
		: GetDataSize() - ullOffsetAligned;
	assert(ullSizeAligned >= ullSize);
	assert((ullSizeAligned - ullOffsetRemainder) >= ullSize);

	auto u64SizeRemain { ullSizeAligned };
	auto u64CacheOffset { 0ULL };   //Offset in the cache to write data to.
	auto u64RegsTotalSize { 0ULL }; //Total size of the commited regions of pages.
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

void CDataLoader::WriteDeviceData(const HEXCTRL::HEXDATAINFO& hdi)
{
	const auto ullOffsetOrig { hdi.stHexSpan.ullOffset };
	const auto ullSizeOrig { hdi.stHexSpan.ullSize };
	assert(ullOffsetOrig >= m_ullOffsetCurr);
	assert(ullSizeOrig <= m_ullSizeCurr);

	const auto ullOffsetRem = ullOffsetOrig % GetDeviceAlign();
	const auto ullOffsetAlign = ullOffsetOrig - ullOffsetRem;
	const auto ullSizeToAdd = GetDeviceAlign() - ((ullSizeOrig + ullOffsetRem) % GetDeviceAlign());
	const auto ullSizeAlign = ullSizeOrig + ullOffsetRem + ullSizeToAdd;
	assert(ullOffsetAlign >= m_ullOffsetCurr);
	assert(ullSizeAlign <= m_ullSizeCurr);
	const auto ullOffsetFromCache = ullOffsetAlign - m_ullOffsetCurr;

	if (SetFilePointerEx(m_hHandle, { .QuadPart { static_cast<LONGLONG>(ullOffsetAlign) } }, nullptr, FILE_BEGIN) == FALSE) {
		LogLastError(L"SetFilePointerEx");
		assert(false);
		return;
	}

	if (WriteFile(m_hHandle, m_pCache.get() + ullOffsetFromCache, static_cast<DWORD>(ullSizeAlign), nullptr, nullptr) == FALSE) {
		LogLastError(L"WriteFile");
		//Some sectors may have Write Protection, whilst a Device or Volume by itself is writable.
		//So no need to hard assert() here, as opposed to WriteFileData.
	}
}

void CDataLoader::WriteFileData(const HEXCTRL::HEXDATAINFO& hdi)
{
	if (!IsDataIOImmediate()) {
		m_fModified = true;
		return;
	}

	const auto ullOffsetOrig { hdi.stHexSpan.ullOffset };
	const auto ullSizeOrig { hdi.stHexSpan.ullSize };
	assert(ullOffsetOrig >= m_ullOffsetCurr);
	assert(ullSizeOrig <= m_ullSizeCurr);
	const auto ullOffsetFromCache = ullOffsetOrig - m_ullOffsetCurr;

	if (SetFilePointerEx(m_hHandle, { .QuadPart { static_cast<LONGLONG>(ullOffsetOrig) } }, nullptr, FILE_BEGIN) == FALSE) {
		LogLastError(L"SetFilePointerEx");
		assert(false);
		return;
	}

	if (WriteFile(m_hHandle, m_pCache.get() + ullOffsetFromCache, static_cast<DWORD>(ullSizeOrig), nullptr, nullptr) == FALSE) {
		LogLastError(L"WriteFile");
		assert(false);
		return;
	}
}

void CDataLoader::WriteProcData(const HEXCTRL::HEXDATAINFO& hdi)
{
	const std::byte* pMemCurr { nullptr };
	const auto pData { hdi.spnData.data() };
	const auto ullOffset { hdi.stHexSpan.ullOffset };
	auto ullSizeRemain { hdi.stHexSpan.ullSize };
	assert(ullOffset >= m_ullOffsetCurr);
	assert(ullSizeRemain <= m_ullSizeCurr);

	auto u64DataOffset { 0ULL };    //Offset in the pData to get data from.
	auto u64RegsTotalSize { 0ULL }; //Total size of the commited regions of pages.
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
				u64SizeToWrite = u64AvailSizeInRegion >= ullSizeRemain ? ullSizeRemain : u64AvailSizeInRegion;
				pAddrToWrite = reinterpret_cast<std::byte*>(mbi.BaseAddress);
			}
			else if (ullOffset < u64RegsTotalSize) {
				const auto u64OffsetFromRegionBegin = ullOffset - (u64RegsTotalSize - mbi.RegionSize);
				const auto u64AvailSizeInRegion = mbi.RegionSize - u64OffsetFromRegionBegin;
				u64SizeToWrite = u64AvailSizeInRegion >= ullSizeRemain ? ullSizeRemain : u64AvailSizeInRegion;
				pAddrToWrite = reinterpret_cast<std::byte*>(mbi.BaseAddress) + u64OffsetFromRegionBegin;
			}

			if (u64SizeToWrite > 0) {
				if (WriteProcessMemory(m_hHandle, pAddrToWrite, pData + u64DataOffset, u64SizeToWrite, nullptr) == FALSE) {
					LogLastError(L"WriteProcessMemory");
					return;
				}

				if (u64SizeToWrite == ullSizeRemain) { //We have written required size.
					break;
				}

				fVestige = true;
				ullSizeRemain -= u64SizeToWrite;
				u64DataOffset += u64SizeToWrite;
			}
		}
	}
}