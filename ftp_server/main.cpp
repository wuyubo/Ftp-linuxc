#include <QApplication>
#include "ftpdialog.h"
extern int port;

/******************主函数*******************/
int main(int argc, char  *argv[])
{
	QApplication app(argc, argv);

	int ret;
	pthread_t tid;
	port = 2121;
	ret = pthread_create(&tid, NULL, &main_routine, NULL);
	
    FtpDialog fd;
    fd.exec();
	return app.exec();
}
