#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crthttp.h"
#include "crttime.h"
#define HAVE_JSON_SERIALIZATION
#ifdef HAVE_JSON_SERIALIZATION
	#include "crtjson.h"
#endif

/**
* @brief Cookie处理类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/
namespace crtfun {
	class crtcookie {
	public:
		crtcookie(){}
		virtual ~crtcookie(){}
		//sockettimeout >0 is timeout in ms, 0=unlimited, -1=determine with globalsetting setup with setup_http_global_timeout
		//retrytimes >0 is retry times, 0=no retry, -1=determine with globalsetting setup with setup_http_global_retrytimes
		string http_download_to_str(const char *url,const char *postdata=NULL,int *psts=NULL,const char *addheader=NULL,
			int sockettimeout=-1, int retrytimes=-1) {
			if (sockettimeout==-1) sockettimeout=crtlib::instance()->http_global_timeout;
			if (retrytimes==-1) retrytimes=crtlib::instance()->http_global_retrytimes;
			ret.clear();
			addheaders.clear();
			if (addheader) addheaders=addheader;
			parser.parse(url);
			int sts;
			while(retrytimes>=0){
				sts = http_download_callback(url, cookie_download_cb, this, postdata, sockettimeout);
				retrytimes--;
				if ((int)(sts / 100) == 2) break;
				ret.clear();
				crtdebug("[COOKIE][RETRY]%s\n",url);
			}
			if (psts) *psts=sts;
			return ret;
		}
		string tostring() {
			string ret;
			map<string,cookie_item>::iterator it=cookies.begin();
			int i=1;
			while(it!=cookies.end()) {
				cookie_item &item=it->second;
				ret+=string_format("[%d] ", i++);
				ret+=item.cookie;
				ret+=" = ";
				ret+=item.value;
				ret+=", path=";
				ret+=item.path;
				ret+=", domain=";
				ret+=item.domain;
				if (item.expires!=0) {
					ret+=", expires=";
					ret+=locallongtime2string(item.expires);
				}
				ret+="\n";
				it++;
			}
			return ret;
		}
		//手工替换已存在的cookie的值
		void replacecookie(const char *key, const char *value) {
			map<string,cookie_item>::iterator it=cookies.begin();
			string host="."+parser.host;
			while(it!=cookies.end()) {
				cookie_item &item=it->second;
				if (parser.request.find(item.path)==0 && host.rfind(item.domain)==host.length()-item.domain.length())
					if (item.cookie==key) {
						it->second.value = value;
						return;
					}
				it++;
			}
		}
		void removecookie(const char *key) {
			map<string,cookie_item>::iterator it=cookies.begin();
			string host="."+parser.host;
			while(it!=cookies.end()) {
				cookie_item &item=it->second;
				if (parser.request.find(item.path)==0 && host.rfind(item.domain)==host.length()-item.domain.length())
					if (item.cookie==key) {
						cookies.erase(it);
						return;
					}
				it++;
			}
		}
		string findlastcookie(const char *key,bool includeexpires=false) {
			map<string,cookie_item>::iterator it=cookies.begin();
			time_t ti=time(NULL);
			string host="."+parser.host;
			while(it!=cookies.end()) {
				cookie_item &item=it->second;
				if (includeexpires || !(item.expires != 0 && item.expires < ti)) {
					if (parser.request.find(item.path)==0 && host.rfind(item.domain)==host.length()-item.domain.length())
						if (item.cookie==key) return item.value;
				}
				it++;
			}
			return "";
		}
		void clear() {cookies.clear();}
		inline void getcookiefromhttpheadersetcookie(const char* httpheader) {savecookie(httpheader);}
		inline void getcookiefromhttpheaderclientcookie(const char* cookiestring) {parsecookie(cookiestring);}
#ifdef HAVE_JSON_SERIALIZATION
		bool getcookiefromjsonstring(const char* jsonstr) {
			crtjsonparser parser;
			crtjson *json=parser.parse(jsonstr);
			if (!json) return false;
			cookies.clear();
			crtjson *item=parser.firstitem(json);
			do{
				cookie_item citem;
				citem.value=parser.findstr(item,"value");
				citem.path=parser.findstr(item,"path");
				citem.domain=parser.findstr(item,"domain");
				citem.expires=(unsigned long)parser.finddouble(item,"expires");
				citem.cookie=parser.itemkey(item);
				cookies[citem.cookie]=citem;
			} while((item=parser.nextitem(item)));
			parser.delete_json(json);
			return true;
		}
		string savecookietojsonstring() {
			map<string,cookie_item>::iterator it=cookies.begin();
			crtjsonparser parser;
			string ret;
			crtjson *root=parser.createobject();
			while(it!=cookies.end()) {
				cookie_item &item=it->second;
				crtjson *sub=parser.createobject();
				parser.objectadd(root,it->first.c_str(),sub);
				parser.objectadd_string(sub,"value",item.value.c_str());
				parser.objectadd_string(sub,"path",item.path.c_str());
				parser.objectadd_string(sub,"domain",item.domain.c_str());
				parser.objectadd_number(sub,"expires",(unsigned long)item.expires);
				it++;
			}
			ret = parser.tostring(root,0);
			parser.delete_json(root);
			return ret;
		}
#endif
	protected:
		typedef struct _cookie_item{
			string cookie;
			string value;
			time_t expires;
			string path;
			string domain;
		}cookie_item;
		static bool cookie_download_cb(enum cb_type type, int totallen, int curlen, const char *buf, unsigned int buflen, const void *add) {
			if (type == cb_httpdata)
				((crtcookie *) add)->ret.append(buf, buflen);
			else if (type == cb_buildhttpheader) {
				//set cookie
				((crtcookie *) add)->putcookie((char *)buf);
			} else if (type == cb_httpheader || type == cb_redirecthttpheader)
				//get cookie
				((crtcookie *) add)->savecookie((char *)buf);
			else if (type == cb_redirect)
				((crtcookie *) add)->parser.parse(buf);
			return true;
		}
		void putcookie(char *buf) {
			string cookie;
			map<string,cookie_item>::iterator it=cookies.begin();
			time_t ti=time(NULL);
			string host="."+parser.host;
			while(it!=cookies.end()) {
				cookie_item &item=it->second;
				if (item.expires != 0 && item.expires < ti)
					cookies.erase(it++);
				else {
					if (parser.request.find(item.path)==0 && host.rfind(item.domain)==host.length()-item.domain.length()) {
						if (!cookie.empty()) cookie+="; ";
						cookie+=item.cookie + "=" + item.value;
					}
					it++;
				}
			}
			if (!cookie.empty()) {
				crtdebug("[COOKIE][SEND]Cookie: %s\n",cookie.c_str());
				strcat(buf, "\r\nCookie: ");
				strcat(buf, cookie.c_str());
			}
			if (!addheaders.empty()) strcat(buf, addheaders.c_str());
		}
		void parsecookie(const char *cookievalues) {
			string tk2,domain,path,key,value;
			time_t exp=0;size_t pos;
			vector<string> keys,values;
			crtstringtoken token2;
			token2.init(cookievalues,";");
			while(token2.ismore()) {
				tk2=token2.nexttoken();
				string_trim(tk2);
				if ((pos=tk2.find('='))!=string::npos)
				{
					key=string_trim_const(tk2.substr(0,pos));
					value=string_trim_const(tk2.substr(pos+1));
					const char *ckey=key.c_str();
					if (string_compare_nocase(ckey,"domain")) domain=value;
					else if (string_compare_nocase(ckey,"path")) path=value;
					else if (string_compare_nocase(ckey,"expires"))
						exp=gmttime2timet(value);
					else if (string_compare_nocase(ckey,"maxage") || string_compare_nocase(ckey,"max-age"))
						exp=time(NULL)+atoi(value.c_str());
					else
					{
						keys.push_back(key);
						values.push_back(value);
					}
				}
			}
			if (path.empty()) path="/";
			if (domain.empty()) domain="."+parser.host;
			for(size_t i=0;i<keys.size();i++) {
				cookie_item item;
				item.cookie=keys[i];
				item.value=values[i];
				item.expires=exp;
				item.path=path;
				item.domain=domain;
				cookies[item.cookie]=item;
			}
		}
		//fixme not parse httponly and secure
		void savecookie(const char *httpheader) {
			crtstringtoken token(httpheader,"\r\n");
			string tk;
			size_t pos;
			while(token.ismore()) {
				tk=token.nexttoken();
				pos=tk.find(':');
				if (pos!=string::npos && string_compare_nocase(string_trim_const(tk.substr(0,pos)).c_str(),"Set-Cookie"))
					parsecookie(tk.substr(pos+1).c_str());
			}
#ifdef CRTDEBUG
			if (cookies.size()>0)
				crtdebug("[COOKIE][RECV]Cookies:\n%s\n",tostring().c_str());
#endif
		}
		string ret;
		string currenthost;
		string addheaders;
		map<string,cookie_item> cookies;
		crturlparser parser;
	};
};
