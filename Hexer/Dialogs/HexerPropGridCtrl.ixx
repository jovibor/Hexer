module;
#include <SDKDDKVer.h>
#include <afxcontrolbars.h>
#include <string>
#include <unordered_map>
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


//CHexerPropGridProp
export class CHexerPropGridProp final : public CMFCPropertyGridProperty {
public:
	using CMFCPropertyGridProperty::CMFCPropertyGridProperty; //All base class ctors.
	BOOL AddOptionEx(LPCWSTR lpszOption, DWORD_PTR dwData);
	void OnDrawValue(CDC* pDC, CRect rect)override;
	void SetValueColor(COLORREF clr);
	void SetValueFromData(DWORD_PTR dwData);
private:
	void OnSelectCombo()override;
private:
	std::unordered_map<std::wstring, DWORD_PTR> m_umapData; //Options->Data map.
	COLORREF m_clrValue { 0xFFFFFFFFUL }; //Value text color.
};

BOOL CHexerPropGridProp::AddOptionEx(LPCWSTR lpszOption, DWORD_PTR dwData)
{
	m_umapData[lpszOption] = dwData;
	return CMFCPropertyGridProperty::AddOption(lpszOption, TRUE);
}

void CHexerPropGridProp::OnDrawValue(CDC* pDC, CRect rect)
{
	if (m_clrValue != 0xFFFFFFFFUL) {
		const auto clrOld = pDC->SetTextColor(m_clrValue);
		CMFCPropertyGridProperty::OnDrawValue(pDC, rect);
		pDC->SetTextColor(clrOld);
	}
	else {
		CMFCPropertyGridProperty::OnDrawValue(pDC, rect);
	}
}

void CHexerPropGridProp::SetValueColor(COLORREF clr)
{
	m_clrValue = clr;
}

void CHexerPropGridProp::SetValueFromData(DWORD_PTR dwData)
{
	for (const auto& [key, value] : m_umapData) {
		if (value == dwData) {
			SetValue(key.data());
			SetData(dwData);
		}
	}
}

void CHexerPropGridProp::OnSelectCombo()
{
	CMFCPropertyGridProperty::OnSelectCombo();
	SetData(m_umapData[GetValue().bstrVal]);
}