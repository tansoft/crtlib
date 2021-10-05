#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crtthread.h"

#define	CRTMEMBUF_ALLOCSIZE	(1024 * 64)	//64k

namespace crtfun {
	/**
	* @brief 内存池管理
	* @author Barry(barrytan@21cn.com,QQ:20962493)
	*/
	/**<pre>
	使用Sample：
	</pre>*/
	template<class T>
	class crtmemmanager{
	public:
		crtmemmanager(){m_waitsize=0;sem.init();}
		virtual ~crtmemmanager() {release_cache();release_queue();}
		static crtmemmanager* instance() {
			static crtmemmanager<T> *man=NULL;
			if (!man) man = new crtmemmanager<T>;
			return man;
		}
		T *new_item() {
			T *p;
			m_mtx.lock();
			if (m_free.size()) {
				p=m_free.front();
				m_free.pop();
			} else p=new T;
			m_mtx.unlock();
			return p;
		}
		void release_item(T *p) {
			m_mtx.lock();
			m_free.push(p);
			m_mtx.unlock();
		}
		size_t get_cache_size() {
			m_mtx.lock();
			size_t size=m_free.size();
			m_mtx.unlock();
			return size;
		}
		void release_cache() {
			m_mtx.lock();
			while(m_free.size()) {
				delete m_free.front();
				m_free.pop();
			}
			m_mtx.unlock();
		}
		T *touch_queue() {
			m_jobmtx.lock();
			T *ret=NULL;
			if (m_job.size()>0) ret=m_job.front();
			m_jobmtx.unlock();
			return ret;
		}
		T *pop_queue() {
			m_jobmtx.lock();
			T *ret=NULL;
			if (m_job.size()>0) {
				ret=m_job.front();
				m_job.pop();
			}
			m_jobmtx.unlock();
			return ret;
		}
		T *wait_for_pop_queue() {
			m_jobmtx.lock();
			T *ret=NULL;
			while(1) {
				if (m_job.size()>0) {
					ret=m_job.front();
					m_job.pop();
					break;
				}
				m_waitsize++;
				m_jobmtx.unlock();
				sem.wait();
				m_jobmtx.lock();
				m_waitsize--;
			}
			m_jobmtx.unlock();
			return ret;
		}
		void push_queue(T *job) {
			m_jobmtx.lock();
			m_job.push(job);
			m_jobmtx.unlock();
			sem.post();
		}
		size_t get_waitqueue_size() {
			m_jobmtx.lock();
			size_t size=m_waitsize;
			m_jobmtx.unlock();
			return size;
		}
		size_t get_queue_size() {
			m_jobmtx.lock();
			size_t size=m_job.size();
			m_jobmtx.unlock();
			return size;
		}
		void release_queue() {
			m_jobmtx.lock();
			while(m_job.size()) {
				delete m_job.front();
				m_job.pop();
			}
			m_jobmtx.unlock();
		}
	protected:
		crtmutex m_mtx;
		queue<T *> m_free;
		crtmutex m_jobmtx;
		queue<T *> m_job;
		crtsem sem;
		volatile size_t m_waitsize;
	};
	class crtmembuf{
	public:
		crtmembuf()
		{
			m_buf=NULL;
			m_bufsize=0;
			m_allocsize=0;
		}

		virtual ~crtmembuf()
		{
			crtsinglelock lock(&m_wmtx);
			if (m_buf) delete [] m_buf;
			m_buf=NULL;
			m_bufsize=0;
			m_allocsize=0;
		}

		inline bool addbufu8(uint8_t key){return addbuf(&key,1);}
		inline bool addbufu16(uint16_t key){return addbuf(&key,2);}
		inline bool addbufu32(uint32_t key){return addbuf(&key,4);}
	
