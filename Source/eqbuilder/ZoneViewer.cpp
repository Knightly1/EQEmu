// ZoneViewer.cpp : implementation file
//

#include "stdafx.h"
#include "ZoneViewer.h"
#include "spawn_list.h"
#include "cspawn.h"
#include "cloc.h"
#include "../common/TextMapFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CZoneViewer

IMPLEMENT_DYNCREATE(CZoneViewer, CWnd)

CZoneViewer::CZoneViewer()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
	
	
//    RegisterWindowClass();
	m_redraw = true;
	m_top = 0;
	m_left = 0;
	m_zoomPercent = 1000;
	m_scale = 0.2f;
	m_bTrack = false;
	m_drawPaths = false;
	m_drawHilite = false;
	m_drawPos = false;

	m_min_spawn_radius = 5;

	m_zone_width = 10;
	m_zone_height = 10;

	m_zone = NULL;

	m_map = NULL;
	m_hiliteSpawn = NULL;
	m_hiliteNPC = NULL;
}

CZoneViewer::~CZoneViewer()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	if(m_zone != NULL)
		delete m_zone;

	AfxOleUnlockApp();
}

// Register the window class if it has not already been registered.
BOOL CZoneViewer::RegisterWindowClass()
{
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if (!(::GetClassInfo(hInst, ZONEVIEWER_CLASSNAME, &wndcls)))
    {
        // otherwise we need to register a new class
        wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc      = ::DefWindowProc;
        wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
        wndcls.hInstance        = hInst;
        wndcls.hIcon            = NULL;
        wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndcls.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wndcls.lpszMenuName     = NULL;
        wndcls.lpszClassName    = ZONEVIEWER_CLASSNAME;

        if (!AfxRegisterClass(&wndcls))
        {
            AfxThrowResourceException();
            return FALSE;
        }
    }

    return TRUE;
}


BEGIN_MESSAGE_MAP(CZoneViewer, CWnd)
	//{{AFX_MSG_MAP(CZoneViewer)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CZoneViewer message handlers

