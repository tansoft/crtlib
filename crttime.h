#pragma once

#include "crtlib.h"
#include "crtstring.h"

namespace crtfun {
	static string getnicetimediff(time_t time1,time_t time2)
	{
	    long diff;
	    if (time1>=time2) diff=0;
	    else diff = (long)(time2 - time1);
	    string str;
	    if (diff >= 0) {
	        long s = diff / (60);
	        long h = s / 60;
	        long d = h / 24;
	        long m = d / 30;
	        long y = m / 12;
	        if (y > 0) {
				if (y == 1) str = "去年";
				else if (y == 2) str = "前年";
	            else str = crtl2s(y) + "年前";
	        } else if (m > 0) {
	            str = crtl2s(m) + "月前";
	        } else if (d > 0) {
				if (d == 1) str = "昨天";
				else if (d == 2) str = "前天";
				else str = crtl2s(d) + "天前";
	        } else if (h > 0) {
#ifdef _WIN32
				str = crtl2s(h) + "\xd0\xa1\xca\xb1\xc7\xb0";
#else
				str = crtl2s(h) + "小时前";//"\xd0\xa1\xca\xb1\xc7\xb0";//小时前
#endif
			} else if (s > 0) {
#ifdef _WIN32
	            str = crtl2s(s) + "\xb7\xd6\xd6\xd3\xc7\xb0";
#else
	            str = crtl2s(s) + "分钟前";//"\xb7\xd6\xd6\xd3\xc7\xb0";//分钟前
#endif
	        } else {
	            str = "刚刚";
	        }
	    }
	    return str;
	}
	//Y(y) m d H M S
	static time_t string2time(const char* time,const char* format="%d-%2d-%2d %2d:%2d:%2d")
	{
		struct tm tm1;
		sscanf(time, format, &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday,
			&tm1.tm_hour, &tm1.tm_min,&tm1.tm_sec);
		if (tm1.tm_year > 100)
			tm1.tm_year -= 1900;
		else
			tm1.tm_year += 100;
		tm1.tm_mon--;
		tm1.tm_isdst=-1;
		return mktime(&tm1);
	}
	static string locallongtime2string(time_t time,const char* format="%4d-%02d-%02d %02d:%02d:%02d")
	{
		struct tm tm1;
	#ifdef WIN32
		tm1 = *localtime(&time);
	#else
		localtime_r(&time, &tm1);
	#endif
		return string_format(format,tm1.tm_year+1900, tm1.tm_mon+1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
	}
	static string localday2string(time_t time,const char* format="%4d-%02d-%02d")
	{
		struct tm tm1;
	#ifdef WIN32
		tm1 = *localtime(&time);
	#else
		localtime_r(&time, &tm1);
	#endif
		return string_format(format,tm1.tm_year+1900, tm1.tm_mon+1, tm1.tm_mday);
	}
	static string localtime2string(time_t time,const char* format="%02d:%02d:%02d")
	{
		struct tm tm1;
	#ifdef WIN32
		tm1 = *localtime(&time);
	#else
		localtime_r(&time, &tm1);
	#endif
		return string_format(format, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
	}
	/**
	* @brief 获取Http中的GMT时间串转换为时间
	* @param str	时间串
	* @return 时间，不足的地方由当前时间代替
	*/
	static time_t gmttime2timet(string str) {
		//Thu, 24 Jul 2008 05:15:02 GMT
		//Thu, 24-Jul-2008 05:15:02 GMT
		//Sat, 06-Aug-22 10:35:33 GMT
		size_t pos=str.find(',');
		if (pos!=string::npos)
			str=str.substr(pos+1);
		string_tolower(str);
		str=string_replace(str,"gmt","");
		str=string_replace(str,"jan","01");
		str=string_replace(str,"feb","02");
		str=string_replace(str,"mar","03");
		str=string_replace(str,"apr","04");
		str=string_replace(str,"may","05");
		str=string_replace(str,"jun","06");
		str=string_replace(str,"jul","07");
		str=string_replace(str,"aug","08");
		str=string_replace(str,"sep","09");
		str=string_replace(str,"oct","10");
		str=string_replace(str,"nov","11");
		str=string_replace(str,"dec","12");
		str=string_replace(str,"-"," ");
		string_trim(str);
		crtstringtoken tk(str," ");
		string day=tk.nexttoken();
		string mon=tk.nexttoken();
		string year=tk.nexttoken();
		year=year+" "+mon+" "+day+" "+tk.nexttoken();
		return string2time(year.c_str(),"%d %2d %2d %2d:%2d:%2d");
	}
	static string timet2gmttime(time_t time) {
		//%a, %d %b %y %h:%m:%s gmt
		//Thu, 24 Jul 2008 05:15:02 GMT
		const char wday[][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
		const char mon[][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
		char buffer[64];
		tm *t=gmtime(&time);
		sprintf(buffer,"%s, %02d %s %04d %02d:%02d:%02d GMT",
			wday[t->tm_wday],t->tm_mday,mon[t->tm_mon],t->tm_year+1900,t->tm_hour,t->tm_min,t->tm_sec);
		return buffer;
	}
	#ifdef _WIN32
	static unsigned int filetime2time_t(FILETIME ft)
	{
		time_t t;
		LONGLONG ll;
		ULARGE_INTEGER ui;
		ui.LowPart = ft.dwLowDateTime;
		ui.HighPart = ft.dwHighDateTime;
		ll = (ft.dwHighDateTime << 32) + ft.dwLowDateTime;
		t = (time_t)((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000000);
		return (unsigned int)t;
	}
	#endif
};
