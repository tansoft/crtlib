#pragma once

#if defined(__APPLE__) && !defined(_DARWIN_UNLIMITED_SELECT)
	#define _DARWIN_UNLIMITED_SELECT
#endif

#ifdef _WIN32
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT   0x0502
	#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <queue>
#include <string>
#include <sstream>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef _WIN32
typedef unsigned __int64 uint64_t;
typedef signed __int64 int64_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef int socklen_t;
#ifndef _SSIZE_T_DEFINED
	#ifdef  _WIN64
		typedef signed __int64 ssize_t;
	#else
		typedef _W64 signed int ssize_t;
	#endif
	#define _SSIZE_T_DEFINED
#endif
//#include <wtypes.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef _WINSOCK2API_
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#endif
#pragma warning(disable:4996)
#pragma warning(disable:4819)//contains code page 936
#define vsnprintf _vsnprintf
#define S_ISDIR(x) ((x) & _S_IFDIR)
#else//end of ifdef _WIN32
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/param.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/syscall.h>
#include <dirent.h>
#define MAX_PATH	1024
typedef int SOCKET;
#define closesocket close
#define INVALID_SOCKET -1
#define INVALID_HANDLE_VALUE -1
#define HANDLE int
#define LONG_PTR long
#define INT_PTR int
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> /* _NSGetExecutablePath */
#endif

namespace crtfun {
	typedef enum _crtdebuglevel{
		crtdebugnone=0,
		crtdebugerror,
		crtdebugwarning,
		crtdebuginfo,
		crtdebugdebug
	}crtdebuglevel;
	//network big-endian read
	#define crtreadu32(p)				\
		(  (((uint32_t) (p)[0]) << 24)			\
		| (((uint32_t) (p)[1]) << 16)			\
		| (((uint32_t) (p)[2]) << 8)			\
		|  ((uint32_t) (p)[3]))
	//big-endian read
	#define crtwriteu32(p, i)			\
		do {						\
		(p)[0] = ((i) >> 24) & 0xff;			\
		(p)[1] = ((i) >> 16) & 0xff;			\
		(p)[2] = ((i) >> 8) & 0xff;			\
		(p)[3] = (i) & 0xff;				\
		} while(0)
	#define crtreadu24(p)				\
		(  (((uint32_t) (p)[0]) << 16)			\
		| (((uint32_t) (p)[1]) << 8)			\
		|  ((uint32_t) (p)[2]))

	#define crtwriteu24(p, i)			\
		do {						\
		(p)[0] = ((i) >> 16) & 0xff;			\
		(p)[1] = ((i) >> 8) & 0xff;			\
		(p)[2] = (i) & 0xff;				\
		} while(0)

	#define crtreadu16(p)				\
		(  (((uint32_t) (p)[0]) << 8)			\
		 |  ((uint32_t) (p)[1]))