void CZoneViewer::OnPaint() 
{
	PAINTSTRUCT ps;
	CDC *pDC = BeginPaint(&ps);

/*	CPaintDC dc(this); // device context for painting
	
    // Create memory DC
    CDC MemDC;
    if (!MemDC.CreateCompatibleDC(&dc))
        return;

    // Get Size of Display area
    CRect rect;
    GetClientRect(rect);

    
    // Get size of bitmap
    BITMAP bm;
    m_zone.GetBitmap(&bm);
        
    // Draw the piece of the bitmap we want
    CBitmap* pOldBitmap = (CBitmap*) MemDC.SelectObject(&m_zone);
    dc.StretchBlt(0, 0, rect.Width(), rect.Height(), 
                  &MemDC, 
                  m_left, m_top, (bm.bmWidth * m_zoomPercent)/1000, (bm.bmHeight*m_zoomPercent)/1000, 
                  SRCCOPY);
    MemDC.SelectObject(pOldBitmap);
	
	// Do not call CWnd::OnPaint() for painting messages
*/

	if(m_redraw)
		RedrawZone(pDC);

	CBrush brFrame(RGB(255, 255, 255));

	CRect rect;
	GetClientRect(rect);
//	pDC->FillRect(&rect, &brFrame);
	
    // Get size of bitmap
    BITMAP bm;
    m_zone->GetBitmap(&bm);
    // Create memory DC
    CDC MemDC;
    if (!MemDC.CreateCompatibleDC(pDC))
        return;
	// point the new DC at our bitmap
    CBitmap* pOldBitmap = (CBitmap*) MemDC.SelectObject(m_zone);
    // Draw the piece of the bitmap we want
    pDC->StretchBlt(0, 0, rect.Width(), rect.Height(), 
                  &MemDC, 
                  m_left, m_top, (bm.bmWidth * m_zoomPercent)/1000, (bm.bmHeight*m_zoomPercent)/1000, 
                  SRCCOPY);
	if(m_drawPos) {
		//fighting stupid shit.
		rect.left++;
		rect.right--;
		rect.top++;
		rect.bottom--;

		float xadd = -min_x;
		float yadd = -min_y;
		float screenx0 = m_left;//float(m_left)/m_scale - xadd;
		float screeny0 = m_top;//float(m_top)/m_scale - yadd;
		float screenx1 = ((float(m_left+bm.bmWidth) * m_zoomPercent)/1000)/m_scale - xadd;
		float screeny1 = ((float(m_top+bm.bmHeight) * m_zoomPercent)/1000)/m_scale - yadd;

		float xpos = screenx0 + (screenx1-screenx0)*(float(m_posPoint.x-rect.left) / float(rect.right-rect.left));
		float ypos = screeny0 + (screeny1-screeny0)*(float(m_posPoint.y-rect.top) / float(rect.bottom-rect.top));
		
		rect.SetRect(rect.left, 20, rect.right, 40);
		CString txt;
		txt.Format("Bounds (%.2f->%.2f, %.2f->%.2f)", screenx0, screenx1, screeny0, screeny1);
		pDC->DrawText(txt, rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

		rect.SetRect(0, 0, 200, 30);
		txt.Format("Click (%.2f,%.2f)", xpos, ypos);
		pDC->DrawText(txt, rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
	}

    MemDC.SelectObject(pOldBitmap);

	

	EndPaint(&ps);
}

BOOL CZoneViewer::OnEraseBkgnd(CDC* pDC) 
{

	//we always paint our area, so dont bother with erasing
	return(TRUE);
	
	//return CWnd::OnEraseBkgnd(pDC);
}

/*void CZoneViewer::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	// In our case this is not needed - yet - so just drop through to
    // the base class
	CWnd::PreSubclassWindow();
}*/

/////////////////////////////////////////////////////////////////////////////
// CZoneViewer methods

//BOOL CZoneViewer::Create(CWnd* pParentWnd, const RECT& rect, UINT nID, DWORD dwStyle /*=WS_VISIBLE*/)
//{
//	return CWnd::Create(ZONEVIEWER_CLASSNAME, _T(""), dwStyle, rect, pParentWnd, nID);
//}

void CZoneViewer::FindBounds() {

	//TODO: we could include the map in here, and reduce the padding

	min_x = 999999;
	min_y = 999999;
	max_x = -999999;
	max_y = -999999;
	vector<spawn_list *>::iterator cur, end;
	cur = m_lists.begin();
	end = m_lists.end();
	for(; cur != end; cur++) {
		spawn_list *l = *cur;
		int r,max;
		max = l->getsize();
		for(r = 0; r < max; r++) {
			cspawn *spawn = l->get(r);
			int k,lmax;
			lmax = spawn->locs->getsize();
			for(k = 0; k < lmax; k++) {
				cloc *loc = spawn->locs->get(k);
				if(loc->x < min_x)
					min_x = loc->x;
				if(loc->y < min_y)
					min_y = loc->y;
				if(loc->x > max_x)
					max_x = loc->x;
				if(loc->y > max_y)
					max_y = loc->y;
			}
		}
	}
	if(min_x == 999999) {
		//assume we had no points, fake a non-huge and non-zero bounds
		min_x = -10;
		min_y = -10;
		max_x = 10;
		max_y = 10;
	}
	if((max_x-min_x) < 600) {
		min_x -= 600;
		max_x += 600;
	}
	if((max_y-min_y) < 600) {
		min_y -= 600;
		max_y += 600;
	}

	//add a little padding to make the boundary points not right on the edges
#define PAD_MIN(x) x = (x<0? x*1.05 : x*0.95)
#define PAD_MAX(x) x = (x<0? x*0.95 : x*1.05)
	PAD_MIN(min_x);
	PAD_MIN(min_y);
	PAD_MAX(max_x);
	PAD_MAX(max_y);
#undef PAD_MIN
#undef PAD_MAX
}


bool CZoneViewer::spawnContainsNPC(cspawn *spawn, cnpc *npc) {
	if(npc == NULL || spawn == NULL)
		return(false);
	int max = spawn->mobs->getsize();
	int r;
	for(r = 0; r < max; r++) {
		cmob *mob = spawn->mobs->get(r);
		if(npc->IsSameAs(mob->npc, true)) {
			return(true);
		}
	}
	return(false);
}

void CZoneViewer::RedrawZone(CDC *pDC) {
	//determine min and max values of x and y
	FindBounds();
	
	float xadd = -min_x;
	float yadd = -min_y;
	int width = m_zone_width = int((max_x - min_x + 1)*m_scale);
	int height = m_zone_height = int((max_y - min_y + 1)*m_scale);

    // Create memory DC
    CDC MemDC;
    if (!MemDC.CreateCompatibleDC(GetDC())) {
		m_redraw = false;
        return;
	}
	
	if(m_zone != NULL)
		delete m_zone;
	m_zone = new CBitmap();
	m_zone->CreateCompatibleBitmap(GetDC(), width, height);
	
	//select our bitmap with the memory dc
    CBitmap* pOldBitmap = (CBitmap*) MemDC.SelectObject(m_zone);
	CDC *draw = &MemDC;
//	CDC *draw = pDC;
	
	//paint the base rectangle.

	float x,y;
	int ix, iy;
	int sphere_mask = 0x80;
	
	COLORREF ccolor;
	HBRUSH hbrush, hbrushOld;
	HPEN hpen, hpenOld;
	
	//paint the bitmap white, with black border
	hpen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	hbrush = CreateSolidBrush(RGB(150, 210, 150));
	hpenOld = (HPEN) draw->SelectObject(hpen);
	hbrushOld = (HBRUSH) draw->SelectObject(hbrush);
	draw->Rectangle(0, 0, width, height);

	//draw axes
	DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, RGB(150, 150, 150))));
	draw->MoveTo(0, int(yadd*m_scale));
	draw->LineTo(width, int(yadd*m_scale));
	draw->MoveTo(int(xadd*m_scale), 0);
	draw->LineTo(int(xadd*m_scale), height);

	//draw map
	if(m_map != NULL) {
		vector<TextMapLine>::const_iterator cur,end;
		cur = m_map->begin_lines();
		end = m_map->end_lines();
		for(; cur != end; cur++) {
			const TextMapLine &ml = *cur;
			
			//Sony's maps have inverted XY
			DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, 
				RGB(ml.color.r, ml.color.g, ml.color.b))));
			draw->MoveTo(int((ml.start.y + xadd)*m_scale), int((ml.start.x + yadd)*m_scale));
			draw->LineTo(int((ml.end.y + xadd)*m_scale), int((ml.end.x + yadd)*m_scale));
		}
	}

	//clear the 'pen' for our radius drawing.
	DeleteObject(draw->SelectObject(CreatePen(PS_NULL, 1, RGB(0, 0, 0))));

	vector<spawn_list *>::iterator cur, end;

	//draw paths first if they are enabled.
	if(m_drawPaths || m_drawHilite) {
		//draw grids in gery
		DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, RGB(0xCC, 0xCC, 0xCC))));

		cur = m_lists.begin();
		end = m_lists.end();
		for(; cur != end; cur++) {
			spawn_list *l = *cur;
			int r,max;
			max = l->getsize();
			for(r = 0; r < max; r++) {
				cspawn *spawn = l->get(r);
				
				if(spawn->grid == NULL || spawn->grid->waypoints == NULL)
					continue;
				
				waypoint_list *wpl = spawn->grid->waypoints;
				
				if(wpl->getsize() < 2)
					continue;

				int i;
				int max = wpl->getsize();
				cwaypoint *lwp = wpl->get(0);
				
				int mode = 0;
				
				bool hilite;
				if(m_drawHilite)
					hilite = (spawn == m_hiliteSpawn || spawnContainsNPC(spawn, m_hiliteNPC));
				else
					hilite = false;
				if(!m_drawPaths && !hilite)
					continue;

				for(i = 1; i < max; i++) {
					cwaypoint *wp = wpl->get(i);
					int xmin = int((lwp->loc->x+xadd)*m_scale);
					int xmax = int((wp->loc->x+xadd)*m_scale);
					int ymin = int((lwp->loc->y+yadd)*m_scale);
					int ymax = int((wp->loc->y+yadd)*m_scale);
					
					if(!wp->valid) {
						if(mode != 1) {
							DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, RGB(0xCC, 0x44, 0x44))));
							mode = 1;
						}
					} else if(wp->colinear) {
						if(mode != 2) {
							DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, RGB(0x44, 0x44, 0xCC))));
							mode = 2;
						}
					} else if(spawn == m_hiliteSpawn || hilite) {
						if(mode != 4) {
							DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, RGB(0xBB, 0x22, 0xBB))));
							mode = 4;
						}
					} else {
						if(mode != 3) {
							DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, RGB(0xCC, 0xCC, 0xCC))));
							mode = 3;
						}
					}
					
					draw->MoveTo(xmin, ymin);
					draw->LineTo(xmax, ymax);

					lwp = wp;
				}
				
			}
		}
	}
	
	DeleteObject(draw->SelectObject(CreatePen(PS_SOLID, 1, RGB(0x77, 0x77, 0xFF))));

	//draw all the spawn 'spheres' first, since they likely overlap
	cur = m_lists.begin();
	end = m_lists.end();
	for(; cur != end; cur++) {
		spawn_list *l = *cur;
		int r,max;
		max = l->getsize();
		for(r = 0; r < max; r++) {
			cspawn *spawn = l->get(r);
			bool hilite;
			if(m_drawHilite)
				hilite = (spawn == m_hiliteSpawn || spawnContainsNPC(spawn, m_hiliteNPC));
			else
				hilite = false;
			
			//select a color for this spawn.
			if(hilite) {
				spawn->r = 0xAA;
				spawn->g = 0x00;
				spawn->b = 0xAA;
			} else if(spawn->roaming) {
				uint32 ran = rand();
				spawn->r = ran&0xFF;
				spawn->g = (ran&0xFF00)>>8;
				spawn->b = (ran&0xFF0000)>>16;
			} else {
				spawn->r = 0;
				spawn->g = 0;
				spawn->b = 255;
			}

			//get the 'sphere' for this spawn group
			//we allready store the center in the spawn
			float radius = 0;
			spawn->locs->get2Dradius(&spawn->center, &radius);
			if(radius < m_min_spawn_radius)
				radius = m_min_spawn_radius;
			if(hilite)
				radius *= 10;

			//TODO: draw our sphere
			//select our sphere color, 'transparent looking.
			DeleteObject(draw->SelectObject(CreateSolidBrush(
				RGB(spawn->r|sphere_mask, spawn->g|sphere_mask, spawn->b|sphere_mask))));
			//draw the sphere.
			int xmin = int((spawn->center.x+xadd-radius)*m_scale);
			int xmax = int((spawn->center.x+xadd+radius)*m_scale);
			int ymin = int((spawn->center.y+yadd-radius)*m_scale);
			int ymax = int((spawn->center.y+yadd+radius)*m_scale);
			draw->Ellipse(xmin, ymin,
						  xmax, ymax);
		}
	}
	
	//now draw the spawn points on top of the spawn spheres
	cur = m_lists.begin();
	end = m_lists.end();
	for(; cur != end; cur++) {
		spawn_list *l = *cur;
		int r,max;
		max = l->getsize();
		for(r = 0; r < max; r++) {
			cspawn *spawn = l->get(r);
			int k,lmax;
			lmax = spawn->locs->getsize();
			
			ccolor = RGB(spawn->r, spawn->g, spawn->b);
//ccolor = RGB(0, 0, 255);

			for(k = 0; k < lmax; k++) {
				cloc *loc = spawn->locs->get(k);
				
				//scale the coords down.
				x = (loc->x+xadd) * m_scale;
				y = (loc->y+yadd) * m_scale;
				
				//convert to integer bitmap coordinates
				ix = int(x);
				iy = int(y);
				
				//draw the point
				draw->SetPixel(ix, iy, ccolor);
			}
		}
	}

	DeleteObject(draw->SelectObject(hpenOld));
	draw->SelectObject(pOldBitmap);
	DeleteObject(draw->SelectObject(hbrushOld));

	m_redraw = false;
}

