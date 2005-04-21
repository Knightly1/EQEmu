#ifndef STREAM_PAIR_MANAGER_H
#define STREAM_PAIR_MANAGER_H

#include "../common/timeoutmgr.h"
#include "../common/Mutex.h"
#include "../common/EQStreamLocator.h"

class EQStreamPair;

class StreamPairManager : public Timeoutable {
public:
	StreamPairManager(bool be_quiet);
	~StreamPairManager();
	
	void CloseAll();
	
	virtual void CheckTimeout();
	
	void Process(uint32 sip, uint16 sport, uint32 dip, uint16 dport, const timeval &ts, const uchar* data, uint32 datalen);
	void CheckQueues();
	
	void SetQuietMode(bool q) { quiet = q; }
	
protected:
	bool quiet;
	void RemovePair(EQStreamPair *p);
	
	EQStreamLocator<EQStreamPair> streams;
	vector<EQStreamPair *> pairs;
	Mutex MLock;
};



#endif


