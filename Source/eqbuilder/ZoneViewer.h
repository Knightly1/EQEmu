#if !defined(AFX_ZONEVIEWER_H__8A5FE6C2_E1F8_411A_95B8_21BA7DA2D7BB__INCLUDED_)
#define AFX_ZONEVIEWER_H__8A5FE6C2_E1F8_411A_95B8_21BA7DA2D7BB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ZoneViewer1.h : header file
//
#pragma warning(disable:4786)
#include <vector>
using namespace std;

class cspawn;
class cnpc;
class spawn_list;
class TextMapReader;

#define ZONEVIEWER_CLASSNAME	_T("MFCZoneViewerCtrl")  // Window class name


/////////////////////////////////////////////////////////////////////////////
// CZoneViewer window

class CZoneViewer : public CWnd
{
	DECLARE_DYNCREATE(CZoneViewer)

// Construction
public:
	CZoneViewer();

// Attributes
public:
	void HiliteSpawn(cspawn *spawn) { m_hiliteSpawn = spawn; m_hiliteNPC = NULL; m_redraw = true; }
	void HiliteNPC(cnpc *npc) { m_hiliteSpawn = NULL; m_hiliteNPC = npc; m_redraw = true; }
	void ClearSpawnLists() { m_lists.clear(); m_redraw = true; m_hiliteSpawn = NULL; m_hiliteNPC = NULL; }
	void AddSpawnList(spawn_list *list) { m_lists.push_back(list); m_redraw = true; }
	void SetScale(int s) { m_scale = s; m_redraw = true; }
	//not sure if we should redraw here:
	void SetMinSpawnRadius(float err) { m_min_spawn_radius = err; }
	void SetMapReader(TextMapReader *tmr) { m_map = tmr; m_redraw = true; }
	void ToggleDrawPaths() { m_drawPaths = !m_drawPaths; m_redraw = true; }
	void ToggleDrawHilite() { m_drawHilite = !m_drawHilite; m_redraw = true; }

// Operations
public:
	static BOOL RegisterWindowClass();
	void ZoomIn();
	void ZoomOut();
	void ResetView();
	void IncreaseResolution();
	void DecreaseResolution();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CZoneViewer)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CZoneViewer();
protected:

	void RedrawZone(CDC *pDC);
	void FindBounds();

protected:
	//our spawn lists and stuff
	cspawn *m_hiliteSpawn;
	cnpc *m_hiliteNPC;
	vector<spawn_list *> m_lists;
	TextMapReader *m_map;
	bool m_drawPos;
	CPoint m_posPoint;

	bool spawnContainsNPC(cspawn *spawn, cnpc *npc);

	//drag position in the source bitmap
	int m_top;
	int m_left;
	bool m_bTrack;
	CPoint m_dragBase;

	int m_zoomPercent;	//percent zoom. 1000 = full zone on screen

	//bitmap drawing info
	bool m_redraw;
	CBitmap *m_zone;
	int m_zone_height;
	int m_zone_width;
	float m_scale;		//number of times to scale up the main image.
						//this should be < 1 so we do not make massive bitmaps
	bool m_drawPaths;
	bool m_drawHilite;


	float m_min_spawn_radius;	//radius of points which builder considers the same point

	//bounds on spawn coordinates
	float min_x;
	float min_y;
	float max_x;
	float max_y;

	// Generated message map functions
protected:
	//{{AFX_MSG(CZoneViewer)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CZoneViewer)
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CZoneViewer)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ZONEVIEWER1_H__8A5FE6C2_E1F8_411A_95B8_21BA7DA2D7BB__INCLUDED_)
