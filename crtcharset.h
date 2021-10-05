#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crtbitparser.h"
#ifndef _CRTNOTUSEICONV
#define _CRTUSEICONV
#endif
#ifdef _CRTUSEICONV
	#if (!defined(_WIN32)) && (!defined(__MINGW32__)) && (!defined(_CYGWIN_))
		#include <iconv.h>
	#endif
#endif

/**
* @brief 字符集处理
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/
namespace crtfun {
	static bool crtu82u16(const char *u8str, vector<uint16_t>& tmp, size_t u8strlen=string::npos){
		/* UCS-2 range (hex.) UTF-8 octet sequence (binary)
			0000-007F 0xxxxxxx
			0080-07FF 110xxxxx 10xxxxxx
			0800-FFFF 1110xxxx 10xxxxxx 10xxxxxx*/
		unsigned char c1, c2, c3;
		unsigned char hiByte, lowByte;
		uint16_t bit16;
		crtbitreader br;
		size_t size_tmp = (u8strlen==string::npos) ? strlen(u8str):u8strlen;
		for (size_t i = 0; i < size_tmp; ++i) {
			c1 = u8str[i];
			//0000-007F <==> 0xxxxxxx
			if (c1 < 0x80) tmp.push_back((uint16_t) c1 & 0x007F);
			else if ( (c1 & 0xC0) && ! (c1 & 0x20)) {
				//0080-07FF <==> 110xxx,xx 10xxxxxx
				hiByte = br.reset().read(c1, 3, 3).getbyte();
				if (1 + i >= size_tmp) return false;
				c2 = u8str[++i];
				if (!(c2&0x80)) return false;
				lowByte = br.reset().read(c1, 6, 2).read(c2, 2, 6).getbyte();
				bit16 = crtmakeword(hiByte, lowByte);
				tmp.push_back(bit16);
			} else if ( (c1 & 0xE0) && ! (c1 & 0x10)) {
				//0800-FFFF <==> 1110xxxx 10xxxx,xx 10xxxxxx
				if (2 + i >= size_tmp) return false;
				c2 = u8str[++i]; c3 = u8str[++i];
				if (! (c2 & 0x80) || ! (c3 & 0x80)) return false;
				hiByte = br.reset().read(c1, 4, 4).read(c2, 2, 4).getbyte();
				lowByte = br.reset().read(c2, 6, 2).read(c3, 2, 6).getbyte();
				bit16 = crtmakeword(hiByte, lowByte);
				tmp.push_back(bit16);
			} else return false;
		}
		return true;
	}
	static bool crtu82u32(const char *u8str, vector<uint32_t>& tmp, size_t u8strlen=string::npos) {
		/* UCS-4 range (hex.) UTF-8 octet sequence (binary)
			0000 0000-0000 007F 0xxxxxxx
			0000 0080-0000 07FF 110xxxxx 10xxxxxx
			0000 0800-0000 FFFF 1110xxxx 10xxxxxx 10xxxxxx
			0001 0000-001F FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			0020 0000-03FF FFFF 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			0400 0000-7FFF FFFF 1111110x 10xxxxxx ... 10xxxxxx*/	
		crtbitreader br;
		unsigned char hiByte, lowByte;
		unsigned char c1, c2(0), c3, c4, c5, c6;
		uint16_t hiWord, lowWord;
		size_t size_tmp = (u8strlen==string::npos) ? strlen(u8str):u8strlen;
		for (size_t i = 0; i < size_tmp; ++i) {
			c1 = u8str[i];
			//0000 0000-0000 007F <==> 0xxxxxxx
			if (c1 < 0x80) tmp.push_back((uint32_t)(c1)& 0x00007F);
			else if ( (c1 & 0xC0) && ! (c1 & 0x20)) {
				//0000 0080-0000 07FF <==> 110xxx,xx 10xx,xxxx
				hiByte = br.reset().read(c1, 3, 3).getbyte();
				if (1 + i >= size_tmp) return false;
				c2 = u8str[++i];
				if (!(c2&0x80)) return false;
				lowByte = br.reset().read(c1, 6, 2).read(c2, 2, 6).getbyte();
				tmp.push_back(crtmakeword(hiByte,lowByte)&0x0000FFFF);
			} else if ( (c1 & 0xE0) && ! (c1 & 0x10)) {
				//0000 0800-0000 FFFF <==> 1110xxxx 10xxxx,xx 10xx,xxxx
				if (2 + i >= size_tmp) return false;
				c2 = u8str[++i]; c3 = u8str[++i];
				if (! (c2 & 0x80) || ! (c3 & 0x80)) return false;
				hiByte = br.reset().read(c1, 4, 4).read(c2, 2, 4).getbyte();
				lowByte = br.reset().read(c2, 6, 2).read(c3, 2, 6).getbyte();
				lowWord = crtmakeword(hiByte, lowByte);
				tmp.push_back(crtmakedword((uint16_t)(0),lowWord));
			} else if ( (c1 & 0xF0) && ! (c2 & 0x08)) {
				//0001 0000-001F FFFF <==> 11110xxx 10xx,xxxx 10xxxx,xx 10xx,xxxx
				if (3 + i >= size_tmp) return false;
				c2 = u8str[++i]; c3 = u8str[++i]; c4 = u8str[++i];
				if (! (c2 & 0x80) || ! (c3 & 0x80) || ! (c4 & 0x80)) return false;
				hiByte = 0;
				lowByte = br.reset().read(c1, 5, 3).read(c2, 2, 2).getbyte();
				hiWord = crtmakeword(hiByte, lowByte);
				hiByte = br.reset().read(c2, 4, 4).read(c3, 2, 4).getbyte();
				lowByte = br.reset().read(c3, 6, 2).read(c4, 2, 6).getbyte();
				lowWord = crtmakeword(hiByte, lowByte);
				tmp.push_back(crtmakedword(hiWord, lowWord));
			} else if ( (c1 &0xF8) && ! (c1 & 0x04)) {
				//0020 0000-03FF FFFF <==> 111110xx 10xxxxxx 10xx,xxxx 10xxxx,xx 10xxxxxx
				if (4 + i >= size_tmp) return false;
				c2 = u8str[++i]; c3 = u8str[++i]; c4 = u8str[++i]; c5 = u8str[++i];
				if (!(c2 & 0x80) || !(c3 & 0x80) || !(c4 & 0x80) || !(c5 & 0x80)) return false;
				hiByte = br.reset().read(c1, 6, 2).getbyte();
				lowByte = br.reset().read(c2, 2, 6).read(c3, 2, 2).getbyte();
				hiWord = crtmakeword(hiByte, lowByte);
				hiByte = br.reset().read(c3, 4, 4).read(c4, 2, 4).getbyte();
				lowByte = br.reset().read(c4, 6, 2).read(c5, 2, 6).getbyte();
				lowWord = crtmakeword(hiByte, lowByte);
				tmp.push_back(crtmakedword(hiWord, lowWord));
			} else if ( (c1 & 0xFC) && ! (c1 & 0x02)) {
				//0400 0000-7FFF FFFF <==> 1111110x 10xxxxxx,10xxxxxx 10xx,xxxx 10xxxx,xx 10xxxxxx
				if (5 + i >= size_tmp) return false;
				c2 = u8str[++i]; c3 = u8str[++i]; c4 = u8str[++i]; c5 = u8str[++i]; c6 = u8str[++i];
				if (!(c2 & 0x80) || !(c3 & 0x80) || !(c4 & 0x80) || !(c5 & 0x80) || !(c6 & 0x80)) return false;
				hiByte = br.reset().read(c1, 7, 1).read(c2, 2, 6).getbyte();
				lowByte = br.reset().read(c3, 2, 6).read(c4, 2, 2).getbyte();
				hiWord = crtmakeword(hiByte, lowByte);
				hiByte = br.reset().read(c4, 4, 4).read(c5, 2, 4).getbyte();
				lowByte = br.reset().read(c5, 6, 2).read(c6, 2, 6).getbyte();
				lowWord = crtmakeword(hiByte, lowByte);
				tmp.push_back(crtmakedword(hiWord, lowWord));
			} else return false;
		}
		return true;
	}
	static bool crtu162u8(const uint16_t * const src_ucs2, const size_t size, string& dst_utf8str) {
		/* UCS-2 range (hex.) UTF-8 octet sequence (binary)
			0000-007F 0xxxxxxx
			0080-07FF 110xxx,xx 10xxxxxx
			0800-FFFF 1110xxxx 10xxxx,xx 10xxxxxx*/
		for (unsigned int i = 0; i < size; ++i)
		{
			uint16_t s = src_ucs2[i];
			unsigned char lowByte = crtlowbyte(s);
			if (s <= 0x7F) {
				dst_utf8str.push_back (static_cast<char> (lowByte));
				continue;
			}
			unsigned char hiByte = crthighbyte(s);
			char lowChar = static_cast<char> ( (lowByte & 0x3F) | 0x80);
			if (s <= 0x07FF) {
				char hiChar = static_cast<char> ( ( (lowByte >> 6) & 0x03) | (hiByte << 2) | 0xC0);
				dst_utf8str.push_back (hiChar);
				dst_utf8str.push_back (lowChar);
			} else {//if ( s <= 0xffff )
				char middleChar = static_cast<char> ( ( (lowByte >> 6) & 0x03) | ( (hiByte & 0x0F) << 2) | 0x80);
				char hiChar = static_cast<char> ( ( (hiByte >> 4) & 0x0F) | 0xE0);
				dst_utf8str.push_back (hiChar);
				dst_utf8str.push_back (middleChar);
				dst_utf8str.push_back (lowChar);
			}
		}
		return true;
	}
	static bool crtu322u8(unsigned int const * const src_ucs4, size_t const size, std::string& dst_utf8str) {
		/* UCS-4 range (hex.) UTF-8 octet sequence (binary)
			0000 0000-0000 007F 0xxxxxxx
			0000 0080-0000 07FF 110xxx,xx 10xxxxxx
			0000 0800-0000 FFFF 1110xxxx 10xxxx,xx 10xxxxxx
			0001 0000-001F FFFF 11110xxx 10xx,xxxx 10xxxx,xx 10xxxxxx
			0020 0000-03FF FFFF 111110xx 10,xxxxxx 10xx,xxxx 10xxxx,xx 10xxxxxx
			0400 0000-7FFF FFFF 1111110x 10xxxxxx ... 10xxxxxx */
		uint32_t bit32;
		uint16_t lowWord, hiWord;
		unsigned char lowByte, hiByte;
		unsigned char ch;
		crtbitreader br;
		for (unsigned int i = 0; i < size; ++i) {
			bit32 = src_ucs4[i];
			lowWord = crtlowword(bit32);
			//0000 0000-0000 007F 0xxxxxxx
			if (bit32 <= 0x0000007F) dst_utf8str.push_back (static_cast<char> (lowWord & 0x00FF));
			else if (bit32 <= 0x000007FF) {
				//0000 0080-0000 07FF <==> -00000111 11111111 <==> 110xxx,xx 10xxxxxx
				hiByte = crthighbyte(lowWord);
				lowByte = crtlowbyte(lowWord);
				ch = br.reset().fill (2).fillzero (1).read(hiByte, 5, 3).read(lowByte, 0, 2).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 2, 6).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
			} else if (bit32 <= 0x0000FFFF) {
				//0000 0800-0000 FFFF <==> -11111111 11111111 <==> 1110xxxx 10xxxx,xx 10xxxxxx
				hiByte = crthighbyte(lowWord);
				lowByte = crtlowbyte(lowWord);
				ch = br.reset().fill (3).fillzero (1).read(hiByte, 0, 4).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(hiByte, 4, 4).read(lowByte, 0, 2).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 2, 6).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
			} else if (bit32 <= 0x001FFFFF) {
				//0001 0000-001F FFFF <==> -00000000 00011111 11111111 <==> 11110xxx 10xx,xxxx 10xxxx,xx 10xxxxxx
				hiWord = crthighword(bit32);
				lowByte = crtlowbyte(hiWord);
				hiByte = crthighbyte(lowWord);
				ch = br.reset().fill (4).fillzero (1).read(lowByte, 3, 3).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 6, 2).read(hiByte, 0, 4).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				lowByte = crtlowbyte(lowWord);
				ch = br.reset().fill (1).fillzero (1).read(hiByte, 4, 4).read(lowByte, 0, 2).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));				
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 2, 6).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
			} else if (bit32 <= 0x03FFFFFF) {
				//0020 0000-03FF FFFF <==> -00000011 11111111 11111111 <==> 111110xx 10,xxxxxx 10xx,xxxx 10xxxx,xx 10xxxxxx
				hiWord = crthighword(bit32);
				hiByte = crthighbyte(hiWord);
				lowByte = crtlowbyte(hiWord);
				ch = br.reset().fill (5).fillzero (1).read(hiByte, 6, 2).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 0 , 6).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				hiByte = crthighbyte(lowWord);
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 6, 2).read(hiByte, 0, 4).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				lowByte = crtlowbyte(lowWord);
				ch = br.reset().fill (1).fillzero (1).read(hiByte, 4, 4).read(lowByte, 0 , 2).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 2, 6).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
			} else if (bit32 <= 0x7FFFFFFF) {
				//0400 0000-7FFF FFFF <==> -01111111 11111111 11111111 <==>1111110x 10xxxxxx, 10xxxxxx 10xx,xxxx 10xxxx,xx 10xxxxxx
				hiWord = crthighword(bit32);
				hiByte = crthighbyte(hiWord);
				lowByte = crtlowbyte(hiWord);
				ch = br.reset().fill (6).fillzero (1).read(hiByte, 1, 1).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(hiByte, 2, 6).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 0, 6).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));		
				hiByte = crthighbyte(lowWord);
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 6, 2).read(hiByte, 0, 4).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				lowByte = crtlowbyte(lowWord);
				ch = br.reset().fill (1).fillzero (1).read(hiByte, 4, 4).read(lowByte, 0, 2).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
				ch = br.reset().fill (1).fillzero (1).read(lowByte, 4, 4).getbyte();
				dst_utf8str.push_back (static_cast<char> (ch));
			} else return false;
		}
		return true;
	}
