#include "srvthread.h"

#include <QJsonDocument>
#include <QJsonObject>

CThread ::CThread (qintptr id, QObject *parent):QThread(parent)
{
 this->socketDescriptor = id;
}

void CThread::ThreadRun()
{
    qDebug() << socketDescriptor<<  " Starting thread";
    socket = new QTcpSocket();
    if (!socket->setSocketDescriptor(this->socketDescriptor))
    {
        emit Error(socket->error());
        return;
    }
    connect(socket, SIGNAL(DataRead()), this, SLOT(DataRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(Disconnected()), this, SLOT(Disconnected()), Qt::DirectConnection);
    qDebug() << socketDescriptor<<  " Client connected";
    exec();
}

void CThread::DataRead()
{
    QByteArray Data = socket->readAll();
    qDebug() << socketDescriptor<<  " Data is: "<< Data;
    QVariant id(1), name("John Doe");
    QJsonObject json;
    json["Name"] = name.toString();
    json.insert("id", id.toString());
    QJsonDocument doc(json);
    QByteArray bytes = doc.toJson();
    qDebug() << bytes;
    socket->write(bytes);
 
}

void CThread::Disconnected()
{
    qDebug() << socketDescriptor<<  " Disconnected: ";
    socket->deleteLater();
    exit(0);
}
