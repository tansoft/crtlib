#pragma once
#include "crtlibuv.h"
#include "crtsystem.h"
#include "crttime.h"
#include "crtstring.h"
#include "crtcookie.h"
#include "crtbuffer.h"
#include "crtstructure.h"
#include "crtregex.h"
#include "crtcharset.h"
#ifndef _CRTUSEICONV
	#if (!defined(_WIN32)) && (!defined(__MINGW32__)) && (!defined(_CYGWIN_))
		#error "must defined _CRTUSEICONV before build"
	#endif
#endif

namespace crtfun{
	//current connection status
	#define CRTHTTPSTATUS_NONE 0
	#define CRTHTTPSTATUS_PARSING 1
	#define CRTCHARSET_UTF8 0
	#define CRTCHARSET_GB2312 1
	#define CRTCHARSET_AUTOFIX_MODE 2
	class crtlibuvhttpserver: protected icrtevent {
	public:
		crtlibuvhttpserver(){uv=NULL;server=NULL;headerbuffer.setmemobjhandler(&memman);defaultsettings();}
		virtual ~crtlibuvhttpserver(){}
		bool startserver(unsigned short port=80, crtlibuv *libuv=NULL){
			if (libuv) uv=libuv;
			else uv=crtlibuv::getinstance();
			crtsrand();
			//uv=crtlibuv::getnewloop();
			uv->seteventcallback(this);
			server = uv->socket_tcpserver_start(crtlibuv::socket_ip4addr("0.0.0.0",port));
			if (!server) return false;
			alivecheck = uv->timer_start(0,1000);
			return true;
		}
		void stopserver(){
			if (server) {
				uv->socket_tcpserver_stop(server);
				server=NULL;
				headerbuffer.clear();
			}
			if (alivecheck) {
				uv->timer_stop(alivecheck, true);
				alivecheck=NULL;
			}
		}
		void defaultsettings() {
			m_keep_alive_timeout_sec = 3;
			m_parse_timeout_sec = 90;
			m_current_sec = time(NULL);
			strcpy(m_server_name,"crthttpserver");
			memman.m_server_name=m_server_name;
			m_use_ip_filter=false;
			m_use_session=false;
			m_session_cookieless_mode=false;
			m_session_cookie_timeout_sec=0;
			m_use_auth=false;
			strcpy(m_auth_domain_name,"crtauthserver");
			m_force_use_auth_base_mode=false;
			m_charset_mode=CRTCHARSET_AUTOFIX_MODE;
		}
		void set_keep_alive_timeout_sec(int sec=0) {m_keep_alive_timeout_sec=sec;}
		void set_parse_timeout_sec(int sec=0) {m_parse_timeout_sec=sec;}
		void set_server_name(const char *servername){strncpy(m_server_name,servername,128);}
		void set_allow_ips(const char *ip,const char *mask) {
			m_allow_ips.add_ip_range(ip,mask);
			m_use_ip_filter=true;
		}
		void set_deny_ips(const char *ip,const char *mask) {
			m_deny_ips.add_ip_range(ip,mask);
			m_use_ip_filter=true;
		}
		void clean_ip_filter() {
			m_use_ip_filter=false;
			m_allow_ips.clear();
			m_deny_ips.clear();
		}
		void set_use_session(bool usesession=true,bool cookieless=false) {
			m_use_session=usesession;
			m_session_cookieless_mode=cookieless;
		}
		void set_session_cookie_timeout_sec(int sec=0){m_session_cookie_timeout_sec=sec;}
		void update_system_values(string key, string value){m_system_values[key]=value;}
		void set_use_auth(bool auth=true,const char *user=NULL,const char *pass=NULL) {
			if (user && pass) {
				m_digest_auth_users.addvalue(user,pass);
				m_base_auth_users.insert(string(user)+":"+pass);
			}
			m_use_auth=auth;
		}
		//加密授权默认使用Digest加密模式增强安全性，在不支持的浏览器访问时，使用Base模式 
		void set_force_use_auth_base_mode(bool bforce){m_force_use_auth_base_mode=bforce;}
		void set_charset_mode(int mode){m_charset_mode=mode;}
	protected:
		int m_keep_alive_timeout_sec;	// sec = 0 means not keepalive support
		int m_parse_timeout_sec;		// sec = 0 means not parse timeout
		time_t m_current_sec;
		char m_server_name[128];
		bool m_use_ip_filter;
		crtiprange m_allow_ips;
		crtiprange m_deny_ips;
		bool m_use_session;
		bool m_session_cookieless_mode;
		// sec = 0 not have exprie time,it is just session cookie, sec != 0 is timeout cookie,will update every request
		int m_session_cookie_timeout_sec;
		map<string, string> m_system_values;
		bool m_use_auth;
		char m_auth_domain_name[128];
		crtstringparser m_digest_auth_users;
		set<string> m_base_auth_users;
		bool m_force_use_auth_base_mode;
		int m_charset_mode;
		typedef struct _crtlibuvhttpstatus{
			int status;
			unsigned int headerlen;
			string scontent;
			crtmembuf header;
			crthttpheaderparser parser;
			crthttpheaderresponsemaker maker;
			crtcookie cookie;
			time_t cachectrl;
		}crtlibuvhttpstatus;
		class crtlibuvhttpstatusmemobj : public icrtmemobj
		{
		public:
			friend class crtlibuvhttpserver;
			crtlibuvhttpstatusmemobj(){m_server_name=NULL;}
			virtual ~crtlibuvhttpstatusmemobj(){}
			virtual void *newobj() {
				void *status=(void *)buf.new_item();
				reset(status);
				return (void *)status;
			}
			void reset(void *obj) {
				crtlibuvhttpstatus *status=(crtlibuvhttpstatus *)obj;
				status->status=CRTHTTPSTATUS_NONE;
				status->headerlen=0;
				status->header.clear();
				status->maker.clear();
				status->cookie.clear();
				status->cachectrl=0;
				if (m_server_name) status->maker.setserver(m_server_name);
			}
			virtual void freeobj(void *obj) {buf.release_item((crtlibuvhttpstatus *)obj);}
			virtual void release_cache(){buf.release_cache();}
		protected:
			crtmemmanager<crtlibuvhttpstatus> buf;
			const char *m_server_name;
		};
		crtlibuv *uv;
		crtlibuvobj server;
		crtlibuvobj alivecheck;
		crtlibuvhttpstatusmemobj memman;
		crtmemobjmanager<crtlibuvobj> headerbuffer;
		crtlru<crtlibuvobj> checklist;
		virtual void onclose(crtlibuv *uv,crtlibuvobj obj) {
			crtdebug("[LIBUVHTTP]socket %p closed.\n",obj);
			headerbuffer.removeobj(obj);
			if (m_keep_alive_timeout_sec!=0 || m_parse_timeout_sec!=0) checklist.cancel(obj);
		}
		virtual void ontimer(crtlibuv *uv,crtlibuvobj obj,int status) {
			if (obj==alivecheck) {
				m_current_sec = time(NULL);
				if (m_keep_alive_timeout_sec==0 && m_parse_timeout_sec==0) return;
				while(1) {
					if (checklist.empty()) break;
					crtlibuvobj sobj=checklist.poptimeout(m_current_sec);
					if (!sobj) break;
					crtlibuvhttpstatus *status=(crtlibuvhttpstatus *)headerbuffer.getobj(sobj);
					if (status->status==CRTHTTPSTATUS_NONE) {
						crtdebug("[LIBUVHTTP]socket %p keepalive shutdown.\n",sobj);
						uv->socket_shutdown(sobj);
					} else {
						crtdebug("[LIBUVHTTP]socket %p timeout to run, shut it down.\n",sobj);
						responseandclosebystatus(sobj, status, 504);
					}
				}
			}
		}
		//return 0 that means need the buffer for next read, not need to free it
		//fixme, check commit contect-length is only int support
		virtual int onreceive(crtlibuv *uv,crtlibuvobj obj, uv_buf_t buf, ssize_t nread) {
			crtdebug("[LIBUVHTTP]socket %p receive %d data.\n",obj,nread);
			crtlibuvhttpstatus *status=(crtlibuvhttpstatus *)headerbuffer.getornewobj(obj);
			status->status=CRTHTTPSTATUS_PARSING;
			if (m_parse_timeout_sec) checklist.update(obj,m_current_sec+m_parse_timeout_sec);
			else if (m_keep_alive_timeout_sec) checklist.updateifexist(obj,m_current_sec+m_keep_alive_timeout_sec);
			status->header.addbuf(buf.base,(unsigned int)nread);
			unsigned int recvlen=status->headerlen;
			string header=status->header.findstr("\r\n\r\n",4,false);
			if (recvlen>0) {
				if (recvlen<=status->header.get_buffered_size()) {
					if (!status->parser.parse(header.c_str())) {
						responseandclosebystatus(obj, status, 400);
						return 1;
					}
					status->scontent=status->header.substr(header.length()+4);
					parseresponse(obj,status);
				}
			} else {
				if (header.length()) {
					recvlen=0;
					size_t pos=header.find("Content-Length");
					if (pos!=string::npos) {
						pos=header.find(":",pos);
						if (pos!=string::npos)
							recvlen=(unsigned int)string2size_t(header.c_str()+pos+1)+header.length()+4;
					}
					if (recvlen==0 || (recvlen>0 && recvlen<=status->header.get_buffered_size())) {
						if (!status->parser.parse(header.c_str())) {
							responseandclosebystatus(obj, status, 400);
							return 1;
						}
						status->scontent=status->header.substr(header.length()+4);
						parseresponse(obj,status);
					}
					else
						status->headerlen=recvlen;
				}
			}
			return 1;
		}
		virtual void onaccept(crtlibuv *uv, crtlibuvobj server, crtlibuvobj client) {crtdebug("[LIBUVHTTP]socket %p accepted.\n",client);}
		virtual void onsended(crtlibuv *uv, crtlibuvobj sock) {crtdebug("[LIBUVHTTP]socket %p sended.\n",sock);}
		void responseandclosebystatus(crtlibuvobj obj, crtlibuvhttpstatus *status, int statuscode, const char *content="", const char *statustext="") {
			status->maker.setresult(statuscode, statustext);
			responseandclose(obj, status, content);
		}
		void responseandclose(crtlibuvobj obj, crtlibuvhttpstatus *status, const char *content="") {
			status->maker.addheaders("Date",timet2gmttime(m_current_sec).c_str(),false);
			if (status->cachectrl) {
				status->maker.addcachecontrol(string_format("max-age=%d",(int)(status->cachectrl-m_current_sec)).c_str());
				status->maker.addexpires(timet2gmttime(status->cachectrl).c_str());
			} else {
				status->maker.addcachecontrol("no-cache");
				status->maker.addexpires(timet2gmttime(m_current_sec).c_str());
			}
			//fixme temp use
			status->maker.setresponsetype("text/html");
			status->maker.setrange(strlen(content));
			bool keepalive=false;
			//just setting is support keepalive and only status is ok to check keepalive settings
			int stscode=status->maker.getresultcode();
			if (m_keep_alive_timeout_sec>0 && stscode>=200 && stscode<=399)
				keepalive=string_compare_nocase(status->parser.get_connection_type().c_str(),"Keep-Alive");
			if (!keepalive) status->maker.setconnectiontype();
			string header=status->maker.makeheaders();
			header+=content;
			responsedata(obj, header.c_str(), (int)header.length(), !keepalive);
			if (keepalive) {
				memman.reset((void *)status);
				checklist.update(obj,m_current_sec+m_keep_alive_timeout_sec);
			}
		}
		inline void responsedata(crtlibuvobj obj, const char *buf, int buflen, bool bshutdown){
			if (m_parse_timeout_sec) checklist.update(obj,m_current_sec+m_parse_timeout_sec);
			uv->socket_tcp_send(obj, buf, buflen, bshutdown);
		}
		string _makesessionid() {
			string ret;
			ret="cs";
			for(int i=0;i<16;i++) ret+=string_format("%c",'a'+rand()%26);
			ret+="se";
			return ret;
		}
		void _getsessionid(string &request,string &sessionid)
		{
			string tmp=request;
			if (tmp.at(0)=='/') tmp=tmp.substr(1);
			if (tmp.length()>20 && tmp.substr(0,2)=="cs" && tmp.substr(18,2)=="se") {
				sessionid=tmp.substr(2,16);
				request=tmp.substr(20);
				if (request.empty()) request="/";
			} else sessionid.clear();
		}
		string _make_nonce() {
			char buf[64];
			sprintf(buf,"%d%d%d%d",rand(),rand(),rand(),rand());
			return crtmd5::md5str2str(buf);
		}
		inline string _make_opaque(){return _make_nonce();}
		void parseresponse(crtlibuvobj obj, crtlibuvhttpstatus *status) {
			string method=status->parser.get_first_header(0);
			string request=status->parser.get_first_header(1).c_str();
			crtdebug("[LIBUVHTTP]request %s\n",request.c_str());
			string unit;
			size_t start_pos,end_pos;
			if (!status->parser.get_range(unit,start_pos,end_pos)) {
				responseandclosebystatus(obj, status, 400);
				return;
			}
			uint32_t peerip=uv->socket_tcp_get_peer_ip4_n(obj);
			if (peerip == 0) {
				responseandclosebystatus(obj, status, 500);
				return;
			}
			bool bok=true;
			int stscode=0;
			string returnval;
			map<string, string> m_session_values;
			//--------------------ip filter---------------------------
			if (m_use_ip_filter) {
				if ((m_allow_ips.size() > 0 && !m_allow_ips.is_ip_in_range_n(peerip)) ||
					m_deny_ips.is_ip_in_range_n(peerip)) {
					bok=false;
					stscode=403;
					returnval="403 Forbidden";
				}
			}
			//--------------------------auth----------------------------
			if (m_use_auth && bok) {
				bok=false;
				string auth=status->parser.gettag("Authorization");
				if (string_startwith(auth.c_str(),"Digest")) {
					//Digest username="ad", realm="TansoftServer", nonce="357c6b2089359f3fe535d5d6ce32f5de",
					//uri="/", algorithm=MD5, response="fb16e27da447417187a084dc82a46806", opaque="8a25a37d590060e6e503c72e3aea8ab2"
					crtstringparser parser;
					parser.parse_with_key_quote_values(auth.substr(7).c_str());
					string username=parser.getvalue("username");
					string uri=crturl_decode(parser.getvalue("uri").c_str());
					string nonce=parser.getvalue("nonce");
					string response=parser.getvalue("response");
					//check if uri is same
					if (uri==request && !username.empty() && !nonce.empty() && !response.empty()) {
						list<string> pass;
						if (m_digest_auth_users.getallvalue(username,pass)) {
							list<string>::iterator it=pass.begin();
							while(it!=pass.end()) {
								const string &val=*it;
								if (!val.empty()) {
									string md5=crtmd5::md5str2str(
										(crtmd5::md5str2str((username + ":" + m_auth_domain_name + ":" + val).c_str())
										+ ":" + nonce + ":" + crtmd5::md5str2str((method + ":" + uri).c_str()).c_str()).c_str());
									if (string_compare_nocase(md5.c_str(),response.c_str())) {
										m_session_values["authuser"]=username;
										m_session_values["authpassmd5"]=crtmd5::md5str2str(val.c_str());
										bok=true;
										crtdebug("[LIBUVHTTP]auth user %s login with digest mode\n",username.c_str());
										break;
									}
								}
								it++;
							}
						}
					}
				} else if (string_startwith(auth.c_str(),"Basic")) {
					auth=crtbase64::decode_str(auth.substr(6));
					if (!auth.empty()) {
						set<string>::iterator it = m_base_auth_users.find(auth);
						if (it!=m_base_auth_users.end()) {
							size_t pos=auth.rfind(":");
							if (pos==string::npos) {
								m_session_values["authuser"]=auth;
								m_session_values["authpassmd5"]=crtmd5::md5str2str(string("").c_str());
							} else {
								m_session_values["authuser"]=auth.substr(0,pos);
								m_session_values["authpassmd5"]=crtmd5::md5str2str(auth.substr(pos+1).c_str());
							}
							bok=true;
							crtdebug("[LIBUVHTTP]auth user login with basic mode\n");
						}
					}
				}
				if (!bok) {
					crthttpuatest test;
					test.init(status->parser.get_user_agent().c_str());
					if (m_force_use_auth_base_mode==false && test.is_support_digest_auth())
						status->maker.addheaders("WWW-Authenticate",
							string_format("Digest algorithm=\"md5\",realm=\"%s\",nonce=\"%s\",opaque=\"%s\"",m_auth_domain_name,
							_make_nonce().c_str(),_make_opaque().c_str()).c_str(),false);
					else
						status->maker.addheaders("WWW-Authenticate",
							string_format("Basic realm=\"%s\"",m_auth_domain_name).c_str(),false);
					stscode=401;
					returnval="Unauthorized";
				}
			}
			//--------------------cookie and session------------------
			status->cookie.getcookiefromhttpheaderclientcookie(status->parser.get_cookie().c_str());
			//-------------------------session------------------------
			if (m_use_session && bok) {
				string sessionid;
				if (m_session_cookieless_mode) {
					//check the path
					_getsessionid(request,sessionid);
					if (sessionid.empty()) {
						//if in session with cookieless mode,and path is not have sessions
						stscode=302;
						sessionid=_makesessionid();
						status->maker.addheaders("Location",(string("/")+sessionid+request).c_str(),false);
						status->cachectrl=0;
						bok=false;
					}
				} else {
					//check cookie
					sessionid=status->cookie.findlastcookie("crtsession");
					if (sessionid.empty() || sessionid.length()!=20) {
						sessionid=_makesessionid();
						string cookietimeout;
						if (m_session_cookie_timeout_sec) cookietimeout=timet2gmttime(m_current_sec+m_session_cookie_timeout_sec);
						status->maker.addcookie((string("crtsession=")+sessionid).c_str(),cookietimeout.c_str());
					} else {
						//update the expiretime if session cookie is timeout cookie
						if (m_session_cookie_timeout_sec) {
							status->maker.addcookie((string("crtsession=")+sessionid).c_str(),
								timet2gmttime(m_current_sec+m_session_cookie_timeout_sec).c_str());
						}
						sessionid=sessionid.substr(2,16);
					}
				}
				m_session_values["crtsession"]=sessionid;
			}
			//----------------------request parse-------------------
			if (bok) {
				ifilestore *pfs=m_store;
				string file,param;
				size_t parampos=request.find('?');
				if (parampos!=string::npos) {
					file=crturl_decode(request.substr(0,pos).c_str());
					param=request.substr(pos+1);
				} else file=crturl_decode(request.c_str());
				//-------------------------convert for read filesystem-------------------
				if (m_charset_mode==CRTCHARSET_GB2312 || (m_charset_mode==CRTCHARSET_AUTOFIX_MODE && is_charset_maybe_utf8(file.c_str())==false)
					file=crta2u8(file.c_str());
				//CRTCHARSET_UTF8 not need do anythings
				//----------------host and virtual path-----------------
				/*if (m_use_host_mapping)
				{
					THString host=parser.GetHost();
					host.MakeLower();
					ITHFileStore *fs;
					if (m_hostmapping.GetAt(host,fs))
					{
						if (fs) pfs=fs;
					}
				}
				if (m_use_virtualdir_mapping)
				{
					THPosition pos=m_dirmapping.GetStartPosition();
					THString host;
					THString tmpfile=file;
					int basecnt=0;
					if (tmpfile.GetAt(0)=='/')
					{
						basecnt=1;
						tmpfile=tmpfile.Mid(1);
					}
					tmpfile.MakeLower();
					ITHFileStore *fs;
					while(!pos.IsEmpty())
					{
						if (m_dirmapping.GetNextPosition(pos,host,fs))
						{
							if (fs && tmpfile.Find(host)==0)
							{
								sysparam+=THSimpleXml::MakeParam(_T("virtualroot"),host);
								file=file.Mid(host.GetLength()+basecnt);
								if (file.IsEmpty())
								{
									file=_T("/");
									//由于该请求是目录，而列目录的时候，会导致相对目录定位错误，这时应该301到目录+/下
									StsCode=301;
									maker.AddHeaders(_T("Location"),orgfile+_T("/"),FALSE);
								}
								pfs=fs;
								break;
							}
						}
					}
				}*/
			}
			/*FILE *fp=crtfopen_utf8("F:/Users/barry/Documents/src/svn/edudoc/网络/iftop查看网卡流量.txt","rb");
			printf("%p\n",fp);
			fclose(fp);
*/
			if (stscode==0) {
				stscode=200;
				returnval="test"+status->cookie.findlastcookie("crtsession");
			}
			responseandclosebystatus(obj, status, stscode, returnval.c_str(), "");
			return;
		}
	};
};