#ifdef _WIN32
	static wstring _crtconvertwina2u(const char *src, size_t len=string::npos) {
		if (len == string::npos) len=strlen(src);
		size_t distsize=(len+1)*2;
		wchar_t *dist=new wchar_t[distsize];
		*dist='\0';
		int ret=MultiByteToWideChar(CP_OEMCP,NULL,src,(int)len,dist,(int)distsize);
		dist[ret]='\0';
		wstring sret=dist;
		delete [] dist;
		return sret;
	}
	static string _crtconvertwinu2a(const wstring & src, size_t len=string::npos) {
		if (len == string::npos) len=src.size();
		size_t distsize=(len+1)*2;
		char *dist=new char[distsize];
		*dist='\0';
		int ret=WideCharToMultiByte(CP_ACP,0,src.c_str(),(int)len,dist,(int)distsize,NULL,NULL);
		dist[ret]='\0';
		string sret=dist;
		delete [] dist;
		return sret;
	}
#endif
	static wstring crtu82ucs(const string &src, size_t size=string::npos) {
		wstring dst;
	#if defined(_WIN32) || defined(__MINGW32__) || defined(_CYGWIN_)
		#define _crtu82ucs(S, D, C) crtu82u16(S, D, C)
	  std::vector<uint16_t> dst_v;
	#else
		#define _crtu82ucs(S, D, C) crtu82u32(S, D, C)
		std::vector<uint32_t> dst_v;
	#endif
		if (size==string::npos) size=src.size();
    	if (_crtu82ucs(src.c_str(), dst_v, size)) dst.append(dst_v.begin(), dst_v.end());
    #undef _crtu82ucs
    	return dst;
	}
	static string crtucs2u8(const wstring &src, size_t size=string::npos) {
		std::string dst;
	#if defined(_WIN32) || defined(__MINGW32__) || defined(_CYGWIN_)
		#define _crtucs2u8(S, D, C) crtu162u8(S, D, C)
		std::vector<uint16_t> src_v(src.begin(), src.end());
	#else
		#define _crtucs2u8(S, D, C) crtu322u8(S, D, C)
		std::vector<uint32_t> src_v(src.begin(), src.end());
	#endif
		if (size==string::npos) size=src.size();
		_crtucs2u8(&(src_v[0]), size, dst);
	#undef _crtucs2u8
		return dst;
	}
	static string crtconvert(const string& src , const char* fromcode, const char* tocode) {
		string result;
	#ifdef _WIN32
		if (strcmp(fromcode,"GBK")==0 && strcmp(tocode,"UTF-8")==0)
			return crtucs2u8(_crtconvertwina2u(src.c_str(),src.length()));
		if (strcmp(fromcode,"UTF-8")==0 && strcmp(tocode,"GBK")==0)
			return _crtconvertwinu2a(crtu82ucs(src,src.length()));
	#else
		#ifdef _CRTUSEICONV
			char buff[1024] = "";
			//#if _LIBICONV_VERSION >= 0x0109
			//	const char * ps = src.c_str();
			//#else
				char *ps = const_cast<char*>(src.c_str());
			//#endif
			size_t len1=strlen(ps),len2;
			int r;
			char *pd;
			do {
				pd = buff;
				len2=sizeof(buff);
				iconv_t cd=iconv_open(tocode,fromcode);
				if (cd == (void *)-1) return "";
				r = iconv(cd, &ps, &len1, &pd, &len2);
				iconv_close(cd);
				if (r == -1 && E2BIG != errno) return "";
				int count = sizeof(buff) - len2;
				result.append(buff, buff+count);
			}
			while (len1 > 0);
		#endif
	#endif
		return result;
	}
	static string crta2u8(const char * src, size_t len=string::npos) {
		if (!src || !*src) return "";
		string ss;
		if (len == string::npos)
			ss = src;
		else
			ss = string(src, src + len);
		return crtconvert(ss, "GBK", "UTF-8");
	}
	static string crtu82a(const char * src, size_t len=string::npos) {
		if (!src || !*src) return "";
		string ss;
		if (len == string::npos)
			ss = src;
		else
			ss = string(src, src + len);
		return crtconvert(src, "UTF-8", "GBK");
	}
	static wstring crta2ucs(const char * src, size_t len=string::npos) {
	#if defined(_WIN32) || defined(__MINGW32__) || defined(_CYGWIN_)
		return _crtconvertwina2u(src,len);
	#else
		return crtu82ucs(crta2u8(src,len));
	#endif
	}
	static string crtucs2a(const wstring &src, size_t len=string::npos) {
	#if defined(_WIN32) || defined(__MINGW32__) || defined(_CYGWIN_)
		return _crtconvertwinu2a(src,len);
	#else
		return crtu82a(crtucs2u8(src,len).c_str());
	#endif
	}
	/**
	* @brief ȷ���ַ����Ƿ�ΪUTF-8����
	* @param src Դ�ַ���
	* @return �Ƿ�ΪUTF8
	*/
	/**
		��ΪUTF-8�����и�����:
			UNICODE������б���С��0X7F��UTF-8����Ϊ�䱾��(ռһ���ֽ�),
			>=0x80�����λ������1��λ����ʾ���ַ�ռ�õ��ֽ���,�����ֽھ���10��ͷ,
			��������ַ�������Ϊ1110xxxx   10xxxxxx(������)����ʽ��
			���������������ı�,��ſ����ж��Ƿ�UTF-8������,����Ȼ����ʮ��ȷ��
	*/
	static bool is_charset_maybe_utf8(const char *src)
	{
		int len=(int)strlen(src);
		const unsigned char *s=(const unsigned char *)src;
		for(int i=0;i<len;i++) {
			unsigned char ch=*(s+i);
			if (ch&0x80 && (ch&0xF0)!=0xE0 && (ch&0xC0)!=0x80) return false;
		}
		return true;
	}
};
