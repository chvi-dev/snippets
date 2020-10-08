#include "srvtcpserver.h"
#include <QDataStream>
#include <QTime>
#include <QTcpSocket>
#include <QtNetwork/QNetworkProxy>
#include "srvblockchainmonitor.h"

std::string RandomString(int len);
std::string RandomString(int len)
{
    srand(static_cast<unsigned int>(time(nullptr)));
    std::string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string newstr;
    int pos;
    while(static_cast<int>(newstr.size()) != len)
    {
        pos = ((rand() % (static_cast<int>(str.size()) - 1)));
        newstr += str.substr(static_cast<unsigned long>(pos),1);
    }
    return newstr;
}
ServerTcp::ServerTcp(quint16 srcClientPort,QObject *parent):QTcpServer(parent)
{
    m_pServerTcp = new QTcpServer(this);
    m_nNextBlockSize = 0;
    m_port_no = srcClientPort;
}

ServerTcp::~ServerTcp()
{

}

bool ServerTcp::StartListen()
{
    m_pServerTcp->setProxy(QNetworkProxy::NoProxy);
    bool ret = m_pServerTcp->listen(QHostAddress::Any,m_port_no);
    if (ret == false)
    {
        qDebug() <<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< "Not started";
        return false;
    }
    qDebug() << __LINE__<<__FILE__<< "Server started.Ok";

    connect( m_pServerTcp, SIGNAL(newConnection()), this , SLOT(slotNewConnection()));

    return true;
}
void ServerTcp::slotNewConnection()
{
    QTcpSocket* pClientSocket = m_pServerTcp->nextPendingConnection();
    connect(pClientSocket, SIGNAL(disconnected()),pClientSocket, SLOT(deleteLater()) );
    connect(pClientSocket, SIGNAL(readyRead()), this,SLOT(slotReadClient()));
}

void ServerTcp::slotReadClient()
{

    QTcpSocket* pClientSocket = static_cast<QTcpSocket*>(sender());
    QString receive_json = pClientSocket->readAll();
    qDebug() << pClientSocket->socketDescriptor()<<  " receive data: "<< receive_json;
    int len = receive_json.size();
    if(len >0)
    {
        QJsonDocument document(QJsonDocument::fromJson(receive_json.toLocal8Bit()));
        QJsonObject root = document.object();
        QJsonValue items = root.value("msg");
        QJsonObject s_root = items.toObject();
        QJsonValue n_items = s_root.value("verb");
        if( n_items.isString())
        {
            QString val = n_items.toString();
            QJsonArray jsonArray =s_root.value("items").toArray();
            int err = MDB_SUCCESS;

            if( val == "newtx")
            {
              foreach (const QJsonValue & value, jsonArray)
                {
                    QJsonObject obj = value.toObject();
                    AddressInputItem ai;
                    MDB_txn* txn = monitor->MdbBeginTxn(err);
                    if( err != MDB_SUCCESS )
                    {
                        qDebug() <<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
                        return;
                    }
                    err = monitor->MdbOpen(txn,
                                           monitor->tbName_inAddrItem.toStdString().c_str(),
                                           monitor->dbi_AddrInTb);
                    if(err != MDB_SUCCESS)
                    {
                        monitor->MdbEndTxn(txn);
                        qDebug() <<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
                        return;
                    }
                    QString ctxid = obj["ctxid"].toString();
                    //std::string random_str = RandomString(10);
                    ai.curr = obj["curr"].toString().toStdString();
                    ai.address = obj["address"].toString().toStdString();
                    ai.pin = obj["pin"].toString().toInt();
                    ai.time = obj["time"].toString().toInt();

                    //                    QByteArray  record;
                    //                    QDataStream in(&record, QIODevice::WriteOnly);
                    //                    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
                    //                    in<<ai.address<<ai.curr<<ai.pin<<ai.time;
                    int res =  monitor->MdbWriteRecordIn(txn,
                                                         monitor->dbi_AddrInTb,
                                                         ctxid,
                                                         ai);
                    if(res != MDB_SUCCESS)
                    {
                        monitor->MdbEndTxn(txn);
                        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(res);
                        return;
                    }
                    err = monitor->MdbCommit(txn) ;
                    if(err != MDB_SUCCESS)
                    {
                        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
                        return;
                    }

                    if (ai.curr == "BTC")
                    {
                        qDebug() <<"\r\n" <<"Send to url:"<<monitor->m_webSocket.requestUrl()<<", data request:" ;
                        std::string msg("{\"op\":\"addr_sub\", \"addr\":\"" + ai.address + "\"}");
                        qDebug() << msg.c_str();
                        monitor->m_webSocket.sendTextMessage(QString(msg.c_str()));
                    }

                    TxTime txt;
                    txt.ctxid = ctxid;
                    txt.time = ai.time;

                    if (monitor->addresses_to_monitor.contains(QString(ai.curr.c_str()) + "|" + QString(ai.address.c_str())))
                    {
                        if (monitor->addresses_to_monitor[QString(ai.curr.c_str()) + "|" + QString(ai.address.c_str())].contains(ai.pin))
                        {
                            monitor->addresses_to_monitor[QString(ai.curr.c_str()) + "|" + QString(ai.address.c_str())][ai.pin] = txt;// Same PIN for same address - overwrite !!!
                        }
                        else
                        {
                            monitor->addresses_to_monitor[QString(ai.curr.c_str()) + "|" + QString(ai.address.c_str())].insert(ai.pin, txt); // New PIN for same address
                        }
                    }
                    else
                    {
                        QMap<int, TxTime> item;
                        item.insert(ai.pin, txt);
                        monitor->addresses_to_monitor.insert(QString(ai.curr.c_str()) + "|" + QString(ai.address.c_str()), item);// New address
                    }

                }
            }
            else
            {
                if( val == "checktx")
                {
                    QString  json("{'result':'ok'}");
                    QJsonDocument doc(QJsonDocument::fromJson(json.toLocal8Bit()));
                    QJsonObject jresp = document.object();
                    QJsonArray items;
                    foreach (const QJsonValue & value, jsonArray)
                    {
                        QString ctxid = value.toString();
                        QJsonObject item;
                        item["ctxid"] = ctxid;
                        AddressOutputItem ao;

                        //                      if (tx.ContainsKey(db, Encoding.UTF8.GetBytes(ctxid)))
                        //                      {
                        //                          var result = tx.Get(db, Encoding.UTF8.GetBytes(ctxid));
                        //                          AddressOutputItem ao = ZeroFormatterSerializer.Deserialize<AddressOutputItem>(result);
                        //                          AddEvent("checktx: " + ao.curr + " - address= " + ao.address + " - value= " + ao.value.ToString() + "  -- txid= " + ao.txid + "  -- ctxid=" + ctxid);

                        item["curr"]    = ao.curr.c_str();
                        item["address"] = ao.address.c_str();
                        item["time"]    = ao.time;
                        item["loctime"] = ao.loctime;
                        item["txid"]    = ao.txid.c_str();
                        item["value"]   = ao.value;
                        //}

                        items.push_back(item);
                    }
                }
            }
        }
    }

    Send2client(pClientSocket,"{'result':'ok'}");
}
void ServerTcp::Send2client(QTcpSocket* pSocket, const QString& str)
{
    //    QByteArray  arrBlock;
    //    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    //    out.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    //    //out << quint16(0) << QTime::currentTime() << str;
    //    out << str;
    //    out.device()->seek(0);
    //    //out << quint16(static_cast<size_t>(arrBlock.size()) - sizeof(quint16));

    pSocket->write(str.toStdString().c_str());
}
