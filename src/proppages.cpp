/*
Copyright (c) 2015 Solveig Multimedia www.solveigmm.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:



The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.



THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "stdafx.h"
#include "proppages.h"
#include "avisplitter.h"
#include "resource.h"
#include "atlconv.h"
#include "commctrl.h"
#include "assert.h"
CAboutPropPage::CAboutPropPage(LPUNKNOWN pUnk, HRESULT *phr)
:CBasePropertyPage( "SolveigMM AVI Splitter About Page", pUnk, IDD_ABOUT, IDS_ABOUTPP)
{
}

CUnknown * WINAPI CAboutPropPage::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	CUnknown *punk = new CAboutPropPage(lpunk, phr);
	if (punk == NULL)
		*phr = E_OUTOFMEMORY;
	return punk;
}

INT_PTR CAboutPropPage::OnReceiveMessage( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//move logo icon to center of the dialog on WM_SIZE
	if(hwnd == m_hwnd && uMsg==WM_SIZE)
	{
		centerLogo();
	}
	return CBasePropertyPage::OnReceiveMessage( hwnd, uMsg, wParam, lParam );
}

HRESULT CAboutPropPage::OnActivate()
{
	// set title
	HWND  hwTitle= GetDlgItem(m_hwnd, IDC_STATIC_TITLE);
	SetWindowText(hwTitle, CAVISplitter::m_sudFilter.strName);

	// set version text on About tab
	string sVersion = getVersionStringFromResource();
	HWND  hwVerInfo = GetDlgItem(m_hwnd, IDC_STATIC_VERSION);
	USES_CONVERSION;
	SetWindowText(hwVerInfo, A2W(sVersion.c_str()));

	return CBasePropertyPage::OnActivate();
}
void CAboutPropPage::centerLogo()
{
	RECT baseRect;
	GetWindowRect(m_hwnd, &baseRect);
	int baseWidth = baseRect.right - baseRect.left;
	HWND  hwLogo = GetDlgItem(m_hwnd, IDC_STATIC_LOGO);
	RECT logoRect;
	GetWindowRect(hwLogo, &logoRect);
	int logoWidth = logoRect.right-logoRect.left;
	int logoHeight = logoRect.bottom-logoRect.top;
	int newPoxX = baseRect.left+ (baseWidth - logoWidth)/2;
	MoveWindow(hwLogo,newPoxX-baseRect.left,logoRect.top-baseRect.top,logoWidth, logoHeight, TRUE);
}
string CAboutPropPage::getVersionStringFromResource()
{
	string sVersion = "";
	char szBuf[128];
	memset(szBuf,0, sizeof(szBuf));
	TCHAR buf[]={_T("#1")}; // Only one VERSION_INFO present
	HRSRC hr = FindResource (g_hInst, buf, RT_VERSION); 
	if ( hr )
	{     
		HGLOBAL hg = LoadResource (g_hInst, hr);
		if (hg)
		{
			BYTE *pByte=(BYTE*)LockResource (hg);
			if (pByte)
			{
				VS_FIXEDFILEINFO *pVS=(VS_FIXEDFILEINFO*)(pByte+40);
				if (pVS)
				{
					sVersion = "Version ";
					sVersion += _itoa(HIWORD (pVS->dwFileVersionMS), szBuf, 10);
					sVersion += ".";
					sVersion += _itoa(LOWORD (pVS->dwFileVersionMS), szBuf, 10);
					sVersion += ".";
					sVersion += _itoa(HIWORD (pVS->dwFileVersionLS), szBuf, 10);
					sVersion += ".";
					sVersion += _itoa(LOWORD (pVS->dwFileVersionLS), szBuf, 10);
				}
			}
		}
	}
	return sVersion;
}

CPropertyPropPage::CPropertyPropPage(LPUNKNOWN pUnk, HRESULT *phr)
:CBasePropertyPage( "SolveigMM AVI Splitter Property Page", pUnk, IDD_PROPERTY, IDS_PROPERTY),

m_pFilterProp(NULL)
{
}

CUnknown * WINAPI CPropertyPropPage::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	CUnknown *punk = new CPropertyPropPage(lpunk, phr);
	if (punk == NULL)
		*phr = E_OUTOFMEMORY;
	return punk;
}

INT_PTR CPropertyPropPage::OnReceiveMessage( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_COMMAND:
		/*if (LOWORD(wParam) == IDC_CHECK_ANNEXBAVC || LOWORD(wParam) == IDC_CHECK_SEPARATE_PINS || LOWORD(wParam) == IDC_CHECK_ADTS_AAC_OUTPUT)
		{
			SetDirty();
			return (LRESULT)1;
		}*/
		break;
	}
	return CBasePropertyPage::OnReceiveMessage( hwnd, uMsg, wParam, lParam );
}
HRESULT CPropertyPropPage::OnConnect(IUnknown *pUnknown)
{
	if (pUnknown == NULL)
		return E_POINTER;
	return pUnknown->QueryInterface(IID_ISMMModuleConfig, reinterpret_cast<void**>(&m_pFilterProp));
}

HRESULT CPropertyPropPage::OnActivate()
{
	HRESULT hr = S_OK;
	assert(m_pFilterProp != NULL);
	
	return hr;
}
HRESULT CPropertyPropPage::OnDisconnect()
{
	// Release of Interface after setting the appropriate old effect value
	if(m_pFilterProp)
	{
		m_pFilterProp->Release();
		m_pFilterProp = NULL;
	}
	return NOERROR;
} 
HRESULT CPropertyPropPage::OnApplyChanges()
{
	HRESULT hr = S_OK;
	assert(m_pFilterProp != NULL);
	return hr;
}
void CPropertyPropPage::SetDirty()
{
	m_bDirty = true;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}
CString CPropertyPropPage::refTimeToString(REFERENCE_TIME time)
{
	char chtmp[100];
	DWORD dwMsec	 = DWORD(time / 10000);
	DWORD dwSec		 = dwMsec / 1000;
	DWORD dwHours	 = dwSec / 3600;
	DWORD dwMinutes  = (dwSec % 3600) / 60;
	DWORD dwSeconds  = (dwSec % 3600) - dwMinutes * 60;
	//DWORD dwMSeconds = dwMsec % 1000;
	//sprintf(chtmp,"%.2u:%.2u:%.2u:%.3u",dwHours,dwMinutes,dwSeconds, dwMSeconds );
	sprintf(chtmp,"%.2u:%.2u:%.2u",dwHours,dwMinutes,dwSeconds );
	USES_CONVERSION;
	LPCTSTR  tch = A2T(chtmp);
	CString ret(tch);
	return ret;
}