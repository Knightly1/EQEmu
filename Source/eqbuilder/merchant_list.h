// merchant_list.h: interface for the merchant_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MERCHANT_LIST_H__26D2E8A3_C7C6_4499_8D82_E84546EDB561__INCLUDED_)
#define AFX_MERCHANT_LIST_H__26D2E8A3_C7C6_4499_8D82_E84546EDB561__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cmerchant.h"
#include "../common/linked_list.h"

class merchant_list  
{
public:
	merchant_list();
	virtual ~merchant_list();

	void add( cmerchant* m );
	cmerchant* get( int pos );
	int getsize() { return size; }

private:
	int size;
	LinkedList<cmerchant*> list;
};

#endif // !defined(AFX_GRID_LIST_H__26D2E8A3_C7C6_4499_8D82_E84546EDB561__INCLUDED_)
