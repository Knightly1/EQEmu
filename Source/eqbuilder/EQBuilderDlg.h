// EQBuilderDlg.h : header file
//
#pragma warning(disable:4786)
#include <afxtempl.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include "database.h"
#include "DBConnect.h"
#include "eqbuilderini.h"
#include "SqlOptionsDlg.h"
#include "ZoneViewer.h"
#include "../common/buildfile.h"
#include "BrowseForFolder.h"
#include "IDGenSet.h"

#if !defined(AFX_EQBUILDERDLG_H__1E1249E5_39BC_4670_9353_17544F903F7F__INCLUDED_)
#define AFX_EQBUILDERDLG_H__1E1249E5_39BC_4670_9353_17544F903F7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct filtre_struct {

	bool movementok;
	CString minprob;
	CString moved;
	CString dead;
	CString occur;
	float coord_error;
	float camp_range;
	float deltaz;
	float path_error;
	float fixedmerge_error;
	int affirm;		//bonus probability when a mob is re-affirmed by another log

};

typedef enum {
	locSamePoint,
	locCampRange,
	locUnrelated
} LocDifference;

class MatchStruct {
public:
	string match;
	uint32 id;
};

class merchant_list;
class CZoneViewer;
class BigZoneDlg;
class TextMapReader;
class Map;
class FactionsDialog;
class LootDialog;
class SpellsDialog;

/////////////////////////////////////////////////////////////////////////////
// CEQBuilderDlg dialog

class CEQBuilderDlg : public CDialog
{
// Construction
public:
	CEQBuilderDlg(CWnd* pParent = NULL);	// standard constructor

	// eqbuilder.ini
	eqbuilderini* eqbini;

// Dialog Data
	//{{AFX_DATA(CEQBuilderDlg)
	enum { IDD = IDD_EQBUILDER_DIALOG };
	CEdit	m_progressText;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEQBuilderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

public:
	void Reprocess();

protected:
	HICON m_hIcon;

	// currentdirectory
	CString chemin;

	// menu
	CMenu* m_pMenu;

	// database
	database db_real;
	database *db;	//points to db_real
	DBConnect* dbconnectdlg;

	// output options
	SqlOptionsDlg* sqloptions;

	// fichier log
	CStdioFile flog;
	int readresult;

	// ids
//	int npcstartid;
//	int npcid;
//	bool usezoneid;
	
	// zones
	int zoneid;
	CString zonename;
	int nbzones;
	zone_list *zones;
	CComboBox* zonecombo;
	TextMapReader *text_map;
	Map *zone_map;

	// doors/teleports
	door_list* doors;
	teleport_list* teleports;

	// spawns
	void cleanSpawn( cspawn* spawn );

	// file dialog
	CFileDialog *fileopendialog;
	CFileDialog *filesavedialog;
	CBrowseForDirectory BrowseForDirectory;

	//zone view
	CZoneViewer vZoneView;
	BigZoneDlg *m_bigZone;
	CButton *bReloadMap;
	void ReloadMap();
/*	
	//faction mapper
	FactionsDialog *m_factionDialog;
	vector<MatchStruct> m_factionMapping;

	//loot mapper
	LootDialog *m_lootDialog;
	vector<MatchStruct> m_lootMapping;
*/

	// logs
	int nblogs;
	clog* currentlog;
	log_list* compiledlogs;
	int selectedlog;
	log_list* zonelogs;
	CTreeCtrl *lLogList;
	CButton *bAddLog;
	CButton *bAddDir;
	CButton *bCompileLog;
	CButton *bCompileAll;
	CButton *bDeleteLog;
	CButton *bDeleteAll;
	int currentpos;
	CString currentline;
	StringArray* currentkeys;
	CComboBox* cTypelogCombo;
	CProgressCtrl *pProgress;
	CButton* bProcessButton;
	void ProcessData();
	void AddLogFile(CString pathname, CString filename, CString fileext);
	void AddFilesMatching(const CString &match);

	//Lists:
	CTabCtrl *lSpawnTabs;
	CTreeCtrl *lMobList;
	CTreeCtrl *lSpawnList;
	void SetDataDisplay(int index);
	void afficherSpawns();
	void showSpawnGroups();
	void showMerchants();

