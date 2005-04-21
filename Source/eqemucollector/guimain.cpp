/*
  EQ Packet Collector
  by Quagmire
  Copyright (C) 2001-2002 Quagmire, quagmire_@users.sourceforge.net
  Many of the included libraries: Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.net)
  Released under GPL, see below.

  Thanks to:
  Agz and the EQEMu project, alot of the EQ packet decode taken from there.
  Xylor for his map of the EQ packet.
  ShowEQ and SINS for alot of concepts of sniffing.
  HackersQuest for the idea of an eq packet collecting program (ItemCollector).



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

#include "StdAfx.h"
#include "../common/debug.h"

#include <iostream>
#include <iomanip>
#include <map>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef WIN32
	#include <conio.h>
	#include <pcap.h>
	#include <process.h>
#include <afxtempl.h>
#else
	#include <pthread.h>
	#include <sys/resource.h>
	#include "../common/unix.h"
	extern "C" {
		#include <pcap.h>
	}
#endif

#include "pcvars.h"


#ifdef WIN32

extern void* GetAdapterFromList(void* device, int index);

bool LookupPcapDevice(int id) {
	char errbuf[PCAP_ERRBUF_SIZE];
	char* devicet = pcap_lookupdev(errbuf);
	if (devicet == NULL) {
		return(false);
	}
	if (IsWin95) {
		pcapdevice = (char*) GetAdapterFromList(devicet, id);
		if (pcapdevice == NULL) {
			return(false);
		}
	}
	else {
		wpcapdevice = (WCHAR*) GetAdapterFromList(devicet, id);
		if (wpcapdevice == NULL) {
			return(false);
		}
	}
	return(true);
}

void GetDeviceList(CArray<CString, CString&> &thelist) {
	
/*	thelist.RemoveAll();

	char ebuf[PCAP_ERRBUF_SIZE];
	char* device = pcap_lookupdev(ebuf);
	if (device == NULL)
		return;

	DWORD dwVersion;
	DWORD dwWindowsMajorVersion;
	const WCHAR* t;
	const char* t95;
	int i=0;
	int DescPos=0;
	char *Desc;
	int n=1;
	
	CString cur;

	dwVersion = GetVersion();
	dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if (IsWin95) {			// Windows '95
		t95=(char*)device;

		while(*(t95+DescPos)!=0 || *(t95+DescPos-1)!=0){
			DescPos++;
		}

		Desc=(char*)t95+DescPos+1;

		cur.Format("%d.",n++);

		while (!(t95[i]==0 && t95[i-1]==0)) {
			if (t95[i]==0){
				cur  += " (";
				while(*Desc!=0){
//					putchar(*Desc);
					cur += *Desc;
					Desc++;
				}
				Desc++;
//				putchar(')');
//				putchar('\n');
				cur += ")";
				thelist.Add(cur);
				cur = "";
			}
			else  {
				//putchar(t95[i]);
				cur += t95[i];
			}

			if((t95[i]==0) && (t95[i+1]!=0)) {
				cur.Format("%d.",n++);
			}

			i++;
		}
	//	if(cur.GetLength() > 1)
			thelist.Add(cur);
	//	putchar('\n');
	}
	
	else {		//Windows NT

		t=(WCHAR*)device;
		while(*(t+DescPos)!=0 || *(t+DescPos-1)!=0){
			DescPos++;
		}

		DescPos<<=1;
		Desc=(char*)t+DescPos+2;

		cur.Format("%d.",n++);

		while (!(t[i]==0 && t[i-1]==0)) {
			if (t[i]==0) {
				cur  += " (";
				while(*Desc!=0){
	//				putchar(*Desc);
					cur += *Desc;
					Desc++;
				}
				Desc++;
				cur += ")";
				thelist.Add(cur);
				cur = "";
			}
			else {
				cur += t[i];
				//putchar(t[i]);
			}

			if(t[i]==0 && t[i+1]!=0) {
				cur.Format("%d.",n++);
			}

			i++;
		}
	//	if(cur.GetLength() > 1)
			thelist.Add(cur);
	}*/
	char ebuf[PCAP_ERRBUF_SIZE];
	//dunno what the return of this thing is:
	pcap_if_t *list;
	pcap_findalldevs(&list, ebuf);
	
	int i = 1;
	
	while(list != NULL) {
		CString tmp;
		tmp.Format("%d. %s - %d", i, list->description, list->name);
		thelist.Add(tmp);
		list = list->next;
		i++;
	}
	
	//im prolly leaking the contents of the device list
	//but im to lazy to look up how to free it
	
}
#endif

