#if !defined(AFX_LOG_H__97FB9FC1_B842_4839_82B4_4AEE16697099__INCLUDED_)
#define AFX_LOG_H__97FB9FC1_B842_4839_82B4_4AEE16697099__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// logs.h : header file
//
#include "database.h"
#include <afxtempl.h>

struct npc_type {
	int id;
	CString nom;
	int level;
	CString gender;
	int hp;
	int size;
	double walkspeed;
	double runspeed;
	CString race;
	CString classe;
	double speed;
	int texture;
	int helmtexture;
};


struct location {
	double x;
	double y;
	double z;
	double heading;
};

struct mob {
	int id;
	CString nom;
	int level;
	CString gender;
	int hp;
	int owned;
	int size;
	double walkspeed;
	double runspeed;
	CString race;
	int type;
	CString classe;
	location loc;
	double speed;
	int texture;
	int helmtexture;
	int isnpc;
	int numlog;
	int typespawn;
	int killed;
	int movement;
};

struct door {
	int id;
	int type;
	CString model;
	int teleportid;
	location loc;
};

struct teleport {
	int id;
	CString zone;
	location loc;
};

/////////////////////////////////////////////////////////////////////////////
// logs window

class logs : public CWnd
{
// Construction
public:
	logs( CString name );

// Attributes
public:
	CString name;
	int currentpos;
	CString currentkey;
	CArray<CString,CString&>* buffer;

// Operations
public:
	void parse();
	CArray<CString,CString&>* loadbuffer();
	int findnext( CString tofind, int start );
	CString get( int index );
	CString value( CString key, int start, int size );
	bool isMobInit();
	bool isMobAdd();
	bool isDoor();
	bool isTeleporter();
	bool isMovement();
	bool isKilled();
	void getMobInitData();
	void getMobAddData();
	void getDoorData();
	void getTeleporterData();
	void getMovementData();
	void getKilledData();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(log)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~logs();

	// Generated message map functions
protected:
	//{{AFX_MSG(log)
		virtual BOOL OnInitDialog();
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOG_H__97FB9FC1_B842_4839_82B4_4AEE16697099__INCLUDED_)
