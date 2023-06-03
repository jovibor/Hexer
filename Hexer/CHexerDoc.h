/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <cstddef>
#include <string>

class CHexerDoc final : public CDocument
{
public:
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetFilePath()const->const std::wstring&;
	[[nodiscard]] auto GetFileSize()const->std::size_t;
	[[nodiscard]] auto GetFileData()const->std::byte*;
	[[nodiscard]] bool GetFileRW()const;
private:
	BOOL OnNewDocument()override;
	BOOL OnOpenDocument(LPCTSTR lpszPathName)override;
	void OnCloseDocument()override;
	bool OpenFile(LPCWSTR lpszFileName);
	void CloseFile();
	DECLARE_DYNCREATE(CHexerDoc);
	DECLARE_MESSAGE_MAP();
private:
	HANDLE m_hFile { };      //Returned by CreateFileW.
	HANDLE m_hMapObject { }; //Returned by CreateFileMappingW.
	LPVOID m_lpBase { };     //Returned by MapViewOfFile.
	LARGE_INTEGER m_stFileSize { };
	std::wstring m_wstrFileName;
	std::wstring m_wstrFilePath;
	bool m_fWritable { false }; //Is file opened RW or RO?
};