// czone.h: interface for the czone class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CZONE_H__7A3DDFB1_C7B8_4FC2_BEBC_8294525BAB51__INCLUDED_)
#define AFX_CZONE_H__7A3DDFB1_C7B8_4FC2_BEBC_8294525BAB51__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class czone  
{
public:
	czone();
	virtual ~czone();

	int id;
	CString short_name;
	CString long_name;

};

#endif // !defined(AFX_CZONE_H__7A3DDFB1_C7B8_4FC2_BEBC_8294525BAB51__INCLUDED_)
