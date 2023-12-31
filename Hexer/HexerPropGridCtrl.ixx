module;
#include <SDKDDKVer.h>
#include <afxcontrolbars.h>
export module HexerPropGridCtrl;

//CHexerPropGridCtrl.
export class CHexerPropGridCtrl final : public CMFCPropertyGridCtrl {
private:
	void OnSize(UINT /*f*/, int /*cx*/, int /*cy*/) {
		EndEditItem();
		AdjustLayout();
	}
	DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CHexerPropGridCtrl, CMFCPropertyGridCtrl)
	ON_WM_SIZE()
END_MESSAGE_MAP()