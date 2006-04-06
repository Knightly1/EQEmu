// MyBitmap.h: interface for the CMyBitmap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MYBITMAP_H__5963F5F7_B5A1_4C76_BBB5_84EC1FDCAE2D__INCLUDED_)
#define AFX_MYBITMAP_H__5963F5F7_B5A1_4C76_BBB5_84EC1FDCAE2D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMyBitmap : public CBitmap  
{
public:
	CMyBitmap();
	virtual ~CMyBitmap();
	HICON CMyBitmap::BitmapToIcon( HBITMAP hBmp );
	HICON CMyBitmap::LoadBMPImage( LPCTSTR sBMPFile );
};

#endif // !defined(AFX_MYBITMAP_H__5963F5F7_B5A1_4C76_BBB5_84EC1FDCAE2D__INCLUDED_)