	// txt log parser
	void parselog();
	int findnext( CString tofind, int start );
	CString getkey( int index );
	int nbkeys();
	void afficherkeys();
	CString value( int start, int size );
	void parsebegin();
	bool parseend();
	void parsenext();
	void parseread();
	bool isFinSection();
	bool isMobInit();
	bool isMobAdd();
	bool isDoor();
	bool isTeleporter();
	bool isMovement();
	bool isKilled();
	bool isWaypoint();
	bool isShopOpen();
	bool isShopItem();
	void getMobInitData();
	void getMobAddData();
	void getMobGeneralData(cmob* mob);
	void getDoorData();
	void getTeleporterData();
	void getMovementData();
	void getWaypointData();
	void getKilledData();
	void getShopOpenData();
	void getShopItemData();
	StringArray* formater( CString &s );
	
	//build file routines
	void load_build_file();
	void doMobInit(const PF_MobSpawn *it, bool is_add);
	void doMovement(const PF_MobMovement *it);
	void doWaypoint(const PF_MobLocation *it);
	void doKilled(const PF_Death *it);
	void doDeleted(const PF_DeleteSpawn *it);
	void doMerchantBegin(const PF_MerchantBegin *it);
	void doMerchantItem(const PF_MerchantItem *it);

	// scores
	float buildpos;
	int nbdoors();
	int nbteleports();
	int nbmobs();
	int nbnpcs();
	int nbgrids();
	int nbfspawns();
	int nbgspawns();
	int nbmerchants();
	int nbvalidspawns();
	CStatic* sDoorStatic;
	CStatic* sTeleportStatic;
	CStatic* sNPCStatic;
	CStatic* sMobStatic;
	CStatic* sSpawnStatic;
	CStatic* sGridStatic;
	CStatic* sMerchantStatic;
	CStatic* sRaceErrorStatic;
	void ResetScores();
	void CountMerchants(int &msets, int &mcount);

	//ID sources
	IDGenSet m_ids;

	//db loading
	CButton* bLoadDBButton;

	// output
	int outputtype;
	CButton* bWriteButton;
	CButton* bSQL;
	CButton* bDatabase;
	bool out_npcs;
	bool out_spawns;
	bool out_doors;
	bool out_teleports;
	bool out_grids;
	bool out_merchants;

	// npcs
	int m_raceErrors;
	npc_list* listNPCs;
	void getNPCData();
	cnpc *isNPCDejaSauve( const cnpc* npc );	//do we allready have this npc?
	int getNPCid( cnpc* npc );
	bool isMerchant ( CString cle );
	bool isSameNPC( const cnpc* npc1, const cnpc* npc2 );
	CString getClassName(int id);
	void processNPCData(const cmob *mob);
	
	// merchants
	merchant_list *listShops;
	cmerchant *cur_merchant;

	// spawns
	mob_list* fixedMobs;		//
//	spawn_list* listSpawns;		//this is filled by getSpawnData from the mob lists in each log
	spawn_list* fixedSpawns;	//filled by clusterFixedMobs from listSpawns
	spawn_list* gridSpawns;		//filled by splitSpawnData from listSpawns
//	void getSpawnData();
	bool IsValidSpawn( cmob* mob );
//	void CreateSpawns();
	void splitSpawnData();
	void splitSpawnList(const mob_list *list, int ci, int compcount, logType type);
	spawn_list *CleanSpawnList(spawn_list *from);
	//-------
	//used by splitSpawnData for roaming mobs:
	void updateSpawnPoint( cspawn *spawn, cmob* mob, bool addspawn, logType typelog );
	cspawn *getSpawnPoint( spawn_list* list, cmob* mob, LocDifference &diff );
	cspawn *getSpawnPointContaining( spawn_list *list, cmob* mob, LocDifference &diff );
	int getSpawnProbability( cspawn* spawn, logType typelog );
	void AddNewSpawn(spawn_list *list, cmob *mob, logType log_type, bool calc_prob);
	void cleanMobMovement(cmob *mob);
	void JoinFixedMobs();
	void JoinPathingMobs();
	//--------
	//mob clustering stuff
	void clusterFixedMobs();
	float MobDistance(cmob *left, cmob *right);
	float SpawnDistance(cmob *left, cspawn *right);
	bool WillMergeSpawns(cspawn *left, cspawn *right);
	bool spawnContainsLoc(cspawn *spawn, const cloc *loc);
	

