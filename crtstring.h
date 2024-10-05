#pragma once

#include "crtlib.h"
#include <algorithm>

#ifdef _WIN32
	#define CRTPATH_SEPERATOR '\\'
	#define CRTPATH_SEPERATOR_STRING "\\"
	#define CRTPATH_SEPERATOR_STRINGL L"\\"
	#define	CRTINT64_PRINT "%I64d"
	#define	CRTUINT64_PRINT "%I64u"
	#define	CRTINT64_PRINTL L"%I64d"
	#define	CRTUINT64_PRINTL L"%I64u"
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#else
	#define CRTPATH_SEPERATOR '/'
	#define CRTPATH_SEPERATOR_STRING "/"
	#define CRTPATH_SEPERATOR_STRINGL L"/"
	#define	CRTINT64_PRINT "%lld"
	#define	CRTUINT64_PRINT "%llu"
	#define	CRTINT64_PRINTL L"%I64d"
	#define	CRTUINT64_PRINTL L"%I64u"
#endif

/**
* @brief 字符串处理类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  //fixme：string装得像个栈变量，但却操作堆空间，而且线程不安全 http://blog.sina.com.cn/s/blog_61469a410100z9ou.html 该问题vc6下肯定存在 
  使用Sample：
</pre>*/

namespace crtfun {
	static bool string_startwith(const char *buf,const char *start) {
		while(*start!='\0') {
			if (*buf=='\0' || *buf!=*start) return false;
			buf++;start++;
		}
		return true;
	}
	static void string_tolower(string& str){transform(str.begin(), str.end(), str.begin(), ::tolower);}
	static void string_toupper(string& str){transform(str.begin(), str.end(), str.begin(), ::toupper);}
	static string& string_ltrim(string& str,const char *trim=" \t\r\n") {
		size_t pos = str.find_first_not_of(trim);
		if (pos != string::npos) str=str.substr(pos);
		else str.clear();
		return str;
	}
	static string& string_rtrim(string& str,const char *trim=" \t\r\n") {
		size_t pos = str.find_last_not_of(trim);
		if (pos != string::npos) str=str.substr(0, pos+1);
		else str.clear();
		return str;
	}
	static string string_trim_const(string str,const char *trim=" \t\r\n") {
		string_ltrim(str,trim);
		string_rtrim(str,trim);
		return str;
	}
	static string& string_trim(string& str,const char *trim=" \t\r\n") {
		string_ltrim(str,trim);
		string_rtrim(str,trim);
		return str;
	}
	static string string_right(const string& str,size_t count) {
		size_t pos=str.length();
		if (pos<=count) return str;
		return str.substr(pos-count,count);
	}
	static bool string_compare_nocase(const char *str1,const char *str2) {
		return strcasecmp(str1,str2)==0;
	}
	static bool string_compare_n_nocase(const char *str1,const char *str2,size_t len) {
		return strncasecmp(str1,str2,len)==0;
	}
	//查找开始为begin，结束为end中间的内容并返回，找不到返回""，remove为把找到的内容从原字符串中删除，needend表示必须匹配end才处理
	static string string_pickup(string& str, const char *begin, const char *end, bool remove=true, bool needend=true) {
        string ret;
        int slen = strlen(begin);
        size_t spos = str.find(begin);
        if (spos == string::npos) return ret;
        size_t epos = str.find(end, spos+slen);
        if (epos == string::npos) {
        	if (needend) return ret;
        	ret = str.substr(spos + slen);
        	str = str.substr(0, spos);
        } else {
        	ret = str.substr(spos + slen, epos - spos - slen);
        	str = str.substr(0, spos) + str.substr(epos + strlen(end));
        }
        return ret;
	}
	static string string_replace(const string& str, const char* src, const char* dest) {
	    string ret;
	    size_t pos_begin = 0;
	    size_t pos = str.find(src);
	    size_t len = strlen(src);
	    while (pos != string::npos)
	    {
	        ret.append(str.data() + pos_begin, pos - pos_begin);
	        ret += dest;
	        pos_begin = pos + len;
	        pos = str.find(src, pos_begin);
	    }
	    if (pos_begin < str.length())
	        ret.append(str.begin() + pos_begin, str.end());
	    return ret;
	}
	//如果start为空，直接找end，如果end为空，直接找start，如果replace=true，取出的value会从原字符串中删除
	static string string_pickup_value(string &str, const char *start, const char *end, bool replace) {
		size_t st=0;
		size_t slen=0;
		size_t et=str.length();
		size_t elen=0;
		string ret;
		if (start) {
			st=str.find(start);
			if (st==string::npos) return ret;
			slen=strlen(start);
		}
		if (end) {
			et=str.find(end,st+slen);
			if (et==string::npos) return ret;
			elen=strlen(end);
		}
		ret=str.substr(st+slen,et-(st+slen));
		if (replace) str=str.substr(0,st)+str.substr(et+elen);
		return ret;
	}
	static bool string_ishex(const char *buf, size_t buflen) {
		if (buflen==0) return false;
		for(size_t i=0;i<buflen;i++) {
			char ch=*(buf+i);
			if(!((ch>='0' && ch<='9') || (ch>='a' && ch<='f') || (ch>='A' && ch<='F'))) return false;
		}
		return true;
	}
	static unsigned int crts2h(const char *str) {return strtol(str,NULL,16);}
	static int64_t crts2i64(const char *str) {
#ifdef _WIN32
		return _atoi64(str);
#else
		return atoll(str);
#endif
	}
	//返回是实际使用长度
	static unsigned int crtstr2bin(const char *str, void *buf, unsigned int buflen){
		if (!str || !buf) return 0;
		string sstr=str;
		unsigned int len=(unsigned int)sstr.length();
		if (len%2!=0 || buflen<len/2) return 0;
		memset(buf,0,buflen);
		unsigned char *tmp=(unsigned char *)buf;
		unsigned int i;
		for(i=0;i<len;i+=2)
			*(tmp+i/2)=(unsigned char)strtol(sstr.substr(i,2).c_str(),NULL,16);
		return i/2;
	}
	static string crtbin2str(const void *buf, size_t buflen,const char *push_backprefix=""){
		string str;
		str.reserve(buflen*2 + 1);
		char tmpbuf[4];
		const unsigned char*tmp=(const unsigned char *)buf;
		for(size_t i=0;i<buflen;i++) {
			str+=push_backprefix;
			snprintf(tmpbuf,4,"%02x",*(tmp+i));
			str+=tmpbuf;
		}
		return str;
	}
	static string string_format(const char *format, ...)
	{
		char buf[65535];
		va_list a;
		va_start(a,format);
		vsnprintf(buf,65535,format,a);
		va_end(a);
		return buf;
	}
	static string get_file_path(const string& filename){
		size_t pos=filename.rfind(CRTPATH_SEPERATOR_STRING);
		if (pos!=string::npos) return filename.substr(0,pos);
		return filename;
	}
	static string get_file_name(const string& filename){
		size_t pos=filename.rfind(CRTPATH_SEPERATOR_STRING);
		if (pos!=string::npos) return filename.substr(pos+1);
		return filename;
	}
	static string get_file_ext(const string& filename){
		size_t pos=filename.rfind(".");
		if (pos!=string::npos) return filename.substr(pos+1);
		return "";
	}
	static string replace_file_ext(const char* file,const char* ext=""){
		string filename=file;
		string fn=get_file_name(filename);
		string fext=get_file_ext(fn);
		if (!fext.empty())
			filename=filename.substr(0,filename.length()-fext.length()-1);
		if (strlen(ext)>0) {
			filename+=".";
			filename+=ext;
		}
		return filename;
	}
	static string crturl_saveprint(string url) {return string_replace(url,"%","%%");}
	static string crturl_encode(const char *str,bool bupper=false){
		string ret;
		char *buf=new char[(strlen(str)+1)*3];
		const char *perfix=bupper?"%%%02X":"%%%02x";
		if (buf)
		{
			char *tmpbuf=buf;
			const unsigned char *tmptmp=(const unsigned char *)str;
			while(*tmptmp!='\0')
			{
				if (isalnum(*tmptmp) || strchr("-_.!~*'()", *tmptmp))
				{
					*tmpbuf=*tmptmp;
					tmpbuf++;
					tmptmp++;
				}
				else
				{
					snprintf(tmpbuf,4,perfix,*tmptmp);
					tmpbuf+=3;
					tmptmp++;
				}
			}
			*tmpbuf='\0';
			ret=buf;
			delete [] buf;
		}
		return ret;
	}
	static string crturl_decode(const char *str)
	{
		string ret;
		char *buf=new char[strlen(str)+1];
		if (buf)
		{
			char *tmpbuf=buf;
			const char *tmptmp=str;
			while(*tmptmp!='\0')
			{
				if (*tmptmp=='%')
				{
					if (*(tmptmp+1)=='%') {
						*tmpbuf='%';
						tmpbuf++;
						tmptmp+=2;
					} else {
						int hi = tolower(*(tmptmp+1));
						int lo = tolower(*(tmptmp+2));
						tmptmp+=3;
						if (hi>='a' && hi<='f') hi=hi-'a'+10;
						else hi=hi-'0';
						if (lo>='a' && lo<='f') lo=lo-'a'+10;
						else lo=lo-'0';
						*tmpbuf++=(char)(uint8_t)((hi<<4) | lo);
					}
				}
				else
				{
					*tmpbuf=*tmptmp;
					tmpbuf++;
					tmptmp++;
				}
			}
			*tmpbuf='\0';
			ret=buf;
			delete [] buf;
		}
		return ret;
	}
	static string get_nice_int(unsigned int value,const char* postfix="B/s")
	{
		double dv;
		const char *d;
		if (value>=1000000000)
		{
			d="G";
			dv=value/1000000000;
		} else if (value>=1000000) {
			d="M";
			dv=value/1000000;
		} else if (value>=1000) {
			d="K";
			dv=value/1000;
		} else {
			d="";
			dv=value;
		}
		const char *keybuf;
		if (dv>=10) keybuf="%.f%s";
		else keybuf="%.1f%s";
		char strbuf[20];
		snprintf(strbuf,20,keybuf,dv,d);
		string ret=strbuf;
		ret+=postfix;
		return ret;
	}
	static string crtl2s(unsigned long value) {
		char buf[64];
		snprintf(buf,64,"%lu",value);
		return buf;
	}
	static string get_http_param(string url,string key)
	{
		string value;
		int push_backtag=2;
		size_t pos=url.find("?"+key+"=",0);
		if (pos==string::npos)
			pos=url.find("&"+key+"=",0);
		if (pos==string::npos) {
			pos=url.find(key+"=",0);
			if (pos==0) push_backtag=1;
			else pos=string::npos;
		}
		if (pos!=string::npos) {
			pos+=key.length()+push_backtag;
			size_t ret=url.find("&",pos);
			if (ret!=string::npos)
				value=url.substr(pos,ret-pos);
			else
				value=url.substr(pos);
		}
		return value;
	}
	class crturlparser{
	public:
		crturlparser(){port=0;request="/";path="/";}
		void parse(string url){
			orgurl=url;
			size_t pos=url.find("://");
			if (pos==string::npos)
				protocol.clear();
			else {
				protocol=url.substr(0,pos);
				url=url.substr(pos+3);
			}
			pos=url.find('#');
			if (pos==string::npos)
				anchor.clear();
			else {
				anchor=url.substr(pos+1);
				url=url.substr(0,pos);
			}
			pos=url.find('?');
			if (pos==string::npos)
				param.clear();
			else {
				param=url.substr(pos+1);
				url=url.substr(0,pos);
			}
			pos=url.find('/');
			if (pos==string::npos) {
				request="/";
				path="/";
				file.clear();
				filename.clear();
				fileext.clear();
			} else {
				request=url.substr(pos);
				url=url.substr(0,pos);
				pos=request.rfind('/');
				if (pos==0) {
					path="/";
					file=request.substr(1);
				} else {
					path=request.substr(0,pos);
					file=request.substr(pos+1);
				}
				pos=file.rfind('.');
				if (pos==string::npos) {
					filename=file;
					fileext.clear();
				} else {
					filename=file.substr(0,pos);
					fileext=file.substr(pos+1);
				}
			}
			pos=url.rfind('@');
			if (pos==string::npos) {
				username.clear();
				password.clear();
			} else {
				username=url.substr(0,pos);
				url=url.substr(pos+1);
				pos=username.find(':');
				if (pos==string::npos)
					password.clear();
				else {
					password=username.substr(pos+1);
					username=username.substr(0,pos);
				}
			}
			pos=url.find(':');
			if (pos==string::npos) {
				port=0;
				host=url;
			} else {
				port=atoi(url.substr(pos+1).c_str());
				host=url.substr(0,pos);
			}
		}
		string tostring(){
			return "orgurl: " + orgurl + ", protocol: " + protocol + ", username:" + username + ", password:" + password
				+ ", host:" + host + ", port:" + crtl2s(port) + ", request:" + request
				+ ", path:" + path + ", file:" + file + ", filename:" + filename + ", fileext:" + fileext
				+ ", param:" + param + ", anchor:" + anchor;
		}
		string orgurl;
		string protocol;
		string username;
		string password;
		string host;
		unsigned short port;
		string request;
		string path;
		string file;
		string filename;
		string fileext;
		string param;
		string anchor;/* 锚点 */
	};
	//use crthttp crturl_saveprint instand
	//static string save_print_url(string url) {return string_replace(url,"%","\xa3\xa5");}
	/*static string save_print_url(string url,string replace="\xa3\xa5")//is "％" char
	{
		string output;
		size_t lastpos=0;
		while(1)
		{
			size_t pos=url.find("%",lastpos);
			if (pos==string::npos)
			{
				output+=url.substr(lastpos);
				break;
			}
			output+=url.substr(lastpos,pos-lastpos);
			output+=replace;
			lastpos=pos+1;
		}
		return output;
	}*/
	class crtstringtoken{
		public:
			crtstringtoken(){}
			crtstringtoken(string data,string tk){init(data,tk);}
			virtual ~crtstringtoken(){}
			void init(string data,string tk){m_remain=data;m_tk=tk;}
			bool ismore(){return !m_remain.empty();}
			string nexttoken(){
				string ret;
				size_t pos=m_remain.find(m_tk);
				if (pos!=string::npos)
				{
					ret=m_remain.substr(0,pos);
					m_remain=m_remain.substr(pos+m_tk.length());
				}
				else
				{
					ret=m_remain;
					m_remain.clear();
				}
				return ret;
			}
		protected:
			string m_remain;
			string m_tk;
	};
	class crtspeedcounting {
		public:
			crtspeedcounting(){reset();}
			virtual ~crtspeedcounting(){}
			void reset(){
				m_firstts = 0;
				m_lastts = 0;
				m_lastspeed = 0;
				m_lastspeedval = 0;
			}
			void push_backdata(int len){
				unsigned int now = (unsigned int) time(NULL);
				if (now - m_lastts >= 10) {
					memset(m_counting, 0, sizeof(m_counting));
					m_lastts = now;
				} else {
					while (m_lastts + 1 <= now) {
						m_counting[(m_lastts + 1) % 10] = 0;
						m_lastts++;
					}
				}
				if (!m_firstts)
					m_firstts = now - 1;
				m_counting[now % 10] += len;
			}
			unsigned int getspeed() {
				unsigned int now = (unsigned int) time(NULL);
				if (now != m_lastspeed) {
					push_backdata(0);
					m_lastspeed = now;
					unsigned int sum = 0, i;
					for (i = 0; i < 10; i++)
						sum += m_counting[i];
					i = min((unsigned int) 10, now - m_firstts);
					m_lastspeedval = sum / i;
				}
				return m_lastspeedval;
			}
		protected:
			unsigned int m_lastspeed;
			unsigned int m_lastspeedval;
			unsigned int m_firstts;
			unsigned int m_lastts;
			unsigned int m_counting[10];
	};
	static const void *bin_search(const void *pbuf, unsigned int len, const void *pkey, unsigned int keylen) {
		const unsigned char *buf = (const unsigned char *) pbuf;
		const unsigned char *key = (const unsigned char *) pkey;
		unsigned int i;
		if (keylen > len) return NULL;
		for (i = 0; i <= len - keylen; i++) {
			if (buf[i] == key[0]) {
				if (memcmp(buf + i, key, keylen) == 0)
					return (const void *) (buf + i);
			}
		}
		return NULL;
	}
	static size_t string2size_t(const char *buf) {return atol(buf);}
	static ssize_t string2ssize_t(const char *buf) {return atol(buf);}
	//因为size_t不同平台位数不一样(32位32，64位64)，且%zd windows不支持，因此应把size_t转换为long再进行打印
	static string size_t2string(size_t size){return string_format("%lu",(unsigned long)size);}
	static string ssize_t2string(size_t size){return string_format("%ld",(long)size);}
	class crthttpheaderparser
	{
	public:
		crthttpheaderparser(){m_ver=11;}
		virtual ~crthttpheaderparser(){}
		bool parse(string headers)
		{
			m_ver=0;
			m_headers.clear();
			m_fh.clear();
			crtstringtoken token(headers,"\r\n");
			string str;
			string header=token.nexttoken();
			string_trim(header);
			if (header.empty()) return false;
			m_headers.push_back(header);
			while(1)
			{
				str=token.nexttoken();
				string_trim(str);
				if (str.empty()) break;
				m_headers.push_back(str);
			}
			token.init(header," ");
			while(1)
			{
				str=token.nexttoken();
				string_trim(str);
				if (str.empty()) break;
				m_fh.push_back(str);
			}
			size_t pos=header.rfind('/');
			if (pos!=string::npos)
				m_ver=(int)((atof(header.substr(pos+1).c_str()))*10.0);
			return true;
		}
	
