#ifndef FTPDIALOG_H
#define FTPDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QDir>
#include <QFileDialog>
#include <QtNetwork/QNetworkInterface>
#include <QMessageBox>
#include "ftpserver.h"

namespace Ui {
class FtpDialog;
}

class FtpDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FtpDialog(QWidget *parent = 0);
    ~FtpDialog();
public slots:
    void display();

private slots:
    void on_pushButton_open_clicked();

    void on_pushButton_cddir_clicked();

    void on_pushButton_usr_clicked();

private:
    Ui::FtpDialog *ui;
    QTimer *timer;
    int count;
    QString path;
    QDir *dir;
    QList<QFileInfo> *file_list;
};

#endif // FTPDIALOG_H
