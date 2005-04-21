#if !defined(AFX_EMUCOLLECTDIALOG_H__EAEF2C9D_773C_4538_9968_6EF57F1AE7C8__INCLUDED_)
#define AFX_EMUCOLLECTDIALOG_H__EAEF2C9D_773C_4538_9968_6EF57F1AE7C8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EMUCollectDialog.h : header file
//

class MobInfoDlg;

/////////////////////////////////////////////////////////////////////////////
// EMUCollectDialog dialog

#pragma pack(1)

//Do not re-order, insert into, or delete from this struct.
//add fields to the end or rename fields to unused.
struct PrefsFileData {
	char name_opcodes;
	char ignore_world;
	char privacy_mode;
	char text_logging;
	char binary_logging;
	char debug_console;
	
	char text_file[1024];
	char binary_file[1024];
	char privacy_name[128];
	char ipaddr[64];	//overkill
	int pcap_device;
	char mob_info;
	char force_flush;
};
#pragma pack()

class EMUCollectDialog : public CDialog
{
// Construction
public:
	EMUCollectDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(EMUCollectDialog)
	enum { IDD = IDD_EMUCOLLECT_DIALOG };
	CButton	m_ForceFlush;
	CButton	m_OnMobInfo;
	CButton	m_MobInfo;
	CButton	m_StopButton;
	CStatic	m_PrivacyLabel;
	CEdit	m_PrivacyName;
	CButton	m_PrivacyMode;
	CButton	m_IgnoreWorld;
	CButton	m_DebugConsole;
	CStatic	m_StatsItems;
	CStatic	m_StatsPackets;
	CComboBox	m_Device;
	CStatic	m_StatsRunning;
	CStatic	m_StatsOpcodes;
	CButton	m_RawLogging;
	CButton	m_ShowOpcodes;
	CStatic	m_UploadLabel;
	CEdit	m_WatchIP;
	CEdit	m_UploadName;
	CButton	m_TextLogs;
	CEdit	m_TextFile;
	CButton	m_BinLogs;
	CEdit	m_BinFile;
	CButton	m_DBUpload;
	CButton	m_CollectZones;
	CButton	m_CollectSpawns;
	CButton	m_CollectObjects;
	CButton	m_CollectItems;
	CButton	m_CollectGrids;
	CButton	m_CollectDoors;
	CButton	m_StartButton;
	CButton	m_SC_group;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(EMUCollectDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
public:
		
	void CollectionComplete();

protected:
	HICON m_hIcon;
	
	// file dialogs
	CFileDialog *SavePacketText;
	CFileDialog *SavePacketFile;
	CFileDialog *SaveOpcodeList;
	CFileDialog *OpenOpcodeList;

	//radio buttons:
	CButton *sItemsOnlyRadio;
	CButton *sAllPacketsRadio;
	CButton *sNoFilterRadio;
	CButton *sIncludeFilterRadio;
	CButton *sExcludeFilterRadio;

	bool dotextlogs;
	bool dobinlogs;


	enum { FILTER_NONE, FILTER_INCLUDE, FILTER_EXCLUDE }
		filterMode;
	
	void ReadPrefsFile();
	void WritePrefsFile();


	bool StartCollectThread();

	// Generated message map functions
	//{{AFX_MSG(EMUCollectDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDBUploadCheck();
	afx_msg void OnShowOpcodes();
	afx_msg void OnCollectItems();
	afx_msg void OnCollectDoors();
	afx_msg void OnCollectObjects();
	afx_msg void OnCollectZones();
	afx_msg void OnCollectSpawns();
	afx_msg void OnCollectGrids();
	afx_msg void OnTextlogs();
	afx_msg void OnBinlogs();
	afx_msg void OnSelectTextLog();
	afx_msg void OnSelectBinLog();
	afx_msg void OnFilterNone();
	afx_msg void OnFilterInclude();
	afx_msg void OnFilterExclude();
	afx_msg void OnOpiAdd();
	afx_msg void OnOpiRemove();
	afx_msg void OnOpiLoad();
	afx_msg void OnOpiSave();
	afx_msg void OnOpeAdd();
	afx_msg void OnOpeRemove();
	afx_msg void OnOpeLoad();
	afx_msg void OnOpeSave();
	afx_msg void OnBeginCollecting();
	afx_msg void OnChangeWatchIp();
	afx_msg void OnItemsonly();
	afx_msg void OnRawLogging();
	afx_msg void OnAllpackets();
	afx_msg void OnStopCollecting();
	afx_msg void OnDebugConsole();
	afx_msg void OnAboutPrivacy();
	afx_msg void OnIgnoreWorld();
	afx_msg void OnPrivacyMode();
	afx_msg void OnExit();
	afx_msg void OnSaveSettings();
	afx_msg void OnMobInfo();
	afx_msg void OnOnMobInfo();
	afx_msg void OnForceFlush();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMUCOLLECTDIALOG_H__EAEF2C9D_773C_4538_9968_6EF57F1AE7C8__INCLUDED_)
