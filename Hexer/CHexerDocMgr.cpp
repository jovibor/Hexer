/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerDocMgr.h"

IMPLEMENT_DYNCREATE(CHexerDocMgr, CDocument)

auto CHexerDocMgr::OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU)->CDocument*
{
	//This code is copy-pasted from the original CDocManager::OpenDocumentFile.
	//We need to override this method to remove calls to AtlStrLen, AfxFullPath, AfxResolveShortcut
	//functions from the original method, because these functions can't handle paths like "\\?\PhysicalDisk".

	if (lpszFileName == NULL) {
		AfxThrowInvalidArgException();
	}
	// find the highest confidence
	POSITION pos = m_templateList.GetHeadPosition();
	CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
	CDocTemplate* pBestTemplate = NULL;
	CDocument* pOpenDocument = NULL;

	while (pos != NULL) {
		CDocTemplate* pTemplate = static_cast<CDocTemplate*>(m_templateList.GetNext(pos));
		ASSERT_KINDOF(CDocTemplate, pTemplate);

		CDocTemplate::Confidence match;
		ASSERT(pOpenDocument == NULL);
		match = pTemplate->MatchDocType(lpszFileName, pOpenDocument);
		if (match > bestMatch) {
			bestMatch = match;
			pBestTemplate = pTemplate;
		}
		if (match == CDocTemplate::yesAlreadyOpen)
			break;      // stop here
	}

	if (pOpenDocument != NULL) {
		auto posOpenDoc = pOpenDocument->GetFirstViewPosition();
		if (posOpenDoc != NULL) {
			const auto pView = pOpenDocument->GetNextView(posOpenDoc); // get first one
			ASSERT_VALID(pView);
			const auto pFrame = pView->GetParentFrame();

			if (pFrame == NULL) {
				TRACE(traceAppMsg, 0, "Error: Can not find a frame for document to activate.\n");
			}
			else {
				pFrame->ActivateFrame();

				if (pFrame->GetParent() != NULL) {
					CFrameWnd* pAppFrame;
					if (pFrame != (pAppFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd))) {
						ASSERT_KINDOF(CFrameWnd, pAppFrame);
						pAppFrame->ActivateFrame();
					}
				}
			}
		}
		else {
			TRACE(traceAppMsg, 0, "Error: Can not find a view for document to activate.\n");
		}

		return pOpenDocument;
	}

	if (pBestTemplate == NULL) {
		AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
		return NULL;
	}

	return pBestTemplate->OpenDocumentFile(lpszFileName, bAddToMRU, TRUE);
}