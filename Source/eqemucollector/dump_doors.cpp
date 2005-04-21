#include "../common/eq_packet_structs.h"
#include "PacketHandler.h"
#include "EQPacket.h"
#include "EQStreamPair.h"
#include <iostream>

using namespace std;
void dump_message(unsigned char *buffer, unsigned long length, string leader);

void HandleDoors(const EQStreamPair *sp, const EQApplicationPacket *p)
{
	int count=p->size/sizeof(Door_Struct);
	int i;
	unsigned char *ptr;
	char temp[10];
	for(i=0,ptr=p->pBuffer;i<count;i++,ptr+=sizeof(Door_Struct)) {
		Door_Struct *d=(Door_Struct *)ptr;
		sprintf(temp,"%d:",d->doorId);
		dump_message(ptr,sizeof(Door_Struct),temp);
		//printf("insert into doors (doorid,zone,name,pos_y,pos_x,pos_z,heading,opentype,doorisopen,invert_state,incline,door_param,size) values ('%d','%s','%s','%f','%f','%f','%f','%d','%d','%d','%d','%d','%d');\n",
			//d->doorId,"gfaydark",d->name,d->yPos,d->xPos,d->zPos,d->heading,d->opentype,d->state_at_spawn,d->invert_state,d->incline,d->door_param,d->size);
	}
}

extern "C" int on_load()
{
	AddClientPacketHandler(0x5339,HandleDoors);
}

extern "C" int on_unload()
{
}

void dump_message(unsigned char *buffer, unsigned long length, string leader)
{
unsigned long i;
	cout << leader;
	for(i=0;i<length;i++)
		printf(" %c ",isprint(buffer[i]) ? (unsigned char)buffer[i] : '.');
	cout << endl;

	cout << leader;
	for(i=0;i<length;i++)
		printf(" %02x",(unsigned char)buffer[i]);
	cout << endl;
}
