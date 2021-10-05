
#pragma once

#include "crtlib.h"
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <semaphore.h>
#ifdef __APPLE__
	#include <mach/mach.h>
#endif
#endif

namespace crtfun{
	class crtmutex{
	public:
#ifdef _WIN32
		crtmutex(){
		#if(_WIN32_WINNT < 0x0400)
			locked=0;
		#endif
			InitializeCriticalSection(&m_cs);
		}
		virtual ~crtmutex(){
		#if(_WIN32_WINNT < 0x0400)
			locked=0;
		#endif
			DeleteCriticalSection(&m_cs);
		}
		void lock(){
			EnterCriticalSection(&m_cs);
		#if(_WIN32_WINNT < 0x0400)
			locked=1;
		#endif
		}
		void unlock(){
		#if(_WIN32_WINNT < 0x0400)
			locked=0;
		#endif
			LeaveCriticalSection(&m_cs);
		}
		bool trylock(){
		#if(_WIN32_WINNT >= 0x0400)
			return TryEnterCriticalSection(&m_cs)!=0;
		#else
			if (locked) return false;
			lock();
			return true;
		#endif
		}
	protected:
		CRITICAL_SECTION m_cs;
	#if(_WIN32_WINNT < 0x0400)
		volatile int locked;
	#endif
#else
		crtmutex(){
			pthread_mutexattr_init(&m_attr);
			pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_PRIVATE);
			pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&m_mtx, &m_attr);
		}
		virtual ~crtmutex(){
			pthread_mutex_destroy(&m_mtx);
			pthread_mutexattr_destroy(&m_attr);
		}
		void lock(){pthread_mutex_lock(&m_mtx);}
		void unlock(){pthread_mutex_unlock(&m_mtx);}
		bool trylock(){return pthread_mutex_trylock(&m_mtx)==0;}
	protected:
		pthread_mutex_t m_mtx;
		pthread_mutexattr_t m_attr;
