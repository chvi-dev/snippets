#include <QCoreApplication>
#include "srvblockchainmonitor.h"

SrvBlockchainMonitor* monitor;

void ParseArg( parse_arg &argm);
void ParseArg(parse_arg &argm)
{

    for(int arg = 1; arg < argm.argc; arg++)
    {
        if(!strcmp( argm.argv [ arg ] , "--ws-url" ))
        {
            arg++;
            if(argm.argc <= arg)
            {
                //Usage( );
                system( "pause" );
                exit( -1 );
            }
           argm.url.clear();
          argm. url.setUrl(argm.argv [ arg ]) ;
        }
        else
        {
            if(!strcmp( argm.argv [ arg ] , "--cl-port" ))
            {
                arg++;
                if(argm.argc <= arg)
                {
                    //Usage( );
                    exit( -1 );
                }
                long  val = strtol( argm.argv [ arg ] , nullptr , 10 );
                argm.srcClientPort = static_cast<uint16_t>(val);
             }
              else
            {
                if(!strcmp( argm.argv [ arg ] , "--tb-inaddr" ))
                {
                    arg++;
                    if(argm.argc <= arg)
                    {
                        //Usage( );
                        exit( -1 );
                    }
                    argm.inAddrTb.append(argm.argv [ arg ]);
                }
                 else
                {
                    if(!strcmp( argm.argv [ arg ] , "--tb-outaddr" ))
                    {
                        arg++;
                        if(argm.argc <= arg)
                        {
                            //Usage( );
                            exit( -1 );
                        }
                       argm.outAddrTb.append(argm.argv [ arg ]);
                    }
                }
            }
         }
    }
}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    parse_arg get_cmd_param;
    memset(&get_cmd_param,0,sizeof(get_cmd_param));
    get_cmd_param.argc = argc;
    get_cmd_param.argv = argv;
    ParseArg( get_cmd_param);
    //QUrl url("wss://ws.blockchain.info/inv");

    monitor = new  SrvBlockchainMonitor(get_cmd_param);
    monitor->startConnection();

    Q_UNUSED(monitor);

    return a.exec();
}
