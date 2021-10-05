#pragma once

#include "crtlib.h"
#include "crtbase64.h"
#include "crtbuffer.h"
#include "crtstructure.h"
#include "crtcookie.h"
#include "crtdebug.h"
#include "crthttp.h"
#include "crtjson.h"
#include "crtmd5.h"
#include "crtsha.h"
#include "crtregex.h"
#include "crtsocket.h"
#include "crtsocketthread.h"
#include "crtstring.h"
#include "crttime.h"
#include "crtfile.h"
#include "crtcharset.h"
#include "crtcmdline.h"
#include "crtthread.h"
#include <assert.h>

namespace crtfun {
	static string crtcmdline_help(const char *key,const char *arg) {crtdebug("help function for callback,key:%s,arg:%s\n",key,arg);return "";}
	static string crtcmdline_parser(const char *key,const char *arg) {crtdebug("parser function for callback,key:%s,arg:%s\n",key,arg);return "";}
	static string crtcmdline_default(const char *key,const char *arg) {crtdebug("default function for callback,key:%s,arg:%s\n",key,arg);return "";}
	static void testcase_crtcmdline()
	{
		int argc=4;
		//const char *argv[]={"test.exe","-h","barrytan","-p",NULL};
		//const char *argv[]={"test.exe","-p","80","test",NULL};
		//const char *argv[]={"test.exe","-u","barrytan","-p",NULL};
		const char *argv[]={"test.exe","-t","barrytan","-p",NULL};
		int port;
		char *buf;
		crtcmdlineinfo infos[]={
			{"h","help",crtcmdline_exit|crtcmdline_func,{(void *)crtcmdline_help},"help text for program"},
			{"p",NULL,crtcmdline_need|crtcmdline_hasarg|crtcmdline_int,{&port},"port for connect"},
			{"u","username",crtcmdline_hasarg|crtcmdline_string,{&buf},"username for login"},
			{"t","testfun",crtcmdline_hasarg|crtcmdline_func,{(void *)crtcmdline_parser},"value for call"},
			{"",NULL,crtcmdline_func,{(void *)crtcmdline_default},"default function for unrecognized param"},
			NULL
		};
		crtdebug(crtcmdline_parse(argc,argv,infos).c_str());
		crtdebug(crtcmdline_printinfo(infos).c_str());
	}
	static void testcase_buffer()
	{
	}
	static void testcase_structure()
	{
		crtlru<int> lru;
		time_t ti=1234567890;
		lru.update(6,ti-1);
		lru.update(7,ti+4);
		lru.update(1,ti+4);
		lru.update(9,ti+4);
		lru.update(9,ti+5);
		lru.update(7,ti+5);
		lru.update(1,ti+9);
		lru.update(2,ti+10);
		lru.update(3,ti+11);
		lru.update(4,ti+12);
		lru.update(5,ti+13);
		lru.update(3,ti+15);
		lru.cancel(5);
		int i=0;
		int result[]={6,0,0,0,0,9,7,0,0,1,2,0,4,0,0,3,0,0,0,0};
		while(i<20) {
			assert(result[i]==lru.poptimeout(ti++));
			i++;
		}
	}
	static void testcase_charset()
	{
		string utf8="\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8\x80\xe6\xae\xb5\xe6\x96\x87\xe5\xad\x97\x0d\x0a\x61\x62\x63\x64\x65\x66";
		string gb2312="\xd5\xe2\xca\xc7\xd2\xbb\xb6\xce\xce\xc4\xd7\xd6\x0d\x0a\x61\x62\x63\x64\x65\x66";
	#if defined(_WIN32) || defined(__MINGW32__) || defined(_CYGWIN_)
		const char utf16[]="\xd9\x8f\x2f\x66\x00\x4e\xb5\x6b\x87\x65\x57\x5b\x0d\x00\x0a\x00\x61\x00\x62\x00\x63\x00\x64\x00\x65\x00\x66\x00";
	#else
		const char utf16[]="\xd9\x8f\x00\x00\x2f\x66\x00\x00\x00\x4e\x00\x00\xb5\x6b\x00\x00\x87\x65\x00\x00\x57\x5b\x00\x00\x0d\x00\x00\x00\x0a\x00\x00\x00\x61\x00\x00\x00\x62\x00\x00\x00\x63\x00\x00\x00\x64\x00\x00\x00\x65\x00\x00\x00\x66\x00\x00\x00";
	#endif
		assert(crtucs2u8(crtu82ucs(utf8))==utf8);
		wstring str=crtu82ucs(utf8);
		crtdebug(crtbin2str(str.c_str(),str.size()*sizeof(wchar_t),"\\x").c_str());
		assert(memcmp(crtu82ucs(utf8).c_str(),utf16,sizeof(utf16))==0);
		assert(crta2u8(gb2312.c_str())==utf8);
		assert(memcmp(crta2ucs(gb2312.c_str()).c_str(),utf16,sizeof(utf16))==0);
		assert(crtu82a(utf8.c_str())==gb2312);
		assert(memcmp(crtu82ucs(utf8).c_str(),utf16,sizeof(utf16))==0);
		return;
	}
	static void testcase_arith()
	{
		assert(crtbase64::encode_str("test")=="dGVzdA==");
		assert(crtbase64::decode_str("ZGVjb2RldGVzdA==")=="decodetest");
		assert(crtmd5::md5str2str("test")=="098f6bcd4621d373cade4e832627b4f6");
		assert(crtsha::shastr2str("test",crtshatype_sha1)=="a94a8fe5ccb19ba61c4c0873d391e987982fbbd3");
		assert(crtsha::shastr2str("test",crtshatype_sha256)=="9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08");
		assert(crtsha::shastr2str("d3b0668687e863369f70170b50325577a5ce84241344654910LJDEZ6",crtshatype_sha1)==
			"f558abdd4de0b3fa8b35261f65ad1c53e72f3093");
		assert(crtsha::shastr2str("d3b0668687e863369f70170b50325577a5ce84241344654910LJDEZ6",crtshatype_sha256)==
			"ecd07b96134fd04970451bcf383336b0fea170f2bb103066180280b8995ef980");
	}
	static void testcase_crtsocket()
	{
		assert(is_ip_string("0.0.0.0")==true);
		assert(is_ip_string("120.10.20.30")==true);
		assert(is_ip_string("123.com")==false);
		assert(is_ip_string("123.test.com")==false);
		assert(is_ip_string("www.test.com")==false);
		assert(get_ip_by_name("127.0.0.1")==0x0100007f);
		assert(get_ip_by_name("www.google.com")!=0);
	}
	static void testcase_crthttp()
	{
		string str;
		str=http_download_to_str("http://fsvod.com/",NULL,NULL,1000,2);
		debug_string0(str.substr(0,100).c_str());
		debug_string(crtdebugdebug,"\nerrorcode:%d,len:%d\n",get_last_http_error(),str.length());
		//assert(str!="");
		str=http_download_to_str("www.baidu.com");
		debug_string0(str.substr(0,100).c_str());
		debug_string(crtdebugdebug,"\nerrorcode:%d,len:%d\n",get_last_http_error(),str.length());
		assert(str!="");
		str=http_download_to_str("http://www.google.com/",NULL,NULL);
		debug_string0(str.substr(0,100).c_str());
		debug_string(crtdebugdebug,"\nerrorcode:%d,len:%d\n",get_last_http_error(),str.length());
		assert(str!="");
		string encodeurl=crturl_encode("barrytan@163.com");
		debug_string(crtdebugdebug,crturl_saveprint(encodeurl).c_str());
		debug_string(crtdebugdebug,crturl_decode(encodeurl.c_str()).c_str());
		assert(encodeurl=="barrytan%40163%2ecom");
		assert(crturl_saveprint(encodeurl)=="barrytan%%40163%%2ecom");
		assert(crturl_decode(encodeurl.c_str())=="barrytan@163.com");
	}
	static void testcase_crtcookie()
	{
		crtcookie cookie;
		cookie.http_download_to_str("http://weibo.com/",NULL,NULL);
		debug_string0(cookie.tostring().c_str());
		cookie.http_download_to_str("http://weibo.com/",NULL,NULL);
		debug_string0(cookie.tostring().c_str());
		string tmpfile=crtget_temp_file();
		debug_string0(tmpfile.c_str());
		assert(crtsave_str_to_file(tmpfile.c_str(),cookie.savecookietojsonstring().c_str()));
		cookie.clear();
		debug_string(crtdebugdebug,"save file:%s\n",crtread_str_from_file(tmpfile.c_str()).c_str());
		debug_string(crtdebugdebug,"cookie now:%s\n",cookie.tostring().c_str());
		assert(cookie.getcookiefromjsonstring(crtread_str_from_file(tmpfile.c_str()).c_str()));
		debug_string(crtdebugdebug,"cookie now:%s\n",cookie.tostring().c_str());
		crtdelete_file(tmpfile.c_str());
	}
#define CRTTESTCASE_HTTPUATEST(ua,ret)	test.init(ua);\
									debug_string0(test.tostring().c_str());\
									debug_string0("\n");\
									assert(test.tostring()==ret)
	static void testcase_crtregex()
	{
		assert(crtregex::iswhole_matched("12345","\\d+"));
		assert(!crtregex::iswhole_matched("12345a","\\d+"));
		assert(!crtregex::iswhole_matched("s12345","\\d+"));
		assert(crtregex::ismatched_email("bob@a.com"));
		assert(!crtregex::ismatched_email("bob@.com"));
		assert(!crtregex::ismatched_email("bob@avd"));
		assert(crtregex::ismatched_email("bob.dew@avd.com"));
		assert(crtregex::match_one("int a; /* a */ \r\n //dsa\r\nadss","/\\*((?!\\*/).)*(\\*/)?|//([^\\x0A-\\x0D\\\\]|\\\\.)*")=="/* a */");
		vector<string> ret=crtregex::match_all("int a; /* a */ \r\n //dsa\r\nadss","/\\*((?!\\*/).)*(\\*/)?|//([^\\x0A-\\x0D\\\\]|\\\\.)*");
		assert(ret[0]=="/* a */");
		assert(ret[1]=="//dsa");
		crthttpuatest test;
		//more useragent in useragentswitcher.xml
		//Chrome 20.0.1092.0 (Win 7)
		CRTTESTCASE_HTTPUATEST(
			"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.6 (KHTML, like Gecko) Chrome/20.0.1092.0 Safari/536.6",
			"ver:536.6 (536.6) msie:0 safari:0 opera:0 mozilla:0 firefox:0 chrome:1 other:0");
		//Firefox 10.0.1 (Win 7 64)
		CRTTESTCASE_HTTPUATEST(
			"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:10.0.1) Gecko/20100101 Firefox/10.0.1",
			"ver:10.0.1 (10) msie:0 safari:0 opera:0 mozilla:0 firefox:1 chrome:0 other:0");
		//MSIE 6 (Win XP)
		CRTTESTCASE_HTTPUATEST(
			"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)",
			"ver:6.0 (6) msie:1 safari:0 opera:0 mozilla:0 firefox:0 chrome:0 other:0");
		//Opera 9.25 - (Vista)
		CRTTESTCASE_HTTPUATEST("Opera/9.25 (Windows NT 6.0; U; en)",
			"ver:9.25 (9.25) msie:0 safari:0 opera:1 mozilla:0 firefox:0 chrome:0 other:0");
		//Safari 531.21.10 (Win XP)
		CRTTESTCASE_HTTPUATEST("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/531.21.8 (KHTML, like Gecko) Version/4.0.4 Safari/531.21.10",
			"ver:531.21.8 (531.21) msie:0 safari:1 opera:0 mozilla:0 firefox:0 chrome:0 other:0");
	}
	static void testcase_crtjson()
	{
		char text[]="{\"status\":200,\"msg\":\"ok\",\"token\":\"3f08475cf19405ad\",\"createdate\":12312421322,\"server\":\"192.168.68.57:80\"}";
		crtjsonparser parser;
		crtjson *json=parser.parse(text);
		assert(json!=NULL);
		printf("%s\n",parser.tostring(json,0).c_str());
		assert(parser.tostring(json,0)==text);
		parser.delete_json(json);
	
		crtjson *root=parser.createobject();
		parser.objectadd(root,"name",parser.createstring("Jack (\"Bee\") Nimble"));
		crtjson *fmt=parser.createobject();
		parser.objectadd(root,"format",fmt);
		parser.objectadd_string(fmt,"type","rect");
		parser.objectadd_number(fmt,"width",1920);
		parser.objectadd_number(fmt,"height",1080);
		parser.objectadd_false(fmt,"interlace");
		parser.objectadd_number(fmt,"frame rate",24);	
		printf("%s\n",parser.tostring(root).c_str());
		assert(parser.tostring(root,0)=="{\"name\":\"Jack (\\\"Bee\\\") Nimble\",\"format\":{\"type\":\"rect\",\"width\":1920,\"height\":1080,\"interlace\":false,\"frame rate\":24}}");
		parser.delete_json(root);

		const char *strings[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
		root=parser.createstringarray(strings,7);
		printf("%s\n",parser.tostring(root).c_str());
		assert(parser.tostring(root)=="[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]");
		parser.delete_json(root);
	
		int numbers[3][3]={{0,-1,0},{1,0,0},{0,0,1}};
		root=parser.createarray();
		int i;
		for (i=0;i<3;i++) parser.arrayadd(root,parser.createintarray(numbers[i],3));
		printf("%s\n",parser.tostring(root).c_str());
		parser.delete_json(root);

		int ids[4]={116,943,234,38793};
		root=parser.createarray();
		crtjson *img=parser.createobject();
		parser.objectadd(root, "Image", img);
		parser.objectadd_number(img,"Width",800);
		parser.objectadd_number(img,"Height",600);
		parser.objectadd_string(img,"Title","View from 15th Floor");
		crtjson *thm=parser.createobject();
		parser.objectadd(img, "Thumbnail", thm);
		parser.objectadd_string(thm, "Url", "http:/*www.example.com/image/481989943");
		parser.objectadd_number(thm,"Height",125);
		parser.objectadd_string(thm,"Width","100");
		parser.objectadd(img,"IDs", parser.createintarray(ids,4));
		printf("%s\n",parser.tostring(root).c_str());
		parser.delete_json(root);

		struct record {const char *precision;double lat,lon;const char *address,*city,*state,*zip,*country;};
		struct record fields[2]={
			{"zip",37.7668,-1.223959e+2,"","SAN FRANCISCO","CA","94107","US"},
			{"zip",37.371991,-1.22026e+2,"","SUNNYVALE","CA","94085","US"}};
		root=parser.createarray();
		for (i=0;i<2;i++)
		{
			crtjson *fld=parser.createobject();
			parser.arrayadd(root,fld);
			parser.objectadd_string(fld, "precision", fields[i].precision);
			parser.objectadd_number(fld, "Latitude", fields[i].lat);
			parser.objectadd_number(fld, "Longitude", fields[i].lon);
			parser.objectadd_string(fld, "Address", fields[i].address);
			parser.objectadd_string(fld, "City", fields[i].city);
			parser.objectadd_string(fld, "State", fields[i].state);
			parser.objectadd_string(fld, "Zip", fields[i].zip);
			parser.objectadd_string(fld, "Country", fields[i].country);
			parser.objectadd_number(fld, "index", i);
		}
		printf("find:\n%s\n",parser.tostring(parser.find(root,"[1]")).c_str());
		assert(parser.tostring(parser.find(root,"[1].index"))=="1");
		printf("%s\n",parser.tostring(root).c_str());
		parser.objectreplace(parser.arrayat(root,1),"City",parser.createintarray(ids,4));	
		assert(parser.tostring(parser.find(root,"[1].City.0"))=="116");
		assert(parser.tostring(parser.find(root,"[1].City[1]"))=="943");
		assert(parser.tostring(parser.find(root,"[1].City.[2]"))=="234");
		printf("%s\n",parser.tostring(root).c_str());
		parser.delete_json(root);
	}
	static void testcase_crttime()
	{
		time_t ti=string2time("2012-08-07 16:42:28");
		debug_string0(string_format("%d",ti).c_str());
		assert(ti==1344328948);
		debug_string0(locallongtime2string(ti).c_str());
		assert(locallongtime2string(ti)=="2012-08-07 16:42:28");
		assert(localday2string(ti)=="2012-08-07");
		assert(localtime2string(ti)=="16:42:28");
		assert(gmttime2timet("Thu, 24 Jul 2008 05:15:02 GMT")==1216847702);
		assert(gmttime2timet("Thu, 24-Jul-2008 05:15:02 GMT")==1216847702);
		assert(gmttime2timet("Sat, 06-Aug-22 10:35:33 GMT")==1659753333);
		assert(getnicetimediff(200,1)=="刚刚");
		assert(getnicetimediff(1,2)=="刚刚");
		debug_string0(getnicetimediff(1,200).c_str());
		debug_string0(getnicetimediff(1,20000).c_str());
		debug_string0(getnicetimediff(1,200000).c_str());
#ifdef _WIN32
		assert(getnicetimediff(1,200)=="3\xb7\xd6\xd6\xd3\xc7\xb0");
		assert(getnicetimediff(1,20000)=="5\xd0\xa1\xca\xb1\xc7\xb0");
#else
		assert(getnicetimediff(1,200)=="3分钟前");
		assert(getnicetimediff(1,20000)=="5小时前");
#endif
		assert(getnicetimediff(1,200000)=="前天");
		assert(getnicetimediff(1,2000000)=="23天前");
		assert(getnicetimediff(1,20000000)=="7月前");
		assert(getnicetimediff(1,200000000)=="6年前");
	}
#define CRTTESTCASE_URLPARSE(url)	parser.parse(url);\
									debug_string0(parser.tostring().c_str()); \
									debug_string0("\n")
	static void testcase_crtstring()
	{
		//base string function
		string rstr="abcde";
		assert(string_right(rstr,4)=="bcde");
		assert(string_right(rstr,5)=="abcde");
		assert(string_right(rstr,6)=="abcde");
		assert(string_right(rstr,3)=="cde");
		string str="DEWfsfw";
		string_tolower(str);
		assert(str=="dewfsfw");
		str="DEWfsfw";
		string_toupper(str);
		assert(str=="DEWFSFW");
		str=" \t\r\nd\te\rw\ne \t \r\n";
		assert(string_trim(str)==str);
		assert(str=="d\te\rw\ne");
		assert(string_trim(str)==str);
		assert(str=="d\te\rw\ne");
		str=string_replace(str,"e","a");
		str=string_replace(str,"\t","");
		str=string_replace(str,"\r","");
		str=string_replace(str,"daw","daw");
		str=string_replace(str,"aw","wa");
		assert(str=="dwa\na");
		assert(string_replace("abcdefgabcdefg","efg","")=="abcdabcd");
		assert(string_replace("abcdefgabcdefg","efg","g")=="abcdgabcdg");
		assert(string_replace("abcdefgabcdefg","g","")=="abcdefabcdef");
		assert(string_replace("abcdefgabcdefg","g","hg")=="abcdefhgabcdefhg");
		//bin_search
		assert(bin_search("",1,"abcd",4)==NULL);
		assert(memcmp(bin_search("abcd",4,"abcd",4),"abcd",4)==0);
		assert(memcmp(bin_search("1abcd",5,"abcd",4),"abcd",4)==0);
		assert(memcmp(bin_search("1abcd2",6,"abcd",4),"abcd",4)==0);
		//crturlparser
		crturlparser parser;
		CRTTESTCASE_URLPARSE("http://abc.com");
		CRTTESTCASE_URLPARSE("file://abc.com");
		CRTTESTCASE_URLPARSE("https://abc.com");
		CRTTESTCASE_URLPARSE("www.abc.com");
		CRTTESTCASE_URLPARSE("http://abc.com/");
		CRTTESTCASE_URLPARSE("http://abc.com?");
		CRTTESTCASE_URLPARSE("http://abc.com?a");
		CRTTESTCASE_URLPARSE("http://abc.com/?a");
		CRTTESTCASE_URLPARSE("http://abc.com/?d#a");
		CRTTESTCASE_URLPARSE("http://abc.com/a?f");
		CRTTESTCASE_URLPARSE("http://abc.com/a/?f");
		CRTTESTCASE_URLPARSE("http://abc.com/a/a?f");
		CRTTESTCASE_URLPARSE("http://abc.com/a/a.a?f");
		CRTTESTCASE_URLPARSE("http://abc.com/a/a.b.c?f");
		CRTTESTCASE_URLPARSE("http://abc.com/a/a.b.c/a?f");
		CRTTESTCASE_URLPARSE("http://abc.com:80/a");
		CRTTESTCASE_URLPARSE("http://123@abc.com:80/a/?f");
		CRTTESTCASE_URLPARSE("http://123:80@abc.com:80/a/?f");
		CRTTESTCASE_URLPARSE("http://12@3:8@0@abc.com:80/a/?f");
		crtstringparser stringparser;
		stringparser.parse_with_http_get_params("a=b&c=d&a=1&c=dd%2c&d=1");
		stringparser.parse_with_key_quote_values("a=abc, b=\"a'd\\\",=\", c='dd,\"=wq\\''");
		assert(stringparser.getvalue("a",0)=="b");
		assert(stringparser.getvalue("a",1)=="1");
		assert(stringparser.getvalue("a",2)=="abc");
		assert(stringparser.getvalue("c",0)=="d");
		assert(stringparser.getvalue("c",1)=="dd,");
		assert(stringparser.getvalue("c",2)=="dd,\"=wq\\'");
		assert(stringparser.getvalue("c",3)=="");
		assert(stringparser.getvalue("d",0)=="1");
		assert(stringparser.getvalue("d",1)=="");
		assert(stringparser.getvalue("b",0)=="a'd\\\",=");
		assert(stringparser.getvalue("k",0)=="");
		assert(stringparser.getvalue("k",1)=="");
	}
	static void testthread(void *data){
		crtsem *sem=(crtsem *)data;
		while(1) {
			crtdebug("inthread\n");
			sem->wait();
			crtdebug("outhread\n");
		}
	}
	static void testcase_crtthread() {
		crtsem sem;
		sem.init(1);
		crtthread::startwithfunction(testthread,&sem);
		crtthread::startwithfunction(testthread,&sem);
		ms_sleep(2000);
		sem.post();
		sem.post();
		sem.post();
		ms_sleep(2000);
		sem.post();
		sem.post();
		ms_sleep(2000);
		sem.post();
	}
	static void testcase_all() {
		init_socket();
		testcase_buffer();
		testcase_structure();
		testcase_charset();
		testcase_arith();
		testcase_crtsocket();
		testcase_crthttp();
		testcase_crtcookie();
		testcase_crtregex();
		testcase_crtjson();
		testcase_crttime();
		testcase_crtstring();
		testcase_crtcmdline();
		testcase_crtthread();
	}
};
