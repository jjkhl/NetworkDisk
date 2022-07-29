#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QWidget>
#include <QListWidget> // 显示文件的列表
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "protocol.h"
#include <QTimer>

class FileSystem : public QWidget
{
    Q_OBJECT
public:
    explicit FileSystem(QWidget *parent = nullptr);
    void updateFileList(PDU* pdu);

    QString strTryEntryDir() const;
    void setStrTryEntryDir(const QString &strTryEntryDir);
    void clearStrTryEntryDir(); // 清空m_strTryEntryDir

public slots:
    void createDir();  // 创建文件夹按钮槽函数
    void flushDir();   // 刷新文件夹按钮槽函数
    void delFileOrDir(); // 删除文件或文件夹按钮槽函数
    void renameFile(); // 重命名文件或文件夹槽函数
    void entryDir(const QModelIndex &index);   // 双击进入文件夹的槽函数
    void returnPreDir(); // 返回上一目录
    void uploadFile();  // 上传文件请求
    void startTimer(); // 开始定时器
    void uploadFileData(); // 上传文件实际数据
    void downloadFile(); // 下载文件请求
    TransFile* getDownloadFileInfo(); // 获取m_downloadFile

signals:

private:
    QListWidget *m_pFileListW;      // 文件列表，显示所有文件

    QPushButton *m_pReturnPrePB;    // 返回上一路径
    QPushButton *m_pCreateDirPB;    // 创建文件夹
    QPushButton *m_pFlushDirPB;     // 刷新文件夹
    QPushButton *m_pDelFileOrDirPB; // 删除文件或文件夹

    QPushButton *m_pRenameFilePB;   // 重命名文件
    QPushButton *m_pUploadFilePB;   // 上传文件
    QPushButton *m_pDownloadFilePB; // 下载文件
    QPushButton *m_pShareFilePB;    // 分享文件

    QString m_strTryEntryDir;       // 临时记录想要进入的目录，如进入成功更新m_strCurPath

    QString m_strUploadFilePath;    // 上传文件的文件路径
    QTimer *m_pTimer;               // 定时器，为了间隔开文件上传时间

    TransFile *m_downloadFile;      // 下载文件属性
};

#endif // FILESYSTEM_H
