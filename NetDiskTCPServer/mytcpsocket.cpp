#include "mytcpsocket.h"
#include "mytcpserver.h"

MyTcpSocket::MyTcpSocket()
{
    connect(this, SIGNAL(readyRead()), // 当接收到客户端的数据时，服务器会发送readyRead()信号
            this, SLOT(receiveMsg())); // 需要由服务器的相应receiveMsg槽函数进行处理
    connect(this, SIGNAL(disconnected()), this, SLOT(handleClientOffline())); // 关联Socket连接断开与客户端下线处理槽函数
}

QString MyTcpSocket::getStrName()
{
    return m_strName;
}

// 处理注册请求并返回响应PDU
PDU* handleRegistRequest(PDU* pdu)
{
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(caName, pdu -> caData, 32);
    strncpy(caPwd, pdu -> caData + 32, 32);
    // qDebug() << pdu -> uiMsgType << " " << caName << " " << caPwd;
    bool ret = DBOperate::getInstance().handleRegist(caName, caPwd); // 处理请求，插入数据库

    // 响应客户端
    PDU *resPdu = mkPDU(0); // 响应消息
    resPdu -> uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
    if(ret)
    {
        strcpy(resPdu -> caData, REGIST_OK);
    }
    else
    {
        strcpy(resPdu -> caData, REGIST_FAILED);
    }
    // qDebug() << resPdu -> uiMsgType << " " << resPdu ->caData;

    return resPdu;
}

// 处理登录请求并返回响应PDU
PDU* handleLoginRequest(PDU* pdu, QString& m_strName)
{
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(caName, pdu -> caData, 32);
    strncpy(caPwd, pdu -> caData + 32, 32);
    // qDebug() << pdu -> uiMsgType << " " << caName << " " << caPwd;
    bool ret = DBOperate::getInstance().handleLogin(caName, caPwd); // 处理请求，插入数据库

    // 响应客户端
    PDU *resPdu = mkPDU(0); // 响应消息
    resPdu -> uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
    if(ret)
    {
        memcpy(resPdu -> caData, LOGIN_OK, 32);
        memcpy(resPdu -> caData + 32, caName, 32); // 将登录后的用户名传回，便于tcpclient确认已经登陆的用户名
        // 在登陆成功时，记录Socket对应的用户名
        m_strName = caName;
        // qDebug() << "m_strName: " << m_strName;
    }
    else
    {
        strcpy(resPdu -> caData, LOGIN_FAILED);
    }
    qDebug() << "登录处理：" << resPdu -> uiMsgType << " " << resPdu ->caData << " " << resPdu ->caData + 32;

    return resPdu;
}

// 处理查询所有在线用户的请求
PDU* handleOnlineUsersRequest()
{
    QStringList strList = DBOperate::getInstance().handleOnlineUsers(); // 查询请求，查询数据库所有在线用户
    uint uiMsgLen = strList.size() * 32; // 消息报文的长度

    // 响应客户端
    PDU *resPdu = mkPDU(uiMsgLen); // 响应消息
    resPdu -> uiMsgType = ENUM_MSG_TYPE_ONLINE_USERS_RESPOND;
    // qDebug() << "在线用户数：" << strList.size();
    for(int i = 0; i < strList.size(); ++ i)
    {
        memcpy((char*)(resPdu -> caMsg) + 32 * i, strList[i].toStdString().c_str(), strList[i].size());
        // qDebug() << "所有在线用户有：" << (char*)(resPdu -> caMsg) + 32 * i;
    }

    return resPdu;
}

// 处理查找用户的请求
PDU* handleSearchUserRequest(PDU* pdu)
{
    char caName[32] = {'\0'};
    strncpy(caName, pdu -> caData, 32);
    int ret = DBOperate::getInstance().handleSearchUser(caName); // 处理请求

    // 响应客户端
    PDU *resPdu = mkPDU(0); // 响应消息
    resPdu -> uiMsgType = ENUM_MSG_TYPE_SEARCH_USER_RESPOND;
    if(ret == 1)
    {
        strcpy(resPdu -> caData, SEARCH_USER_OK);
    }
    else if(ret == 0)
    {
        strcpy(resPdu -> caData, SEARCH_USER_OFFLINE);
    }
    else
    {
        strcpy(resPdu -> caData, SEARCH_USER_EMPTY);
    }

    return resPdu;
}