		/**
		* @brief 获取指定头信息中的内容
		* @param tagname		键名
		* @param idx			第n次出现的键值，以0开始
		* @return 内容字符串
		*/
		string gettag(const char* tagname,int idx=0)
		{
			string str;
			string key,cur;
			for(size_t i=0;i<m_headers.size();i++)
			{
				cur=m_headers[i];
				size_t pos=cur.find(":");
				if (pos!=string::npos) {
					key=cur.substr(0,pos);
					string_trim(key);
					if (string_compare_nocase(key.c_str(),tagname)) {
						if (idx<=0) {
							str=cur.substr(pos+1);
							string_trim(str);
							break;
						}
						idx--;
					}
				}
			}
			return str;
		}
		string get_all_header_with_key(const char *tagname) {
			string str;
			string key,cur;
			for(size_t i=0;i<m_headers.size();i++)
			{
				cur=m_headers[i];
				size_t pos=cur.find(":");
				if (pos!=string::npos) {
					key=cur.substr(0,pos);
					string_trim(key);
					if (string_compare_nocase(key.c_str(),tagname)) {
						if (!str.empty()) str+="\r\n";
						str+=cur;
					}
				}
			}
			return str;
		}
		/**
		* @brief 获取头信息条数
		* @return 头信息条数
		*/
		inline size_t headersize(){return m_headers.size();}
	
