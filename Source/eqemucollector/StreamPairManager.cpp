#include "StdAfx.h"
#include "StreamPairManager.h"
#include "EQStreamPair.h"
#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <string>

using namespace std;

StreamPairManager::StreamPairManager(bool be_quiet)
: Timeoutable(CONNECTION_TIMEOUT/3)	//kinda arbitrary
{
	quiet = be_quiet;
}

	
StreamPairManager::~StreamPairManager() {
}
	
void StreamPairManager::CloseAll() {
	MLock.lock();
	vector<EQStreamPair *>::iterator cur,end;
	cur = pairs.begin();
	end = pairs.end();
	for(; cur != end; cur++) {
		EQStreamPair *p = *cur;
		safe_delete(p);
	}
	pairs.clear();
	streams.Clear();
	MLock.unlock();
}
	
void StreamPairManager::CheckTimeout() {
	unsigned long now = Timer::GetCurrentTime();

	MLock.lock();
	vector<EQStreamPair *>::iterator cur,end;
	cur = pairs.begin();
	end = pairs.end();
	for(; cur != end; cur++) {
		EQStreamPair *p = *cur;
		//if the connection times out, it will return false for IsValid()
		//which will get detected somewhere else (CheckQueues)
		p->CheckTimeouts(now);
	}
	MLock.unlock();
}

void StreamPairManager::Process(uint32 sip, uint16 sport, uint32 dip, uint16 dport, const timeval &ts, const uchar* udp_data, uint32 datalen) {
	//create our stream descriptor
	EQStreamInfo sinfo(sip, dip, sport, dport);
	
	MLock.lock();
	
	EQStreamPair *spair;
	spair = streams.GetStream(sinfo);
	if(spair == NULL) {
		//unknown stream... see if we care... this is not fool proof
		if(udp_data[0] != 0 || udp_data[1] != OP_SessionRequest) {
			MLock.unlock();
			return;
		}
		
		//new stream pair
		spair = new EQStreamPair(quiet);

/*string sips = inet_ntoa(*((struct in_addr *)&sip));
string dips = inet_ntoa(*((struct in_addr *)&dip));
printf("Adding new stream pair 0x%x: %s:%d -> %s:%d\n", spair, sips.c_str(), sport, dips.c_str(), dport);
*/
		streams.AddStream(sinfo, spair);
		pairs.push_back(spair);
	} else if(!spair->IsValid()) {
		MLock.unlock();
		return;
	}
	
	spair->Process(udp_data, datalen, sip, sport, dip, dport, ts.tv_sec, ts.tv_usec);
	
	MLock.unlock();
}

void StreamPairManager::CheckQueues() {
	MLock.lock();
	vector<EQStreamPair *>::iterator cur,end;
	cur = pairs.begin();
	end = pairs.end();
	for(; cur != end;) {
		EQStreamPair *p = *cur;
		
		p->CheckQueues();
		
		//we check validity here so we kill properly closed connections fast
		if(!p->IsValid()) {
			RemovePair(p);
			//reset our iterators since we modified the array
			cur = pairs.begin();
			end = pairs.end();
			//could prolly skip ahead if we cared a lot
			continue;
		}
		cur++;
	}
	MLock.unlock();
}


//assumes MLock is locked.
void StreamPairManager::RemovePair(EQStreamPair *p) {
	vector<EQStreamPair *>::iterator cur,end;
	cur = pairs.begin();
	end = pairs.end();
	while(cur != end) {
		if(*cur == p) {
			pairs.erase(cur);
			//reset our iterators since we modified the array
			cur = pairs.begin();
			end = pairs.end();
			continue;
		}
		cur++;
	}
	
	streams.RemoveStream(p);
	
	safe_delete(p);
}
	






