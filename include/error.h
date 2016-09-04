/******************************************************************************
	项目名称	：V2X项目
	文件		：uart.h
	描述		：本文件定义了串口数据流设备模块的接口
	版本		：0.1
	作者		：彭瑞
	创建日期	：2012.6.24
******************************************************************************/

#ifndef ERROR_H_
#define ERROR_H_
/*************************************************
  宏定义
*************************************************/


#define ERR_SYS			1		/* 系统异常 */
#define ERR_NODEV		2		/* 无此设备 */
#define ERR_NOFILE		2		/* 文件不存在 */
#define ERR_TIMEOUT		3		/* 超时 */
#define ERR_INVAL		4		/* 参数值无效 */
#define ERR_NOFUN		5		/* 无此功能 */
#define ERR_BUSY		6		/* 设备忙 */
#define ERR_NOINIT		7		/* 未初始化 */
#define	ERR_NOMEM		8		/* 无可用内存 */
#define ERR_NODISK		9		/* 无可用存储空间 */
#define ERR_NORECORD	10     	/* 记录不存在 */
#define ERR_CFG			11     	/* 配置出错 */
#define ERR_NOCFG		12		/* 未配置 */
#define ERR_DEVUNSTAB	13     	/* 设备不稳定 */
#define ERR_DISCONNECT	14      /* 连接已断开 */

#define ERR_OTHER		80		/* 其他错误 */
#endif /* ERROR_H_ */
