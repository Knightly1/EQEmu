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
#include "../common/debug.h"
#include <iostream>
using namespace std;
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../common/version.h"
#include "console.h"
#include "net.h"
#include "zoneserver.h"
#include "../common/database.h"
#include "../common/packet_dump.h"
#include "../common/seperator.h"
#include "../common/eq_packet_structs.h"
#include "LoginServer.h"
#include "../common/serverinfo.h"
#include "../common/md5.h"

#ifdef WIN32
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#endif

extern Database database;
extern NetConnection net;
extern ZSList	zoneserver_list;
extern uint32	numzones;
extern LoginServer loginserver;

ConsoleList console_list;

Console::Console(TCPConnection* itcpc) : WorldTCPConnection() {
	tcpc = itcpc;
	tcpc->SetEcho(true);
	timeout_timer = new Timer(CONSOLE_TIMEOUT);
	state = 0;
	paccountid = 0;
	memset(paccountname, 0, sizeof(paccountname));
	admin = 0;
	pAcceptMessages = false;
	tcpc->Send((uchar *)const_cast<char *>("Username: "), strlen("Username: "));
}

Console::~Console() {
	delete timeout_timer;
	if (tcpc)
		tcpc->Free();
}

void Console::Die() {
	state = CONSOLE_STATE_CLOSED;
	struct in_addr  in;
	in.s_addr = GetIP();
	cout << "Removing console from ip: " << inet_ntoa(in) << " port:" << GetPort() << endl;
	tcpc->Disconnect();
}

bool Console::SendChannelMessage(const ServerChannelMessage_Struct* scm) {
	if (!pAcceptMessages)
		return false;
	switch (scm->chan_num) {
		case 5: {
			SendMessage(1, "%s says ooc, '%s'", scm->from, scm->message);
			break;
		}
		case 6: {
			SendMessage(1, "%s BROADCASTS, '%s'", scm->from, scm->message);
			break;
		}
		case 7: {
			SendMessage(1, "%s tells you, '%s'", scm->from, scm->message);
			ServerPacket* pack = new ServerPacket(ServerOP_ChannelMessage, sizeof(ServerChannelMessage_Struct) + strlen(scm->message) + 1);
			memcpy(pack->pBuffer, scm, pack->size);
			ServerChannelMessage_Struct* scm2 = (ServerChannelMessage_Struct*) pack->pBuffer;
			strcpy(scm2->deliverto, scm2->from);
			scm2->noreply = true;
			zoneserver_list.SendPacket(scm->from, pack);
			safe_delete(pack);
			break;
		}
		case 11: {
			SendMessage(1, "%s GMSAYS, '%s'", scm->from, scm->message);
			break;
		}
		default: {
			return false;
		}
	}
	return true;
}

bool Console::SendEmoteMessage(int32 type, const char* message, ...) {
	if (!message)
		return false;
	if (!pAcceptMessages)
		return false;
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);

	SendMessage(1, message);
	return true;
}

bool Console::SendEmoteMessageRaw(int32 type, const char* message) {
	if (!message)
		return false;
	if (!pAcceptMessages)
		return false;
	SendMessage(1, message);
	return true;
}

void Console::SendEmoteMessage(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message, ...) {
	if (!message)
		return;
	if (to_guilddbid != 0 || to_minstatus > Admin())
		return;
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);

	SendEmoteMessageRaw(to, to_guilddbid, to_minstatus, type, buffer);
}

void Console::SendEmoteMessageRaw(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message) {
	if (!message)
		return;
	if (to_guilddbid != 0 || to_minstatus > Admin())
		return;
	SendMessage(1, message);
}

