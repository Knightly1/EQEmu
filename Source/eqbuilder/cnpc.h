// npc.h: interface for the npc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CNPC_H__9DECED82_156F_4F4E_AEF6_5153F8EA4B73__INCLUDED_)
#define AFX_CNPC_H__9DECED82_156F_4F4E_AEF6_5153F8EA4B73__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/classes.h"

class cmerchant;

class cnpc  
{
public:
	cnpc();
	cnpc( const cnpc* npc );
	virtual ~cnpc();

	bool db;
	//int id;
	int db_id;
	CString nom;
	CString last_name;
	int level;
	int gender;
	double size;
	double walkspeed;
	double runspeed;
	int race;
	int type;
	int classe;
	int texture;
	int helmtexture;
	int face;
    int luclin_hairstyle;
	int luclin_haircolor;
	int luclin_eyecolor;
	int luclin_eyecolor2;
	int luclin_beard;
	int luclin_beardcolor;
	cmerchant *merchant;
	
	int faction_id;
	int loot_id;
	int spells_id;

	bool IsSameAs(const cnpc *other, bool compare_level) const;
	bool IsSpecial() const { return(classe > PLAYER_CLASS_COUNT); }
};

#endif // !defined(AFX_CNPC_H__9DECED82_156F_4F4E_AEF6_5153F8EA4B73__INCLUDED_)
