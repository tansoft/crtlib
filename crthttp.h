#pragma once

#include "crtlib.h"
#include "crtsocket.h"

/**
* @brief Http处理类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/
namespace crtfun {
	enum cb_type{
		cb_httpdata,//http数据
		cb_connected,//已连接上通知
		cb_redirect,//301,302重定向通知
		cb_httpheader,//获得httpheader通知（最终的header）
		cb_buildhttpheader,//生成请求header回调
		cb_redirecthttpheader//重定向请求的httpheader通知
	};
	typedef bool (*download_process)(enum cb_type type,int totallen,int curlen,const char *buf,unsigned int buflen,const void *add);
	static void setup_http_user_agent(const char *agent){strcpy(crtlib::instance()->http_user_agent,agent);}
	//sockettimeout >0 is timeout in ms, 0=unlimited
	static void setup_http_global_timeout(int timeout){crtlib::instance()->http_global_timeout=timeout;}
	//retrytimes >0 is retry times, 0=no retry
	static void setup_http_global_retrytimes(int retrytimes){crtlib::instance()->http_global_retrytimes=retrytimes;}
	static void set_last_http_error(unsigned int result) {crtlib::instance()->http_last_errcode=result;}
	static unsigned int get_last_http_error() {return crtlib::instance()->http_last_errcode;}
	//处理http header,如果多收的正文,通过buf返回,返回值-1出错,>=0为正文大小
	static int ensure_recv_http_header(SOCKET s, char *buf, int buflen, string &header, int timeout) {
		int len;
		while (1) {
			len = recv_timeout(s, buf, buflen, 0, timeout);
			if (len <= 0)
			{
				set_last_http_error(10);
				return -1;
			}
			const char *p=NULL;
			if (header.length()>0) {
				//double check last state
				int count=0;
				while(buf[count]=='\r' || buf[count]=='\n') count++;
				if (count>0) {
					string str2=string_right(header,4);
					str2.append(buf,count);
					if (str2.find("\r\n\r\n")!=string::npos)
						p=buf+count;
				}
			}
			if (!p) {
				p = (const char *) bin_search(buf, len, "\r\n\r\n", 4);
				if (p) p+=4;
			}
			if (p) {
				header.append(buf, p - buf);
				len -= (long) (p - buf);
				if (len) memcpy(buf, p, len);
				return len;
			} else
				header.append(buf, len);
		}
		return 0;
	}
	static string crtmap2post(const map<string,string> &postdata,bool bupper=false) {
		string ret;
		map<string,string>::const_iterator it=postdata.begin();
		while(it!=postdata.end()) {
			if (!ret.empty()) ret+="&";
			ret+=it->first+"="+crturl_encode(it->second.c_str(),bupper);
			it++;
		}
		return ret;
	}
	static SOCKET build_http_request(const char *url, download_process cb, const void *add,const char *postdata, int sockettimeout) {
		uint32_t ip;
		uint16_t port;
		time_t cms;
		const char *tmp1, *tmp2;
		char tmpbuf[8192], tmpbuf2[1024];
		SOCKET s;
		sockaddr_in addr;
		unsigned int postlen;
		if (strncmp(url, "http://", 7) == 0) url += 7;
		tmp1 = strstr(url, ":");
		tmp2 = strstr(url, "/");
		if (tmp1 && (tmp1 < tmp2 || tmp2 == NULL)) {
			strncpy(tmpbuf2, url, tmp1 - url);
			tmpbuf2[tmp1 - url] = '\0';
			port = (uint16_t) atoi(tmp1 + 1);
		} else {
			if (tmp2) {
				strncpy(tmpbuf2, url, tmp2 - url);
				tmpbuf2[tmp2 - url] = '\0';
			} else
				strcpy(tmpbuf2, url);
			port = 80;
		}
		if (tmp2 == NULL) tmp2 = "/";
		ip = get_ip_by_name(tmpbuf2);
		if (ip == 0)
		{
			set_last_http_error(1);
			return INVALID_SOCKET;
		}
		/*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET)
		{
			set_last_http_error(2);
			return INVALID_SOCKET;
		}*/
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = ip;
		addr.sin_port = htons(port);
		cms = time(NULL);
		s = create_connect_socket(&addr, 0);
		if (s==INVALID_SOCKET) {
		//if (connect(s, (sockaddr *) &addr, sizeof(addr)) != 0) {
			closesocket(s);
			set_last_http_error(3);
			return INVALID_SOCKET;
		}
		cms = time(NULL) - cms;
		if (wait_connect(s, sockettimeout)!=0) {
			closesocket(s);
			set_last_http_error(3);
			return INVALID_SOCKET;
		}
		if (cb) cb(cb_connected, (int) cms, (int) cms, tmpbuf2, (unsigned int) strlen(tmpbuf2), add);
		strcpy(tmpbuf, postdata?"POST ":"GET ");
		strcat(tmpbuf, tmp2);
		strcat(tmpbuf, " HTTP/1.1\r\nAccept: */*");
		sprintf(tmpbuf+strlen(tmpbuf), "\r\nHost: %s\r\nUser-Agent: %s", tmpbuf2, crtlib::instance()->http_user_agent);
		if (cb) cb(cb_buildhttpheader, 0, 0, tmpbuf, 8192, add);
		if (postdata)
		{
			if (strncmp("[crtbin]",postdata,8)==0) {
				postlen=atoi(postdata+8);
				postdata=postdata+strlen(postdata)+1;
			} else
				postlen=(unsigned int)(strlen(postdata)/*+4*/);
			sprintf(tmpbuf+strlen(tmpbuf), "\r\nContent-Length: %u",postlen);
			strcat(tmpbuf,"\r\nContent-Type: application/x-www-form-urlencoded");
		}
		strcat(tmpbuf, "\r\n\r\n");
		crtdebug("[HTTP][SEND]header:%s\n",tmpbuf);
		if (ensure_send_str(s, tmpbuf) <= 0) {
			closesocket(s);
			set_last_http_error(4);
			return INVALID_SOCKET;
		}
		if (postdata)
		{
			crtdebug("[HTTP][SEND]postdata len:%u\n",postlen);
			if (ensure_send(s, postdata, postlen) <= 0) {
				closesocket(s);
				set_last_http_error(5);
				return INVALID_SOCKET;
			}
			/*if (ensure_send_str(s, "\r\n\r\n") <= 0) {
				closesocket(s);
				set_last_http_error(6);
				return INVALID_SOCKET;
			}*/
		}
		return s;
	}

	//support http chunked mode,301 302,if not have http length totallen=-1 in cb
	//return val:0 socket error,1 httpheader error,2 callback cancel,3 server break the connection,1xx-5xx http status
	static int http_download_callback(const char *url, download_process cb, const void *add, const char *postdata, int sockettimeout=0) {
		string ret;
		string httpheader;
		int r, recvlen = 0, totallen, tmplen, passedlen = 0;
		//totallen:总长度 recvlen:已接收长度 partlen:chunked模式当前块长度,-1为未知当前块长度,否则为总长度
		//passedlen:缓冲区中已处理数据的长度 r:缓冲区中待处理数据的长度 tmplen:临时变量
		char *pbuf;
		size_t pos;
		SOCKET s;
		char tmpbuf[4096];
		int sts;
		set_last_http_error(0);
		crtdebug("[HTTP][CONNECT]%s\n",url);
		s = build_http_request(url, cb, add, postdata, sockettimeout);
		if (s == INVALID_SOCKET) return 0;
		crtdebug("[HTTP][SENDED]\n");
		r = ensure_recv_http_header(s, tmpbuf, 4096, httpheader, sockettimeout);
		crtdebug("[HTTP][RECV]remain:%d,header:%s\n",r,httpheader.c_str());
		if (r < 0) {
			closesocket(s);
			return 0;
		}
		pos = httpheader.find(" ");
		if (pos == string::npos) {
			closesocket(s);
			set_last_http_error(20);
			return 1;
		}
		sts = atoi(httpheader.c_str() + pos + 1);
		pos = httpheader.find("Location:");
		if (pos != string::npos) {
			if (cb) cb(cb_redirecthttpheader, sts, sts, httpheader.c_str(), (unsigned int) httpheader.length(), add);
			httpheader = httpheader.substr(pos + 9);
			if (httpheader[0] == ' ')
				httpheader = httpheader.substr(1);
			pos = httpheader.find("\r\n");
			if (pos != string::npos)
				httpheader = httpheader.substr(0, pos);
			closesocket(s);
			crtdebug("[HTTP][REDIRECT]%s\n",httpheader.c_str());
			if (cb && !cb(cb_redirect, sts, sts, httpheader.c_str(), (unsigned int) httpheader.length(), add)) return 2;
			return http_download_callback(httpheader.c_str(), cb, add, postdata, sockettimeout);
		}
		if (cb) cb(cb_httpheader, sts, sts, httpheader.c_str(), (unsigned int) httpheader.length(), add);
		if (sts != 200) {
			closesocket(s);
			set_last_http_error(21);
			return sts;
		}
		pos = httpheader.find("Content-Length:");
		if (pos == string::npos)
			totallen = -1;
		else
			totallen = atoi(httpheader.substr(pos + 15).c_str());
		if (totallen==-1)
		{//http chunk mode
			int partlen=-1;
			passedlen=0;
			int nonstandard=0;
			while(1)
			{
				if (partlen==-1)
				{
					if (nonstandard) partlen=r;
					else {
						pbuf = (char *) bin_search(tmpbuf+passedlen, r,"\r\n",2);
						if (pbuf && string_ishex(tmpbuf+passedlen,((int)(pbuf-tmpbuf))-passedlen))
						{
							*pbuf='\0';
							pbuf+=2;
							partlen=strtol(tmpbuf+passedlen,0,16);
							if (partlen==0) break;
							tmplen=((int)(pbuf-tmpbuf))-passedlen;
							passedlen+=tmplen;
							r-=tmplen;
							partlen+=2;
						} else if (r>8 /*&& recvlen>0*/) {//fixme，这个地方是为了解决一个bug增加的，但是增加了对于没content-length，又不是chunked模式的，处理不了了
							crtdebug("[HTTP]warning: nonstandard http found!\n");
							nonstandard=1;
							partlen=r;
						} else {
							//当前buffer可能刚好是交界处，\r\n还没出现
							memmove(tmpbuf,tmpbuf+passedlen,r);
							passedlen=0;
							int rr=recv_timeout(s,tmpbuf+passedlen+r,4096-passedlen-r,0,sockettimeout);
							if (rr<=0)
							{
								closesocket(s);
								set_last_http_error(24);
								return (nonstandard==0)?3:sts;
							}
							r+=rr;
						}
					}
				}
				while(partlen>0 && r>0)
				{
					int readed=min(partlen,r);
					if (readed>0 && cb && !cb(cb_httpdata,totallen,recvlen,tmpbuf+passedlen,
						(!nonstandard && readed==partlen)?readed-2:readed,add))
					{
						closesocket(s);
						set_last_http_error(22);
						return (nonstandard==0)?2:sts;
					}
					partlen-=readed;
					r-=readed;
					passedlen+=readed;
					recvlen+=readed;
					if (r==0) passedlen=0;
					if (partlen==0) partlen=-1;
				}
				if (r==0)
				{
					crtdebugdot();
					int rr=recv_timeout(s,tmpbuf+passedlen+r,4096-passedlen-r,0,sockettimeout);
					if (rr<=0)
					{
						closesocket(s);
						set_last_http_error(23);
						return (nonstandard==0)?3:sts;
					}
					r+=rr;
				}
			}
		}
		else
		{
			if (r>0 && cb && !cb(cb_httpdata,totallen,recvlen,tmpbuf,r,add))
			{
				closesocket(s);
				return 2;
			}
			recvlen+=r;
			while(totallen>recvlen)
			{
				crtdebugdot();
				r=recv_timeout(s,tmpbuf,4096,0,sockettimeout);
				if (r<=0)
				{
					closesocket(s);
					set_last_http_error(23);
					return 3;
				}
				if (cb && !cb(cb_httpdata,totallen,recvlen,tmpbuf,r,add))
				{
					closesocket(s);
					return 2;
				}
				recvlen+=r;
			}
		}
		closesocket(s);
		return sts;
	}
	static bool str_download_cb(enum cb_type type, int totallen, int curlen, const char *buf, unsigned int buflen, const void *add) {
		if (type == cb_httpdata)
			((string *) add)->append(buf, buflen);
		return true;
	}
	//support http chunked mode,301 302,if psts specify,return str will not empty if error
	//sockettimeout >0 is timeout in ms, 0=unlimited, -1=determine with globalsetting setup with setup_http_global_timeout
	//retrytimes >0 is retry times, 0=no retry, -1=determine with globalsetting setup with setup_http_global_retrytimes
	//postdata can be have '\0' if start with "[crtbin]reallen\0"
	static string http_download_to_str(const char *url,const char *postdata = NULL,
		int *psts = NULL, int sockettimeout=-1, int retrytimes = -1) {
		string ret;
		int sts;
		if (sockettimeout==-1) sockettimeout=crtlib::instance()->http_global_timeout;
		if (retrytimes==-1) retrytimes=crtlib::instance()->http_global_retrytimes;
		while(retrytimes>=0){
			sts = http_download_callback(url, str_download_cb, &ret, postdata, sockettimeout);
			retrytimes--;
			if ((int)(sts / 100) == 2) break;
			ret.clear();
			crtdebug("[HTTP][RETRY]%s\n",url);
		}
		if (psts) *psts=sts;
		return ret;
	}
	static bool file_download_cb(enum cb_type type, int totallen, int curlen, const char *buf, unsigned int buflen, const void *add) {
		if (type == cb_httpdata)
			if (fwrite(buf,1,buflen,(FILE *) add)!=buflen) return false;
		return true;
	}
	//fixme,not have Content-Length, and not have Tranfer-Encoding: chunk, and have Connection: Close
	/*HTTP/1.0 200 OK
	Date: Sat, 09 Mar 2013 03:28:44 GMT
	Server: Apache
	Last-Modified: Sat, 09 Mar 2013 03:27:11 GMT
	ETag: "3be004d-2c745-4d77583efb920"
	Accept-Ranges: bytes
	Vary: Accept-Encoding,User-Agent
	Content-Encoding: gzip
	Content-Type: text/javascript
	X-Cache: MISS from testsdfsdfsdfzlsflsdfl.abc.com
	X-Cache-Lookup: MISS from testsdfsdfsdfzlsflsdfl.abc.com:80
	Via: 1.0 testsdfsdfsdfzlsflsdfl.abc.com (squid/3.0.STABLE20)
	Connection: close
	*/
	//sts=-1 file error
	//support http chunked mode,301 302,if psts specify,return str will not empty if error
	//sockettimeout >0 is timeout in ms, 0=unlimited, -1=determine with globalsetting setup with setup_http_global_timeout
	//retrytimes >0 is retry times, 0=no retry, -1=determine with globalsetting setup with setup_http_global_retrytimes
	static int http_download_to_file(const char *url,const char *filename,const char *postdata = NULL,
		int sockettimeout = -1, int retrytimes = -1) {
		int sts=-1;
		FILE *ret=fopen(filename, "wb");
		if (!ret) return sts;
		if (sockettimeout==-1) sockettimeout=crtlib::instance()->http_global_timeout;
		if (retrytimes==-1) retrytimes=crtlib::instance()->http_global_retrytimes;
		while(retrytimes>=0){
			sts = http_download_callback(url, file_download_cb, ret, postdata, sockettimeout);
			retrytimes--;
			if ((int)(sts / 100) == 2) break;
			fclose(ret);
			ret=fopen(filename, "wb");
			crtdebug("[HTTP][RETRY]%s\n",url);
		}
		fclose(ret);
		if (sts==2) sts=-1;
		return sts;
	}
};
