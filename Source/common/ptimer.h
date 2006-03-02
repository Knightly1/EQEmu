/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef PTIMER_H
#define PTIMER_H

#include "types.h"
#include <map>
#include <vector>
using namespace std;

enum {	//values for pTimerType
	pTimerStartAdventureTimer = 1,
	pTimerAdventureTimer = 2,
	pTimerFeignDeath = 3,
	pTimerSneak = 4,
	pTimerHide = 5,
	pTimerTaunt = 6,
	pTimerInstillDoubt = 7,
	pTimerFishing = 8,
	pTimerForaging = 9,
	pTimerMend = 10,
	pTimerTracking = 11,
	pTimerSenseTraps = 12,
	pTimerDisarmTraps = 13,
	pTimerDisciplineReuse = 14,
	pTimerCombatAbility = 15,
	
	pTimerLayHands = 87,		//these IDs are used by client too
	pTimerHarmTouch = 89,		//so dont change them
	
	pTimerAAStart = 1000,		//AA re-use timers
	pTimerAAEnd = 1999,
	pTimerAAEffectStart = 2001,	//AA effect timers
	pTimerAAEffectEnd	= 2999,
	
	pTimerSpellStart = 5000		//Do not put any timer IDs above this one
								//if needed, increase its starting ID
};

typedef uint16 pTimerType;

class PersistentTimer {
public:
	static PersistentTimer *LoadTimer(int32 char_id, pTimerType type);
	
	PersistentTimer(int32 char_id, pTimerType type, int32 duration);
	PersistentTimer(int32 char_id, pTimerType type, int32 start_time, int32 duration, bool enable);

	bool Expired(bool iReset = true);
	void Start(int32 set_timer_time=0);
	
	void SetTimer(int32 set_timer_time=0);
	int32 GetRemainingTime();
	inline void Enable() { enabled = true; }
	inline void Disable() { enabled = false; }
	inline const int32 GetTimerTime() const { return timer_time; }
	inline const int32 GetStartTime() const { return start_time; }
	inline const pTimerType GetType() const { return _type; }

	inline bool Enabled() { return enabled; }

	bool Load();
	bool Store();
	bool Clear();

protected:
	int32 get_current_time();
	
	int32	start_time;
	int32	timer_time;
	bool	enabled;
	
	int32 _char_id;
	pTimerType _type;
};

//a list of persistent timers for a specific character
class PTimerList {
public:
	PTimerList(int32 char_id = 0);
	
	~PTimerList();
	
	bool Load();
	bool Store();
	bool Clear();
	
	void Start(pTimerType type, int32 duration);
	bool Expired(pTimerType type, bool reset = true);
	void Clear(pTimerType type);
	void Enable(pTimerType type);
	bool Enabled(pTimerType type);
	void Disable(pTimerType type);
	int32 GetRemainingTime(pTimerType type);
	PersistentTimer *Get(pTimerType type);
	
	inline void SetCharID(int32 char_id) { _char_id = char_id; }
	
	void ToVector(vector< pair<pTimerType, PersistentTimer *> > &out);
	
	//Clear a timer for a char not logged in
	//this is not defined on a char which is logged in!
	static bool ClearOffline(int32 char_id, pTimerType type);
	
	typedef map<pTimerType, PersistentTimer *>::iterator iterator;
	iterator begin() { return(_list.begin()); }
	iterator end() { return(_list.end()); }
protected:
	int32 _char_id;
	
	map<pTimerType, PersistentTimer *> _list;
};

//code prettying macros
#define AA_Choose3(val, v1, v2, v3) (val==1?v1:(val==2?v2:v3))
#define AA_Choose5(val, v1, v2, v3, v4, v5) (val==1?v1:(val==2?v2:(val==3?v3:(val==4?v4:v5))))

#endif
