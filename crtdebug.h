#pragma once

#include "crtlib.h"
#ifdef CRTDEBUG
#include "crtstring.h"
#include "crtregex.h"
#include "crtthread.h"
#endif

#define CRTDEBUG_MAXLENGTH	65535
#define CRTDEBUG_NODATE		0x1
#define CRTDEBUG_TS			0x2
#define CRTDEBUG_LN			0x4
#define CRTDEBUG_THREADID	0x8
#define CRTDEBUG_LEVEL		0x10

/**
* @brief 调试类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/
namespace crtfun {
	static void debug_string0(const char *str) {
		#if (defined(_CONSOLE) || !defined(_WIN32))
			printf("%s",str);
		#else
			OutputDebugStringA(str);
		#endif
	}
	//buffer only CRTDEBUG_MAXLENGTH,debug print directly function
	static void debug_string(crtdebuglevel level,const char *str, ...)
	{
		if (crtlib::instance()->debuglevel<level) return;
		char buf[CRTDEBUG_MAXLENGTH];
		va_list a;
		va_start(a,str);
		vsnprintf(buf,CRTDEBUG_MAXLENGTH,str,a);
		va_end(a);
		debug_string0(buf);
	}
	static void set_debug(crtdebuglevel debuglevel){crtlib::instance()->debuglevel=debuglevel;}
	static void set_debugformat(int debugformat){crtlib::instance()->debugformat=debugformat;}
	static void set_debugnotify(crtdebugnotify debugnotify){crtlib::instance()->debugnotify=debugnotify;}
	static void set_debugfilter(const char *filter,bool bregex=false) {
	#ifdef CRTDEBUG
		static crtregex regex;
		crtlib *lib=crtlib::instance();
		char *debugfilter=lib->debugfilter;
		if (!filter || filter[0]=='\0') {
			debugfilter[0]='\0';
			debugfilter[1]='\0';
			return;
		}
		strcpy(&debugfilter[1],filter);
		if (bregex) {
			crtregex *regex=(crtregex *)lib->debugfilterregex;
			if (!regex) {
				regex=new crtregex();
				lib->debugfilterregex=(void *)regex;
			}
			regex->init(filter);
		}
		debugfilter[0]=bregex?1:0;
	#endif
	}
#ifdef CRTDEBUG
	static bool filter_debugstring(const char *debugstr) {
		crtlib *lib=crtlib::instance();
		char *debugfilter=lib->debugfilter;
		if (debugfilter[1]=='\0') return true;
		if (debugfilter[0]==0)
			return string_startwith(debugstr,&debugfilter[1]);
		return ((crtregex *)lib->debugfilterregex)->ismatched(debugstr);
	}
	static void crtdebugdot() {
		if (filter_debugstring("."))
			debug_string0(".");
	}
	static const char* _crtlevelname(crtdebuglevel level){
		switch(level){
			case crtdebugerror:return "EROR";
			case crtdebugwarning:return "WARN";
			case crtdebuginfo:return "INFO";
			case crtdebugdebug:return "DEBG";
			default:break;
		}
		return "UNKN";
	}
	static void _crtdebug(crtdebuglevel level,int flags,const char *str, va_list a)
	{
		char buf[CRTDEBUG_MAXLENGTH];
		size_t off=0;
		crtlib *lib=crtlib::instance();
		flags|=lib->debugformat;
		if (!(flags&CRTDEBUG_NODATE)) {
			time_t st=time(NULL);
			struct tm *t=localtime(&st);
			sprintf(buf+off,"[%02d-%02d %02d:%02d:%02d.%03d]",t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,get_ms_tick()%1000);
			off+=strlen(buf);
		}
		if (flags&CRTDEBUG_TS) {
			sprintf(buf+off,"[%u]",(unsigned int)time(NULL));
			off+=strlen(buf);
		}
		if (flags&CRTDEBUG_THREADID){
			sprintf(buf+off,"[%u]",crtthreadid());
			off+=strlen(buf);
		}
		if (flags&CRTDEBUG_LEVEL){
			sprintf(buf+off,"[%s]",_crtlevelname(level));
			off+=strlen(buf);
		}
		size_t ret=vsnprintf(buf+off,CRTDEBUG_MAXLENGTH-off-1,str,a)+off;
		buf[CRTDEBUG_MAXLENGTH-1]='\0';
		if (flags&CRTDEBUG_LN) {
			if (ret==-1 || ret>CRTDEBUG_MAXLENGTH-3) ret=CRTDEBUG_MAXLENGTH-3;
			buf[ret]='\r';
			buf[ret+1]='\n';
			buf[ret+2]='\0';
		}
		if (filter_debugstring(buf+off)) {
			if (lib->debugnotify==NULL || lib->debugnotify(buf,buf+off,level))
				debug_string0(buf);
		}
	}
	static void crtdebug(const char *str, ...){
		va_list a;
		va_start(a,str);
		_crtdebug(crtdebugdebug,0,str,a);
		va_end(a);
	}
	static void crtdebugln(const char *str, ...){
		va_list a;
		va_start(a,str);
		_crtdebug(crtdebugdebug,CRTDEBUG_LN,str,a);
		va_end(a);
	}
	static void crtdbginfo(const char *str,...) {
		va_list a;
		va_start(a,str);
		_crtdebug(crtdebuginfo,0,str,a);
		va_end(a);
	}
	static void crtdbgwarn(const char *str,...) {
		va_list a;
		va_start(a,str);
		_crtdebug(crtdebugwarning,0,str,a);
		va_end(a);
	}
	static void crtdbgerr(const char *str,...) {
		va_list a;
		va_start(a,str);
		_crtdebug(crtdebugerror,0,str,a);
		va_end(a);
	}
#else
	#define crtdebug
	#define crtdebugln
	#define crtdbginfo
	#define crtdbgwarn
	#define crtdbgerr
	#define crtdebugdot()
#endif
};
