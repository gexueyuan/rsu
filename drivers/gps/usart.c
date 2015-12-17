/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : usart.c
 @brief  : linux uart api
 @author : wanglei
 @history:
           2015-10-15    wanglei    Created file
           ...
******************************************************************************/
#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>   
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <sys/time.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include "cv_osal.h"

#ifndef OK
#define OK  0
#endif

#ifndef ERR
#define ERR -1
#endif



/*设置串口通信速率*/
static int speed_arr[] = {B230400, B115200, B57600, B38400, \
		B19200, B9600, B4800, B2400, B1800, B1200, B600, B300};
static int name_arr[]  = {230400,  115200,  57600,  38400,  \
		19200,  9600,  4800,  2400,  1800,  1200,  600,  300};



/***************************************************
Name:       set_speed
Function:   设置波特率
Input:      无
Return:     ERR OK
****************************************************/
int uart_set_speed(int fd, int speed)
{ 
	int i;   
	int status;   
	struct termios Opt; 

    if (fd < 0){
        return -1;
    }
	tcgetattr(fd, &Opt);   
	
	for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++)
	{   
		if (speed == name_arr[i]) 
		{   
			tcflush(fd, TCIOFLUSH);   
			cfsetispeed(&Opt, speed_arr[i]);   
			cfsetospeed(&Opt, speed_arr[i]);  
			
			status = tcsetattr(fd, TCSANOW, &Opt);   
			if (status != 0) 
			{   
				perror("tcsetattr fd1");   
				return ERR;
			}  
			
			tcflush(fd,TCIOFLUSH);  
			return OK;
		}  
    } 
	
	return ERR;
}

/***************************************************
Name:       set_parity
Function:   设置串口数据位，停止位和效验位
Input:      1.fd          打开的串口文件句柄
      		2.databits    数据位  取值  为 7  或者 8
      		3.stopbits    停止位  取值为 1 或者2
      		4.parity      效验类型  取值为 N,E,O,S 
Return:     ERR:-1, OK: 0
****************************************************/
int uart_set_parity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;
    if (tcgetattr( fd,&options) != 0){
        fprintf(stderr, "SetupSerial 1");
        return (ERR);
    }
    options.c_cflag &= ~CSIZE;
    switch (databits) /*设置数据位数*/
    {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr, "Unsupported data size\n");
            return (ERR);
    }

    switch (parity)
    {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;    /* Clear parity enable */
            options.c_iflag &= ~INPCK;     /* Enable parity checking */
#if 0
            options.c_iflag &= ~(ICRNL|IGNCR);
            options.c_lflag &= ~(ICANON );
#endif
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/ 
            options.c_iflag |= INPCK;              /* Disnable parity checking */
            break;

        case 'e':
        case 'E':
            options.c_cflag |= PARENB;      /* Enable parity */
            options.c_cflag &= ~PARODD;     /* 转换为偶效验*/  
            options.c_iflag |= INPCK;       /* Disnable parity checking */
            break;

        case 'S':
        case 's':  /*as no parity*/
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        default:
            fprintf(stderr, "Unsupported parity\n");
            return (ERR);
    }

    /* 设置停止位*/   
    switch (stopbits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr, "Unsupported stop bits\n");
            return (ERR);
    }

    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;


    options.c_cc[VTIME] = 150;  /* 设置超时 15 seconds (150)*/
    options.c_cc[VMIN] = 0;

    tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */
    
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        fprintf(stderr, "SetupSerial 3");
        return (ERR);
    }
    return (OK);
}

/**
*@breif 打开串口
*/
int os_device_open(char *dev) 
{ 
#if 1
	int fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);					
#else
	int fd = open(dev, O_RDWR);   					
#endif
   
	if (-1 == fd) {  
		perror("Can't Open Serial Port:%s\n"); 
		return -1;   
	}  
	else {
        fcntl(fd, F_SETFL, 0);
        return fd; 
	}
}

/* read from uart */
int os_device_read(int fd, char *buf, int len)
{
    int nread = 0;
    do {
        nread = read(fd, buf, len);
        if(nread < 0)
        {
            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                continue;
        }
        if(nread == 0)
        {            
            fprintf(stderr, "uart read timeout, len=%d\r\n");
            return ERR;
        }    
    }while(nread < len);
    return nread;     
}

int os_device_write(int fd, void *buf, int len)
{
    int ret = -1;
    ret = write(fd, buf, len);
    if (ret < 0){
        fprintf(stderr, "os_device_write failed. %s\r\n", strerror(ret));
    }
    return ret;
}
