/* CWebapp Class by Sushi */
#ifndef GAME_SERVER_WEBAPP_H
#define GAME_SERVER_WEBAPP_H

#include <string>
#include <base/tl/array.h>
#include <engine/shared/jobs.h>

#include "webapp/user.h"
#include "webapp/run.h"
#include "webapp/ping.h"
#include "webapp/map.h"

class CWebapp
{
	class CGameContext *m_pGameServer;
	class IServer *m_pServer;
	
	CJobPool m_JobPool;
	
	NETADDR m_Addr;
	NETSOCKET m_Socket;
	
	array<std::string> m_lMapList;
	array<CJob*> m_Jobs;
	
	IDataOut *m_pFirst;
	IDataOut *m_pLast;
	
	bool m_Online;
	
	class CGameContext *GameServer() { return m_pGameServer; }
	class IServer *Server() { return m_pServer; }
	
	void LoadMaps();
	int UpdateJobs();
	
	static void MaplistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);
	
public:
	
	static const char GET[];
	static const char POST[];
	static const char PUT[];
	static const char DOWNLOAD[];
	
	CWebapp(CGameContext *pGameServer);
	~CWebapp();
	
	const char *ApiKey();
	const char *ServerIP();
	const char *MapName();
	
	bool IsOnline() { return m_Online; }
	
	void AddOutput(class IDataOut *pOut);
	void Tick();
	
	bool Connect();
	void Disconnect();
	
	int Send(const void *pData, int Size);
	int Recv(void *pData, int MaxSize);
	std::string SendAndReceive(const char* pString);
	
	CJob *AddJob(JOBFUNC pfnFunc, class IDataIn *pUserData, bool NeedOnline = 1);
};

#endif
