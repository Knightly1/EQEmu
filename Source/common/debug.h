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

// Debug Levels
#ifndef EQDEBUG
#define EQDEBUG 0
#else
////// File/Console options
// 0 <= Quiet mode Errors to file Status and Normal ignored
// 1 >= Status and Normal to console, Errors to file
// 2 >= Status, Normal, and Error  to console and logfile
// 3 >= Lite debug
// 4 >= Medium debug
// 5 >= Debug release (Anything higher is not recommended for regular use)
// 6 == (Reserved for special builds) Login  opcode debug All packets dumped
// 7 == (Reserved for special builds) Chat Opcode debug All packets dumped
// 8 == (Reserved for special builds) World opcode debug All packets dumped
// 9 == (Reserved for special builds) Zone Opcode debug All packets dumped
// 10 >= More than you ever wanted to know
//
/////
// Add more below to reserve for file's functions ect.
/////
// Any setup code based on defines should go here
//
#endif

#if defined(_EQDEBUG) && defined(WIN32)
	#ifndef _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
		#if (_MSC_VER < 1300)
			#include <new>
			#include <memory>
			#define _CRTDBG_MAP_ALLOC
			#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
			#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
		#endif
	#endif
#endif


#ifndef ThrowError
	void CatchSignal(int);
	#if defined(CATCH_CRASH) || defined(_EQDEBUG)
		#define ThrowError(errstr)	{ cout << "Fatal error: " << errstr << " (" << __FILE__ << ", line " << __LINE__ << ")" << endl; LogFile->write(EQEMuLog::Error, "Thown Error: %s (%s:%i)", errstr, __FILE__, __LINE__); throw errstr; }
	#else
		#define ThrowError(errstr)	{ cout << "Fatal error: " << errstr << " (" << __FILE__ << ", line " << __LINE__ << ")" << endl; LogFile->write(EQEMuLog::Error, "Thown Error: %s (%s:%i)", errstr, __FILE__, __LINE__); CatchSignal(0); }
	#endif
#endif

#ifdef WIN32
	// VS6 doesn't like the length of STL generated names: disabling
	#pragma warning(disable:4786)
#endif

#ifndef EQDEBUG_H
#define EQDEBUG_H

#ifndef WIN32
	#define DebugBreak()			if(0) {}
#endif

#if defined(WIN32) && defined(PACKETCOLLECTOR)
	// Packet Collector on win32 requires winsock2.h due to latest pcap.h
	// winsock2.h must come before windows.h
	#include <winsock2.h>
#endif

#include "../common/Mutex.h"
#include <stdio.h>

class EQEMuLog {
public:
	EQEMuLog();
	~EQEMuLog();

	enum LogIDs { Status, Normal, Error, Debug, MaxLogID };

	bool write(LogIDs id, const char *fmt, ...);
	bool Dump(LogIDs id, int8* data, int32 size, int32 cols=16, int32 skip=0);
private:
	bool open(LogIDs id);
	bool writeNTS(LogIDs id, bool dofile, const char *fmt, ...); // no error checking, assumes is open, no locking, no timestamp, no newline

	Mutex*	MOpen;
	Mutex**	MLog;
	FILE**	fp;
/* LogStatus: bitwise variable
	1 = output to file
	2 = output to stdout
	4 = fopen error, dont retry
	8 = use stderr instead (2 must be set)
*/
	int8*	pLogStatus;
};

extern EQEMuLog* LogFile;

#ifdef _EQDEBUG
class PerformanceMonitor {
public:
	PerformanceMonitor(sint64* ip) {
		p = ip;
		QueryPerformanceCounter(&tmp);
	}
	~PerformanceMonitor() {
		LARGE_INTEGER tmp2;
		QueryPerformanceCounter(&tmp2);
		*p += tmp2.QuadPart - tmp.QuadPart;
	}
	LARGE_INTEGER tmp;
	sint64* p;
};
#endif
#endif

