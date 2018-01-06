#include "mainwindow.h"
//#include "yuyv_to_rgb888.h"
#include "camera_header.h"
#include <QApplication>

V4l::V4l()
{

    bi.biSize = 40;                                     //设定BMP图片头
    bi.biWidth = Image_width;
    bi.biHeight = Image_high;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = 0;
    bi.biSizeImage = Image_width*Image_high*3;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    bf.bfType = 0x4d42;
    bf.bfSize = 54 + bi.biSizeImage;
    bf.bfReserved = 0;
    bf.bfOffBits = 54;

    if(V4l_Init())                                      //初始化V4L
        state=true;
    else
        state=false;
}

V4l::~V4l()
{
    if(fd != -1)
    {
        ioctl(fd, VIDIOC_STREAMOFF, buffers);     	//结束图像显示
        close(fd);											//关闭视频设备
    }
    if(buffers!=NULL)                                      //释放申请的内存
    {
        free(buffers);
    }
}

bool V4l::V4l_Init()
{
    int n_buffers;
    if((fd=open(Video_path,O_RDWR)) == -1)                      //读写方式打开摄像头
    {
        qDebug()<<"Error opening V4L interface";                //send messege
        return false;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)                 //检查cap的属性
    {
        qDebug()<<"Error opening device "<<Video_path<<": unable to query device.";
        return false;
    }
    else                                                        //打印cap信息
    {
        qDebug()<<"driver:\t\t"         <<QString::fromLatin1((char *)cap.driver);			//驱动名
        qDebug()<<"card:\t\t"           <<QString::fromLatin1((char *)cap.card);			//Device名
        qDebug()<<"bus_info:\t\t"       <<QString::fromLatin1((char *)cap.bus_info);		//在Bus系统中存放位置
        qDebug()<<"version:\t\t"        <<cap.version;                                      //driver 版本
        qDebug()<<"capabilities:\t"     <<cap.capabilities;                                 //能力集,通常为：V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING
    }

    fmtdesc.index=0;                                            //获取摄像头支持的格式
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qDebug()<<"Support format:";
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        qDebug()<<"\t\t"<<fmtdesc.index+1<<QString::fromLatin1((char *)fmtdesc.description);
        fmtdesc.index++;
    }

    fmt.type 				=V4L2_BUF_TYPE_VIDEO_CAPTURE;	//设置像素格式
    fmt.fmt.pix.pixelformat 	=V4L2_PIX_FMT_YUYV;             //使用YUYV格式输出
    fmt.fmt.pix.height 			=Image_high;				//设置图像尺寸
    fmt.fmt.pix.width 			=Image_width;
    fmt.fmt.pix.field 			=V4L2_FIELD_INTERLACED;         //设置扫描方式
    if(ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        qDebug()<<"Unable to set format";
        return false;
    }
    if(ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)                     //重新读取结构体，以确认完成设置
    {
        qDebug()<<"Unable to get format";
        return false;
    }
    else
    {
        qDebug()<<"fmt.type:\t\t"   <<fmt.type;
        qDebug()<<"pix.height:\t"   <<fmt.fmt.pix.height;
        qDebug()<<"pix.width:\t\t"  <<fmt.fmt.pix.width;
        qDebug()<<"pix.field:\t\t"  <<fmt.fmt.pix.field;
    }

    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                  //设置预期的帧率，实际值不一定能达到
    setfps.parm.capture.timeperframe.denominator = 30;          //fps=30/1=30
    setfps.parm.capture.timeperframe.numerator = 1;
    if(ioctl(fd, VIDIOC_S_PARM, &setfps)==-1)
    {
        qDebug()<<"Unable to set fps";
        return false;
    }
    if(ioctl(fd, VIDIOC_G_PARM, &setfps)==-1)
    {
        qDebug()<<"Unable to get fps";
        return false;
    }
    else
    {
        qDebug()<<"fps:\t\t"<<setfps.parm.capture.timeperframe.denominator/setfps.parm.capture.timeperframe.numerator;
    }
    qDebug()<<"init "<<Video_path<<" \t[OK]\n";

    req.count=Video_count;							   //申请2个缓存区
    req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;					//Buffer的类型。此处肯定为V4L2_BUF_TYPE_VIDEO_CAPTURE
    req.memory=V4L2_MEMORY_MMAP;						//Memory Mapping模式，则此处设置为：V4L2_MEMORY_MMAP
    if(ioctl(fd,VIDIOC_REQBUFS,&req)==-1)
    {
        qDebug()<<"request for buffers error";
        return false;
    }

    buffers = (buffer *)malloc(req.count*sizeof (*buffers));			//malloc缓冲区
    if (!buffers)
    {
        qDebug()<<"Out of memory";
        return false ;
    }
    for (n_buffers = 0; n_buffers < Video_count; n_buffers++)           //mmap四个缓冲区
    {
        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = n_buffers;
       //query buffers
        if (ioctl (fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            qDebug()<<"query buffer error";
            return false;
        }

        buffers[n_buffers].length = buf.length;
         //map
        buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ |PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[n_buffers].start == MAP_FAILED)
        {
            qDebug()<<"buffer map error";
            return false;
        }
    }
    for (n_buffers = 0; n_buffers < Video_count; n_buffers++)   //更新buff
    {
        buf.index = n_buffers;
        ioctl(fd, VIDIOC_QBUF, &buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl (fd, VIDIOC_STREAMON, &type);                         //开始采集

    return true;
}
bool V4l::Get_Frame(void)                                       //获取原始数据
{
    if(ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
    {
        return false;
    }
    return true;
}

bool V4l::Free_Frame()                                          //允许更新原始数据
{
    if(ioctl(fd, VIDIOC_QBUF, &buf) == -1)
    {
        return false;
    }
    return true;
}

bool V4l::Get_state(void)                                       //获取当前摄像头状态
{
    return state;
}

bool V4l::YUYV_to_RGB888(void)                                  //图片转码
{
    int     i,j;
    unsigned char y1,y2,u,v,r1,b1,r2,b2;
    int     g1,g2;
    char    *pointer;
    int high,width;

    pointer = (char *)buffers[0].start;
    high=Image_high;
    width=Image_width/2;
    for(i=0;i<high;i++)
    {
        for(j=0;j<(Image_width/2);j++)
        {
            y1 = *( pointer + (i*width+j)*4);
            u  = *( pointer + (i*width+j)*4 + 1);
            y2 = *( pointer + (i*width+j)*4 + 2);
            v  = *( pointer + (i*width+j)*4 + 3);


            r1 = y1 + 1.042*(v-128);//r1=(int)R_table[y1][v];
            g1 = y1 - 0.34414*(u-128)-0.71414*(v-128);//g1=y1-UV_table[u][v];
            b1 = y1 + 1.772*(u-128);//b1=(int)B_table[y1][u];

            r2 = y2 + 1.042*(v-128);//r2=(int)R_table[y2][v];
            g2 = y2-0.34414*(u-128)-0.71414*(v-128);//g2=y2-UV_table[u][v];
            b2 = y2 + 1.772*(u-128);//b2=(int)B_table[y2][u];

            if(g1>255)      g1 = 255;
            else if(g1<0)   g1 = 0;

            if(g2>255)      g2 = 255;
            else if(g2<0)   g2 = 0;

            *(frame_buffer + ((high-1-i)*width+j)*6    ) = b1;
            *(frame_buffer + ((high-1-i)*width+j)*6 + 1) = (unsigned char)g1;
            *(frame_buffer + ((high-1-i)*width+j)*6 + 2) = r1;
            *(frame_buffer + ((high-1-i)*width+j)*6 + 3) = b2;
            *(frame_buffer + ((high-1-i)*width+j)*6 + 4) = (unsigned char)g2;
            *(frame_buffer + ((high-1-i)*width+j)*6 + 5) = r2;
        }
    }
    return true;
}

QPixmap V4l::Get_image(void)
{
    Get_Frame();
    YUYV_to_RGB888();

    QByteArray temp;
    temp.append((char *)&bf,14);
    temp.append((char *)&bi,40);
    temp.append((char *)frame_buffer,Image_high*Image_width*3);
    image.loadFromData(temp);

    Free_Frame();
    return image;
}

bool V4l::Save_BMP(char *path)
{
    Get_Frame();
    YUYV_to_RGB888();
    FILE *fp1;
    fp1=fopen(path,"wb");
    if(fp1==NULL)
    {
        qDebug()<<"open file fail";
        return false;
    }
    fwrite(&bf.bfType, 14, 1, fp1);
    fwrite(&bi.biSize, 40, 1, fp1);
    fwrite(frame_buffer, bi.biSizeImage, 1, fp1);
    fclose(fp1);

    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
