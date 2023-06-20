/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerMDTemplate.h"
#include "CHexerDoc.h"

auto CHexerMDTemplate::OpenDocumentFile(const Utility::FILEOPEN& fos)->CDocument*
{
	//This code is copy-pasted from the original CMultiDocTemplate::OpenDocumentFile.
	//And adapted to work with the FILEOPEN struct.

	auto pDocument = static_cast<CHexerDoc*>(CreateNewDocument());
	if (pDocument == nullptr) {
		TRACE(traceAppMsg, 0, "CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return nullptr;
	}
	ASSERT_VALID(pDocument);

	BOOL bAutoDelete = pDocument->m_bAutoDelete;
	pDocument->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
	CFrameWnd* pFrame = CreateNewFrame(pDocument, nullptr);
	pDocument->m_bAutoDelete = bAutoDelete;
	if (pFrame == nullptr) {
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDocument;       // explicit delete on error
		return nullptr;
	}
	ASSERT_VALID(pFrame);

	CWaitCursor wait;
	if (!pDocument->OnOpenDocument(fos)) {
		// user has be alerted to what failed in OnOpenDocument
		TRACE(traceAppMsg, 0, "CDocument::OnOpenDocument returned FALSE.\n");
		pFrame->DestroyWindow();
		return nullptr;
	}
	pDocument->SetPathName(fos.wstrFilePath.data(), FALSE);
	pDocument->OnDocumentEvent(CDocument::onAfterOpenDocument);

	InitialUpdateFrame(pFrame, pDocument, TRUE);
	return pDocument;
}