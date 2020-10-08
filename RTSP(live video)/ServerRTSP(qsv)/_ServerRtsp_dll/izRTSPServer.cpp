#include "izRTSPServer.h"
#include "izRTSPClientConnection.h"
#include "izRTSPClientSession.h"
#include <liveMedia.hh>

#include <sstream>
#include <string>
IZRTSPServer::IZRTSPServer(Streamer* stream, UsageEnvironment& env,
	int ourSocket, Port ourPort,
	UserAuthenticationDatabase* authDatabase,
	unsigned reclamationTestSeconds) : RTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds)
{
	m_stream = stream;
}

IZRTSPServer::~IZRTSPServer()
{

}

IZRTSPServer* IZRTSPServer::createNew(Streamer* stream, UsageEnvironment& env, Port ourPort,
	UserAuthenticationDatabase* authDatabase,
	unsigned reclamationTestSeconds)
{
	int ourSocket = setUpOurSocket(env, ourPort);
	if (ourSocket == -1) return NULL;

	return new IZRTSPServer(stream, env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
}

RTSPServer::RTSPClientConnection* IZRTSPServer::createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr)
{	 

	return new IZRTSPClientConnection(m_stream, *this, clientSocket, clientAddr);	
}
RTSPServer::RTSPClientSession* IZRTSPServer::createNewClientSession(u_int32_t sessionId)
{
	return new IZRTSPClientSession(m_stream, *this,sessionId);	
}

ServerMediaSession* IZRTSPServer::lookupServerMediaSession(char const* streamName)
{
	std::string streamNameStr = std::string(streamName);
	std::string::size_type pos = streamNameStr.find('?');
	if (pos != std::string::npos)
	{
		std::string correctNameStr = streamNameStr.substr(0, pos);
		return RTSPServer::lookupServerMediaSession(correctNameStr.c_str());
	}
	
	return RTSPServer::lookupServerMediaSession(streamName);
}