		/**
		* @brief 获取指定序号的头信息
		* @param idx		第n行头信息
		* @param key		返回键名
		* @return 返回内容
		*/
		string getheader(size_t idx,string *key=NULL)
		{
			string ret;
			if (key) key->clear();
			if (idx<m_headers.size())//idx>=0 always true
			{
				ret=m_headers[idx];
				size_t pos=ret.find(":");
				if (pos!=string::npos)
				{
					if (key) {
						string header=ret.substr(pos);
						string_trim(header);
						*key=header;
					}
					string header=ret.substr(pos+1);
					ret=string_trim(header);
				}
			}
			return ret;
		}
		/**
		* @brief 获取Cache信息
		* @param gmttistr	最后修改GMT时间，使用gmttime2timet转换time_t
		* @param filelen	文件长度，取不到为0
		* @return 是否成功获取
		*/
		bool get_if_modified_since(string &gmttistr,size_t &filelen)
		{
			//If-Modified-Since: Thu, 24 Jul 2008 05:15:02 GMT; length=62856
			string tag=gettag("If-Modified-Since");
			if (tag.empty()) return false;
			crtstringtoken t(tag,";");
			gmttistr=t.nexttoken();
			t.init(t.nexttoken(),"=");
			t.nexttoken();
			string len=t.nexttoken();
			if (gmttistr.empty()) return false;
			//check file length if have
			if (!len.empty()) filelen=string2size_t(len.c_str());
			else filelen=0;
			return true;
		}
		/**
		* @brief 获取首行的头信息
		* @param idx		第n个空格后的信息
		* @return 返回内容
		*/
		inline string get_first_header(int idx){return m_fh.size()>(size_t)idx?m_fh[idx]:"";}
		inline int get_http_version(){return m_ver;}
		inline string get_host(){return gettag("Host");}
		inline string get_accept(){return gettag("Accept");}
		inline bool is_accept_encoding_gzip(){return (gettag("Accept-Encoding").find("gzip")!=string::npos);}
		inline string get_user_agent(){return gettag("User-Agent");}
		inline string get_connection_type(){return gettag("Connection");}
		inline string get_content_length(){return gettag("Content-Length");}
		inline string get_content_type(){return gettag("Content-Type");}
		inline string get_range(){return gettag("Range");}
		inline string get_cookie(){return gettag("Cookie");}
		void get_cookie_to_array(vector<string> &ar)
		{
			string cookie=gettag("Cookie");
			crtstringtoken t(cookie,";");
			string tmp;
			while(t.ismore()) {
				tmp=t.nexttoken();
				string_trim(tmp);
				ar.push_back(tmp);
			}
		}
		string get_cookie_by_key(const char *key,const char *defvalue="") {
			string cookie=gettag("Cookie");
			crtstringtoken t(cookie,";");
			crtstringtoken t2;
			string temp;
			while(t.ismore()) {
				t2.init(string_trim_const(t.nexttoken()),"=");
				temp=t2.nexttoken();
				string_trim(temp);
				if (string_compare_nocase(temp.c_str(),key)) return string_trim_const(t2.nexttoken());
			}
			return defvalue;
		}
		bool get_range(string &unit,size_t &startpos,size_t &endpos)
		{
			string range=get_range();
			if (range.empty())
			{
				//default value
				startpos=-1;
				endpos=-1;
				return true;
			}
			size_t off=range.find("=",0);
			if (off==-1) return false;
			size_t off2=range.find("-",off);
			if (off2==-1) return false;
			unit=string_trim_const(range.substr(0,off));
			string start=string_trim_const(range.substr(0,off2).substr(off+1));
			if (start.empty())
				startpos=-1;
			else
				startpos=string2size_t(start.c_str());
			start=string_trim_const(range.substr(off2+1));
			if (start.empty())
				endpos=-1;
			else
				endpos=string2size_t(start.c_str())+1;//index will +1
			return true;
		}
		inline string get_set_cookie(int idx=0){return gettag("Set-Cookie",idx);}
		//use crtcookie.getcookiefromhttpheader(get_set_cookies().c_str()) for parse cookie
		inline string get_set_cookies() { return get_all_header_with_key("Set-Cookie");}
	private:
		int m_ver;
		vector<string> m_headers;
		vector<string> m_fh;
	};

