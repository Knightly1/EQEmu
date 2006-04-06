// cteleport.h: interface for the cteleport class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTELEPORT_H__1C82A73E_3B38_42C8_A497_340944153B72__INCLUDED_)
#define AFX_CTELEPORT_H__1C82A73E_3B38_42C8_A497_340944153B72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cloc.h"

class cteleport  
{
public:
	cteleport();
	virtual ~cteleport();

	int id;
	CString zone;
	cloc* loc;

};

#endif // !defined(AFX_CTELEPORT_H__1C82A73E_3B38_42C8_A497_340944153B72__INCLUDED_)
