#if !defined(AFX_DATABASE_H__A36E71B8_A738_4534_873E_369985A5BA7B__INCLUDED_)
#define AFX_DATABASE_H__A36E71B8_A738_4534_873E_369985A5BA7B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// database.h : header file
//
#pragma warning(disable:4786)
#include <winsock.h>
#include <mysql.h>
#include <afxtempl.h>
#include "door_list.h"
#include "grid_list.h"
#include "kill_list.h"
#include "log_list.h"
#include "mob_list.h"
#include "npc_list.h"
#include "spawn_list.h"
#include "teleport_list.h"
#include "waypoint_list.h"
#include "zone_list.h"
#include "merchant_list.h"

#include <map>
#include <string>

using namespace std;

typedef CArray<CString,CString&> StringArray;
typedef CArray<int,int&> IntArray;

class merchant_list;
class CEQBuilderDlg;
class IDGenSet;

class database : public CWnd
{
// Construction
public:
	database();

// Attributes
private:
	MYSQL mysql;

// Operations
public:
	bool connected;
	void Connexion(	CString host, CString user, CString password, CString database );
	zone_list* GetZones();
	log_list* GetLogs( CString zonename );
	void ClearLogs( CString zonename );
	bool logexists( CString zonename, CString filename );
	void SaveLog( CString zonename, CString filename );
	void UpdateLog( CString zonename, clog* log );
	CString getZoneName( int id );
	int GetRace( CString nomrace );
	void extractdoors( door_list* doors, teleport_list* teleports, CString path, CString zonename, bool delquerryok );
	void extractteleports( teleport_list* teleports, CString path, CString zonename, bool delquerryok );
	void extractnpcs( npc_list* npcs, CString path, CString zonename, bool delquerryok, bool usedb );
	void extractspawns( spawn_list* fspawns, spawn_list* gspawns, CString path, CString zonename, bool delquerryok );
	void extractASpawn( cspawn *spawn, CString &zonename, CFile &f);
	void extractgrids( grid_list* grids, CString path, CString zonename, uint32 zone_id, bool delquerryok );
	void extractmerchants( merchant_list* grids, CString path, CString zonename, bool delquerryok );
	void extractdeletes( CString path, CString zonename, bool delquerryok );
	int FindNPCInDatabase( cnpc* npc );
//	int getbestgroupid( IntArray* usedgroupids, IntArray* npcids );
	int getclasse( CString classenom );
//	int getfreenpcid( int start, IntArray* usednpcids );
//	bool isnpcindb( int npcid );
	int load_value(const char *query);

	void loadZone(int zoneid, CString zonename, CEQBuilderDlg *d);
	void loadFactionMapping(const char *zonename, map<string, int> &mapping);
	void loadLootMapping(const char *zonename, map<string, int> &mapping);

	int getNextMerchantID();

	void InvalidateCaches();

protected:
	void runquery( CString query, MYSQL_RES** result, unsigned __int64* affected_rows, unsigned __int64* last_insert_id );
	
//	int getNextSpawn2ID(int min);
	
	int _merchantid;
	int _spawn2id;

	void LoadRaceCache();
	void LoadClassCache();
	bool m_raceCachesFilled;
	bool m_classCachesFilled;
	map<uint8, string> m_raceToNames;
	map<uint8, string> m_classToNames;
	map<string, uint8> m_raceToIDs;
	map<string, uint8> m_classToIDs;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(database)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~database();

	// Generated message map functions
protected:
	//{{AFX_MSG(database)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATABASE_H__A36E71B8_A738_4534_873E_369985A5BA7B__INCLUDED_)
