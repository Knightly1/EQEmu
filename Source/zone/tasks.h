/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2005  EQEMu Development Team (http://eqemulator.net)

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
#ifndef TASKS_H
#define TASKS_H

#include "../common/types.h"
#include <vector>
#include <string>
#include <queue>

class Client;
class EQStream;

using namespace std;

class ClientTaskState {
	class ActivityProgress {
		uint32 goal_count;  //number achieved towards goal.
	};
	class TaskProgress {
		vector<ActivityProgress> activities;
	};
	
	map<uint32, uint32> completed;  //map from task ID to completed time
	map<uint32, TaskProgress> progress; //task id -> progress on that task
};

class TaskManager {
public:
	TaskManager();
	
	//load the tasks from the database
	bool LoadTasks();
	
	//loads a client's task state
	bool LoadClientState(Client *c, ClientTaskState *state);
	
	//Packet building routines
	//build all task packets to send to a client during zone in
	void MakeAllTaskDescriptions(ClientTaskState *state, EQStream *into);
	//build all task packets for a specific task.
	void MakeTaskDescriptions(uint32 task_id, EQStream *into);
	//build a task history packet for this client state
	EQZonePacket *MakeHistoryPacket(ClientTaskState *state);
	
protected:
	typedef enum {
		taskEnd = 1,		//end of task
		taskKillCreature = 2,
		taskItem = 3,
		taskConfrontation = 5,  //not sure on name
		taskLocation = 11,		//not sure on name
		taskNothing = 0xFFFFFFFF	//not yet known
	} TaskActivityType;

	class TaskActivity {
	public:
		//uint32 task_id;
		uint32 activity_id;
		
		TaskActivityType type;
		string name;
		uint32 goal_count;
		uint32 goal_id;
	};

	//class CreatureTaskActivity {
	//};

	class Task {
	public:
		uint32 task_id;
		uint32 task_id3;
		string task_name;
		string task_description;
		string reward_link;
		
		vector<TaskActivity> activities;
	};
	map<uint32, Task> tasks;
};

#endif