	#define crtwriteu16(p, i)			\
		do {						\
		(p)[0] = ((i) >> 8) & 0xff;			\
		(p)[1] = (i) & 0xff;				\
		} while(0)
	/* little-endian, byteorder */
	#define crtreadu32le(p)			\
		(  (((uint32_t) (p)[3]) << 24)			\
		| (((uint32_t) (p)[2]) << 16)			\
		| (((uint32_t) (p)[1]) << 8)			\
		|  ((uint32_t) (p)[0]))
	#define crtwriteu32le(p, i)			\
		do {						\
		(p)[3] = ((i) >> 24) & 0xff;			\
		(p)[2] = ((i) >> 16) & 0xff;			\
		(p)[1] = ((i) >> 8) & 0xff;			\
		(p)[0] = (i) & 0xff;				\
		} while(0)
	#define crtreadu16le(p)			\
		(  (((uint32_t) (p)[1]) << 8)			\
			|  ((uint32_t) (p)[0]))
	#define crtwriteu16le(p, i)			\
		do {						\
		(p)[1] = ((i) >> 8) & 0xff;			\
		(p)[0] = (i) & 0xff;			\
		} while(0)
	#define crtmakeword(crthighbyte,crtlowbyte) ((uint16_t)(((uint16_t)((uint8_t)(crthighbyte))<<8)&0xFF00)|((uint16_t)((uint8_t)(crtlowbyte))&0x00FF))
	#define crtmakedword(crthighword,crtlowword) ((uint32_t)(((uint32_t)((uint16_t)(crthighword))<<16)&0xFFFF0000)|((uint32_t)((uint16_t)(crtlowword))&0x0000FFFF))
	#define crtmakeddword(crthighdword,crtlowdword) ((uint64_t)(((uint64_t)((uint32_t)(crthighdword))<<32)&0xFFFFFFFF00000000L)|((uint64_t)((uint32_t)(crtlowdword))&0x0FFFFFFFFL))
	#define crthighbyte(word) (uint8_t((((uint16_t)(word))>>8)&0x00FF))
	#define crtlowbyte(word) (uint8_t(((uint16_t)(word))&0x00FF))
	#define crthighword(dword) (uint16_t((((uint32_t)(dword))>>16)&0x0000FFFF))
	#define crtlowword(dword) (uint16_t(((uint32_t)(dword))&0x0000FFFF))
	#define crthighdword(ddword) (uint32_t((((uint64_t)(ddword))>>32)&0x0FFFFFFFF))
	#define crtlowdword(ddword) (uint32_t(((uint64_t)(ddword))&0x0FFFFFFFF))
};

using namespace std;

namespace crtfun {
	//return false to cancel the debug print
	typedef bool (*crtdebugnotify)(const char *str,const char *orgstr,crtdebuglevel level);
	class crtlib{
		public:
			static crtlib* instance() {
				static crtlib *lib=NULL;
				if (!lib) {
					lib=new crtlib();
					//init settings
					lib->debuglevel=crtdebugdebug;
					strcpy(lib->http_user_agent,"crthttp");
					lib->http_last_errcode=0;
					lib->http_global_retrytimes=0;
					lib->http_global_timeout=30000;//default connect is 30 sec
					lib->debugfilter[0]='\0';
					lib->debugfilter[1]='\0';
					lib->debugfilterregex=NULL;
					lib->debugformat=0;
					lib->debugnotify=NULL;
				}
				return lib;
			}
			crtdebuglevel debuglevel;
			char http_user_agent[1024];
			volatile unsigned int http_last_errcode;
			int http_global_retrytimes;
			int http_global_timeout;
			char debugfilter[256];
			void *debugfilterregex;
			int debugformat;
			crtdebugnotify debugnotify;
	};
#ifndef _WIN32
	static char *strlwr(char *s)
	{
	    char *str = s;
	    while(*str != '\0')
	    {
	    	if(*str > 'A' && *str < 'Z') *str += 'a'-'A';
	        str++;
	    }
	    return s;
	}
	static char *strupr(char *s)
	{
	    char *str = s;
	    while(*str != '\0')
	    {
	    	if(*str > 'a' && *str < 'Z') *str -= 'a'-'A';
	        str++;
	    }
	    return s;
	}
#endif
	static void ms_sleep(int ms) {
		#ifdef _WIN32
			Sleep(ms);
		#else
			usleep(ms * 1000);
		#endif
	}
	static void min_sleep() {
		#ifdef _WIN32
			Sleep(1);
		#else
			usleep(1);
		#endif
	}
	static uint32_t get_ms_tick() //the same as windows GetTickCount
	{
	#ifdef _WIN32
		return GetTickCount();
	#else
		static long int g_fristsec = 0;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		if (!g_fristsec)
			g_fristsec = tv.tv_sec - 1000;
		return (uint32_t)((tv.tv_sec - g_fristsec) * 1000 + tv.tv_usec / 1000);
	#endif
	}
	#ifndef _WIN32
	static uint32_t GetLastError() {return errno;}
	#endif
};

#include "crtdebug.h"
