// cloc.h: interface for the cloc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLOC_H__735DA8C1_4DF9_4241_9FEE_9161537BCAFB__INCLUDED_)
#define AFX_CLOC_H__735DA8C1_4DF9_4241_9FEE_9161537BCAFB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cloc  
{
public:
	cloc();
	cloc( const cloc* loc );
	cloc( double x, double y, double z );
	cloc( double x, double y, double z, double heading );
	
	double dist2xy(const cloc *to) const {
		double dx = x - to->x;
		double dy = y - to->y;
		return(dx*dx + dy*dy);
	}

	double x;
	double y;
	double z;
	double heading;

};


//a spawn2 entry
class cspawnpoint : public cloc {
public:
	cspawnpoint( const cloc* loc );
	cspawnpoint( const cspawnpoint* loc );
	int db_id;
};

#endif // !defined(AFX_CLOC_H__735DA8C1_4DF9_4241_9FEE_9161537BCAFB__INCLUDED_)