void Console::SendMessage(int8 newline, const char* message, ...) {
	if (!message)
		return;
	char* buffer = 0;
	int32 bufsize = 1500;
	if (message)
		bufsize += strlen(message);
	buffer = new char[bufsize];
	memset(buffer, 0, bufsize);
	if (message != 0) {
		va_list argptr;

		va_start(argptr, message);
		vsnprintf(buffer, bufsize - 512, message, argptr);
		va_end(argptr);
	}

	if (newline) {
		char outbuf[3];
		outbuf[0] = 13;
		outbuf[1] = 10;
		outbuf[2] = 0;
		for (int i=0; i < newline; i++)
			strcat(buffer, outbuf);
	}
	tcpc->Send((uchar*) buffer, strlen(buffer));
	safe_delete(buffer);
}

bool Console::Process() {
	if (state == CONSOLE_STATE_CLOSED)
		return false;
	if (!tcpc->Connected()) {
		struct in_addr  in;
		in.s_addr = GetIP();
		cout << Timer::GetCurrentTime() << " Removing console (!tcpc->Connected): " << inet_ntoa(in) << ":" << tcpc->GetrPort() << endl;
		return false;
	}
	if (timeout_timer->Check()) {
		SendMessage(1, 0);
		SendMessage(1, "Timeout, disconnecting...");
		struct in_addr  in;
		in.s_addr = GetIP();
		cout << "TCP connection timeout: " << inet_ntoa(in) << ":" << GetPort() << endl;
		return false;
	}
	if (tcpc->GetMode() == modePacket) {
		struct in_addr	in;
		in.s_addr = GetIP();
		ZoneServer* zs = new ZoneServer(tcpc);
		cout << "New zoneserver: #" << zs->GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
		zoneserver_list.Add(zs);
		numzones++;
		tcpc = 0;
		return false;
	}
	char* command = 0;
	while ((command = tcpc->PopLine())) {
		timeout_timer->Start();
		ProcessCommand(command);
		delete command;
	}
	return true;
}

void ConsoleList::Add(Console* con) {
	list.Insert(con);
}

void ConsoleList::Process() {
	LinkedListIterator<Console*> iterator(list);
	iterator.Reset();

	while(iterator.MoreElements()) {
		if (!iterator.GetData()->Process())
			iterator.RemoveCurrent();
		else
			iterator.Advance();
	}
}

void ConsoleList::KillAll() {
	LinkedListIterator<Console*> iterator(list);
	iterator.Reset();

	while(iterator.MoreElements()) {
		iterator.GetData()->Die();
		iterator.RemoveCurrent();
	}
}

void ConsoleList::SendConsoleWho(WorldTCPConnection* connection, const char* to, sint16 admin, char** output, int32* outsize, int32* outlen) {
	LinkedListIterator<Console*> iterator(list);
	iterator.Reset();
	struct in_addr  in;
	int x = 0;

	while(iterator.MoreElements()) {
		in.s_addr = iterator.GetData()->GetIP();
		if (admin >= iterator.GetData()->Admin())
			AppendAnyLenString(output, outsize, outlen, "  Console: %s:%i AccID: %i AccName: %s", inet_ntoa(in), iterator.GetData()->GetPort(), iterator.GetData()->AccountID(), iterator.GetData()->AccountName());
		else
			AppendAnyLenString(output, outsize, outlen, "  Console: AccID: %i AccName: %s", iterator.GetData()->AccountID(), iterator.GetData()->AccountName());
		if (*outlen >= 3584) {
			connection->SendEmoteMessageRaw(to, 0, 0, 10, *output);
			safe_delete(*output);
			*outsize = 0;
			*outlen = 0;
		}
		else {
			if (connection->IsConsole())
				AppendAnyLenString(output, outsize, outlen, "\r\n");
			else
				AppendAnyLenString(output, outsize, outlen, "\n");
		}
		x++;
		iterator.Advance();
	}
	AppendAnyLenString(output, outsize, outlen, "%i consoles connected", x);
}

void ConsoleList::SendChannelMessage(const ServerChannelMessage_Struct* scm) {
	LinkedListIterator<Console*> iterator(list);
	iterator.Reset();

	while(iterator.MoreElements()) {
		iterator.GetData()->SendChannelMessage(scm);
		iterator.Advance();
	}
}

