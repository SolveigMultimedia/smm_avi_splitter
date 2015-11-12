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

/*
	DllMain - entry point.
	DllRegisterServer/DllUnRegisterServer.
	Factory template for the filter.
*/

#include "stdafx.h"
#include "proppages.h"
#include "strsafe.h"
#include "avisplitter.h"
//declare the factory template for filter
CFactoryTemplate g_Templates[] = {
	// one entry for each CoCreate-able object
	{
		CAVISplitter::m_sudFilter.strName,// Name.
		CAVISplitter::m_sudFilter.clsID,// CLSID.
		CAVISplitter::CreateInstance,// Creation function.
		NULL,
		&CAVISplitter::m_sudFilter// Pointer to filter information.
	},
	{
		L"AVI Splitter about page", 
		(const CLSID*)&__uuidof(CAboutPropPage), 
		CAboutPropPage::CreateInstance, 
		NULL, 
		&CAVISplitter::m_sudFilter// Pointer to filter information.
	},
	{
		L"AVI Splitter property page", 
		(const CLSID*)&__uuidof(CPropertyPropPage), 
		CPropertyPropPage::CreateInstance, 
		NULL, 
		&CAVISplitter::m_sudFilter// Pointer to filter information.
	}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// DLL entry point for initialization of directshow base classes
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
    BOOL ret = DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpReserved);
#ifdef DEBUG

	if (dwReason ==  DLL_PROCESS_ATTACH)
	{
		
	}
	else  if (dwReason ==  DLL_PROCESS_DETACH )
	{
		// test leak
		_CrtDumpMemoryLeaks();

	}
#endif
	return ret;
}

bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
	bool bOK = false;

	TCHAR szKey[MAX_PATH];
	StringCbCopy(szKey, MAX_PATH, pszKey);
	if(pszSubkey != 0)
	{
		StringCbCat(szKey,MAX_PATH,_T("\\"));
		StringCbCat(szKey,MAX_PATH,pszSubkey);
	}
	HKEY hKey;
	LONG ec = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
	if(ec == ERROR_SUCCESS)
	{
		if(pszValue != 0)
		{
			ec = ::RegSetValueEx(hKey, pszValueName, 0, REG_SZ,
				reinterpret_cast<BYTE*>(const_cast<LPTSTR>(pszValue)),
				(_tcslen(pszValue) + 1) * sizeof(TCHAR));
		}

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}


bool DeleteRegKey(LPCTSTR pszKey, LPCTSTR pszSubkey)
{
	bool bOK = false;

	HKEY hKey;
	LONG ec = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, pszKey, 0, KEY_ALL_ACCESS, &hKey);
	if(ec == ERROR_SUCCESS)
	{
		if(pszSubkey != 0)
			ec = ::RegDeleteKey(hKey, pszSubkey);

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCTSTR chkbytes, LPCTSTR ext, ...)
{
	TCHAR null[MAX_PATH];
	memset(null, 0, sizeof(null));
	StringFromGUID2(GUID_NULL, null, MAX_PATH);

	TCHAR majortype[MAX_PATH];
	memset(majortype, 0, sizeof(majortype));
	StringFromGUID2(MEDIATYPE_Stream, majortype, MAX_PATH);
	
	TCHAR subtype[MAX_PATH];
	memset(subtype, 0, sizeof(subtype));
	StringFromGUID2(subtype2, subtype, MAX_PATH);

	TCHAR clsidstr[MAX_PATH];
	memset(clsidstr, 0, sizeof(clsidstr));
	StringFromGUID2(clsid, clsidstr, MAX_PATH);


	TCHAR keyPath[MAX_PATH];
	memset(keyPath, 0, sizeof(keyPath));
	StringCbCat(keyPath,MAX_PATH,_T("Media Type\\"));
	StringCbCat(keyPath,MAX_PATH,majortype);
	SetRegKeyValue(keyPath, subtype, _T("0"), chkbytes);
	SetRegKeyValue(keyPath, subtype, _T("Source Filter"), clsidstr);

	memset(keyPath, 0, sizeof(keyPath));
	StringCbCat(keyPath,MAX_PATH,_T("Media Type\\"));
	StringCbCat(keyPath,MAX_PATH,null);
	DeleteRegKey(keyPath, subtype);

	va_list marker;
	va_start(marker, ext);
	for(; ext; ext = va_arg(marker, LPCTSTR))
		DeleteRegKey(_T("Media Type\\Extensions"), ext);
	va_end(marker);
}

void UnRegisterSourceFilter(const GUID& subtype2)
{
	TCHAR majortype[MAX_PATH];
	memset(majortype, 0, sizeof(majortype));
	StringFromGUID2(MEDIATYPE_Stream, majortype, MAX_PATH);

	TCHAR subtype[MAX_PATH];
	memset(subtype, 0, sizeof(subtype));
	StringFromGUID2(subtype2, subtype, MAX_PATH);
	TCHAR keyPath[MAX_PATH];
	memset(keyPath, 0, sizeof(keyPath));
	StringCbCat(keyPath,MAX_PATH,_T("Media Type\\"));
	StringCbCat(keyPath,MAX_PATH,majortype);

	DeleteRegKey(keyPath, subtype);
}

// self-registration entrypoint
STDAPI DllRegisterServer()
{
	// Teach file source to understand avi format
	//RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_Avi, _T("0,4,,52494646"), _T(".avi"), NULL);
    // base classes will handle registration using the factory template table
    HRESULT hr = AMovieDllRegisterServer2(true);
    return hr;
}

STDAPI DllUnregisterServer()
{
	//UnRegisterSourceFilter(MEDIASUBTYPE_Avi);
    // base classes will handle de-registration using the factory template table
    HRESULT hr = AMovieDllRegisterServer2(false); 
    return hr;
}


