/************************************************************
 FileName: 		guardian.h
 Description:
 该文件中封装了互斥锁的接口
 History:
 sagezou    2008-02-28     1.0     新建
 Other:
 该文件依赖于httpsvr架构，从该架构改造而来，本文件基本未修改过
 ***************************/
#ifndef _GUARDIAN_H_
#define _GUARDIAN_H_

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

//////////////////////////////////////////////////////////////////////////

class CMutex
{
public:
    CMutex()
    {
        pthread_mutex_init(&_mutex, NULL);
    }
    ~CMutex()
    {
        pthread_mutex_destroy(&_mutex);
    }
    void lock()
    {
        pthread_mutex_lock(&_mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }

protected:
    pthread_mutex_t _mutex;
};

//////////////////////////////////////////////////////////////////////////

class CSem
{
public:
    CSem(unsigned value = 0)
    {
        int ret = sem_init(&_sem, 0, value);
        assert(!ret);
    }
    ~CSem()
    {
        sem_destroy(&_sem);
    }
    int lock()
    {
        return sem_wait(&_sem);
    }
    int try_lock()
    {
        return sem_trywait(&_sem);
    }
    int unlock()
    {
        return sem_post(&_sem);
    }

protected:
    sem_t _sem;
};

//////////////////////////////////////////////////////////////////////////

template<typename T>
class CLock
{
public:
    CLock(T& t, bool b = false) :
            _t(t), bLock(b)
    {
        if (!b)
        {
            t.lock();
            bLock = true;
        }
    }
    ~CLock()
    {
        if (bLock)
            _t.unlock();
    }

    void unlock()
    {
        if (bLock)
        {
            _t.unlock();
            bLock = false;
        }
    }

protected:
    T& _t;
    bool bLock;
};

typedef CLock<CMutex> ml;
typedef CLock<CSem> sl;
class CRunTimeCount
{
public:
    CRunTimeCount(const char * pFuncName)
    {
        memset(szFuncName, 0x00, sizeof(szFuncName));
        strncpy(szFuncName, pFuncName, sizeof(szFuncName) - 1);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        lBeginTime = tv.tv_usec;

    }
    ~CRunTimeCount()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        printf("Func :%s Run Cost Time is %ld\n", szFuncName,
                tv.tv_usec - lBeginTime);
    }
protected:
    char szFuncName[64];
    long lBeginTime;

};

//////////////////////////////////////////////////////////////////////////
#endif//_GUARDIAN_H_
///:~
