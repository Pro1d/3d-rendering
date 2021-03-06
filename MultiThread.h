#ifndef MULTITHREAD_H
#define MULTITHREAD_H

#include <SDL_thread.h>
#include <SDL.h>
#include <queue>

class ThreadSync {
    public:
        ThreadSync() : sync(false), mutex(SDL_CreateMutex()), cond(SDL_CreateCond()) {}
        ~ThreadSync() {
            SDL_DestroyCond(cond);
            SDL_DestroyMutex(mutex);
        }
        void wait() {
            SDL_LockMutex(mutex);
            while(!sync)
                SDL_CondWaitTimeout(cond, mutex, 5);
            SDL_UnlockMutex(mutex);
            sync = false;
        }
        void waitAndPollEvent(std::queue<SDL_Event> & eventQueue) {
            SDL_LockMutex(mutex);
            while(!sync) {
                SDL_CondWaitTimeout(cond, mutex, 5);
                SDL_Event event;
                if(SDL_PollEvent(&event))
                    eventQueue.push(event);
            }
            SDL_UnlockMutex(mutex);
            sync = false;
        }
        void signal() {
            SDL_LockMutex(mutex);
            sync = true;
            SDL_CondSignal(cond);
            SDL_UnlockMutex(mutex);
        }
    private:
        bool sync;
        SDL_mutex *mutex;
        SDL_cond *cond;
};
typedef struct thread_params_t thread_params_t;
struct thread_params_t {
    void* params;

    bool end;
    int threadNum, threadCount;
    ThreadSync syncStart;
    ThreadSync syncEnd;
    void (*func)(thread_params_t*);
};
class MultiThread
{
    public:
        MultiThread(int threadCount);
        virtual ~MultiThread();
        void execute(void (*func)(thread_params_t*), void* params);
    protected:
    private:
        int threadCount;
        SDL_Thread **threads;
        thread_params_t *params;
        std::queue<SDL_Event> eventQueue;
};

#endif // MULTITHREAD_H