void ConsoleList::SendEmoteMessage(int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);

	SendEmoteMessageRaw(type, buffer);
}

void ConsoleList::SendEmoteMessageRaw(int32 type, const char* message) {
	LinkedListIterator<Console*> iterator(list);
	iterator.Reset();

	while(iterator.MoreElements()) {
		iterator.GetData()->SendEmoteMessageRaw(type, message);
		iterator.Advance();
	}
}

Console* ConsoleList::FindByAccountName(const char* accname) {
	LinkedListIterator<Console*> iterator(list);
	iterator.Reset();

	while(iterator.MoreElements()) {
		if (strcasecmp(iterator.GetData()->AccountName(), accname) == 0)
			return iterator.GetData();

		iterator.Advance();
	}
	return 0;
}

void Console::ProcessCommand(const char* command) {
	switch(state)
	{
		case CONSOLE_STATE_USERNAME:
		{
			if (strlen(command) >= 16) {
				SendMessage(1, 0);
				SendMessage(2, "Username buffer overflow.");
				SendMessage(1, "Bye Bye.");
				state = CONSOLE_STATE_CLOSED;
				return;
			}
			strcpy(paccountname, command);
			state = CONSOLE_STATE_PASSWORD;
			SendMessage(0, "Password: ");
			tcpc->SetEcho(false);
			break;
		}
		case CONSOLE_STATE_PASSWORD:
		{
			if (strlen(command) >= 16) {
				SendMessage(1, 0);
				SendMessage(2, "Password buffer overflow.");
				SendMessage(1, "Bye Bye.");
				state = CONSOLE_STATE_CLOSED;
				return;
			}
			paccountid = database.CheckLogin(paccountname,command);
			if (paccountid == 0) {
				SendMessage(1, 0);
				SendMessage(2, "Login failed.");
				SendMessage(1, "Bye Bye.");
				state = CONSOLE_STATE_CLOSED;
				return;
			}
			database.GetAccountName(paccountid, paccountname); // fixes case and stuff
			admin = database.CheckStatus(paccountid);
			if (!(admin >= 100)) {
				SendMessage(1, 0);
				SendMessage(2, "Access denied.");
				SendMessage(1, "Bye Bye.");
				state = CONSOLE_STATE_CLOSED;
				return;
			}
			cout << "TCP console authenticated: Username=" << paccountname << ", Admin=" << (sint16) admin << endl;
			SendMessage(1, 0);
			SendMessage(2, "Login accepted.");
			state = CONSOLE_STATE_CONNECTED;
			tcpc->SetEcho(true);
			SendPrompt();
			break;
		}
		case CONSOLE_STATE_CONNECTED: {
//			cout << "TCP command: " << paccountname << ": \"" << command << "\"" << endl;
			Seperator sep(command);
			if (strcasecmp(sep.arg[0], "help") == 0 || strcmp(sep.arg[0], "?") == 0) {
				SendMessage(1, "  whoami");
				SendMessage(1, "  who");
				SendMessage(1, "  zonestatus");
				SendMessage(1, "  uptime [zoneID#]");
				SendMessage(1, "  emote [zonename or charname or world] [type] [message]");
				SendMessage(1, "  echo [on/off]");
				SendMessage(1, "  acceptmessages [on/off]");
				SendMessage(1, "  tell [name] [message]");
				SendMessage(1, "  broadcast [message]");
				SendMessage(1, "  gmsay [message]");
				SendMessage(1, "  ooc [message]");
				if (admin >= 150) {
					SendMessage(1, "  kick [charname]");
					SendMessage(1, "  lock/unlock");
					SendMessage(1, "  zoneshutdown [zonename or ZoneServerID]");
					SendMessage(1, "  zonebootup [ZoneServerID] [zonename]");
				}
				if (admin >= 200) {
// SCORPIOUS2K - reversed parameter order for flag
					SendMessage(1, "  flag [status] [accountname]");
					SendMessage(1, "  setpass [accountname] [newpass]");
					SendMessage(1, "  version");
					SendMessage(1, "  worldshutdown");
				}
				if (admin >= 201) {
					SendMessage(1, "  IPLookup [name]");
				}
			}
			else if (strcasecmp(sep.arg[0], "ping") == 0) {
				// do nothing
			}
			else if (strcasecmp(sep.arg[0], "setpass") == 0 && admin >= 200) {
				if (sep.argnum != 2)
					SendMessage(1, "Format: setpass accountname password");
				else {

					sint16 tmpstatus = 0;
					int32 tmpid = database.GetAccountIDByName(sep.arg[1], &tmpstatus);
					if (!tmpid)
						SendMessage(1, "Error: Account not found");
					else if (tmpstatus > admin)
						SendMessage(1, "Cannot change password: Account's status is higher than yours");
					else if (database.SetLocalPassword(tmpid, sep.arg[2]))
						SendMessage(1, "Password changed.");
					else
						SendMessage(1, "Error changing password."); 
				}
			}
			else if (strcasecmp(sep.arg[0], "uptime") == 0) {
				if (sep.IsNumber(1) && atoi(sep.arg[1]) > 0) {
					ServerPacket* pack = new ServerPacket(ServerOP_Uptime, sizeof(ServerUptime_Struct));
					ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
					snprintf(sus->adminname, sizeof(sus->adminname), "*%s", this->GetName());
					sus->zoneserverid = atoi(sep.arg[1]);
					ZoneServer* zs = zoneserver_list.FindByID(sus->zoneserverid);
					if (zs)
						zs->SendPacket(pack);
					else
						SendMessage(1, "Zoneserver not found.");
					delete pack;
				}
				else {
					ZSList::ShowUpTime(this);
				}
			}
			else if (strcasecmp(sep.arg[0], "md5") == 0) {
				int8 md5[16];
				MD5::Generate((const uchar*) sep.argplus[1], strlen(sep.argplus[1]), md5);
				SendMessage(1, "MD5: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7], md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
			}
			else if (strcasecmp(sep.arg[0], "whoami") == 0) {
				SendMessage(1, "You are logged in as '%s'", this->AccountName());
				SendMessage(1, "You are known as '*%s'", this->AccountName());
				SendMessage(1, "AccessLevel: %d", this->Admin());
			}
			else if (strcasecmp(sep.arg[0], "echo") == 0) {
				if (strcasecmp(sep.arg[1], "on") == 0)
					tcpc->SetEcho(true);
				else if (strcasecmp(sep.arg[1], "off") == 0) {
					if (pAcceptMessages)
						SendMessage(1, "Echo can not be turned off while acceptmessages is on");
					else
						tcpc->SetEcho(false);
				}
				else
					SendMessage(1, "Usage: echo [on/off]");
			}
			else if (strcasecmp(sep.arg[0], "acceptmessages") == 0) {
				if (strcasecmp(sep.arg[1], "on") == 0)
					if (tcpc->GetEcho())
						SendMessage(1, "AcceptMessages can not be turned on while echo is on");
					else
						pAcceptMessages = true;
				else if (strcasecmp(sep.arg[1], "off") == 0)
					pAcceptMessages = false;
				else
					SendMessage(1, "Usage: acceptmessages [on/off]");
			}
			else if (strcasecmp(sep.arg[0], "tell") == 0) {
				char tmpname[64];
				tmpname[0] = '*';
				strcpy(&tmpname[1], paccountname);
				zoneserver_list.SendChannelMessage(tmpname, sep.arg[1], 7, 0, sep.argplus[2]);
			}
			else if (strcasecmp(sep.arg[0], "broadcast") == 0) {
				char tmpname[64];
				tmpname[0] = '*';
				strcpy(&tmpname[1], paccountname);
				zoneserver_list.SendChannelMessage(tmpname, 0, 6, 0, sep.argplus[1]);
			}
			else if (strcasecmp(sep.arg[0], "ooc") == 0) {
				char tmpname[64];
				tmpname[0] = '*';
				strcpy(&tmpname[1], paccountname);
				zoneserver_list.SendChannelMessage(tmpname, 0, 5, 0, sep.argplus[1]);
			}
			else if (strcasecmp(sep.arg[0], "gmsay") == 0 || strcasecmp(sep.arg[0], "pr") == 0) {
				char tmpname[64];
				tmpname[0] = '*';
				strcpy(&tmpname[1], paccountname);
				zoneserver_list.SendChannelMessage(tmpname, 0, 11, 0, sep.argplus[1]);
			}
			else if (strcasecmp(sep.arg[0], "emote") == 0) {
				if (strcasecmp(sep.arg[1], "world") == 0)
					zoneserver_list.SendEmoteMessageRaw(0, 0, 0, atoi(sep.arg[2]), sep.argplus[3]);
				else {
					ZoneServer* zs = zoneserver_list.FindByName(sep.arg[1]);
					if (zs != 0)
						zs->SendEmoteMessageRaw(0, 0, 0, atoi(sep.arg[2]), sep.argplus[3]);
					else
						zoneserver_list.SendEmoteMessageRaw(sep.arg[1], 0, 0, atoi(sep.arg[2]), sep.argplus[3]);
				}
			}
			else if (strcasecmp(sep.arg[0], "movechar") == 0) {
				if(sep.arg[1][0]==0 || sep.arg[2][0] == 0)
					SendMessage(1, "Usage: movechar [charactername] [zonename]");
				else {
					if (!database.GetZoneID(sep.arg[2]))
						SendMessage(1, "Error: Zone '%s' not found", sep.arg[2]);
					else if (!database.CheckUsedName((char*) sep.arg[1])) {
						if (!database.MoveCharacterToZone((char*) sep.arg[1], (char*) sep.arg[2]))
							SendMessage(1, "Character Move Failed!");
						else
							SendMessage(1, "Character has been moved.");
					}
					else
						SendMessage(1, "Character Does Not Exist");
				}
			}
			else if (strcasecmp(sep.arg[0], "flag") == 0 && this->Admin() >= 200) {
// SCORPIOUS2K - reversed parameter order for flag
				if(sep.arg[2][0]==0 || !sep.IsNumber(1))
					SendMessage(1, "Usage: flag [status] [accountname]");
				else 
				{
					if (atoi(sep.arg[1]) > this->Admin())
						SendMessage(1, "You cannot set people's status to higher than your own");
					else if (atoi(sep.arg[1]) < 0 && this->Admin() < 100)
							SendMessage(1, "You have too low of status to suspend/ban");
					else if (!database.SetGMFlag(sep.arg[2], atoi(sep.arg[1])))
							SendMessage(1, "Unable to flag account!");
					else
							SendMessage(1, "Account Flaged");
				}
			}
			else if (strcasecmp(sep.arg[0], "kick") == 0 && admin >= 150) {
				char tmpname[64];
				tmpname[0] = '*';
				strcpy(&tmpname[1], paccountname);
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_KickPlayer;
				pack->size = sizeof(ServerKickPlayer_Struct);
				pack->pBuffer = new uchar[pack->size];
				ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
				strcpy(skp->adminname, tmpname);
				strcpy(skp->name, sep.arg[1]);
				skp->adminrank = this->Admin();
				zoneserver_list.SendPacket(pack);
				delete pack;
			}
			else if (strcasecmp(sep.arg[0], "who") == 0) {
				Who_All_Struct* whom = new Who_All_Struct;
				memset(whom, 0, sizeof(Who_All_Struct));
				whom->lvllow = 0xFFFF;
				whom->lvlhigh = 0xFFFF;
				whom->wclass = 0xFFFF;
				whom->wrace = 0xFFFF;
				whom->gmlookup = 0xFFFF;
				for (int i=1; i<=sep.argnum; i++) {
					if (strcasecmp(sep.arg[i], "gm") == 0)
						whom->gmlookup = 1;
					else if (sep.IsNumber(i)) {
						if (whom->lvllow == 0xFFFF) {
							whom->lvllow = atoi(sep.arg[i]);
							whom->lvlhigh = whom->lvllow;
						}
						else if (atoi(sep.arg[i]) > whom->lvllow)
							whom->lvlhigh = atoi(sep.arg[i]);
						else
							whom->lvllow = atoi(sep.arg[i]);
					}
					else
						strn0cpy(whom->whom, sep.arg[i], sizeof(whom->whom));
				}
				//zoneserver_list.SendWhoAll(0,0, admin, whom, this);
				zoneserver_list.ConsoleSendWhoAll(0, admin, whom, this);
				delete whom;
			}
			else if (strcasecmp(sep.arg[0], "zonestatus") == 0) {
				zoneserver_list.SendZoneStatus(0, admin, this);
			}
			else if (strcasecmp(sep.arg[0], "exit") == 0 || strcasecmp(sep.arg[0], "quit") == 0) {
				SendMessage(1, "Bye Bye.");
				state = CONSOLE_STATE_CLOSED;
			}
			else if (strcasecmp(sep.arg[0], "zoneshutdown") == 0 && admin >= 150) {
				if (sep.arg[1][0] == 0) {
					SendMessage(1, "Usage: zoneshutdown zoneshortname");
				} else {
					char tmpname[64];
					tmpname[0] = '*';
					strcpy(&tmpname[1], paccountname);

					ServerPacket* pack = new ServerPacket;
					pack->size = sizeof(ServerZoneStateChange_struct);
					pack->pBuffer = new uchar[pack->size];
					memset(pack->pBuffer, 0, sizeof(ServerZoneStateChange_struct));
					ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
					pack->opcode = ServerOP_ZoneShutdown;
					strcpy(s->adminname, tmpname);
					if (sep.arg[1][0] >= '0' && sep.arg[1][0] <= '9')
						s->ZoneServerID = atoi(sep.arg[1]);
					else
						s->zoneid = database.GetZoneID(sep.arg[1]);

					ZoneServer* zs = 0;
					if (s->ZoneServerID != 0)
						zs = zoneserver_list.FindByID(s->ZoneServerID);
					else if (s->zoneid != 0)
						zs = zoneserver_list.FindByName(database.GetZoneName(s->zoneid));
					else
						SendMessage(1, "Error: ZoneShutdown: neither ID nor name specified");

					if (zs == 0)
						SendMessage(1, "Error: ZoneShutdown: zoneserver not found");
					else
						zs->SendPacket(pack);

					delete pack;
				}
			}
			else if (strcasecmp(sep.arg[0], "zonebootup") == 0 && admin >= 150) {
				if (sep.arg[2][0] == 0 || !sep.IsNumber(1)) {
					SendMessage(1, "Usage: zonebootup ZoneServerID# zoneshortname");
				} else {
					char tmpname[64];
					tmpname[0] = '*';
					strcpy(&tmpname[1], paccountname);

					cout << "Console ZoneBootup: " << tmpname << ", " << sep.arg[2] << ", " << sep.arg[1] << endl;
					ZSList::SOPZoneBootup(tmpname, atoi(sep.arg[1]), sep.arg[2], (bool) (strcasecmp(sep.arg[3], "static") == 0));
				}
			}
			else if (strcasecmp(sep.arg[0], "worldshutdown") == 0 && admin >= 200) {
				ServerPacket* pack = new ServerPacket(ServerOP_ShutdownAll);
				zoneserver_list.SendPacket(pack);
				delete pack;
				SendMessage(1, "Sending shutdown packet... goodbye.");
				CatchSignal(0);
			}
			else if (strcasecmp(sep.arg[0], "lock") == 0) {
				net.world_locked = true;
				if (loginserver.Connected()) {
					loginserver.SendStatus();
					SendMessage(1, "World locked.");
				}
				else {
					SendMessage(1, "World locked, but login server not connected.");
				}
			}
			else if (strcasecmp(sep.arg[0], "unlock") == 0) {
				net.world_locked = false;
				if (loginserver.Connected()) {
					loginserver.SendStatus();
					SendMessage(1, "World unlocked.");
				}
				else {
					SendMessage(1, "World unlocked, but login server not connected.");
				}
			}
			else if (strcasecmp(sep.arg[0], "version") == 0 && admin >= 200) {
				SendMessage(1, "Current version information.");
				SendMessage(1, "  %s", CURRENT_WORLD_VERSION);
				SendMessage(1, "  Compiled on: %s at %s", COMPILE_DATE, COMPILE_TIME);
				SendMessage(1, "  Last modified on: %s", LAST_MODIFIED);
			}
			else if (strcasecmp(sep.arg[0], "serverinfo") == 0 && admin >= 200) {
				if (strcasecmp(sep.arg[1], "os") == 0)	{
				#ifdef WIN32
					GetOS();
					char intbuffer [sizeof(unsigned long)];
					SendMessage(1, "Operating system information.");
					SendMessage(1, "  %s", Ver_name);
					SendMessage(1, "  Build number: %s", ultoa(Ver_build, intbuffer, 10));
					SendMessage(1, "  Minor version: %s", ultoa(Ver_min, intbuffer, 10));
					SendMessage(1, "  Major version: %s", ultoa(Ver_maj, intbuffer, 10));
					SendMessage(1, "  Platform Id: %s", ultoa(Ver_pid, intbuffer, 10));
				#else
					char os_string[100];
					SendMessage(1, "Operating system information.");
					SendMessage(1, "  %s", GetOS(os_string));
				#endif
				}
				else {
					SendMessage(1, "Usage: Serverinfo [type]");
					SendMessage(1, "  OS - Operating system version information.");
				}
			}
			else if (strcasecmp(sep.arg[0], "IPLookup") == 0 && admin >= 201) {
				zoneserver_list.SendCLEList(admin, 0, this, sep.argplus[1]);
			}
			else if (strcasecmp(sep.arg[0], "zonelock") == 0) {
				if (strcasecmp(sep.arg[1], "list") == 0) {
					zoneserver_list.ListLockedZones(0, this);
				}
				else if (strcasecmp(sep.arg[1], "lock") == 0 && admin >= 101) {
					int16 tmp = database.GetZoneID(sep.arg[2]);
					if (tmp) {
						if (zoneserver_list.SetLockedZone(tmp, true))
							zoneserver_list.SendEmoteMessage(0, 0, 80, 15, "Zone locked: %s", database.GetZoneName(tmp));
						else
							SendMessage(1, "Failed to change lock");
					}
					else
						SendMessage(1, "Usage: #zonelock lock [zonename]");
				}
				else if (strcasecmp(sep.arg[1], "unlock") == 0 && admin >= 101) {
					int16 tmp = database.GetZoneID(sep.arg[2]);
					if (tmp) {
						if (zoneserver_list.SetLockedZone(tmp, false))
							zoneserver_list.SendEmoteMessage(0, 0, 80, 15, "Zone unlocked: %s", database.GetZoneName(tmp));
						else
							SendMessage(1, "Failed to change lock");
					}
					else
						SendMessage(1, "Usage: #zonelock unlock [zonename]");
				}
				else {
					SendMessage(1, "#zonelock sub-commands");
					SendMessage(1, "  list");
					if (admin >= 101) {
						SendMessage(1, "  lock [zonename]");
						SendMessage(1, "  unlock [zonename]");
					}
				}
			}
			else {
				SendMessage(1, "Command unknown.");
			}
			if (state == CONSOLE_STATE_CONNECTED)
				SendPrompt();
			break;
		}
		default: {
			break;
		}
	}
}

void Console::SendPrompt() {
	if (tcpc->GetEcho())
		SendMessage(0, "%s> ", paccountname);
}