	// filters
	CEdit* eAroundEdit;
	CButton* bAroundCheck;
	CEdit* eMinprobEdit;
	CButton* bMinprobCheck;
	CEdit* eOccurEdit;
	CButton* bOccurCheck;
	CEdit* eMovedEdit;
	CButton* bMovedCheck;
	CEdit* eDeadEdit;
	CButton* bDeadCheck;
	CButton* bMovingCheck;
	CEdit* eAffirmEdit;
	CButton* bAffirmCheck;
	CEdit* eCampEdit;
	CButton* bCampCheck;
	CEdit* eDeltazEdit;
	CButton* bDeltazCheck;
	CEdit* ePathingErrEdit;
	CButton* bPathingErrCheck;
	CEdit* eFixedMergeErrEdit;
	CButton* bFixedMergeErrCheck;
	filtre_struct filtres;

	// movements
	grid_list* listGrids;
	bool isMovingAtInit( cmob* mob );
	void extractGridData();
	void buildGridList();
	cgrid* getGrid( cmob* mob );
	int getGridIndex( cgrid* grid );
//	bool isInGrid( cspawn* spawn, int &gridid, int &wpindex );
	LocDifference isSameLoc( const cloc* loc1, const cloc* loc2 ) const;
	bool isSameLoc( const cloc* loc1, const cloc* loc2, const float err2 ) const;
	static bool isSameCoord( const cloc* loc1, const cloc* loc2 );
	static bool arePointsColinear(const cloc *pt1, const cloc *pt2, const cloc *pt3);
	bool pointCloseToLine( const cloc *Point, const cloc *LineStart, const cloc *LineEnd, float threshold2) const;


	// memoire
	void ReleaseMemory();
	void ClearBuildResults();

	// Generated message map functions
	//{{AFX_MSG(CEQBuilderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeZoneCombo();
	afx_msg void OnAddButton();
	afx_msg void OnSelchangedLogTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCompileButton();
	afx_msg void OnDoorButton();
	afx_msg void OnSqlRadio();
	afx_msg void OnDatabaseRadio();
	afx_msg void OnTeleportButton();
	afx_msg void OnNpcButton();
	afx_msg void OnAroundCheck();
	afx_msg void OnMinprobCheck();
	afx_msg void OnSelchangeTypelogCombo();
	afx_msg void OnMovedCheck();
	afx_msg void OnDeadCheck();
	afx_msg void OnOccurCheck();
	afx_msg void OnProcessButton();
	afx_msg void OnSpawnButton();
	afx_msg void OnGridButton();
	afx_msg void OnMerchantButton();
	afx_msg void OnMovingCheck();
	afx_msg void OnChangeMovedEdit();
	afx_msg void OnChangeDeadEdit();
	afx_msg void OnChangeOccurEdit();
	afx_msg void OnChangeMinprobEdit();
	afx_msg void OnChangeAroundEdit();
	afx_msg void OnDestroy();
	afx_msg void OnDatabaseConnection();
	afx_msg void OnOutputSqlopt();
	afx_msg void OnWriteButton();
	afx_msg void OnOutNpcs();
	afx_msg void OnOutSpawns();
	afx_msg void OnOutDoors();
	afx_msg void OnOutTeleports();
	afx_msg void OnOutGrids();
	afx_msg void OnOutMerchants();
	afx_msg void OnLoaddbButton();
	afx_msg void OnDatamode(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeAffirmEdit();
	afx_msg void OnAffirmCheck();
	afx_msg void OnChangeCampEdit();
	afx_msg void OnCampCheck();
	afx_msg void OnDeltazCheck();
	afx_msg void OnChangeDeltazEdit();
	afx_msg void OnZoneZoomIn();
	afx_msg void OnZoneZoomOut();
	afx_msg void OnZoneReset();
	afx_msg void OnZoneBig();
	afx_msg void OnReloadMap();
	afx_msg void OnFactionsButton();
	afx_msg void OnLootButton();
	afx_msg void OnSpellsButton();
	afx_msg void OnAddDirButton();
	afx_msg void OnDeleteAllButton();
	afx_msg void OnDeleteButton();
	afx_msg void OnCompileAllButton();
	afx_msg void OnCloseupZoneCombo();
	afx_msg void OnGoButton();
	afx_msg void OnSaveIds();
	afx_msg void OnResetIds();
	afx_msg void OnLoadIds();
	afx_msg void OnFixedMerge();
	afx_msg void OnPathingMerge();
	afx_msg void OnChangeFixedMergeEdit();
	afx_msg void OnChangePathMergeEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EQBUILDERDLG_H__1E1249E5_39BC_4670_9353_17544F903F7F__INCLUDED_)
