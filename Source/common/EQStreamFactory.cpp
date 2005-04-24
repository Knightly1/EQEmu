#include "EQStreamFactory.h"
#ifdef WIN32
	#include <winsock2.h>
	#include <process.h>
	#include <windows.h>
	#include <io.h>
	#include <stdio.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/select.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <pthread.h>
#endif
#include <fcntl.h>
#include <iostream>
#include "op_codes.h"
#include "EQStream.h"

using namespace std;

ThreadReturnType EQStreamFactoryReaderLoop(void *eqfs)
{
EQStreamFactory *fs=(EQStreamFactory *)eqfs;
	fs->ReaderLoop();

	THREAD_RETURN(NULL);
}

ThreadReturnType EQStreamFactoryWriterLoop(void *eqfs)
{
	EQStreamFactory *fs=(EQStreamFactory *)eqfs;
	fs->WriterLoop();

	THREAD_RETURN(NULL);
}

EQStreamFactory::EQStreamFactory(EQStreamType type, int port) : Timeoutable(5000)
{
	StreamType=type;
	Port=port;
}

void EQStreamFactory::Close()
{
	Stop();

	close(sock);
	sock=-1;
}

bool EQStreamFactory::Open()
{
struct sockaddr_in address;
#ifndef WIN32
	pthread_t t1,t2;
#endif
	/* Setup internet address information.  
	This is used with the bind() call */
	memset((char *) &address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(Port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Setting up UDP port for new clients */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		return false;
	}

	if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
		close(sock);
		sock=-1;
		return false;
	}
	#ifdef WIN32
		unsigned long nonblock = 1;
		ioctlsocket(sock, FIONBIO, &nonblock);
	#else
		fcntl(sock, F_SETFL, O_NONBLOCK);
	#endif
	//moved these because on windows the output was delayed and causing the console window to look bad
	cout << "Starting factory Reader" << endl;
	cout << "Starting factory Writer" << endl;
	#ifdef WIN32
		_beginthread(EQStreamFactoryReaderLoop,0, this);
		_beginthread(EQStreamFactoryWriterLoop,0, this);
	#else
		pthread_create(&t1,NULL,EQStreamFactoryReaderLoop,this);
		pthread_create(&t2,NULL,EQStreamFactoryWriterLoop,this);
	#endif
	return true;
}

EQStream *EQStreamFactory::Pop()
{
EQStream *s=NULL;
	//cout << "Pop():Locking MNewStreams" << endl;
	MNewStreams.lock();
	if (NewStreams.size()) {
		s=NewStreams.front();
		NewStreams.pop();
		s->SetInUse(true);
	}
	MNewStreams.unlock();
	//cout << "Pop(): Unlocking MNewStreams" << endl;

	return s;
}

void EQStreamFactory::Push(EQStream *s)
{
	//cout << "Push():Locking MNewStreams" << endl;
	MNewStreams.lock();
	NewStreams.push(s);
	MNewStreams.unlock();
	//cout << "Push(): Unlocking MNewStreams" << endl;
}

void EQStreamFactory::ReaderLoop()
{
fd_set readset;
map<string,EQStream *>::iterator stream_itr;
int num;
int length;
unsigned char buffer[2048];
sockaddr_in from;
int socklen=sizeof(sockaddr_in);
timeval sleep_time;

	ReaderRunning=true;
	while(sock!=-1) {
		MReaderRunning.lock();
		if (!ReaderRunning)
			break;
		MReaderRunning.unlock();

		FD_ZERO(&readset);
		FD_SET(sock,&readset);

		sleep_time.tv_sec=30;
		sleep_time.tv_usec=0;
		if ((num=select(sock+1,&readset,NULL,NULL,&sleep_time))<0) {
			// What do we wanna do?
		} else if (num==0)
			continue;

		if (FD_ISSET(sock,&readset)) {
#ifdef WIN32
			if ((length=recvfrom(sock,(char*)buffer,sizeof(buffer),0,(struct sockaddr*)&from,(int *)&socklen))<0) {		
#else
			if ((length=recvfrom(sock,buffer,2048,0,(struct sockaddr *)&from,(socklen_t *)&socklen))<0) {
#endif
				// What do we wanna do?
			} else {
				char temp[25];
				sprintf(temp,"%lu.%d",ntohl(from.sin_addr.s_addr),ntohs(from.sin_port));
				if ((stream_itr=Streams.find(temp))==Streams.end()) {
					if (buffer[1]==OP_SessionRequest) {
						EQStream *s=new EQStream(from);
						s->SetFactory(this);
						s->SetStreamType(StreamType);
						Streams[temp]=s;
						Push(s);
						s->Process(buffer,length);
						s->SetLastPacketTime(Timer::GetCurrentTime());
					}
				} else {
					stream_itr->second->Process(buffer,length);
					stream_itr->second->SetLastPacketTime(Timer::GetCurrentTime());
				}
			}
		}
	}
}

void EQStreamFactory::CheckTimeout()
{
unsigned long now=Timer::GetCurrentTime();
map<string,EQStream *>::iterator stream_itr;
	for(stream_itr=Streams.begin();stream_itr!=Streams.end();) {
		bool in_use=stream_itr->second->InUse();
		int state=stream_itr->second->GetState();
		bool remove_connection=false;
		//cout << "Checking timeout" << endl;
		if (state==CLOSING && !stream_itr->second->HasOutgoingData()) {
			remove_connection=true;

		} else if (state==CLOSED) {
			if (in_use)
				;//stream_itr->second->Closed();
			else
				remove_connection=true;
		} else if (stream_itr->second->CheckTimeout(now,30000)) { 
			cout << "Timeout up!, state=" << state << endl;
			if (state==ESTABLISHED) {
				//if (in_use)
					//stream_itr->second->Timeout();
				stream_itr->second->SendDisconnect();
			} else if (state==CLOSING) {
				stream_itr->second->SetState(CLOSED);
			}
		}

		if (remove_connection) {
			cout << "Removing connection" << endl;
			map<string,EQStream *>::iterator temp=stream_itr;
			stream_itr++;
			delete temp->second;
			Streams.erase(temp);
			continue;
		}

		stream_itr++;
	}
}

void EQStreamFactory::WriterLoop()
{
map<string,EQStream *>::iterator stream_itr;
bool havework=true;
	WriterRunning=true;
	while(sock!=-1) {
		//if (!havework) {
			//WriterWork.Wait();
		//}
		MWriterRunning.lock();
		if (!WriterRunning)
			break;
		MWriterRunning.unlock();

		havework=false;
		for(stream_itr=Streams.begin();stream_itr!=Streams.end();stream_itr++) {
			if (stream_itr->second->HasOutgoingData()) {
				havework=true;
				stream_itr->second->Write(sock);
			}
		}

		Sleep(40);
	}
}

