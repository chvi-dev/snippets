#include "h264LvSrvMediaSession.h"
#include "mJpegStreamSource.h"
#include "ServiceApp.h"
#include "Streamer.h"
h264LvSrvMediaSession*  h264LvSrvMediaSession::createNew(UsageEnvironment& env, bool reuseFirstSource,int fps,int bitrate,Streamer* streamer)
{

	return new h264LvSrvMediaSession(env, reuseFirstSource,fps,bitrate,streamer);
}

h264LvSrvMediaSession::h264LvSrvMediaSession(UsageEnvironment& env, bool reuseFirstSource,int fps,int bitrate,Streamer* streamer):OnDemandServerMediaSubsession(env,reuseFirstSource),fAuxSDPLine(NULL), fDoneFlag(0), fDummySink(NULL),FPS(fps),BR(bitrate), hS_streamer(streamer)
{

}


h264LvSrvMediaSession::~h264LvSrvMediaSession(void)
{
	delete[] fAuxSDPLine;
}


static void afterPlayingDummy(void* clientData)
{
	h264LvSrvMediaSession *session = (h264LvSrvMediaSession*)clientData;

	session->afterPlayingDummy1();

}

void h264LvSrvMediaSession::afterPlayingDummy1()
{

	envir().taskScheduler().unscheduleDelayedTask(nextTask());
	setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
	h264LvSrvMediaSession* session = (h264LvSrvMediaSession*)clientData;
	session->checkForAuxSDPLine1();

}

void h264LvSrvMediaSession::checkForAuxSDPLine1()
{
	char const* dasl;
	if(fAuxSDPLine != NULL)
	{
		setDoneFlag();
	}
	else if(fDummySink != NULL && (dasl = fDummySink->auxSDPLine()) != NULL)
	{
		fAuxSDPLine = strDup(dasl);
		fDummySink = NULL;
		setDoneFlag();
	}
	else
	{
		nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)checkForAuxSDPLine, this);
	}
}

char const* h264LvSrvMediaSession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
	if(fAuxSDPLine != NULL) return fAuxSDPLine;
	if(fDummySink == NULL)
	{
		fDummySink = rtpSink;
		fDummySink->startPlaying(*inputSource, afterPlayingDummy, this);
		checkForAuxSDPLine(this);
	}
	envir().taskScheduler().doEventLoop(&fDoneFlag);
	return fAuxSDPLine;
}

FramedSource* h264LvSrvMediaSession::createNewStreamSource(unsigned clientSessionID, unsigned& estBitRate)
{
	estBitRate=1;
	return H264VideoStreamFramer::createNew(envir(),h264FrameSource::createNew(envir(),hS_streamer,FPS,BR,clientSessionID));
}

RTPSink* h264LvSrvMediaSession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
	H264VideoRTPSink* pSink=H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
	return  pSink;

} 

