#include "srvblockchainmonitor.h"
#include <QtCore/QDebug>
#include <QtWebSockets/QWebSocket>
#include <QCoreApplication>
#include <QDebug>
#include <qglobal.h>


QT_USE_NAMESPACE
void SrvBlockchainMonitor::Test_ParseWebMessage()
{
    QString message = "{\n  \"op\" : \"utx\",\n  \"x\" : {\n    \"lock_time\" : 540532,\n    \"ver\" : 1,\n    \"size\" : 257,\n    \"inputs\" : [ {\n      \"sequence\" : 4294967294,\n      \"prev_out\" : {\n        \"spent\" : true,\n        \"tx_index\" : 372665209,\n        \"type\" : 0,\n        \"addr\" : \"1FoWyxwPXuj4C6abqwhjDWdz6D4PZgYRjA\",\n        \"value\" : 1823022,\n        \"n\" : 2,\n        \"script\" : \"76a914a25dec4d0011064ef106a983c39c7a540699f22088ac\"\n      },\n      \"script\" : \"483045022100955e32df1a95f51b4c967ca0e636bc23f308bcd3100a1f9cdb756ac66f33c02b022020332d1cc31550a82a2fc49b89464f2a909211a80607740d5e5a655f6ac0f3c0012103da5bac7b36d5aa38f531c6b9601e21bb598a4b6716ebed38b009a55dabde9440\"\n    } ],\n    \"time\" : 1536439337,\n    \"tx_index\" : 372665905,\n    \"vin_sz\" : 1,\n    \"hash\" : \"cd8c8b315fa81033d169a1d23542361171bceecbd38ac5aa257ff5aec6e51e34\",\n    \"vout_sz\" : 3,\n    \"relayed_by\" : \"0.0.0.0\",\n    \"out\" : [ {\n      \"spent\" : false,\n      \"tx_index\" : 372665905,\n      \"type\" : 0,\n      \"addr\" : null,\n      \"value\" : 0,\n      \"n\" : 0,\n      \"script\" : \"6a146f6d6e69000000000000001f0000000388cc6b00\"\n    }, {\n      \"spent\" : false,\n      \"tx_index\" : 372665905,\n      \"type\" : 0,\n      \"addr\" : \"1CBg9HnptpTYQwmVczyWY4pzeoHRbPqbaw\",\n      \"value\" : 546,\n      \"n\" : 1,\n      \"script\" : \"76a9147aae37d5c0b9269c1c7f34ab668b51c4e2ce58ee88ac\"\n    }, {\n      \"spent\" : false,\n      \"tx_index\" : 372665905,\n      \"type\" : 0,\n      \"addr\" : \"1FoWyxwPXuj4C6abqwhjDWdz6D4PZgYRjA\",\n      \"value\" : 1815689,\n      \"n\" : 2,\n      \"script\" : \"76a914a25dec4d0011064ef106a983c39c7a540699f22088ac\"\n    } ]\n  }\n}";
    int len = message.size();
    int err = MDB_SUCCESS;
    if(len >0)
    {
        AddressOutputItem a_oi;
        QJsonDocument document(QJsonDocument::fromJson(message.toLocal8Bit()));
        QJsonObject root = document.object();
        QJsonValue  array_items = root.value("x");
        QJsonObject sub_root = array_items.toObject();
        //fiil ao_i
        QString curr("BTC");
        a_oi.curr = curr.toStdString();

        QJsonValue sch_item = sub_root.value("time");
        QString valOfItem = sch_item.toString();
        a_oi.time = valOfItem.toInt();

        QDateTime loc_time;
        a_oi.loctime = static_cast<int>(loc_time.currentDateTime().toUTC().toTime_t());

        sch_item = sub_root.value("hash");
        QString val = sch_item.toString();
        a_oi.txid = val.toStdString();

        foreach (const QJsonValue & value, sub_root.value("out").toArray())
        {
            QJsonObject obj = value.toObject();
            QString val_obj = obj["addr"].toString();
            QString ctrl_key(val_obj + "|" + curr);

            std::string addr = val_obj.toStdString();
            if(addr.empty() == true)
            {
                continue;
            }
            if(addresses_to_monitor.contains(ctrl_key))
            {
                a_oi.address = addr;
                a_oi.value = obj["value"].toString().toLongLong();
                qDebug()<<"\r\n$+"<<"WS send tx for "
                       <<a_oi.curr.c_str()
                      <<"-"<<a_oi.address.c_str()
                     <<", value="
                    <<a_oi.value;
                qint32 PIN;
                PIN = static_cast<qint32>(a_oi.value - (a_oi.value / 1000 * 1000));
                if (addresses_to_monitor[ctrl_key].contains(PIN))
                {
                    if (addresses_to_monitor[ctrl_key][PIN].time < a_oi.time)
                    {
                        // Write to Output

                    }
                }
            }

            if(err != MDB_SUCCESS)
            {

                qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
                return;
            }
        }
        //err = monitor->MdbCommit( monitor->txn_AddrInTb) ;

        if(err != MDB_SUCCESS)
        {

            qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
            return;
        }


    }
}
SrvBlockchainMonitor::SrvBlockchainMonitor(parse_arg &arg, QObject *parent) : QObject(parent)
{
    //Test_ParseWebMessage();
    m_env  = nullptr ;
    dbi_AddrInTb = 0;
    dbi_AddrOutTb = 0 ;
    cur_AddrInTb = nullptr;
    cur_AddrOutTb = nullptr ;

    m_url=arg.url;
    tbName_inAddrItem=arg.inAddrTb.c_str();
    tbName_outAddrItem=arg.outAddrTb.c_str();
    obj_Server = new ServerTcp(arg.srcClientPort);

    m_webSocket.setProxy(QNetworkProxy::NoProxy);
    signal(SIGINT,      &SrvBlockchainMonitor::OnSignalexit);
    signal(SIGTERM,  &SrvBlockchainMonitor::OnSignalexit);
}

