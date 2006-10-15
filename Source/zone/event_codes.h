#ifndef EVENT_CODES_H
#define EVENT_CODES_H

typedef enum {
	EVENT_SAY = 0,
	EVENT_ITEM,
	EVENT_DEATH,
	EVENT_SPAWN,
	EVENT_ATTACK,
	EVENT_SLAY,
	EVENT_NPC_SLAY,
	EVENT_WAYPOINT,
	EVENT_TIMER,
	EVENT_SIGNAL,
	EVENT_HP,
	EVENT_AGGRO,
	EVENT_ENTER,
	EVENT_EXIT,
	
	_LargestEventID
} QuestEventID;

extern const char *QuestEventSubroutines[_LargestEventID];

#endif

