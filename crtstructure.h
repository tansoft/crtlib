#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crtsocket.h"

namespace crtfun{
	template <typename K>
	class icrtlrudeletecb{
	public:
		virtual void crtlrudelete(K key)=0;
	};
	//非线程安全
	template <typename K>
	class crtlru{
	protected:
		/* forward declaration */
		struct _lru_info;
		typedef map<K, struct _lru_info> keymap;
		typedef multimap<time_t, struct _lru_info> valuemap;
		struct _lru_info{
			typename map<K, struct _lru_info>::iterator keyit;
			//keymap::iterator keyit;
			typename multimap<time_t, struct _lru_info>::iterator valueit;
		};
		keymap keys;
		valuemap values;
		typename icrtlrudeletecb<K> *cb;
	public:
		crtlru(){cb=NULL;}
		virtual ~crtlru(){clear();}
		void setcb(typename icrtlrudeletecb<K> *cbfun){cb=cbfun;}
		//更新键值，ti=0表示使用当前时间
		void update(K k,time_t ti=0) {
			if (ti==0) ti=time(NULL);
			typename keymap::iterator it = keys.find(k);
			if (it!=keys.end()) {
				values.erase(it->second.valueit);
				keys.erase(it);
			}
			struct _lru_info info;
			info.keyit = keys.insert(typename keymap::value_type(k,info)).first;
			info.keyit->second.valueit = values.insert(typename valuemap::value_type(ti,info));
		}
		void updateifexist(K k,time_t ti=0) {
			if (ti==0) ti=time(NULL);
			typename keymap::iterator it = keys.find(k);
			if (it==keys.end()) return;
			values.erase(it->second.valueit);
			keys.erase(it);
			struct _lru_info info;
			info.keyit = keys.insert(typename keymap::value_type(k,info)).first;
			info.keyit->second.valueit = values.insert(typename valuemap::value_type(ti,info));
		}
		void clear(){
			if (cb) {
				typename valuemap::iterator it=values.begin();
				while (it!=values.end()) {
					K k=it->second.keyit->first;
					keys.erase(it->second.keyit);
					values.erase(it++);
					cb->crtlrudelete(k);
				}
			} else {
				keys.clear();
				values.clear();
			}
		}
		bool empty(){return keys.size()==0;}
		size_t size(){return keys.size();}
		//获取最旧数据
		K head(){
			typename valuemap::iterator it=values.begin();
			if (it==values.end()) return NULL;
			return it->second.keyit->first;
		}
		//弹出最旧数据
		void pop(){
			typename valuemap::iterator it=values.begin();
			if (it==values.end()) return NULL;
			K k=it->second.keyit->first;
			keys.erase(it->second.keyit);
			values.erase(it);
			if (cb) cb->crtlrudelete(k);
		}
		//如果过期，弹出数据
		K poptimeout(time_t ti=0) {
			if (ti==0) ti=time(NULL);
			typename valuemap::iterator it=values.begin();
			if (it!=values.end() && it->first <= ti) {
				K k=it->second.keyit->first;
				keys.erase(it->second.keyit);
				values.erase(it);
				if (cb) cb->crtlrudelete(k);
				return k;
			}
			return NULL;
		}
		inline void erase(K k){return cancel(k);}
		//删除所有已经过期的键值
		void cleantimeout(time_t ti=0) {
			if (ti==0) ti=time(NULL);
			typename valuemap::iterator it=values.begin();
			while (it!=values.end() && it->first <= ti) {
				K k=it->second.keyit->first;
				keys.erase(it->second.keyit);
				values.erase(it++);
				if (cb) cb->crtlrudelete(k);
			}
		}
		//取消某个键值
		void cancel(K k){
			typename keymap::iterator it = keys.find(k);
			if (it!=keys.end()) {
				values.erase(it->second.valueit);
				keys.erase(it);
				if (cb) cb->crtlrudelete(k);
			}
		}
	};
	class crtiprange {
	public:
		crtiprange() {
			m_ip=NULL;
			m_size=0;
			m_allocsize=0;
		}
		virtual ~crtiprange() {if (m_ip) delete [] m_ip;}
		//192.168.0.1:255.255.255.0|192.168.1.50:255.255.255.255
		string tostring() {
			string str;
			for(unsigned int i=0;i<m_size;i++)
				str+=string_format("%s:%s|",inet_ntoa_safe_u32n(m_ip[i*2]).c_str(),inet_ntoa_safe_u32n(m_ip[i*2+1]).c_str());
			return str;
		}
		//192.168.0.1:255.255.255.0|192.168.1.50:255.255.255.255,mask 默认为255.255.255.255
		void fromstring(const char *str) {
			m_size=0;
			crtstringtoken t(str,"|"),t1;
			string ip,mask;
			while(t.ismore()) {
				t1.init(t.nexttoken(),":");
				ip=t1.nexttoken();
				mask=t1.nexttoken();
				if (!ip.empty()) {
					if (mask.empty()) mask="255.255.255.255";
					add_ip_range(ip.c_str(),mask.c_str());
				}
			}
		}
		//注意：ip 为网络顺序，性能更高
		//只要有任一bNot=0的Range成立即退出
		bool is_ip_in_range_n(uint32_t ip) {
			bool bret=false;
			for(unsigned int i=0;i<m_size;i++) {
				unsigned int off=i*2;
				if ((m_ip[off+1]&ip) == m_ip[off]) {
					bret=true;
					break;
				}
			}
			return bret;
		}
		bool add_ip_range_n(uint32_t ip,uint32_t mask) {
			if (m_allocsize<=m_size) {
				uint32_t *newip=new uint32_t[(m_allocsize+10)*2];
				if (!newip) return false;
				if (m_size>0) {
					memcpy(newip,m_ip,m_allocsize*2);
					delete [] m_ip;
				}
				m_ip=newip;
				m_allocsize+=10;
			}
			m_ip[m_size*2]=ip&mask;
			m_ip[m_size*2+1]=mask;
			m_size++;
			return true;
		}
		bool add_ip_range(const char* ip,const char* mask="") {
			if (!ip || !mask) return false;
			return add_ip_range_n(inet_addr(ip),inet_addr(mask[0]=='\0'?"255.255.255.255":mask));
		}
		unsigned int size(){return m_size;}
		void clear(){m_size=0;}
	protected:
		uint32_t *m_ip;
		unsigned int m_size;
		unsigned int m_allocsize;
	};
	//可用作map的key,buf开辟了空间来保存
	template<unsigned int len>
	class crtbinobj{
	public:
		crtbinobj(){memset(m_buf,0,len);}
		crtbinobj(const void *buf){setbuf(buf);}
		virtual ~crtbinobj(){}
		void setbuf(const void *buf){memcpy(m_buf,buf,len);}
		bool operator < (typename const crtbinobj<len> &other) const
		{return memcmp(m_buf,other.m_buf,len)<0;}
		void *getbuf(){return m_buf;}
	protected:
		uint8_t m_buf[len];
	};
	//可用作map的key,buf只是引用，需保证buf的生存期
	template<unsigned int len>
	class crtrefbinobj{
	public:
		crtrefbinobj(){m_buf=NULL;}
		crtrefbinobj(const void *buf){setbuf(buf);}
		virtual ~crtrefbinobj(){}
		void setbuf(const void *buf){m_buf=buf;}
		bool operator < (typename const crtrefbinobj<len> &other) const
		{return memcmp(m_buf,other.m_buf,len)<0;}
		const void *getbuf(){return m_buf;}
	protected:
		const void *m_buf;
	};
};
