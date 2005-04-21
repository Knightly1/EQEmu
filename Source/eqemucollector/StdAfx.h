// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifdef WIN32
#if !defined(AFX_STDAFX_H__0468667E_5A0F_433D_8056_828DFD63B121__INCLUDED_)
#define AFX_STDAFX_H__0468667E_5A0F_433D_8056_828DFD63B121__INCLUDED_

#pragma warning(disable : 4786) 

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <winsock2.h>		//included here to prevent some random ass include chain with winsock.h
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>		// MFC socket extensions

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0468667E_5A0F_433D_8056_828DFD63B121__INCLUDED_)
#endif

