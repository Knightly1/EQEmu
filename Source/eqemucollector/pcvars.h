#ifndef PCVARS_H
#define PCVARS_H
#include "../common/types.h"

#include <string>
#include <map>
using namespace std;

#include <pcap.h>
#ifdef WIN32
	#include <conio.h>
	#include <process.h>
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif


class PluginManager;

extern PluginManager *plugins;

typedef enum {
	noMode,
	liveCollectMode,
	instantFileMode,
	timedFileMode
} PCMode;

extern PCMode mode;
extern int debug;
extern bool OpcodeLoadFailed;
extern bool DoLoop;
extern bool IsWin95;
extern bool ProcessThreadRunning;
extern char sniffhost[255];
extern char*	pcapdevice;
#ifdef WIN32
extern WCHAR*	wpcapdevice;
#endif

extern bool privacy_mode;
extern char privacy_name[64];
extern char _zero_buffer[64];	//just a buffer of zeroes

extern void SetupGlobals();
extern void SetupCollectEnvironment();
extern bool SetupLiveCollect();
extern void StartCollecting();
extern bool SetupFileCollect(const char *filename);
extern void ClearCollectEnvironment();


#endif