	/**
	* @brief http响应头生成类
	* @author Barry(barrytan@21cn.com,QQ:20962493)
	* @2007-09-09 新建类
	*/
	/**<pre>
	用法：
	</pre>*/
	class crthttpheaderresponsemaker
	{
	public:
		crthttpheaderresponsemaker(){clear();}
		virtual ~crthttpheaderresponsemaker(){}	
		void clear()
		{
			m_totallen=0;
			m_startpos=-1;
			m_endpos=-1;
			m_rangeunit="bytes";
			m_code=200;
			m_codetext.clear();
			m_httpver=11;
			m_resptype="application/octet-stream";
			m_servername="crthttpserver";
			m_lastmodify.clear();
			m_time.clear();
			m_addheaders.clear();
			m_addendheaders.clear();
			m_connectiontype="Keep-Alive";
			m_contentencoding.clear();
		}
		/**
		* @brief 增加返回的cookie
		* @param keyvalue		键名和键值，如：key=value
		* @param expirestime	超时时间，空为没有超时时间，请使用timet2gmttime生成时间
		* @param spath			cookie有效范围，默认为路径/
		* @param sdomain		cookie主机范围，如：.tansoft.net，默认不给出
		*/
		void addcookie(const char* keyvalue,const char* expirestime="",const char* spath="",const char* sdomain="")
		{
			string skeyvalue=keyvalue;
			if (!spath || spath[0]=='\0') spath="/";
			if (expirestime && expirestime[0]!='\0') {
				skeyvalue+="; expires=";
				skeyvalue+=expirestime;
			}
			skeyvalue+="; path="; skeyvalue+=spath;
			if (sdomain && sdomain[0]!='\0') {
				skeyvalue+="; domain=";
				skeyvalue+=sdomain;
			}
			addheaders("Set-Cookie",skeyvalue.c_str(),false);
		}
		inline void addetag(const char* tag) {addheaders("ETag",tag,true);}
		//no-cache
		inline void addcachecontrol(const char* cachectrl="max-age=315360000") {addheaders("Cache-Control",cachectrl,false);}
		//请使用timet2gmttime生成时间 
		inline void addexpires(const char* expirestime) {addheaders("Expires",expirestime,false);}
		/**
		* @brief 增加返回的头信息
		* @param key		键名
		* @param value		键值
		* @param bhead		是否增加在前面
		*/
		void addheaders(const char* key,const char* value,bool bhead=true) {
			string *str=&m_addheaders;
			if (!bhead) str=&m_addendheaders;
			if (!str->empty()) *str+="\r\n";
			*str+=key; *str+=": "; *str+=value;
		}
		/**
		* @brief 设置返回数据范围
		* @param totallen		总长度
		* @param startpos		起始位置，-1为不指定，-2为返回*，用于http 416情况，这时endpos为文件正常的长度
		* @param endpos			结束位置，-1为不指定，-2为返回*，用于http 416情况
		* @param rangeunit		长度单位
		*/
		inline void setrange(int64_t totallen,int64_t startpos=-1,int64_t endpos=-1,const char* rangeunit="bytes") {
			m_totallen=totallen;
			m_startpos=startpos;
			m_endpos=endpos;
			m_rangeunit=rangeunit;
		}
		/**
		* @brief 设置返回结果
		* @param code			结果代码
		* @param codetext		结果文字
		* @param httpver		协议版本
		*/
		inline void setresult(int code=200,const char* codetext="",int httpver=11) {
			m_code=code;
			m_codetext=codetext;
			m_httpver=httpver;
		}
		/**
		* @brief 设置连接类型
		* @param connectiontype	连接类型
		*/
		inline void setconnectiontype(const char* connectiontype="Close") {m_connectiontype=connectiontype;}
		/**
		* @brief 设置返回文件格式
		* @param resptype		文件格式
		*/
		inline void setresponsetype(const char* resptype) {m_resptype=resptype;}
		/**
		* @brief 根据后缀名设置返回文件格式
		* @param ext			文件后缀名
		* @param deftype		默认返回格式
		*/
		void setresponsetypebyext(const char* ext,const char* deftype="application/octet-stream") {
			char buf[][2][40]={
				{"doc","application/msword"},
				{"ogg","application/ogg"},
				{"pdf","application/pdf"},
				{"xls","application/vnd.ms-excel"},
				{"ppt","application/vnd.ms-powerpoint"},
				{"js","application/x-javascript"},
				{"swf","application/x-shockwave-flash"},
				{"tar","application/x-tar"},
				{"zip","application/zip"},
				{"mid","audio/midi"},
				{"mp3","audio/mpeg"},
				{"aif","audio/x-aiff"},
				{"m3u","audio/x-mpegurl"},
				{"ram","audio/x-pn-realaudio"},
				{"ra","audio/x-pn-realaudio"},
				{"rmf","audio/x-pn-realaudio"},
				{"rm","application/vnd.rn-realmedia"},
				{"rmvb","application/vnd.rn-realmedia"},
				{"wav","audio/x-wav"},
				{"bmp","image/bmp"},
				{"gif","image/gif"},
				{"jpg","image/jpeg"},
				{"png","image/png"},
				{"tif","image/tiff"},
				{"ico","image/x-icon"},
				{"css","text/css"},
				{"html","text/html"},
				{"shtml","text/html"},
				{"htm","text/html"},
				{"tsp","text/html"},
				{"xml","text/xml"},
				{"txt","text/plain"},
				{"rtf","text/rtf"},
				{"mpeg","video/mpeg"},
				{"mpg","video/mpeg"},
				{"mpe","video/mpeg"},
				{"qt","video/quicktime"},
				{"mov","video/quicktime"},
				{"moov","video/quicktime"},
				{"m4u","video/vnd.mpegurl"},
				{"avi","video/x-msvideo"},
				{"asx","video/x-ms-asf"},
				{"asf","video/x-ms-asf"},
				{"flv","video/x-flv"},
				{"pls","audio/x-scpls"},
				{"m3u","audio/x-mpegurl"},
				{"qtl","application/x-quicktimeplayer"},
				{"mpcpl","application/x-mpc-playlist"},
				{"\0","\0"}
			};
			int i=0;
			while(buf[i][0][0]!='\0')
			{
				if (string_compare_nocase(ext,buf[i][0])) {
					m_resptype=buf[i][1];
					return;
				}
				i++;
			}
			m_resptype=deftype;
		}
		/**
		* @brief 设置服务器名称
		* @param servername		服务器名称
		*/
		inline void setserver(const char* servername){m_servername=servername;}
		/**
		* @brief 设置修改时间
		* @param lastmodify		修改时间
		*/
		inline void setmodifytime(const char* lastmodify){m_lastmodify=lastmodify;}
		/**
		* @brief 设置是否返回时间
		* @param busetime		是否返回时间
		*/
		inline void settime(const char* time){m_time=time;}
		/**
		* @brief 设置返回编码格式
		* @param encoding		为空，默认为deflate，可设置为gzip
		*/
		inline void setcontentencoding(const char* encoding="gzip"){m_contentencoding=encoding;}
		/**
		* @brief 生成头信息
		* @return 返回生成资料
		*/
		string makeheaders() {
			string ret;
			ret=string_format("HTTP/%0.1f %d %s\r\n",m_httpver/10.0,m_code,m_codetext.empty()?getcodetext(m_code).c_str():m_codetext.c_str());
			if (!m_addheaders.empty()) ret+=m_addheaders+"\r\n";
			ret+="Content-Type: "+m_resptype+"\r\n";
			//firefox 响应301时，如果没有Content-Length会一直等待
			//if (m_totallen>0)
			{
				if (m_startpos==-2) {
					//http 416
					ret+=string_format("Content-Length: "CRTINT64_PRINT"\r\n",m_totallen);//这时为错误信息的长度
					ret+=string_format("Content-Range: %s */"CRTINT64_PRINT"\r\n",m_rangeunit.c_str(),m_endpos);//这时e为文件长度
					ret+="Accept-Ranges: "+m_rangeunit+"\r\n";
				} else if (m_startpos!=-1 || m_endpos!=-1) {
					int64_t s,e;
					if (m_startpos==-1) s=0;
					else s=m_startpos;
					if (m_endpos==-1) e=m_totallen;
					else e=m_endpos;
					ret+=string_format("Content-Length: "CRTINT64_PRINT"\r\n",e-s);
					ret+=string_format("Content-Range: %s "CRTINT64_PRINT"-"CRTINT64_PRINT"/"CRTINT64_PRINT"\r\n",
						m_rangeunit.c_str(),s,e-1,m_totallen);//index will -1
					ret+="Accept-Ranges: "+m_rangeunit+"\r\n";
				}
				else
					ret+=string_format("Content-Length: "CRTINT64_PRINT"\r\n",m_totallen);
			}
			if (!m_contentencoding.empty()) ret+="Content-Encoding: "+m_contentencoding+"\r\n";
			ret+="Server: "+m_servername+"\r\n";
			if (!m_lastmodify.empty()) ret+="Last-Modified: "+m_lastmodify+"\r\n";
			ret+="Connection: "+m_connectiontype+"\r\n";
			if (!m_time.empty()) ret+="Date: "+m_time+"\r\n";
			if (!m_addendheaders.empty())
				ret+=m_addendheaders+"\r\n";
			ret+="\r\n";
			return ret;
		}
		/**
		* @brief 获取对应状态的返回文字
		* @param code	代码
		* @return 返回生成资料
		*/
		string getcodetext(int code) {
			string ret;
			switch(code) {
				case 100:ret="Continue";break;
				case 101:ret="Switching Protocols";break;
				case 200:ret="OK";break;
				case 201:ret="Created";break;
				case 202:ret="Accepted";break;
				case 203:ret="Non-Authoritative Information";break;
				case 204:ret="No Content";break;
				case 205:ret="Reset Content";break;
				case 206:ret="Partial Content";break;
				case 300:ret="Multiple Choices";break;
				case 301:ret="Moved Permanently";break;
				case 302:ret="Moved Temporarily";break;
				case 303:ret="See Other";break;
				case 304:ret="Not Modified";break;
				case 305:ret="Use Proxy";break;
				case 307:ret="Temporary Redirect";break;
				case 400:ret="Bad Request";break;
				case 401:ret="Unauthorized";break;
				case 402:ret="Payment Required";break;
				case 403:ret="Forbidden";break;
				case 404:ret="Not Found";break;
				case 405:ret="Method Not Allowed";break;
				case 406:ret="Not Acceptable";break;
				case 407:ret="Proxy Authentication Required";break;
				case 408:ret="Request Timeout";break;
				case 409:ret="Conflict";break;
				case 410:ret="Gone";break;
				case 411:ret="Length Required";break;
				case 412:ret="Precondition Failed";break;
				case 413:ret="Request Entity Too Large";break;
				case 414:ret="Request-URI Too Long";break;
				case 415:ret="Unsupported Media Type";break;
				case 416:ret="Requested Range Not Satisfiable";break;
				case 417:ret="Expectation Failed";break;
				case 500:ret="Internal Server Error";break;
				case 501:ret="Not Implemented";break;
				case 502:ret="Bad Gateway";break;
				case 503:ret="Service Unavailable";break;
				case 504:ret="Gateway Timeout";break;
				case 505:ret="HTTP Version Not Supported";break;
				default:ret="Unknown Response";
			}
			return ret;
		}
		inline int getresultcode() {return m_code;}
		inline string getresulttext() {return m_codetext;}
	private:
		int64_t m_totallen;		//使用int64_t支持大于4g的文件，size_t在32位系统是32位的，64位系统64位，不够大
		int64_t m_startpos;
		int64_t m_endpos;
		string m_rangeunit;
		int m_code;
		string m_codetext;
		int m_httpver;
		string m_resptype;
		string m_servername;
		string m_lastmodify;
		string m_connectiontype;
		string m_addheaders;
		string m_addendheaders;
		string m_contentencoding;
		string m_time;
	};

