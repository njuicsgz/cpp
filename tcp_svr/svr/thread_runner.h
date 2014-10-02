#ifndef _THREAD_RUNNER_H_
#define _THREAD_RUNNER_H_

#include <pthread.h>
#include <deque>

//////////////////////////////////////////////////////////////////////////

class CRunner
{
public:
    virtual ~CRunner()
    {
    }
    virtual void Run() = 0;
};

class CThreadLauncher
{
public:
    void LaunchThread(CRunner* runner);

protected:
    std::deque<pthread_t> _threads;
    std::deque<CRunner*> _runners;

    static void* Run(void*);
};

inline void CThreadLauncher::LaunchThread(CRunner* runner)
{
    pthread_t p;
    pthread_create(&p, NULL, Run, runner);
    _threads.push_back(p);
    _runners.push_back(runner);
}

inline void* CThreadLauncher::Run(void* runner)
{
    CRunner* r = (CRunner*) runner;
    r->Run();
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
#endif//_THREAD_RUNNER_H_
///:~
