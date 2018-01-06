#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "camera_header.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    v4l = new V4l();
    //ui->setupUi(this);

    connect(ui->ok,SIGNAL(clicked()),this,SLOT(photo()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(showtime()));
    timer->start(33);
}

MainWindow::~MainWindow()
{
    delete ui;
}
/*  v4l不知道在哪里创建对象 */
void MainWindow::photo()
{
    char *temp;
    //V4l v4l;
    temp=(char *)ui->path->text().toStdString().data();
    if(v4l->Save_BMP(temp))
    {
        QMessageBox::information(this,"成功","拍照成功",QMessageBox::Ok);
    }
    else
    {
        QMessageBox::warning(this,"失败","拍照失败",QMessageBox::Ok);
    }
}

void MainWindow::showtime()
{
    //V4l v4l;
    ui->camera->setPixmap(v4l->Get_image());
}