	/**
	* @brief http请求构造类
	* @author barry(barrytan@21cn.com,qq:20962493)
	* @2011-01-29 新建类
	*/
	/**<pre>
	用法：
	</pre>*/
	class crthttpheaderrequestmaker
	{
	public:
		crthttpheaderrequestmaker(){clear();}
		virtual ~crthttpheaderrequestmaker(){}
		void clear() {
			m_smethod="GET";
			m_sfile="/";
			m_nhttpver=11;
			m_saccept="image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, */*";
			m_sreferer.clear();
			m_sacceptlanguage="zh-cn";
			m_sacceptencoding="gzip, deflate";
			m_suseragent="tansoft tiniclient";
			m_shost.clear();
			m_srange.clear();
			m_sconnection="Keep-Alive";
			m_scookie.clear();
			m_saddheaders.clear();
			m_saddendheaders.clear();
		}
	
		string makeheaders()
		{
			string ret;
			ret=m_smethod+" ";
			ret+=m_sfile;
			ret+=string_format(" HTTP/%0.1f\r\n",m_nhttpver);
			if (!m_saddheaders.empty()) ret+=m_saddheaders+"\r\n";
			if (!m_saccept.empty()) ret+="Accept: "+m_saccept+"\r\n";
			if (!m_sreferer.empty()) ret+="Referer: "+m_sreferer+"\r\n";
			if (!m_sacceptlanguage.empty()) ret+="Accept-Language: "+m_sacceptlanguage+"\r\n";
			if (!m_sacceptencoding.empty()) ret+="Accept-Encoding: "+m_sacceptencoding+"\r\n";
			if (!m_suseragent.empty()) ret+="User-Agent: "+m_suseragent+"\r\n";
			if (!m_shost.empty()) ret+="Host: "+m_shost+"\r\n";
			if (!m_srange.empty()) ret+="Range: "+m_srange+"\r\n";
			if (!m_sconnection.empty()) ret+="Connection: "+m_sconnection+"\r\n";
			if (!m_scookie.empty()) ret+="Cookie: "+m_scookie+"\r\n";
			if (!m_saddendheaders.empty()) ret+=m_saddendheaders+"\r\n";
			ret+="\r\n";
			return ret;
		}
		string buildrequest(const char* url,string *phost=NULL,unsigned short *pport=NULL)
		{
			crturlparser e;
			e.parse(url);
			if (phost) *phost=e.host;
			if (pport) *pport=e.port;
			return buildrequest(e.request.c_str(),e.host.c_str(),e.port);
		}
		string buildrequest(const char* requestfile,const char* host,unsigned short port=80)
		{
			setrequestfile(requestfile);
			string shost=host;
			if (port!=80) shost+=string_format(":%u",port);
			sethost(shost.c_str());
			return makeheaders();
		}
		void emptyaddheaders(){m_saddheaders.clear();m_saddendheaders.clear();}
		void addheaders(const char* header,bool bend)
		{
			string sheader=header;
			string_trim(sheader);
			if (sheader.empty()) return;
			string *ret;
			if (bend) ret=&m_saddendheaders;
			else ret=&m_saddheaders;
			if (!ret->empty()) (*ret)+="\r\n";
			(*ret)+=sheader;
		}
		inline void setmethod(const char *action="GET"){m_smethod=action;}
		void setrequestfile(const char* file="/"){m_sfile=file;}
		void sethttpversion(int ver=11){m_nhttpver=ver;}
		void setaccept(const char* accept="*/*"){m_saccept=accept;}
		void setreferer(const char* referer=""){m_sreferer=referer;}
		void setacceptlanguage(const char* acceptlang="zh-cn"){m_sacceptlanguage=acceptlang;}
		void setacceptencoding(const char* acceptenc="deflate"){m_sacceptencoding=acceptenc;}
		void setuseragent(const char* useragent=""){m_suseragent=useragent;}
		void sethost(const char* host=""){m_shost=host;}
		void setrange(const char* range=""){m_srange=range;}
		void setconnection(const char* connection="Keep-Alive"){m_sconnection=connection;}
		void setcookie(const char* cookie=""){m_scookie=cookie;}
	private:
		string m_smethod;
		string m_sfile;
		int m_nhttpver;
		string m_saccept;
		string m_sreferer;
		string m_sacceptlanguage;
		string m_sacceptencoding;
		string m_suseragent;
		string m_shost;
		string m_srange;
		string m_sconnection;
		string m_scookie;
		string m_saddheaders;
		string m_saddendheaders;
	};
	class crtstringparser {
	public:
		void clear(){values.clear();}
		//a=b&b=c&a=c,urlencoded
		void parse_with_http_get_params(const char* orgdata){
			crtstringtoken tk(orgdata,"&"),tk2;
			string key,value;
			while(tk.ismore()) {
				tk2.init(tk.nexttoken(),"=");
				while(tk2.ismore()) {
					key=crturl_decode(tk2.nexttoken().c_str());
					value=crturl_decode(tk2.nexttoken().c_str());
					values.insert(multimap<string,string>::value_type(key,value));
				}
			}
		}
		//username="d,\"=", algorithm=MD5, a=
		void parse_with_key_quote_values(const char* orgdata) {
			string remain=orgdata;
			string key,value;
			size_t pos;
			char quote=0;//0 for find key, char !=0 for find the quote
			while(!remain.empty()){
				pos=remain.find('=');
				if (pos==string::npos) break;
				key=string_trim_const(remain.substr(0,pos));
				pos=remain.find_first_not_of(" \t=",pos);
				if (pos!=string::npos) remain=remain.substr(pos);
				quote=remain.at(0);
				if (quote=='\'' || quote=='"') {
					//quote mode, find the end
					pos=1;
					while(true) {
						pos=remain.find(quote,pos);
						if (pos==string::npos) {
							//error
							remain.clear();
							break;
						}
						if (remain.at(pos-1)!='\\') {
							value=remain.substr(1,pos-1);
							pos=remain.find_first_not_of(" \t,",pos+1);
							if (pos!=string::npos) remain=remain.substr(pos);
							else remain.clear();
							value=string_replace(string_replace(value,"\\\"","\""),"\\\\","\\");
							values.insert(multimap<string,string>::value_type(key,value));
							break;
						}
						pos++;
					}
				} else {
					quote=0;
					//not quote here ,just find ,
					pos=remain.find(',');
					if (pos!=string::npos) {
						value=string_trim_const(remain.substr(0,pos));
						remain=remain.substr(pos+1);
					} else {
						value=string_trim_const(remain);
						remain.clear();
					}
					values.insert(multimap<string,string>::value_type(key,value));
				}
			}
		}
		void addvalue(string key, string value) {
			values.insert(multimap<string,string>::value_type(key,value));
		}
		string getvalue(const string &key, int idx=0) {
			//first is the first in key, second is the first in next key, so while it!=second
			pair< multimap<string,string>::iterator, multimap<string,string>::iterator> p = values.equal_range(key);
			multimap<string,string>::iterator it=p.first;
			if (it==values.end()) return "";
			while(idx!=0 && it!=p.second) {idx--;it++;}
			if (idx==0 && it!=p.second) return it->second;
			return "";
		}
		bool getallvalue(const string &key, list<string> &allvalue) {
			pair< multimap<string,string>::iterator, multimap<string,string>::iterator> p = values.equal_range(key);
			multimap<string,string>::iterator it=p.first;
			if (it==values.end()) return false;
			while(it!=p.second) {
				allvalue.push_back(it->second);
				it++;
			}
			return true;
		}
		void removevalue(const string &key, int idx=0) {
			pair< multimap<string,string>::iterator, multimap<string,string>::iterator> p = values.equal_range(key);
			multimap<string,string>::iterator it=p.first;
			if (it==values.end()) return;
			while(idx!=0 && it!=p.second) {idx--;it++;}
			if (idx==0 && it!=p.second) values.erase(it);
		}
		void removeallvalue(const string &key) {
			pair< multimap<string,string>::iterator, multimap<string,string>::iterator> p = values.equal_range(key);
			multimap<string,string>::iterator it=p.first;
			if (it==values.end()) return;
			while(it!=p.second)
				values.erase(it++);
		}
	protected:
		multimap<string,string> values;
	};
};
