#include "MultiThread.h"
#include "SDL_Thread.h"
#include <cstdio>

int host(void* p) {
    thread_params_t *thp = (thread_params_t*)p;

    while(!thp->end) {
        #ifdef DEBUG
        //    printf("wait[%d]\n", thp->threadNum);
        #endif
        thp->syncStart.wait();


        #ifdef DEBUG
        //    printf("execute[%d]\n", thp->threadNum);
        #endif
        if(thp->func != NULL)
            thp->func(thp);

        #ifdef DEBUG
        //    printf("signal[%d]\n", thp->threadNum);
        #endif
        thp->syncEnd.signal();
    }

    return 0;
}

MultiThread::MultiThread(int threadCount) :
        threadCount(threadCount),
        threads(new SDL_Thread*[threadCount]),
        params(new thread_params_t[threadCount])
{
    for(int i = 0; i < threadCount; i++) {
        params[i].threadNum = i;
        params[i].threadCount = threadCount;
        params[i].end = false;
        threads[i] = SDL_CreateThread(&host, (void*) &params[i]);
    }
}

MultiThread::~MultiThread()
{
    for(int i = 0; i < threadCount; i++) {
        params[i].func = NULL;
        params[i].params = NULL;
        params[i].end = true;
        params[i].syncStart.signal();
        params[i].syncEnd.wait();
        SDL_WaitThread(threads[i], NULL);

        #ifdef DEBUG
            printf("join[%d]\n", i);
        #endif
    }
    delete[] params;
    delete[] threads;
}

void MultiThread::execute(void (*func)(thread_params_t*), void* p)
{
    #ifdef DEBUG
    //    printf("NEW TASK\n");
    #endif
    for(int i = 0; i < threadCount; i++)
    {
        params[i].func = func;
        params[i].params = p;
        params[i].syncStart.signal();
    }

    /// Proccessing time...
    #ifdef DEBUG
    //    printf("TASK...ING\n");
    #endif

    for(int i = 0; i < threadCount; i++)
        params[i].syncEnd.wait();
    #ifdef DEBUG
    //    printf("END TASK\n");
    #endif
}
