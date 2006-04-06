// MyBitmap.cpp: implementation of the CMyBitmap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyBitmap.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMyBitmap::CMyBitmap()
{
	
}

CMyBitmap::~CMyBitmap()
{

}

HICON CMyBitmap::BitmapToIcon( HBITMAP hBmp ) 
{ 
	if ( hBmp == NULL ) {
		return NULL;
	}
	

  HICON hIcon;

  ICONINFO IconInfo;


  IconInfo.hbmColor = hBmp;

  IconInfo.hbmMask = hBmp;


  IconInfo.fIcon = TRUE;
  hIcon = CreateIconIndirect(&IconInfo);

  return hIcon;

}
	
HICON CMyBitmap::LoadBMPImage( LPCTSTR sBMPFile )
{

	CFile file;
	if( !file.Open( sBMPFile, CFile::modeRead) )
		return NULL;

	BITMAPFILEHEADER bmfHeader;

	// Read file header
	if (file.Read((LPSTR)&bmfHeader, sizeof(bmfHeader)) != sizeof(bmfHeader))
		return NULL;

	// File type should be 'BM'
	if (bmfHeader.bfType != ((WORD) ('M' << 8) | 'B'))
		return NULL;

	// Get length of the remainder of the file and allocate memory
	DWORD nPackedDIBLen = file.GetLength() - sizeof(BITMAPFILEHEADER);
	HGLOBAL hDIB = ::GlobalAlloc(GMEM_FIXED, nPackedDIBLen);
	if (hDIB == 0)
		return NULL;

	// Read the remainder of the bitmap file.
	if (file.ReadHuge((LPSTR)hDIB, nPackedDIBLen) != nPackedDIBLen )
	{
		::GlobalFree(hDIB);
		return NULL;
	}


	BITMAPINFOHEADER &bmiHeader = *(LPBITMAPINFOHEADER)hDIB ;
	BITMAPINFO &bmInfo = *(LPBITMAPINFO)hDIB ;

	// If bmiHeader.biClrUsed is zero we have to infer the number
	// of colors from the number of bits used to specify it.
	int nColors = bmiHeader.biClrUsed ? bmiHeader.biClrUsed : 
						1 << bmiHeader.biBitCount;

	LPVOID lpDIBBits;
	if( bmInfo.bmiHeader.biBitCount > 8 )
		lpDIBBits = (LPVOID)((LPDWORD)(bmInfo.bmiColors + bmInfo.bmiHeader.biClrUsed) + 
			((bmInfo.bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
	else
		lpDIBBits = (LPVOID)(bmInfo.bmiColors + nColors);

	CClientDC dc(NULL);
	CPalette* pOldPalette = NULL;

	HBITMAP hBmp = CreateDIBitmap( dc.m_hDC,		// handle to device context 
				&bmiHeader,	// pointer to bitmap size and format data 
				CBM_INIT,	// initialization flag 
				lpDIBBits,	// pointer to initialization data 
				&bmInfo,	// pointer to bitmap color-format data 
				DIB_RGB_COLORS);		// color-data usage 

	if( pOldPalette )
		dc.SelectPalette( pOldPalette, FALSE );

	::GlobalFree(hDIB);

	return BitmapToIcon( hBmp );
}