		/**
		* @brief 加入数据包
		* @param buf		数据指针
		* @param len		数据长度
		* @return			是否成功
		*/
		bool addbuf(const void *buf,unsigned int len)
		{
			crtsinglelock lock(&m_wmtx);
			int malloccnt=0;
			if (!m_buf)
				malloccnt=(len+CRTMEMBUF_ALLOCSIZE-1)/CRTMEMBUF_ALLOCSIZE;
			else if (m_allocsize-m_bufsize<len)
				malloccnt=(len+m_bufsize*2+CRTMEMBUF_ALLOCSIZE-1)/CRTMEMBUF_ALLOCSIZE;
			if (malloccnt!=0)
			{
				char *tmp=new char[malloccnt*CRTMEMBUF_ALLOCSIZE];
				if (!tmp)
				{
					if (m_buf) delete [] m_buf;
					m_buf=NULL;
					m_bufsize=0;
					m_allocsize=0;
					return false;
				}
				if (m_buf)
				{
					if (m_bufsize>0) memcpy(tmp,m_buf,m_bufsize);
					delete [] m_buf;
				}
				m_allocsize=malloccnt*CRTMEMBUF_ALLOCSIZE;
				m_buf=tmp;
			}
			memcpy(m_buf+m_bufsize,buf,len);
			m_bufsize+=len;
			return true;
		}
		/**
		* @brief 获取数据包，注意此时buffer被锁住，调用release_getbuf_lock释放，注意无论返回是否失败，都需要调用release_getbuf_lock
		* @param len		数据长度，可为空
		* @param lock		是否锁住缓存区
		* @return			数据指针
		*/
		void *getbuf(unsigned int *len,bool lock=true)
		{
			if (lock) m_wmtx.lock();
			if (len) *len=m_bufsize;
			return m_buf;
		}
		/**
		* @brief 查找数据包
		* @param key		查找的数据
		* @param len		查找的数据长度
		* @param lock		是否锁住缓存区，选择lock也会在返回时给释放
		* @return			找到数据的指针离开始的偏移
		*/
		size_t findpos(const char *key, unsigned int len, bool lock=true)
		{
			size_t pos=-1;
			if (lock) m_wmtx.lock();
			if (m_buf) {
				const char *find=(const char *)bin_search(m_buf,m_bufsize, key, len);
				if (find) pos=find-m_buf;
			}
			if (lock) m_wmtx.unlock();
			return pos;
		}
		/**
		* @brief 查找数据包
		* @param key		查找的数据
		* @param len		查找的数据长度
		* @param lock		是否锁住缓存区，选择lock也会在返回时给释放
		* @return			找到数据的指针离开始的偏移
		*/
		string findstr(const char *key, unsigned int len, bool lock=true)
		{
			string ret;
			if (lock) m_wmtx.lock();
			if (m_buf) {
				const char *find=(const char *)bin_search(m_buf,m_bufsize, key, len);
				if (find) ret.assign(m_buf,find-m_buf);
			}
			if (lock) m_wmtx.unlock();
			return ret;
		}
		string substr(size_t pos, bool lock=true)
		{
			string ret;
			if (lock) m_wmtx.lock();
			if (m_buf && pos>0 && m_bufsize>0) {
				if (pos>=m_bufsize) pos=m_bufsize-1;
				ret.assign(m_buf,pos);
			}
			if (lock) m_wmtx.unlock();
			return ret;
		}
		/**
		* @brief 释放获取数据包时获得的数据指针
		* @param deletesize	需要移除的数据的大小，为0为不移除数据
		*/
		void release_getbuf_lock(unsigned int deletesize=0)
		{
			_removebuf(deletesize);
			m_wmtx.unlock();
		}
		/**
		* @brief 移除数据
		* @param deletesize	需要移除的数据的大小，为0为不移除数据
		*/
		void removebuf(unsigned int deletesize=0)
		{
			crtsinglelock lock(&m_wmtx);
			_removebuf(deletesize);
		}
		/**
		* @brief 清空缓冲
		*/
		void clear()
		{
			crtsinglelock lock(&m_wmtx);
			m_bufsize=0;
		}
		/**
		* @brief 强制截短已使用的大小，如果缓冲区没有newsize大，newsize变为缓冲区大小
		* @param newsize	强制截短大小
		*/
		unsigned int setsize(unsigned int newsize)
		{
			crtsinglelock lock(&m_wmtx);
			if (newsize>m_allocsize) newsize=m_allocsize;
			m_bufsize=newsize;
			return m_bufsize;
		}
		/**
		* @brief 获取缓冲数据长度
		* @return			返回缓冲中的数据长度
		*/
		unsigned int get_buffered_size(){return m_bufsize;}
		bool addbuf(crtmembuf *other)
		{
			bool ret=false;
			if (other)
			{
				unsigned int len;
				void *buf=other->getbuf(&len);
				ret=addbuf(buf,len);
				other->release_getbuf_lock();
			}
			return ret;
		}
		/**
		* @brief 获取并删除数据
		* @param buf       接收数据
		* @param size      数据长度
		* @return			返回获取的数据长度
		*/
		unsigned int getandremovebuf(void *buf,unsigned int size){
			crtsinglelock lock(&m_wmtx);
			if (size > m_bufsize) size = m_bufsize;
			if (size > 0) {
				memcpy(buf, m_buf, size);
				_removebuf(size);
			}
			return size;
		}
	private:
		void _removebuf(unsigned int deletesize=0)
		{
			if (deletesize>0)
			{
				if (deletesize>m_bufsize)
				{
					//assert(false);
					deletesize=m_bufsize;
				}
				if (deletesize==m_bufsize) m_bufsize=0;
				else
				{
					m_bufsize-=deletesize;
					memmove(m_buf,m_buf+deletesize,m_bufsize);
				}
			}
		}
		char *m_buf;
		crtmutex m_wmtx;
		volatile unsigned int m_bufsize;
		volatile unsigned int m_allocsize;
	};
	//注意，为了保证效率，该类是没带锁访问的
	template <typename T>
	class crtmembufmanager
	{
	public:
		crtmembufmanager(void) {}
		virtual ~crtmembufmanager(void) {clear();}
		void clear()
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.begin();
			while(it!=m_mem.end()){
				m_memman.release_item(it->second);
				m_mem.erase(it++);
			}
			m_memman.release_cache();
			m_adddata.clear();
		}
		bool ishave(T id)
		{
			return m_mem.find(id)!=m_mem.end();
		}
		bool addbuf(T id,const void *buf,int len)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			crtmembuf *m;
			if (it==m_mem.end()) {
				m=m_memman.new_item();
				if (!m) return false;
				m_mem[id]=m;
			} else m=it->second;
			return m->addbuf(buf,len);
		}
		void *getbuf(T id,unsigned int *len,bool lock=true)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) {
				if (len) *len=0;
				return NULL;
			}
			return it->second->getbuf(len,lock);
		}
		size_t findpos(T id,const char *key,unsigned int len,bool lock=true)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return NULL;
			return it->second->findpos(key,len,lock);
		}
		string findstr(T id,const char *key,unsigned int len,bool lock=true)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return "";
			return it->second->findstr(key,len,lock);
		}
		string substr(T id,size_t pos,bool lock=true)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return "";
			return it->second->substr(pos,lock);
		}
		void release_getbuf_lock(T id,unsigned int deletesize=0)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return;
			it->second->release_getbuf_lock(deletesize);
		}
		void removebuf(T id,unsigned int deletesize=0)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return;
			it->second->removebuf(deletesize);
		}
		void releasebuf(T id)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return;
			m_memman.release_item(it->second);
			m_mem.erase(it);
			typename map<T,int>:iterator it2=m_adddata.find(id);
			if (it2!=m_adddata.end()) m_adddata.erase(it2);
		}
		void clear(T id)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return;
			it->second->clear();
		}
		unsigned int get_buffered_size(T id)
		{
			typename map<T,crtmembuf *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return 0;
			return it->second->get_buffered_size();
		}
		void setadddata(T id,int data) {m_adddata[id]=data;}
		int getadddata(T id) {return m_adddata[id];}
	private:
		map<T,crtmembuf *> m_mem;
		map<T,int> m_adddata;
		crtmemmanager<crtmembuf> m_memman;
	};
	class icrtmemobj
	{
	public:
		icrtmemobj(){}
		virtual ~icrtmemobj(){release_cache();}
		virtual void *newobj()=0;
		virtual void freeobj(void *obj)=0;
		virtual void release_cache(){}
	};
	//注意，为了保证效率，该类是没带锁访问的
	template <typename T>
	class crtmemobjmanager
	{
	public:
		crtmemobjmanager(void) {obj=NULL;}
		virtual ~crtmemobjmanager(void) {clear();}
		void setmemobjhandler(icrtmemobj *o){obj=o;}
		void clear()
		{
			if (!obj) return;
			typename map<T,void *>::iterator it=m_mem.begin();
			while(it!=m_mem.end()){
				obj->freeobj(it->second);
				m_mem.erase(it++);
			}
			obj->release_cache();
		}
		bool ishave(T id) {return m_mem.find(id)!=m_mem.end();}
		void *getobj(T id) {
			typename map<T,void *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return NULL;
			return it->second;
		}
		void *getornewobj(T id) {
			typename map<T,void *>::iterator it=m_mem.find(id);
			void *m;
			if (it==m_mem.end()) {
				m=obj->newobj();
				if (!m) return NULL;
				m_mem[id]=m;
			} else m=it->second;
			return m;
		}
		void removeobj(T id)
		{
			typename map<T,void *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return;
			if (obj) obj->freeobj(it->second);
			m_mem.erase(it);
		}
		map<T,void *> *getinnerobj(){return &m_mem;}
	protected:
		icrtmemobj *obj;
		map<T,void *> m_mem;
	};
	typedef struct _crtblockbuffer{
		uint8_t *buf;			///<buf指针
		unsigned int len;		///<已使用长度
		uint8_t *end;			///<结束指针，就是buffer最大的偏移，因为buf会作出修改，因此不保存长度，保存最后的偏移指针
		unsigned int flag;		///<buffer的属性
		void *adddata;			///<用户附加值
		struct _crtblockbuffer *next;///<下一个buffer指针
		void appendbuffer(struct _crtblockbuffer *pbuf){
			crtblockbuffer *p=this;
			while(p->next) p=p->next;
			p->next=pbuf;
		}
		//如果成功合并buffer，返回true，否则追加到末尾，且返回false，返回true时，应把pbuf回收
		bool appendorcombinbuffer(struct _crtblockbuffer *pbuf){
			crtblockbuffer *p=this;
			while(p->next) p=p->next;
			if (p->end-p->buf-p->len>=pbuf->len){
				memcpy(p->buf + p->len,pbuf->buf,pbuf->len);
				p->len+=pbuf->len;
				return true;
			} else
				p->next=pbuf;
			return false;
		}
		unsigned int bufferlen() {
			crtblockbuffer *p=this;
			unsigned int nlen=p->len;
			while(p->next) {
				p=p->next;
				if (p) nlen+=p->len;
			}
			return nlen;
		}
	}crtblockbuffer;
	//注意，为了保证效率，该类是没带锁访问的
	class crtblockbuffermanager{
	public:
		crtblockbuffermanager() {
			queue<crtblockbuffer *> l;
			unsigned int ilist[]={16,32,64,128,256,512};
			int i;
			for(i=0;i<sizeof(ilist)/sizeof(unsigned int);i++)
				m_hit.insert(map<uint32_t,queue<crtblockbuffer *> >::value_type(ilist[i],l));
			for(i=1000;i<256000;i+=1000)
				m_hit.insert(map<uint32_t,queue<crtblockbuffer *> >::value_type(i,l));
		}
		virtual ~crtblockbuffermanager() {
			map<uint32_t,queue<crtblockbuffer *> >::iterator it=m_hit.begin();
			while(it!=m_hit.end()) {
				queue<crtblockbuffer *> &l=it->second;
				while(l.size()) {
					delete [] (uint8_t *)(l.front());
					l.pop();
				}
				it++;
			}
			m_hit.clear();
		}
		crtblockbuffer *newbuf(unsigned int size) {
			map<uint32_t,queue<crtblockbuffer *> >::iterator it=m_hit.lower_bound(size);
			crtblockbuffer *buf=NULL;
			if (it!=m_hit.end()) {
				size=it->first;
				if (it->second.size()>0) {
					buf=it->second.front();
					it->second.pop();
				}
			}
			if (!buf) {
				buf=(crtblockbuffer *)new uint8_t[sizeof(crtblockbuffer)+size];
				buf->buf=(uint8_t *)(buf+1);
				buf->end=buf->buf+size;
			} else
				buf->buf=(uint8_t *)(buf+1);//make sure the buf pointer is ok
			buf->len=0;
			buf->flag=0;
			buf->adddata=NULL;
			buf->next=NULL;
			return buf;
		}
		void releasebuf(crtblockbuffer *buf) {
			crtblockbuffer *next;
			while(1) {
				next=buf->next;
				map<uint32_t,queue<crtblockbuffer *> >::iterator it=m_hit.find((unsigned int)(buf->end-((uint8_t *)buf)-sizeof(crtblockbuffer)));
				if (it!=m_hit.end())
					it->second.push(buf);
				else
					delete [] (uint8_t *)buf;
				if (!next) break;
				buf=next;
			}
		}
	protected:
		map<uint32_t,queue<crtblockbuffer *> > m_hit;
	};
};