void SrvBlockchainMonitor::OnSignalexit(int sig)
{
    (void)sig;
    qDebug() << "Shutdown application CTRL+C.";
    QCoreApplication::exit(0);
}

SrvBlockchainMonitor::~SrvBlockchainMonitor()
{

    MdbClose();
    delete obj_Server ;
    delete monitor;

}

MDB_env* SrvBlockchainMonitor::MdbInit(QString &db_root,int &err)
{
    MDB_env* env=nullptr;
    QString str_fun;
    const char* root = db_root.toStdString().c_str();
    size_t size = 1024 * 1024 * 200;
    err = mdb_env_create(&env);
    if(err != MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
        return nullptr;
    }

    mdb_env_set_mapsize (env, size);
    mdb_env_set_maxdbs (env,2);
    err = mdb_env_open(env,root , 0, 0777);
    if(err != MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<<  mdb_strerror(err);
        return nullptr;
    }
    return env;
}

MDB_txn* SrvBlockchainMonitor::MdbBeginTxn(int &err)
{
    MDB_txn* txn;
    err = MDB_SUCCESS;
    int res = mdb_txn_begin(m_env,nullptr, 0, &txn);
    if(res != MDB_SUCCESS)
    {
        err = MDB_BAD_TXN;
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror( MDB_BAD_TXN);
        return nullptr;
    }
    return  txn;
}

void  SrvBlockchainMonitor::MdbEndTxn(MDB_txn* txn)
{
    mdb_txn_abort(txn);
}

int  SrvBlockchainMonitor::MdiGetNumRecords(MDB_txn *txn, MDB_dbi &dbi,int &err)
{
    MDB_stat stat;
    err = mdb_stat (txn, dbi, &stat)	;
    if(err != MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
        return err;
    }
    return static_cast<int>(stat.ms_entries);
}

int  SrvBlockchainMonitor::MdbOpen(MDB_txn* txn, const char* mbd_name, MDB_dbi &dbi )
{

    int res = mdb_dbi_open(txn, mbd_name , MDB_CREATE , &dbi);
    if(res != MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(res);
    }
    return res;

}

