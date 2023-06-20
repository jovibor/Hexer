/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxwin.h>
#include <string>
import Utility;

class CHexerMDTemplate final : public CMultiDocTemplate
{
public:
	CHexerMDTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass)
		: CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}
	[[nodiscard]] auto OpenDocumentFile(const Utility::FILEOPEN& fos) -> CDocument*;
};