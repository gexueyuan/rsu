/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cli_user_lib.c
 @brief  : cli user info share memary
 @author : wanglei
 @history:
           2015-12-1    wanglei    Created file
           ...
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "cli_user_lib.h"

#define USER_CONTROL_SHM_SEM_KEY   700
#define USER_CONTROL_SHM_KEY   701

strUserInfo *g_astUserArr = NULL;

#ifndef _SEMUN_H 
#define _SEMUN_H 
union semun { 
    int val; /* value for SETVAL */ 
    struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */ 
    unsigned short int *array;  /* array for GETALL, SETALL */ 
    struct seminfo *__buf; /* buffer for IPC_INFO */ 
}; 
#endif 

typedef struct UserMem
{
    strUserInfo astUserArr[MAX_CLI_USER];
}strUserMem;

//#define _DEBUG_TEST_
static int g_shm_id;
static strUserMem *g_pstUserMem;
static int g_sem_id;


static int get_shmid(int key, unsigned int size)
{
    int shm_id = 0;
    struct shmid_ds buf;

    shm_id = shmget(key, size, (0600 | IPC_EXCL | IPC_CREAT));
    if (shm_id < 0)
    {
		/* File exists is ok */
		if (errno != EEXIST)
		{
            printf("get shm_id failed :%s\r\n",strerror(errno));
            return -1;
		}
        else
        {
            shm_id = shmget(key, 0, (0666 | IPC_EXCL));
            if (shm_id < 0)
            {
                printf("get shm_id failed :%s\r\n",strerror(errno));
                return -1;
            }
            shmctl(shm_id, IPC_STAT, &buf);

            if (size > buf.shm_segsz)
            {
                printf("shm truncate shm size[%u],in size[%u]\r\n", buf.shm_segsz, size);
                return -1;
            }
#ifdef _DEBUG_TEST_            
            else
            {
                printf("shm shm size[%u],in size[%u]\r\n", buf.shm_segsz, size);                
            }
#endif            
        }
    }

#ifdef _DEBUG_TEST_
    printf("shm_id :%d\r\n",shm_id);
#endif
    return shm_id;
}

static int dt_shm(void *addr)
{
    int res = 0;
    struct shmid_ds buf;

    res = shmdt(addr);
    if (res < 0)
    {
        printf("detach shmdt failed :%s\r\n",strerror(errno));
        return -1;
    }

    shmctl(g_shm_id, IPC_STAT, &buf);

    if (0 == buf.shm_nattch)
    {
        shmctl(g_shm_id, IPC_RMID, 0);
    }
    return 0;
}

static void *get_shm_addr(void *shmaddr, int flag)
{
    void *p_addr = NULL;

    p_addr = shmat(g_shm_id, shmaddr, flag);
    if (p_addr ==  (void *)-1)
    {
        printf("shmat failed:%s\r\n",strerror(errno));
        return NULL;
    }

    return p_addr;
}

static int user_sem_init(int key)
{
	int tr_semid;
    union semun sem_val;

    if ((tr_semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1)
    {
		/* File exists is ok */
		if (errno == EEXIST)
		{
            tr_semid = semget(key, 0, 0);
			return tr_semid;
		}
		else
		{
		    return -1;
		}
    }

    sem_val.val = 1;
    if (semctl(tr_semid, 0, SETVAL, sem_val) == -1)
    {
		printf("error: semctl!\n");
        return -1;
    }	

    return tr_semid;
}

int user_sem_lock(void)
{
    static struct sembuf lock =
    {
        0, -1, SEM_UNDO
    };

	if (semop(g_sem_id, &lock, 1) == -1)
	{

		printf("error: semop!\n");
        return -1;

	}//if lock
	
	return 0;
}

int user_sem_unlock(void)
{
    static struct sembuf unlock =
    {
        0, 1, SEM_UNDO | IPC_NOWAIT
    };

	if (semop(g_sem_id, &unlock, 1) == -1)
	{
		printf("error: semop!\n");
        return -1;
	}

	return 0;
}

/*****************************************************************************
 函 数 名  : get_user_info
 功能描述  : 获取用户信息
 输入参数  : user_name
 输出参数  : *online
 返 回 值  : 
*****************************************************************************/
int get_cli_user_info(char *user_name, char *user_pwd, int *pri, int *online, int *terminal_type)
{
    int i;

    user_sem_lock();
    printf("your name is %s\n",user_name);
    for (i=0; i<MAX_CLI_USER; ++i)
    {
        printf("store name is %s\n",g_astUserArr[i].szName);
        if (0 == strcmp(g_astUserArr[i].szName, user_name))
        {
            *pri = g_astUserArr[i].iPriority;
            *online = USERSTATUS_ONLINE;
            *terminal_type = g_astUserArr[i].iTerminalType;
            memcpy(user_pwd, g_astUserArr[i].szPwd, MAX_PASSWD_LEN);
            user_sem_unlock();
            printf("password is %s\n",user_pwd);
            return 0;
        }
    }

    /* 用户不在线 */
    *online = USERSTATUS_OFFLINE;
    *terminal_type = 0;
    user_sem_unlock();
    return 0;
}

int get_cli_user_pwd(char *user_name, char *user_pwd)
{
    int i;

    user_sem_lock();
    
    for (i=0; i<MAX_CLI_USER; ++i)
    {
        if (0 == strcmp(g_astUserArr[i].szName, user_name))
        {
            memcpy(user_pwd, g_astUserArr[i].szPwd, MAX_PASSWD_LEN);
            
            user_sem_unlock();
            return 0;
        }
    }

    user_sem_unlock();
    return 0;
}


__attribute((constructor)) void _user_init(void)
{
    g_sem_id = user_sem_init(USER_CONTROL_SHM_SEM_KEY);
    if (g_sem_id < 0)
    {    
        return;
    }

    user_sem_lock();
    g_shm_id = get_shmid(USER_CONTROL_SHM_KEY, sizeof(strUserMem));
    if (g_shm_id < 0)
    {
        user_sem_unlock();
        return;
    }

    g_pstUserMem = (strUserMem*)get_shm_addr(0, 600);
    if (NULL == g_pstUserMem)
    {
        user_sem_unlock();
        return;
    }
    g_astUserArr = &g_pstUserMem->astUserArr[0];
    user_sem_unlock();

    /* BEGIN: Added by wanglei, 2015/11/24 */
    sprintf(g_astUserArr[0].szName, "vanet", 5);
    sprintf(g_astUserArr[0].szPwd, "vanet", 5);
    g_astUserArr[0].iPriority = 15;
    /* END:   Added by wanglei, 2015/11/24 */

    return;
}

__attribute ((destructor)) void _user_fini(void)/*程序退出时调用*/
{
    user_sem_lock();
    if (NULL != g_pstUserMem)
    {
        /* 无论oam还是命令行退出，登陆用户都无效了 */
        memset(g_pstUserMem, 0, sizeof(strUserMem));
        dt_shm(g_pstUserMem);
    }
    user_sem_unlock();

    return;
}


