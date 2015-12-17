/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : loop_buf.h
 @brief  : loop_buf.h header file
 @author : wanglei
 @history:
           2015-10-29    wanglei    Created file
           ...
******************************************************************************/

#ifndef __LOOP_BUF_H__
#define __LOOP_BUF_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <pthread.h>

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define LOOP_BUF_LEN 4096

#define min(x,y) ({ \
typeof(x) _x = (x); \
typeof(y) _y = (y); \
(void) (&_x == &_y);   \
_x < _y ? _x : _y; })

/*----------------------------------------------*
 * 类型定义                                     *
 *----------------------------------------------*/
typedef struct loop_buf {
    pthread_mutex_t mutex;
    unsigned char buffer[LOOP_BUF_LEN];
    unsigned int in;
    unsigned int out;
}loop_buf_t;

/*----------------------------------------------*
 * 常量、变量声明                               *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 函数声明                                     *
 *----------------------------------------------*/

extern void fini_loop_buf(loop_buf_t *buf);
extern int init_loop_buf(loop_buf_t *buf);
extern int loop_buf_isempty(loop_buf_t *buf);
extern int loop_buf_isfull(loop_buf_t *buf);
extern int loop_buf_pop_n(loop_buf_t *buf, unsigned char *p, unsigned int len);
extern int loop_buf_push_n(loop_buf_t *buf, unsigned char *p, unsigned int len);
extern unsigned int loop_buf_remain(loop_buf_t *buf);
extern void loop_buf_reset(loop_buf_t *buf);
extern unsigned int loop_buf_sz(loop_buf_t *buf);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __LOOP_BUF_H__ */
