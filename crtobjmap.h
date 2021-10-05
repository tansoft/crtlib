#pragma once

#include "crtlib.h"
#include "crtbuffer.h"

/**
* @brief 对象操作类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/

namespace crtfun {
	template<typename T>
	class crtobjmap{
	public:
		crtobjmap(){}
		virtual ~crtobjmap(){
			typename map<T,crtmagicobj *>::iterator it=m_obj.begin();
			while(it!=m_obj.end()) {
				releaseobj(it->second);
				it++;
			}
			m_obj.clear();
		}
		void setint(T key, int value) {
			crtmagicobj *obj=popobj(key);
			obj->type=crtmagictype_int;
			obj->v.ival=value;
		}
		void setuint(T key, unsigned int value) {
			crtmagicobj *obj=popobj(key);
			obj->type=crtmagictype_uint;
			obj->v.uval=value;
		}
		void setint64(T key, int64_t value) {
			crtmagicobj *obj=popobj(key);
			obj->type=crtmagictype_int64;
			obj->v.i64val=value;
		}
		void setdouble(T key, double value) {
			crtmagicobj *obj=popobj(key);
			obj->type=crtmagictype_double;
			obj->v.dval=value;
		}
		void setstring(T key, const char* value) {
			crtmagicobj *obj=popobj(key);
			obj->type=crtmagictype_string;
			obj->v.pval=new char[strlen(value)+1];
			strcpy(obj->v.pval,value);
		}
		//mem���ڴ���objmap����
		void setmem(T key, const void* value, unsigned int len) {
			crtmagicobj *obj=popobj(key);
			obj->type=crtmagictype_mem;
			obj->v.pval=new char[len+1];
			memcpy(obj->v.pval,value,len);
		}
		//pointer���ⲿָ��
		void setpointer(T key, const void* value) {
			crtmagicobj *obj=popobj(key);
			obj->type=crtmagictype_pointer;
			obj->v.pval=(char *)value;
		}
		bool ishavekey(T key){return m_obj.find(key)!=m_obj.end();}
		void removekey(T key){
			typename map<T,crtmagicobj *>::iterator it=m_obj.find(key);
			if (it!=m_obj.end()) {
				releaseobj(it->second);
				m_obj.earse(it);
			}
		}
		int getint(T key,int defvalue=0){
			crtmagicobj *obj=getobj(key);
			if (obj && obj->type==crtmagictype_int) return obj->v.ival;
			return defvalue;
		}
		unsigned int getuint(T key,unsigned int defvalue=0){
			crtmagicobj *obj=getobj(key);
			if (obj && obj->type==crtmagictype_uint) return obj->v.uval;
			return defvalue;
		}
		int64_t getint64(T key,int64_t defvalue=0){
			crtmagicobj *obj=getobj(key);
			if (obj && obj->type==crtmagictype_int64) return obj->v.i64val;
			return defvalue;
		}
		double getdouble(T key,double defvalue=0.0){
			crtmagicobj *obj=getobj(key);
			if (obj && obj->type==crtmagictype_double) return obj->v.dval;
			return defvalue;
		}
		string getstring(T key,const char *defvalue=""){
			crtmagicobj *obj=getobj(key);
			if (obj && obj->type==crtmagictype_string) return obj->v.pval;
			return defvalue;
		}
		void *getpointer(T key,void *defvalue=NULL) {
			crtmagicobj *obj=getobj(key);
			if (obj && (obj->type==crtmagictype_pointer || obj->type==crtmagictype_mem)) return obj->v.pval;
			return defvalue;
		}
	protected:
		typedef enum _crtmagictype{
			crtmagictype_int,
			crtmagictype_uint,
			crtmagictype_int64,
			crtmagictype_double,
			crtmagictype_string,
			crtmagictype_mem,
			crtmagictype_pointer
		}crtmagictype;
		typedef struct _crtmagicobj{
			crtmagictype type;
			union value{
				int ival;
				unsigned int uval;
				double dval;
				int64_t i64val;
				char *pval;
			}v;
		}crtmagicobj;
		void releaseobj(crtmagicobj *obj){
			if (obj->type==crtmagictype_mem || obj->type==crtmagictype_string) delete [] obj->v.pval;
			m_pool.release_item(obj);
		}
		crtmagicobj *getobj(T key){
			typename map<T,crtmagicobj *>::iterator it=m_obj.find(key);
			if (it!=m_obj.end()) return it->second;
			return NULL;
		}
		crtmagicobj *popobj(T key){
			typename map<T,crtmagicobj *>::iterator it=m_obj.find(key);
			crtmagicobj *obj;
			if (it==m_obj.end()) {
				obj=m_pool.new_item();
				m_obj.insert(map<typename T,crtmagicobj *>::value_type(key,obj));
			} else {
				obj=it->second;
				if (obj->type==crtmagictype_mem || obj->type==crtmagictype_string) delete [] obj->v.pval;
			}
			return obj;
		}
		typename map<T,crtmagicobj *> m_obj;
		crtmemmanager<crtmagicobj> m_pool;
	};
	template <typename T,typename K>
	class crtobjmapmanager
	{
	public:
		crtobjmapmanager(void) {}
		virtual ~crtobjmapmanager(void) {clear();}
		void clear()
		{
			typename map<T,crtobjmap<K> *>::iterator it=m_mem.begin();
			while(it!=m_mem.end()){
				delete it->second;
				m_mem.erase(it++);
			}
		}
		bool ishave(T id) {return m_mem.find(id)!=m_mem.end();}
		void remove(T id)
		{
			typename map<T,crtobjmap<K> *>::iterator it=m_mem.find(id);
			if (it==m_mem.end()) return;
			delete it->second;
			m_mem.erase(it);
		}
		void setint(T id,K key,int value) {popmap(id)->setint(key,value);}
		void setuint(T id,K key,unsigned int value) {popmap(id)->setuint(key,value);}
		void setint64(T id,K key,int64_t value) {popmap(id)->setint64(key,value);}
		void setdouble(T id,K key, double value) {popmap(id)->setdouble(key,value);}
		void setstring(T id,K key, const char* value) {popmap(id)->setstring(key,value);}
		//mem���ڴ���objmap����
		void setmem(T id,K key, const void* value, unsigned int len) {popmap(id)->setmem(key,value);}
		//pointer���ⲿָ��
		void setpointer(T id,K key, const void* value) {popmap(id)->setpointer(key,value);}
		bool ishavekey(T id,K key) {
			crtobjmap<K> *map=findmap(id);
			if (map) return map->ishavekey(key);
			return false;
		}
		void removekey(T id,K key){
			crtobjmap<K> *map=findmap(id);
			if (map) map->removekey(key);
		}
		int getint(T id,K key,int defvalue=0){
			crtobjmap<K> *map=findmap(id);
			if (map) return map->getint(key,defvalue);
			return defvalue;
		}
		unsigned int getuint(T id,K key,unsigned int defvalue=0){
			crtobjmap<K> *map=findmap(id);
			if (map) return map->getuint(key,defvalue);
			return defvalue;
		}
		int64_t getint64(T id,K key,int64_t defvalue=0){
			crtobjmap<K> *map=findmap(id);
			if (map) return map->getint64(key,defvalue);
			return defvalue;
		}
		double getdouble(T id,K key,double defvalue=0.0){
			crtobjmap<K> *map=findmap(id);
			if (map) return map->getdouble(key,defvalue);
			return defvalue;
		}
		string getstring(T id,K key,const char *defvalue=""){
			crtobjmap<K> *map=findmap(id);
			if (map) return map->getstring(key,defvalue);
			return defvalue;
		}
		void *getpointer(T id,K key,void *defvalue=NULL) {
			crtobjmap<K> *map=findmap(id);
			if (map) return map->getpointer(key,defvalue);
			return defvalue;
		}
		typename crtobjmap<K> *findmap(T id){
			typename map<T,crtobjmap<K> *>::iterator it=m_mem.find(id);
			if (it!=m_mem.end()) return it->second;
			return NULL;
		}
	protected:
		crtobjmap<K> *popmap(T id){
			typename map<T,crtobjmap<K> *>::iterator it=m_mem.find(id);
			typename crtobjmap<K> *obj;
			if (it==m_mem.end()) {
				obj=new crtobjmap<K>;
				m_mem.insert(map<T,crtobjmap<K> *>::value_type(id,obj));
			} else
				obj=it->second;
			return obj;
		}
		typename map<T,crtobjmap<K> *> m_mem;
	};
};