#endif
	};
	class crtsinglelock{
	public:
		explicit crtsinglelock(crtmutex *mtx){m_mtx=mtx;mtx->lock();}
		virtual ~crtsinglelock(){m_mtx->unlock();}
	protected:
		crtmutex *m_mtx;
	};
	class crtsem {
	public:
		crtsem(){
		#ifdef _WIN32
			sem = NULL;
		#else
			memset(&sem,0,sizeof(sem_t));
		#endif
		}
		virtual ~crtsem() {
		#ifdef _WIN32
			if (sem) {
				CloseHandle(sem);
				sem = NULL;
			}
		#elif defined(__APPLE__) && defined(__MACH__)
			semaphore_destroy(mach_task_self(), sem);
		#else
			sem_destroy(&sem);
		#endif
		}
		int init(int value=1){
		#ifdef _WIN32
			sem = CreateSemaphore(NULL, value, INT_MAX, NULL);
			return sem ? 0 : -1;
		#elif defined(__APPLE__) && defined(__MACH__)
			return semaphore_create(mach_task_self(), &sem, SYNC_POLICY_FIFO, value);
		#else
			return sem_init(&sem, 0, value);
		#endif
		}
		inline void signal() {return post();}
		void post() {
		#ifdef _WIN32
			ReleaseSemaphore(sem, 1, NULL);
		#elif defined(__APPLE__) && defined(__MACH__)
			semaphore_signal(sem);
		#else
			sem_post(&sem);
		#endif
		}
		void wait() {
		#ifdef _WIN32
			WaitForSingleObject(sem, INFINITE);
		#elif defined(__APPLE__) && defined(__MACH__)
			semaphore_wait(sem);
		#else
			int r;
			do { r = sem_wait(&sem);} while (r == -1 && errno == EINTR);
		#endif
		}
		//0 for success, -1 for failed
		int trywait() {
		#ifdef _WIN32
			DWORD r = WaitForSingleObject(sem, 0);
			if (r == WAIT_OBJECT_0) return 0;
			return -1;
		#elif defined(__APPLE__) && defined(__MACH__)
			mach_timespec_t interval;
			interval.tv_sec = 0;
			interval.tv_nsec = 0;
			if (semaphore_timedwait(sem, interval) == KERN_SUCCESS) return 0;
			return -1;
		#else
			int r;
			do { r = sem_trywait(&sem);} while (r == -1 && errno == EINTR);
			return r;
		#endif
		}
	protected:
	#ifdef _WIN32
		HANDLE sem;
	#elif defined(__APPLE__) && defined(__MACH__)
		semaphore_t sem;
	#else
		sem_t sem;
	#endif
	};
	/*class crtcond {
	public:
		crtcond(){abort_request=0;pthread_cond_init(&cond, NULL);}
		virtual ~crtcond(){pthread_cond_destroy(&cond);}
		int signal(){return pthread_cond_signal(&cond);}
		int broadcast(){return pthread_cond_broadcast(&cond);}
		int wait(crtmutex *mtx){return pthread_cond_wait(&cond, &mtx->m_mtx);}
		int waittimeout(crtmutex *mtx,unsigned int ms){
			int retval=0;
			struct timespec abstime;
		#ifdef _WIN32
			abstime.tv_sec = (unsigned int)time(NULL) + (ms / 1000);
			abstime.tv_nsec = (ms % 1000) * 1000 * 1000;
		#else
			struct timeval delta;
			gettimeofday(&delta, NULL);
			abstime.tv_sec = delta.tv_sec + (ms / 1000);
			abstime.tv_nsec = (delta.tv_usec + (ms % 1000) * 1000) * 1000;
		#endif
			if (abstime.tv_nsec > 1000000000) {
				abstime.tv_sec += 1;
				abstime.tv_nsec -= 1000000000;
			}
			do {retval = pthread_cond_timedwait(&cond, &mtx->m_mtx, &abstime);} while(retval==EINTR);
			return retval;
		}
		int abort(){abort_request=1;return signal();}
		//aborted return !=0
		int isaborted(){return abort_request;}
	protected:
		volatile int abort_request;
		pthread_cond_t cond;
	};*/
	class icrtthreadrun{
	public:
		icrtthreadrun(){}
		virtual ~icrtthreadrun(){}
		// thread init
		virtual bool threadinit(void *key){return true;}
		// thread run return is how many ms for sleep
		// return 0 to quit the thread
		virtual int threadrun(void *key)=0;
		// thread uninit
		virtual void threaduninit(void *key){}
	};
	typedef void (*crtthreadfunction)(void *arg);
	class crtthread;
	typedef struct _crtthreadfuncdata{
		crtthreadfunction fn;
		icrtthreadrun *cls;
		crtthread *thdcls;
		void *adddata;
		volatile bool exit;
	}crtthreadfuncdata;
	class crtthread : public icrtthreadrun{
	public:
		crtthread(){thread=this;}
		virtual ~crtthread(){
			stopallthread();
		}
		virtual void setthreadcb(icrtthreadrun *cls){thread=cls?cls:this;}
		virtual int startthread(void *adddata)
		{
			crtthreadfuncdata *data=new crtthreadfuncdata();
			data->cls=thread;
			data->thdcls=this;
			data->exit=false;
			data->adddata=adddata;
#ifdef _WIN32
			return _beginthread(threadfn,0,(void*)data)==-1;
#else
			pthread_t id;
			int ret=pthread_create(&id,NULL,threadfn,(void*)data);
			pthread_detach(id);
			return ret;
#endif
		}
		static int startthreadfunction(crtthreadfunction fn,void *key) {
			crtthreadfuncdata *data=new crtthreadfuncdata();
			data->fn=fn;
			data->adddata=key;
#ifdef _WIN32
			return _beginthread(threadotherfn,0,(void*)data)==-1;
#else
			pthread_t id;
			int ret=pthread_create(&id,NULL,threadotherfn,(void*)data);
			pthread_detach(id);
			return ret;
#endif
		}
		virtual void poststopthread(void *key){
			m_mtx.lock();
			map<void *,crtthreadfuncdata *>::iterator it=m_running.find(key);
			if (it!=m_running.end()) it->second->exit=true;
			m_mtx.unlock();
		}
		virtual void stopthread(void *key) {
			poststopthread(key);
			map<void *,crtthreadfuncdata *>::iterator it;
			do{
				m_sem.wait();
				it=m_running.find(key);
			}while(it!=m_running.end());
		}
		virtual void stopallthread(){
			poststopallthread();
			while(m_running.size()) {
				m_sem.wait();
			}
		}
		virtual void poststopallthread(){
			m_mtx.lock();
			map<void *,crtthreadfuncdata *>::iterator it=m_running.begin();
			while(it!=m_running.end()) {
				it->second->exit=true;
				it++;
			}
			m_mtx.unlock();
		}
		virtual int threadrun(void *key){return 1;}
	protected:
		icrtthreadrun *thread;
		map<void *,crtthreadfuncdata *> m_running;
		crtsem m_sem;
		crtmutex m_mtx;
#ifdef _WIN32
		static void threadfn(void *arg)
#else
		static void *threadfn(void *arg)
#endif
		{
			crtthreadfuncdata *p=(crtthreadfuncdata *)arg;
			icrtthreadrun *cls=p->cls;
			void *adddata=p->adddata;
			p->thdcls->m_mtx.lock();
			pair<map<void *,crtthreadfuncdata *>::iterator, bool> ret=p->thdcls->m_running.insert(map<void *,crtthreadfuncdata *>::value_type(adddata,p));
			p->thdcls->m_mtx.unlock();
			if (ret.second) {
				if (cls->threadinit(adddata)) {
					int sleep;
					while(p->exit==false) {
						sleep=cls->threadrun(adddata);
						if (sleep==0) break;
						ms_sleep(sleep);
					}
					cls->threaduninit(adddata);
				}
				p->thdcls->m_mtx.lock();
				p->thdcls->m_running.erase((void *)adddata);
				p->thdcls->m_sem.signal();
				p->thdcls->m_mtx.unlock();
			}
			delete p;
#ifndef _WIN32
			return NULL;
#endif
		}
#ifdef _WIN32
		static void threadotherfn(void *arg)
#else
		static void *threadotherfn(void *arg)
#endif
		{
			crtthreadfuncdata *p=(crtthreadfuncdata *)arg;
			p->fn(p->adddata);
			delete p;
#ifndef _WIN32
			return NULL;
#endif
		}
	};
	static uint32_t crtthreadid(){
#ifdef _WIN32
		return GetCurrentThreadId();
#else
		return (uint32_t)(long)pthread_self();
#endif
	}
};
