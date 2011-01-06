/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011  Martin Raiber
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "vld.h"
#ifdef _WIN32
#include <winsock2.h>
#endif
#include "ServiceAcceptor.h"
#include "Server.h"
#include "stringtools.h"
#include "ServiceWorker.h"
#include <memory.h>

#include "Interface/Mutex.h"
#include "Interface/Condition.h"

CServiceAcceptor::CServiceAcceptor(IService * pService, std::string pName, unsigned short port)
{
	name=pName;
	service=pService;
	exitpipe=Server->createMemoryPipe();
	do_exit=false;

	int rc;
#ifdef _WIN32
	WSADATA wsadata;
	rc = WSAStartup(MAKEWORD(2,0), &wsadata);
	if(rc == SOCKET_ERROR)	return;
#endif

	s=socket(AF_INET,SOCK_STREAM,0);
	if(s<1)
	{
		Server->Log(name+": Creating SOCKET failed",LL_ERROR);
		return;
	}

	sockaddr_in addr;

	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=INADDR_ANY;

	rc=bind(s,(sockaddr*)&addr,sizeof(addr));
	if(rc==SOCKET_ERROR)
	{
		Server->Log(name+": Failed binding SOCKET to Port "+nconvert(port),LL_ERROR);
		return;
	}

	listen(s, 10000);

	Server->Log(name+": Server started up sucessfully!",LL_INFO);
}

CServiceAcceptor::~CServiceAcceptor()
{
	do_exit=true;
	closesocket(s);
	for(size_t i=0;i<workers.size();++i)
	{
		workers[i]->stop();
	}
	size_t c=0;
	while(c<workers.size()+1)
	{
		std::string r;
		exitpipe->Read(&r);
		if(r=="ok")
			++c;
	}
	Server->destroy(exitpipe);
	for(size_t i=0;i<workers.size();++i)
	{
		delete workers[i];
	}
}

void CServiceAcceptor::operator()(void)
{
	while(do_exit==false)
	{
		fd_set fdset;
		socklen_t addrsize=sizeof(sockaddr_in);

		FD_ZERO(&fdset);

		FD_SET(s, &fdset);

		timeval lon;
	
		lon.tv_sec=100;
		lon.tv_usec=0;

		_i32 rc=select((int)s+1, &fdset, 0, 0, &lon);

		if( FD_ISSET(s,&fdset) && do_exit==false)
		{
			sockaddr_in naddr;
			SOCKET ns=accept(s, (sockaddr*)&naddr, &addrsize);
			if(ns>0)
			{
				Server->Log(name+": New Connection incomming "+nconvert(Server->getTimeMS())+" s: "+nconvert((int)ns), LL_DEBUG);

#ifdef _WIN32
				int window_size=512*1024;
				setsockopt(ns, SOL_SOCKET, SO_SNDBUF, (char *) &window_size, sizeof(window_size));
				setsockopt(ns, SOL_SOCKET, SO_RCVBUF, (char *) &window_size, sizeof(window_size));
#endif
				AddToWorker(ns);				
			}
		}
	}
	exitpipe->Write("ok");
}

void CServiceAcceptor::AddToWorker(SOCKET pSocket)
{
	for(size_t i=0;i<workers.size();++i)
	{
		if( workers[i]->getAvailableSlots()>0 )
		{
			workers[i]->AddClient(pSocket);
			return;
		}
	}

	Server->Log(name+": No available slots... starting new Worker", LL_DEBUG);

	CServiceWorker *nw=new CServiceWorker(service, name, exitpipe);
	workers.push_back(nw);

	Server->createThread(nw);

	nw->AddClient( pSocket );
}	
