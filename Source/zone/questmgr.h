/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2004  EQEMu Development Team (http://eqemulator.net)

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
#ifndef __QUEST_MANAGER_H__
#define __QUEST_MANAGER_H__

#include "../common/Mutex.h"

class NPC;
class Client;

class QuestManager {
public:
	QuestManager();
	virtual ~QuestManager();
	
	void StartQuest(NPC *_npc, Client *_initiator = NULL);
	void EndQuest();
	
	void Process();
	
	void ClearTimers(NPC *who);
	
	//quest perl functions
	void echo(const char *str);
	void say(const char *str);
	void me(const char *str);
	void summonitem(int32 itemid, uint8 charges = 0);
	//getZoneID(const char *short_name)
	void write(const char *file, const char *str);
	int16 spawn2(int npc_type, int grid, int unused, float x, float y, float z, float heading);
	int16 unique_spawn(int npc_type, int grid, int unused, float x, float y, float z, float heading = 0);
	void setstat(int stat, int value);
	void castspell(int spell_id, int target_id);
	void selfcast(int spell_id);
	void addloot(int item_id, int charges = 0);
	void Zone(const char *zone_name);
	void settimer(const char *timer_name, int seconds);
	void stoptimer(const char *timer_name);
	void emote(const char *str);
	void shout(const char *str);
	void shout2(const char *str);
	void depop(int npc_type = 0);
	void settarget(const char *type, int target_id);
	void follow(int entity_id);
	void sfollow();
	void cumflag();
	void flagnpc(int32 flag_num, int8 flag_value);
	void flagcheck(int32 flag_to_check, int32 flag_to_set);
	//bool isflagset(int flag_num);
	void changedeity(int diety_id);
	//flagmob->CastToClient() WTF??
	void exp(int amt);
	void level(int newlevel);
	void traindisc(int discipline_tome_item_id);
	bool isdisctome(int item_id);
	void safemove();
	void rain(int weather);
	void snow(int weather);
	void surname(const char *name);
	void permaclass(int class_id);
	void permarace(int race_id);
	void permagender(int gender_id);
	void scribespells();
	void givecash(int copper, int silver, int gold, int platinum);
	void pvp(const char *mode);
	void movepc(int zone_id, float x, float y, float z);
	void gmmove(float x, float y, float z);
	void movegrp(int zoneid, float x, float y, float z);
	void doanim(int anim_id);
	void addskill(int skill_id, int value);
	void setlanguage(int skill_id, int value);
	void setskill(int skill_id, int value);
	void setallskill(int value);
	void attack(const char *client_name);
	void save();
	void faction(int faction_id, int faction_value);
	void setsky(uint8 new_sky);
	void setguild(int32 new_guild_id, int8 new_rank);
	void settime(int8 new_hour, int8 new_min);
	void itemlink(int item_id);
	void signal(int npc_id, int wait_ms = 0);
	void signalwith(int npc_id, int signal_id, int wait_ms = 0);
	void setglobal(const char *varname, const char *newvalue, int options, const char *duration);
	void targlobal(const char *varname, const char *value, const char *duration, int npcid, int charid, int zoneid);
	void delglobal(const char *varname);
	void ding();
	void rebind(int zoneid, float x, float y, float z);
	void start(int wp);
	void stop();
	void pause(int duration);
	void moveto(float x, float y, float z);
	void resume();
	void addldonpoints(sint32 points, int32 theme);
	void setnexthpevent(int at);
	void respawn(int npc_type, int grid);
	void set_proximity(float minx, float maxx, float miny, float maxy, float minz=-999999, float maxz=999999);
	void clear_proximity();
	
	//not in here because it retains perl types
	//thing ChooseRandom(array_of_things)
	
	inline Client *GetInitiator() const { return(initiator); }
	inline NPC *GetNPC() const { return(npc); }
protected:
	NPC *npc;	//NPC is never NULL when functions are called.
	Client *initiator;	//this can be null.
	
	bool depop_npc;	//true if EndQuest should depop the NPC
	
	Mutex quest_mutex;
	
	static int32 QGexpdate(const char * name, const char * options);


	class QuestTimer {
	public:
		inline QuestTimer(int duration, NPC *_mob, string _name) : mob(_mob), name(_name), Timer_(duration) { Timer_.Start(duration, false); }
		NPC*   mob;
		string name;
		Timer Timer_;
	};
	class SignalTimer {
	public:
		inline SignalTimer(int duration, int _npc_id, int _signal_id) : npc_id(_npc_id), signal_id(_signal_id), Timer_(duration) { Timer_.Start(duration, false); }
		int npc_id;
		int signal_id;
		Timer Timer_;
	};
	list<QuestTimer>	QTimerList;
	list<SignalTimer>	STimerList;

};

extern QuestManager quest_manager;

#endif