MDB_cursor* SrvBlockchainMonitor::MdiCursorEnable(MDB_txn *txn,MDB_dbi  &dbi ,int &err)
{
    MDB_cursor*  cursor = nullptr;
    err = mdb_cursor_open 	(txn , dbi , &cursor);
    if(err != MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
    }
    return cursor;
}

int  SrvBlockchainMonitor::MdbReadRecordByKeyIn(MDB_txn* txn, MDB_dbi &dbi , QString key , AddressInputItem &record)
{
    MDB_val v_key;
    v_key.mv_size = static_cast<unsigned long>(key.size());
    v_key.mv_data = (void*)(key.toStdString().c_str());

    MDB_val v_data;
    int err = mdb_get( txn,  	dbi, 	&v_key, &v_data );
    if(err!=MDB_SUCCESS )
    {
        return err;
    }
    const char* ch_data = static_cast<const char*>(v_data.mv_data);
    record.curr.append(ch_data);
    char *tmp = static_cast<char*>(v_data.mv_data)+record.curr.size()+1;
    record.address.append(static_cast<const char*>(tmp));
    tmp += (record.address.size()+1);
    memcpy(&record.pin,tmp,sizeof(record.pin));
    tmp += sizeof(record.pin);
    memcpy(&record.time,tmp,sizeof(record.time));
    return err ;
}

int  SrvBlockchainMonitor::MdbReadRecordFromCursorIn(MDB_cursor*  cursor , QString &key ,AddressInputItem &record,MDB_cursor_op flag)
{
    MDB_val v_key;
    MDB_val v_data;
    QByteArray bt_data;

    int err = mdb_cursor_get(cursor, &v_key, &v_data, flag);
    if(err!=MDB_SUCCESS )
    {
        return err;
    }
    const char* ch_data = static_cast<const char*>(v_key.mv_data);
    bt_data.append(ch_data,static_cast<int>(v_key.mv_size));
    key=bt_data;

    ch_data = static_cast<const char*>(v_data.mv_data);
    record.curr.append(ch_data);
    char *tmp = static_cast<char*>(v_data.mv_data)+record.curr.size()+1;
    record.address.append(static_cast<const char*>(tmp));
    tmp += (record.address.size()+1);
    memcpy(&record.pin,tmp,sizeof(record.pin));
    tmp += sizeof(record.pin);
    memcpy(&record.time,tmp,sizeof(record.time));
    return err ;
}

int  SrvBlockchainMonitor::MdbWriteRecordIn(MDB_txn* txn,MDB_dbi &dbi,QString key ,AddressInputItem &record)
{

    MDB_val v_key;
    v_key.mv_size = static_cast<unsigned long>(key.size());
    v_key.mv_data = (void*)(key.toStdString().c_str());

    MDB_val data;

    size_t size = record.curr.size()+1 + record.address.size()+1 + sizeof(record.pin) + sizeof(record.time);
    unsigned char *ch_data = static_cast<unsigned char*>(malloc(size));
    memset(ch_data,'\0',size);

    memcpy(ch_data,record.curr.c_str(),record.curr.size());
    unsigned char *tmp=ch_data+record.curr.size()+1;
    memcpy(tmp,record.address.c_str(),record.address.size());
    tmp += (record.address.size()+1);
    memcpy(tmp,&record.pin,sizeof(record.pin));
    tmp +=sizeof(record.pin);
    memcpy(tmp,&record.time,sizeof(record.time));

    data.mv_size = size;
    data.mv_data = ch_data;
    qDebug()<<ch_data;
    int err = mdb_put(txn,dbi,&v_key,&data,0) ;
    if(err != MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
    }
    free(ch_data);
    return err;
}