void CZoneViewer::ResetView() {
	//m_scale /= 0.75;
	m_zoomPercent = 1000;
	m_top = 0;
	m_left = 0;
	RedrawWindow();
}

void CZoneViewer::ZoomIn() {
	//m_scale /= 0.75;
	m_zoomPercent = (m_zoomPercent * 75) / 100;
	RedrawWindow();
}

void CZoneViewer::ZoomOut() {
	//m_scale *= 0.75;
	m_zoomPercent = (m_zoomPercent * 100) / 75;
	RedrawWindow();
}

void CZoneViewer::IncreaseResolution() {
	m_scale /= 0.75;
	m_redraw = true;
	RedrawWindow();
}

void CZoneViewer::DecreaseResolution() {
	m_scale *= 0.75;
	m_redraw = true;
	RedrawWindow();
}

void CZoneViewer::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bTrack)
	{
		
		int base_dx = m_dragBase.x - point.x;
		int base_dy = m_dragBase.y - point.y;
		
		int dx = (base_dx * m_zoomPercent) / 1000;
		int dy = (base_dy * m_zoomPercent) / 1000;
		if(dx == 0 && base_dx > 0)
			dx = 1;
		if(dy == 0 && base_dy > 0)
			dy = 1;


		m_left = m_left + dx;
		m_top = m_top + dy;

		//bounds
/*		if(m_left <= -(m_zone_width+20))
			m_left = -(m_zone_width+20);
		if(m_left >= (m_zone_width-20))
			m_left = (m_zone_width-20);
		if(m_top <= -(m_zone_height+20))
			m_top = -(m_zone_height+20);
		if(m_top >= (m_zone_height-20))
			m_top = (m_zone_height-20);
*/		
		m_dragBase = point;
		m_drawPos = false;

		RedrawWindow();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CZoneViewer::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_drawPos = false;
	if (m_bTrack)
	{
		//::ReleaseDC(m_hWnd, m_DrawDC.Detach());
		m_bTrack = false;
		ReleaseCapture();

		RedrawWindow();
	}
		
	CWnd::OnLButtonUp(nFlags, point);
}

