#pragma once

#include "crtlib.h"
#include "crtstring.h"

namespace crtfun {
	/**
	* @brief Base64处理
	* @author Barry(barrytan@21cn.com,QQ:20962493)
	*/
	/**<pre>
	使用Sample：
	</pre>*/
	class crtbase64{
	public:
		static string encode_str(string str) {return encode((const void *)str.c_str(),str.length());}
		static string encode(const void *encbuf,size_t bufsize)
		{
			char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			string result;
			size_t size = (bufsize + 2) / 57 * 2;
			size += bufsize % 3 != 0 ? (bufsize - bufsize % 3 + 3) / 3 * 4 : bufsize / 3 * 4;
			result.reserve(size + 1);
			const char *input=(const char*)encbuf;
			int remaining = 0;
			register int bitstorage = 0, scratch = 0;
			size_t i, lp;	
			for(i=0, lp=0; lp < bufsize; lp++)
			{
				bitstorage = (bitstorage << 8) | (input[lp] & 0xff);
				remaining += 8;
				do {
					scratch = bitstorage >> (remaining - 6) & 0x3f;
					result.append(1,base64[scratch]);
					i++;
					remaining -= 6;
				}while(remaining >= 6);
			}
			if(remaining != 0)
			{
				scratch = bitstorage << (6-remaining);
				scratch &= 0x3f;
				result.append(1,base64[scratch]);
				i++;
			}
			while(i % 4 != 0) {result.append(1,'=');i++;}
			return result;
		}
		static string decode_str(string str) {
			string ret;
			void *data;
			int size;
			if (decode(str.c_str(),&data,&size)) {
				ret.assign((const char *)data,size);
				free_ptr(data);
			}
			return ret;
		}
		static bool decode(const char *text,void **data, int *size)
		{
			unsigned char *out = NULL;
			unsigned char tmp = 0;
			const char *c;
			int tmp2 = 0;
			int len = 0, n = 0;
			if (!text || !data) return false;	
			//Normalize the input
			string input=string_replace(text,"\r","");
			input=string_replace(input,"\n","");
			input=string_replace(input,"\t","");
			c=input.c_str();
			while (*c) {
				if (*c >= 'A' && *c <= 'Z') {
					tmp = *c - 'A';
				} else if (*c >= 'a' && *c <= 'z') {
					tmp = 26 + (*c - 'a');
				} else if (*c >= '0' && *c <= 57) {
					tmp = 52 + (*c - '0');
				} else if (*c == '+') {
					tmp = 62;
				} else if (*c == '/') {
					tmp = 63;
				} else if (*c == '=') {
					if (n == 3) {
						out = (unsigned char *)realloc(out, len + 2);
						if (!out) return false;
						out[len] = (char)(tmp2 >> 10) & 0xff;
						len++;
						out[len] = (char)(tmp2 >> 2) & 0xff;
						len++;
					} else if (n == 2) {
						out = (unsigned char *)realloc(out, len + 1);
						if (!out) return false;
						out[len] = (char)(tmp2 >> 4) & 0xff;
						len++;
					}
					break;
				}
				tmp2 = ((tmp2 << 6) | (tmp & 0xff));
				n++;
				if (n == 4) {
					out = (unsigned char *)realloc(out, len + 3);
					if (!out) return false;
					out[len] = (char)((tmp2 >> 16) & 0xff);
					len++;
					out[len] = (char)((tmp2 >> 8) & 0xff);
					len++;
					out[len] = (char)(tmp2 & 0xff);
					len++;
					tmp2 = 0;
					n = 0;
				}
				c++;
			}
			out = (unsigned char *)realloc(out, len + 1);
			if (!out) return false;
			out[len] = 0;
			*data = out;
			if (size) *size = len;
			return true;
		}
	
		static void free_ptr(void *buf){free(buf);}
	};
};