int  SrvBlockchainMonitor::MdbReadRecordByKeyOut(MDB_txn* txn, MDB_dbi &dbi , QString key , AddressOutputItem &record)
{
    MDB_val v_key;
    v_key.mv_size = static_cast<unsigned long>(key.size());
    v_key.mv_data = (void*)(key.toStdString().c_str());

    MDB_val v_data;
    int err = mdb_get( txn,  	dbi, 	&v_key, &v_data );
    if(err!=MDB_SUCCESS )
    {
        return err;
    }
    const char* ch_data = static_cast<const char*>(v_data.mv_data);
    record.curr.append(ch_data);
    char *tmp = static_cast<char*>(v_data.mv_data)+record.curr.size()+1;
    record.address.append(static_cast<const char*>(tmp));
    tmp += (record.address.size()+1);
    memcpy(&record.time,tmp,sizeof(record.time));
    tmp += sizeof(record.time);
    memcpy(&record.loctime,tmp,sizeof(record.loctime));
    tmp += sizeof(record.loctime);
    memcpy(tmp,&record.txid,record.txid.size());
    tmp +=record.txid.size() + 1;
    memcpy(tmp,&record.value,sizeof(record.value));
    tmp +=sizeof(record.value);
    return err ;
}

int  SrvBlockchainMonitor::MdbReadRecordFromCursorOut(MDB_cursor*  cursor , QString &key ,AddressOutputItem &record,MDB_cursor_op flag)
{
    MDB_val v_key;
    MDB_val v_data;
    QByteArray bt_data;

    int err = mdb_cursor_get(cursor, &v_key, &v_data, flag);
    if(err!=MDB_SUCCESS )
    {
        return err;
    }
    const char* ch_data = static_cast<const char*>(v_key.mv_data);
    bt_data.append(ch_data,static_cast<int>(v_key.mv_size));
    key=bt_data;

    ch_data = static_cast<const char*>(v_data.mv_data);
    record.curr.append(ch_data);
    char *tmp = static_cast<char*>(v_data.mv_data)+record.curr.size()+1;
    record.address.append(static_cast<const char*>(tmp));
    tmp += (record.address.size()+1);
    memcpy(&record.time,tmp,sizeof(record.time));
    tmp += sizeof(record.time);
    memcpy(&record.loctime,tmp,sizeof(record.loctime));
    tmp += sizeof(record.loctime);
    memcpy(tmp,&record.txid,record.txid.size());
    tmp +=record.txid.size() + 1;
    memcpy(tmp,&record.value,sizeof(record.value));
    tmp +=sizeof(record.value);

    return err ;
}

int  SrvBlockchainMonitor::MdbWriteRecordOut(MDB_txn* txn,MDB_dbi &dbi,QString key ,AddressOutputItem &record)
{

    MDB_val v_key;
    v_key.mv_size = static_cast<unsigned long>(key.size());
    v_key.mv_data = (void*)(key.toStdString().c_str());

    MDB_val data;

    size_t size =  record.curr.size()+1
            + record.address.size()+1
            + sizeof(record.time)
            + sizeof(record.loctime)
            + record.txid.size()+1
            + sizeof(record.value);

    unsigned char *ch_data = static_cast<unsigned char*>(malloc(size));
    memset(ch_data,'\0',size);

    memcpy(ch_data,record.curr.c_str(),record.curr.size());
    unsigned char *tmp=ch_data+record.curr.size()+1;
    memcpy(tmp,record.address.c_str(),record.address.size());
    tmp += (record.address.size()+1);
    memcpy(tmp,&record.time,sizeof(record.time));
    tmp +=sizeof(record.time);
    memcpy(tmp,&record.loctime,sizeof(record.loctime));
    tmp +=sizeof(record.loctime);
    memcpy(tmp,&record.txid,record.txid.size());
    tmp +=record.txid.size() + 1;
    memcpy(tmp,&record.value,sizeof(record.value));
    tmp +=sizeof(record.value);

    data.mv_size = size;
    data.mv_data = ch_data;
    qDebug()<<ch_data;
    int err = mdb_put(txn,dbi,&v_key,&data,0) ;
    if(err != MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
    }
    free(ch_data);
    return err;
}

int  SrvBlockchainMonitor::MdbCommit(MDB_txn* txn)
{
    return mdb_txn_commit(txn);
}

void SrvBlockchainMonitor::MdiCursorDisable(MDB_cursor * cursor)
{
    mdb_cursor_close(cursor);
}

