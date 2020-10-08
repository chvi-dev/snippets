#include "izRTSPClientSession.h"

#include <sstream>
#include <string>

IZRTSPClientSession::IZRTSPClientSession(Streamer* stream,RTSPServer& ourServer, u_int32_t sessionId): RTSPServer::RTSPClientSession(ourServer,sessionId)
{
	m_stream = stream;
}

IZRTSPClientSession::~IZRTSPClientSession()
{

}

void IZRTSPClientSession::handleCmd_SETUP(RTSPServer::RTSPClientConnection* ourClientConnection,char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr)
{
	RTSPClientSession::handleCmd_SETUP(ourClientConnection,urlPreSuffix,urlSuffix,fullRequestStr);
}
void IZRTSPClientSession::handleCmd_withinSession(RTSPServer::RTSPClientConnection* ourClientConnection,char const* cmdName,char const* urlPreSuffix, char const* urlSuffix,char const* fullRequestStr)
{
	RTSPClientSession::handleCmd_withinSession(ourClientConnection,cmdName,urlPreSuffix,urlSuffix,fullRequestStr);
}
void IZRTSPClientSession::handleCmd_TEARDOWN(RTSPServer::RTSPClientConnection* ourClientConnection,ServerMediaSubsession* subsession)
{
	RTSPClientSession::handleCmd_TEARDOWN(ourClientConnection,subsession);
}
void IZRTSPClientSession::handleCmd_PLAY(RTSPServer::RTSPClientConnection* ourClientConnection,ServerMediaSubsession* subsession, char const* fullRequestStr)
{
	RTSPClientSession::handleCmd_PLAY(ourClientConnection,subsession,fullRequestStr);
}
void IZRTSPClientSession::handleCmd_PAUSE(RTSPServer::RTSPClientConnection* ourClientConnection,ServerMediaSubsession* subsession)
{
	RTSPClientSession::handleCmd_PAUSE(ourClientConnection,subsession);
}
void IZRTSPClientSession::handleCmd_GET_PARAMETER(RTSPServer::RTSPClientConnection* ourClientConnection,ServerMediaSubsession* subsession, char const* fullRequestStr)
{
	RTSPClientSession::handleCmd_GET_PARAMETER(ourClientConnection,subsession,fullRequestStr);
}
void IZRTSPClientSession::handleCmd_SET_PARAMETER(RTSPServer::RTSPClientConnection* ourClientConnection,ServerMediaSubsession* subsession, char const* fullRequestStr)
{
	RTSPClientSession::handleCmd_SET_PARAMETER(ourClientConnection,subsession,fullRequestStr);
}