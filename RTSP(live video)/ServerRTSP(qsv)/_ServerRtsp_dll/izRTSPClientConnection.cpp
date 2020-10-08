#include "izRTSPClientConnection.h"

#include <sstream>
#include <string>

IZRTSPClientConnection::IZRTSPClientConnection(Streamer* stream,RTSPServer& ourServer, int clientSocket, struct sockaddr_in clientAddr) : RTSPServer::RTSPClientConnection(ourServer, clientSocket, clientAddr)
{
	m_stream = stream;
}

IZRTSPClientConnection::~IZRTSPClientConnection()
{

}

void IZRTSPClientConnection::handleCmd_DESCRIBE(char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr)
{
	std::stringstream ssTemp;
	ssTemp << "!!!! RTSP2 URL arguments:" << "urlPreSuffix: " << std::string(urlPreSuffix) << " urlSuffix: " << std::string(urlSuffix) << " fullRequestStr: " << std::string(fullRequestStr) << std::endl;
	OutputDebugString((char *)ssTemp.str().c_str());
	m_stream->GetLastEncParam();
	m_stream->SetParamByURL(urlSuffix);
	RTSPClientConnection::handleCmd_DESCRIBE(urlPreSuffix, urlSuffix, fullRequestStr);
}