void SrvBlockchainMonitor::MdbClose()
{

    if(m_env != nullptr)
    {
        if( cur_AddrInTb != nullptr )
        {
            mdb_cursor_close(cur_AddrInTb);
        }
        if( cur_AddrOutTb != nullptr )
        {
            mdb_cursor_close(cur_AddrOutTb);
        }

        if(dbi_AddrInTb != 0)
        {
            mdb_close(m_env, dbi_AddrInTb);
        }
        if(dbi_AddrOutTb != 0)
        {
            mdb_close(m_env, dbi_AddrOutTb);
        }

        mdb_env_close(m_env);
    }
}

void SrvBlockchainMonitor::startConnection()
{
    qDebug() << "WebSocket client:" << m_url;
    QString root = "./test_db";
    int err = 0;
    m_env = MdbInit(root,err);
    if(m_env ==nullptr)
    {
        switch(err)
        {
        case ENOENT:// - the directory specified by the path parameter doesn't exist.
            qDebug() << "the directory specified by the path parameter doesn't exist";
            break;
        case EACCES:// - the user didn't have permission to access the environment files.
            qDebug() << "the user didn't have permission to access the environment files";
            break;
        case EAGAIN:// - the environment was locked by another process.
            qDebug() << "the environment was locked by another process:";
            break;
        }

        qApp->quit();
    }
    connect(&m_webSocket, &QWebSocket::connected, this, &SrvBlockchainMonitor::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &SrvBlockchainMonitor::onClosed);
    typedef void (QWebSocket:: *sslErrorsSignal)(const QList<QSslError> &);
    connect(&m_webSocket, static_cast<sslErrorsSignal>(&QWebSocket::sslErrors),this, &SrvBlockchainMonitor::onSslErrors);

    QNetworkRequest request=QNetworkRequest(QUrl(m_url));
    m_webSocket.open(request);
    obj_Server->StartListen();

}

void SrvBlockchainMonitor::onConnected()
{
    qDebug() << "WebSocket connected";
    connect(&m_webSocket, &QWebSocket::textMessageReceived,this, &SrvBlockchainMonitor::onTextMessageReceived);
    int err = 	MDB_SUCCESS;
    MDB_txn  *txn;
    txn =  MdbBeginTxn(err);
    if(err!=MDB_SUCCESS )
    {
        MdbEndTxn(txn);
        txn = nullptr;
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<<  mdb_strerror(err);
        return;
    }
    err = MdbOpen(txn,  tbName_inAddrItem.toStdString().c_str(), dbi_AddrInTb );
    if(err!=MDB_SUCCESS )
    {
        MdbEndTxn(txn);
        txn = nullptr;
        dbi_AddrInTb = 0;
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<<  mdb_strerror(err);
        return;
    }

    int num_cn = MdiGetNumRecords(txn, dbi_AddrInTb,err);
    if( num_cn == 0 || err !=MDB_SUCCESS )
    {
        MdbEndTxn(txn);
        txn = nullptr;
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<<  mdb_strerror(err);
        return ;
    }

    cur_AddrInTb = MdiCursorEnable(txn,dbi_AddrInTb ,err);
    if(err !=MDB_SUCCESS)
    {
        qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
        return ;
    }
    for(;;)
    {

        AddressInputItem value;
        QString key;
        err = MdbReadRecordFromCursorIn(cur_AddrInTb ,key ,value,MDB_NEXT);
        if(err !=MDB_SUCCESS)
        {
            qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
            break;
        }

        //        QByteArray record;
        //        QDataStream out(record);
        //        out.setVersion(QDataStream::Qt_DefaultCompiledVersion);
        //        out>>value.address>>value.curr>>value.pin>>value.time;

        if (value.curr == "BTC")
        {
            std::string msg("{\"op\":\"addr_sub\", \"addr\":\"" + value.address + "\"}");
            qint64 send_bytes=m_webSocket.sendTextMessage(QString(msg.c_str()));
            qDebug() << msg.c_str()<<send_bytes;
            QThread::msleep(1000);
        }
        num_cn--;
        if(num_cn == MDB_SUCCESS )
        {
            qDebug() <<"\r\n$+ req to wss is completed "<< mdb_strerror(err);
            break;
        }
    }
    MdbEndTxn(txn);
}

