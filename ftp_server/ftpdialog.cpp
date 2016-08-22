#include "ftpdialog.h"

#include "ui_ftpdialog.h"

extern int clt_count;
extern QString current_path;
extern int is_fchge;
extern char clientip[3][20];
int t_count = 0;
pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;

FtpDialog::FtpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FtpDialog)
{
    ui->setupUi(this);
    timer = new QTimer();
    count = 0;
    path = "//";
	//显示当前ip
    ui->label_ip->setText(QNetworkInterface::allAddresses().at(2).toString());
	//开一个每秒更新一次界面的定时器
    connect(timer, SIGNAL(timeout()), this, SLOT(display()));
    timer->start(1000);
}

FtpDialog::~FtpDialog()
{
    delete ui;
}

//更新界面
void FtpDialog::display()
{
	//有客户断开、连接
    if(count != clt_count)
    {
        if(count < clt_count)
            ui->msg_lb->setText("有新客户连接");
        else
            ui->msg_lb->setText("有客户断开连接");
        count = clt_count;
        ui->count_lb->setText(QString::number(count));

    }
	//当前目录改变/接收到新文件: 刷新显示列表
    if(path != current_path || is_fchge == 1)
    {
        if(is_fchge == 1)
        {
            is_fchge = 0;
            ui->msg_lb->setText("接收到新文件");
        }
        path = current_path;
        ui->dir_lb->setText(path);
        dir = new QDir(path);
        QStringList filter;
        int c, i;
        filter<<"*";
        dir->setNameFilters(filter);
        file_list =new QList<QFileInfo>(dir->entryInfoList((filter)));
        c = file_list->count();
        ui->file_lw->clear();
        for(i = 0; i < c; i++)
        {
            if(file_list->at(i).fileName() != "." && file_list->at(i).fileName() != "..")
                 ui->file_lw->addItem(file_list->at(i).fileName());
        }

        delete dir;
    }



}

//浏览文件的按键事件
void FtpDialog::on_pushButton_open_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("scan file"), " ",  tr("Allfile(*.*)"));
}
//改变当前目录的按键事件
void FtpDialog::on_pushButton_cddir_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("select dir"));
    if(dir != "")
    {
        current_path = dir;
        cd_dir(current_path.toLocal8Bit().data());
    }

}

//查看已连接客户的ip
void FtpDialog::on_pushButton_usr_clicked()
{
    int i=0;
    QString clients="";
    for(;i < 3; i++)
    {
        if(clientip[i]!=NULL)
        {
            clients += QString::number(i+1)+"、"+ QString::fromUtf8(clientip[i])+"\n" ;
        }
    }
    QMessageBox::information(this,"已连接的客户",clients);
}
