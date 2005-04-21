/* 
	Copyright (C) 2005 Michael S. Finger

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
#include <stdio.h>
#include <string>
#include <iostream>
#include <map>
#include "pcvars.h"	//get our network headers from here.
#include "EQStreamPair.h"
#include "StreamPairManager.h"
#include "misc.h"
#include "PluginManager.h"

#ifdef WIN32
#include "../common/win_getopt.h"
#endif

extern pcap_t *handle;
extern StreamPairManager stream_mgr;

void usage() {
	printf("Usage: eqemucollector -d (device) [options] (Host IP)\n");
	printf("   device - the device to capture from (use 'list' on windows to list them)\n");
	printf("   Host IP - the IP address of the client or server machine.\n");
	printf("Options:\n");
	printf("   -q - Enable quiet mode to disable new stream printing\n");
	printf("   -p (file.so) - load the plugin file.so\n");
	printf("   -f (file.pcap) - read packets from pcap file file.pcap instead of network.\n");
	printf("   -t - Play back the file with its original delays. Only valid after -f\n");
}

int main(int argc, char **argv) {
	SetupGlobals();
	
	char opt;
	const char *file = NULL;
	while((opt=getopt(argc,argv,"d:f:Dp:tq"))!=-1) {
		switch (opt) {
		case 'p':
			if(plugins == NULL)
				plugins = new PluginManager();
			plugins->load(optarg);
			break;
		case 'd':
			pcapdevice = optarg;
			file = NULL;
			mode = liveCollectMode;
			break;
		case 'f':
			pcapdevice = NULL;
			file = optarg;
			mode = instantFileMode;
			break;
		case 'D':
			debug++;
			break;
		case 'q':
			stream_mgr.SetQuietMode(true);
			break;
		case 't':
			if(mode == instantFileMode)
				mode = timedFileMode;
			else {
				printf("Error: -t is invalid unless following -f (file)\n");
				return(1);
			}
			break;
		default:
			printf("Unrecognized argument: '-%c'\n", opt);
			usage();
			return(1);
		}
	}
	argc -= optind;
	argv += optind;
	
	if(mode == noMode || (file == NULL && pcapdevice == NULL)) {
		printf("Missing a -d or -f flag\n");
		usage();
		return(2);
	}
	
	if(argc > 0) {
		strncpy(sniffhost, argv[0], 255);
	}
	

/*	if (file) {
		if ((handle=pcap_open_offline(file,errbuf))==NULL) {
			cerr << "pcap_open_offline() on '" << file << "' failed: " << errbuf << endl;
			return 1;
		}
	} else {
		if (!pcapdev) {
			if ((pcapdev=pcap_lookupdev(errbuf))==NULL) {
				cerr << "pcap_lookupdev() failed: " << errbuf << endl;
				return 2;
			}
		}
		if ((p=pcap_open_live(pcapdev,2048,1,100,errbuf))==NULL) {
			cerr << "pcap_open_live() on '" << pcapdev << "' failed: " << errbuf << endl;
			return 1;
		}

		strcpy(filter_text,"udp");
		if (argc-optind) {
			strcat(filter_text," and host ");
			strcat(filter_text,argv[optind+1]);
		}
		if (pcap_compile(handle,&filter,filter_text,0,0)==-1) {
			cerr << "pcap_compile() on '" << filter_text << "' failed: " << pcap_geterr(p) << endl;
			return 1;
		}
		if (pcap_setfilter(handle,&filter)==-1) {
			cerr << "pcap_setfilter() failed: " << pcap_geterr(p) << endl;
			return 1;
		}
	}*/
	
	SetupCollectEnvironment();
	
	if(mode == instantFileMode || mode == timedFileMode) {
		if(file == NULL) {
			cout << "No file specified for file playback." << endl;
			return(1);
		}
		//pcap file collect
		if(!SetupFileCollect(file)) {
			return(1);
		}
		StartCollecting();
	} else {
		//live collect
		if(!SetupLiveCollect()) {
			return(1);
		}
		StartCollecting();
	}
	ClearCollectEnvironment();

	return 0;
}