void SrvBlockchainMonitor::stopConnection()
{
    m_webSocket.disconnect();
    qDebug() << "WebSocket stopped";
}

void SrvBlockchainMonitor::onTextMessageReceived(QString message)
{
    QTime currTime;
    currTime = QTime::currentTime();
    //qDebug() <<"\r\n"<< currTime.toString("hh:mm:ss")<<"\r\n"<< message<<"\r\n";
    emit processIncomingMessage(message);
    int len = message.size();
    int err = MDB_SUCCESS;
    if(len >0)
    {
        AddressOutputItem a_oi;
        QJsonDocument document(QJsonDocument::fromJson(message.toLocal8Bit()));
        QJsonObject root = document.object();
        QJsonValue  array_items = root.value("x");
        QJsonObject sub_root = array_items.toObject();
        //fiil ao_i
        QString curr("BTC");
        a_oi.curr = curr.toStdString();

        QJsonValue sch_item = sub_root.value("time");
        QString valOfItem = sch_item.toString();
        a_oi.time = valOfItem.toInt();

        QDateTime loc_time;
        a_oi.loctime = static_cast<int>(loc_time.currentDateTime().toUTC().toTime_t());

        sch_item = sub_root.value("hash");
        QString val = sch_item.toString();
        a_oi.txid = val.toStdString();

        foreach (const QJsonValue & value, sub_root.value("out").toArray())
        {
            QJsonObject obj = value.toObject();
            QString val_obj = obj["addr"].toString();
            QString ctrl_key(curr + "|" + val_obj);

            std::string addr = val_obj.toStdString();
            if(addr.empty() == true)
            {
                continue;
            }
            a_oi.address = addr;
            QVariant val = obj["value"].toVariant();
            a_oi.value = val.toLongLong();

            qDebug()<<"\r\n$+"<<"WS send tx for "
                    <<a_oi.curr.c_str()
                    <<"-"<<a_oi.address.c_str()
                    <<", value="
                    <<a_oi.value;

            if(addresses_to_monitor.contains(ctrl_key))
            {

                qint32 PIN;
                PIN = static_cast<qint32>(a_oi.value - (a_oi.value / 1000 * 1000));
                if (addresses_to_monitor[ctrl_key].contains(PIN))
                {
                    if (addresses_to_monitor[ctrl_key][PIN].time < a_oi.time)
                    {
                        // Write to Output
                        MDB_txn* txn = MdbBeginTxn(err);
                        err = MdbOpen(txn,
                                      tbName_outAddrItem.toStdString().c_str(),
                                      dbi_AddrOutTb);
                        if(err != MDB_SUCCESS)
                        {
                            qDebug() <<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
                            return;
                        }
                        QString ctxid = addresses_to_monitor[ctrl_key][PIN].ctxid;
                        err  =  MdbWriteRecordOut(txn,
                                                  dbi_AddrOutTb,
                                                  ctxid,
                                                  a_oi);
                        if(err != MDB_SUCCESS)
                        {

                            qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
                            return;
                        }

                        err = MdbCommit(txn) ;
                        if(err != MDB_SUCCESS)
                        {

                            qDebug()<<"\r\n$-"<<Q_FUNC_INFO<<__LINE__<<__FILE__<< mdb_strerror(err);
                            return;
                        }
                       qDebug()<<"Add Output: "<<a_oi.curr.c_str()
                               <<" ,address= "<<a_oi.address.c_str()
                               <<" ,PIN= "<<PIN
                               <<" ,txid= "
                               <<a_oi.txid.c_str()
                               <<"  ,ctxid="
                               <<ctxid;
                    }
                }
            }
        }
    }
}

void SrvBlockchainMonitor::onSslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);

    // WARNING: Never ignore SSL errors in production code.
    // The proper way to handle self-signed certificates is to add a custom root
    // to the CA store.

    m_webSocket.ignoreSslErrors();

}

void SrvBlockchainMonitor::onClosed()
{

}
