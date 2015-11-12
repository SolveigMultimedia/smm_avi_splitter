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

#pragma once

#include "tchar.h"


class DECLSPEC_UUID("17C1CA4D-CB12-4ab5-A770-CA484E4B2CAE")
CAboutPropPage : public CBasePropertyPage
{

public:
	CAboutPropPage(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAboutPropPage(){};

	static CUnknown * WINAPI CreateInstance( LPUNKNOWN lpunk, HRESULT *phr );
	INT_PTR OnReceiveMessage( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    HRESULT OnActivate();
private:
	void centerLogo();
	string getVersionStringFromResource();
};


class DECLSPEC_UUID("EEB121F6-C68E-4f26-B52B-EE96A734981B")
CPropertyPropPage : public CBasePropertyPage
{

public:
	CPropertyPropPage(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CPropertyPropPage(){};

	static CUnknown * WINAPI CreateInstance( LPUNKNOWN lpunk, HRESULT *phr );
	INT_PTR OnReceiveMessage( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnActivate();
	HRESULT OnApplyChanges();
	HRESULT OnDisconnect();
protected:
private:
	CString refTimeToString(REFERENCE_TIME time);
	
    ISMMModuleConfig* m_pFilterProp;
	void SetDirty();

};