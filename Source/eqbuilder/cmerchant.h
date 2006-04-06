
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMERCHANT_H__68C9EAF7_6EF4_4EEA_A471_D10C3F259F8D__INCLUDED_)
#define AFX_CMERCHANT_H__68C9EAF7_6EF4_4EEA_A471_D10C3F259F8D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/types.h"

#pragma warning(disable: 4786)

#include <map>
using namespace std;

class cnpc;

class cmerchant  
{
public:
	cmerchant();
	cmerchant( cmerchant* m );
	virtual ~cmerchant();
	
	bool db;
	cnpc *owner;
	int id;
	float rate;
	map<uint16, uint32> items;	//slot -> item id
	
};

#endif // !defined(AFX_CGRID_H__68C9EAF7_6EF4_4EEA_A471_D10C3F259F8D__INCLUDED_)