// 处理添加好友请求
PDU* handleAddFriendRequest(PDU* pdu)
{
    char addedName[32] = {'\0'};
    char sourceName[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(addedName, pdu -> caData, 32);
    strncpy(sourceName, pdu -> caData + 32, 32);
    qDebug() << "handleAddFriendRequest  " << addedName << " " << sourceName;
    int iSearchUserStatus = DBOperate::getInstance().handleAddFriend(addedName, sourceName);
    // 0对方存在不在线，1对方存在在线，2不存在，3已是好友，4请求错误
    PDU* resPdu = NULL;

    switch (iSearchUserStatus) {
    case 0: // 0对方存在不在线
    {
        resPdu = mkPDU(0);
        resPdu -> uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        strcpy(resPdu -> caData, ADD_FRIEND_OFFLINE);
        break;
    }
    case 1: // 1对方存在在线
    {
        // 需要转发给对方请求添加好友消息
        MyTcpServer::getInstance().forwardMsg(addedName, pdu);

        resPdu = mkPDU(0);
        resPdu -> uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        strcpy(resPdu -> caData, ADD_FRIEND_OK); // 表示加好友请求已发送
        break;
    }
    case 2: // 2用户不存在
    {
        resPdu = mkPDU(0);
        resPdu -> uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        strcpy(resPdu -> caData, ADD_FRIEND_EMPTY);
        break;
    }
    case 3: // 3已是好友
    {
        resPdu = mkPDU(0);
        resPdu -> uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        strcpy(resPdu -> caData, ADD_FRIEND_EXIST);
        break;
    }
    case 4: // 4请求错误
    {
        resPdu = mkPDU(0);
        resPdu -> uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        strcpy(resPdu -> caData, UNKNOWN_ERROR);
        break;
    }
    default:
        break;
    }

    return resPdu;
}

// 同意加好友
void handleAddFriendAgree(PDU* pdu)
{
    char addedName[32] = {'\0'};
    char sourceName[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(addedName, pdu -> caData, 32);
    strncpy(sourceName, pdu -> caData + 32, 32);

    // 将新的好友关系信息写入数据库
    DBOperate::getInstance().handleAddFriendAgree(addedName, sourceName);

    // 服务器需要转发给发送好友请求方其被同意的消息
    MyTcpServer::getInstance().forwardMsg(sourceName, pdu);
}

// 拒绝加好友
void handleAddFriendReject(PDU* pdu)
{
    char sourceName[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(sourceName, pdu -> caData + 32, 32);
    // 服务器需要转发给发送好友请求方其被拒绝的消息
    MyTcpServer::getInstance().forwardMsg(sourceName, pdu);
}

void MyTcpSocket::receiveMsg()
{
    // qDebug() << this -> bytesAvailable(); // 输出接收到的数据大小
    uint uiPDULen = 0;
    this -> read((char*)&uiPDULen, sizeof(uint)); // 先读取uint大小的数据，首个uint正是总数据大小
    uint uiMsgLen = uiPDULen - sizeof(PDU); // 实际消息大小，sizeof(PDU)只会计算结构体大小，而不是分配的大小
    PDU *pdu = mkPDU(uiMsgLen);
    this -> read((char*)pdu + sizeof(uint), uiPDULen - sizeof(uint)); // 接收剩余部分数据（第一个uint已读取）
    // qDebug() << pdu -> uiMsgType << ' ' << (char*)pdu -> caMsg; // 输出

    // 根据不同消息类型，执行不同操作
    PDU* resPdu = NULL; // 响应报文
    switch(pdu -> uiMsgType)
    {
    case ENUM_MSG_TYPE_REGIST_REQUEST: // 注册请求
    {
        resPdu = handleRegistRequest(pdu); // 请求处理
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST: // 登录请求
    {
        resPdu = handleLoginRequest(pdu, m_strName);
        break;
    }
    case ENUM_MSG_TYPE_ONLINE_USERS_REQUEST: // 查询所有在线用户请求
    {
        resPdu = handleOnlineUsersRequest();
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USER_REQUEST: // 查找用户请求
    {
        resPdu = handleSearchUserRequest(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: // 添加好友请求
    {
        resPdu = handleAddFriendRequest(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE: // 同意加好友
    {
        handleAddFriendAgree(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REJECT: // 拒绝加好友
    {
        handleAddFriendReject(pdu);
        break;
    }
    default:
        break;
    }

    // 响应客户端
    if(NULL != resPdu)
    {
        // qDebug() << resPdu -> uiMsgType << " " << resPdu ->caData;
        this -> write((char*)resPdu, resPdu -> uiPDULen);
        // 释放空间
        free(resPdu);
        resPdu = NULL;
    }
    // 释放空间
    free(pdu);
    pdu = NULL;
}

void MyTcpSocket::handleClientOffline()
{
    DBOperate::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this); // 发送给mytcpserver该socket删除信号
}