void CZoneViewer::OnLButtonDown(UINT nFlags, CPoint point) 
{
	m_drawPos = false;
	//m_DrawDC.Attach(::GetDC(m_hWnd));
	m_bTrack = true;
	SetCapture();

	m_dragBase = point;

	CWnd::OnLButtonDown(nFlags, point);

	OnMouseMove(0, point);
}

void CZoneViewer::OnRButtonDown(UINT nFlags, CPoint point) 
{
	m_drawPos = true;
	m_posPoint = point;

	CWnd::OnRButtonDown(nFlags, point);

	RedrawWindow();
}

void CZoneViewer::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CWnd::OnFinalRelease();
}


BEGIN_DISPATCH_MAP(CZoneViewer, CWnd)
	//{{AFX_DISPATCH_MAP(CZoneViewer)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


static const IID IID_IZoneViewer =
{ 0x17282c3f, 0x5894, 0x4826, { 0x9a, 0x9b, 0xbc, 0xb0, 0xa2, 0xbd, 0x7, 0x8a } };

BEGIN_INTERFACE_MAP(CZoneViewer, CWnd)
	INTERFACE_PART(CZoneViewer, IID_IZoneViewer, Dispatch)
END_INTERFACE_MAP()

// {70E3F2F5-37D0-493E-B4B5-2FC4341843C5}
IMPLEMENT_OLECREATE(CZoneViewer, "MFCZoneViewerCtrl", 0x70e3f2f5, 0x37d0, 0x493e, 0xb4, 0xb5, 0x2f, 0xc4, 0x34, 0x18, 0x43, 0xc5)

