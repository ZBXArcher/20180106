#ifndef CAMERA_HEADER
#define CAMERA_HEADER
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <QtGui>
//#include "yuyv_to_rgb888.h"


#define Video_path		"/dev/video0"	//usb摄像头挂载路径
#define Aim_path		"./test.jpeg"	//目标文件存储路径
#define Image_high		480             //目标图片的分辨率720*1280
#define Image_width		640
#define Video_count     2               //缓冲帧数

typedef struct _buffer										//定义缓冲区结构体
{
    void *start;
    unsigned int length;
}buffer;

typedef unsigned short  WORD;       //定义BMP文件头的数据类型
typedef unsigned int  DWORD;

#pragma pack(2)                     //修改结构体字节对齐规则
typedef struct _BITMAPFILEHEADER{
     WORD     bfType;                //BMP文件头，必须为BM
     DWORD    bfSize;                //文件大小，字节为单位
     DWORD    bfReserved;            //保留字，值为0
     DWORD    bfOffBits;             //位图数据起始位置，此处取54
}BITMAPFILEHEADER;
typedef struct _BITMAPINFOHEADER{
     DWORD    biSize;               //本结构所占字节数，此处取40
     DWORD    biWidth;              //图片宽度，像素为单位
     DWORD    biHeight;             //高度，同上
     WORD     biPlanes;             //目标设备级别，此处必须为1
     WORD     biBitCount;           //像素位数，此处取24（真彩色）
     DWORD    biCompression;        //位图类型，必须为0
     DWORD    biSizeImage;          //位图大小
     DWORD    biXPelsPerMeter;      //水平像素数，此处取0
     DWORD    biYPelsPerMeter;      //竖直像素数，此处取0
     DWORD    biClrUsed;            //位图实际使用的颜色表中的颜色数，此处取0
     DWORD    biClrImportant;       //位图显示过程中重要的颜色数，此处取0
}BITMAPINFOHEADER;
#pragma pack()

class V4l
{
private:
    int fd;					                              //驱动文件句柄
    bool state;                                             //是否打开成功

    buffer *buffers;                                        //原始数据buff

    struct v4l2_capability      cap;		    //V4l2参数结构体
    struct v4l2_fmtdesc 		fmtdesc;
    struct v4l2_format 			fmt;
    struct v4l2_streamparm 		setfps;
    struct v4l2_requestbuffers 	req;
    struct v4l2_buffer 			buf;
    enum   v4l2_buf_type 		type;

    BITMAPFILEHEADER   bf;                                  //BMP图片头
    BITMAPINFOHEADER   bi;
    unsigned char frame_buffer[Image_high*Image_width*3];   //RGB图片buff
    bool YUYV_to_RGB888(void);                              //YUYV转RGB888
    bool V4l_Init(void);                                    //V4L初始化
    bool Get_Frame(void);                                   //获取原始数据
    bool Free_Frame(void);                                  //更新原始数据

    QPixmap image;

public:
    V4l();
    ~V4l();
    QPixmap Get_image(void);                                //获取图片
    bool Get_state(void);                                   //获取当前V4L的状态
    bool Save_BMP(char *path);                              //保存BMP图片
};




#endif // CAMERA_HEADER

