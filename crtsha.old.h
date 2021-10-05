#pragma once

#include "crtlib.h"
#include "crtstring.h"

//有场景 final 时会导致死循环，参见THSha.h

namespace crtfun {
	typedef enum _crtshatype{
		crtshatype_sha1,
		crtshatype_sha256
	}crtshatype;
	typedef struct _crtshactx{
		crtshatype type;
		unsigned int state[8];//SHA1:5 SHA256:8
		unsigned int count[2];
		unsigned char buffer[64];
		void *callbackdata;
	}crtshactx;

	class crtsha{
	public:
		static string sha_string(const char *str,crtshatype type)
		{
			crtshactx context;
			char buffer[200];
			_shainit(&context,type);
			_shaupdate(&context,(unsigned char *)str,(int)strlen(str));
			_shafinal(&context);
			_shagetresult(&context,buffer);
			return buffer;
		}
		static string sha(const void *buf,int len,crtshatype type)
		{
			crtshactx context;
			char buffer[200];
			_shainit(&context,type);
			_shaupdate(&context,(unsigned char *)buf,len);
			_shafinal(&context);
			_shagetresult(&context,buffer);
			return buffer;
		}
		static string sha_file(const char *path,crtshatype type)
		{
			int i;
			crtshactx context;
			unsigned char filebuf[16384];
			char buffer[200];
			FILE* file;
			if (!(file=fopen(path,"rb"))) return "";
			_shainit(&context,type);
			while (!feof(file))
			{
				i=(int)fread(filebuf,1,16384,file);
				_shaupdate(&context,filebuf,i);
			}
			_shafinal(&context);
			fclose(file);
			_shagetresult(&context,buffer);
			return buffer;
		}
	protected:
		static inline unsigned int SHR(unsigned int lX,int n) {return lX>>n;}
		static inline unsigned int ROTR(unsigned int lX,int n) {return (lX>>n)|(lX<<(32-n));}
		static inline unsigned int ROTL(unsigned int lX,int n) {return (lX<<n)|(lX>>(32-n));}
		static inline unsigned int Ch(unsigned int x,unsigned int y,unsigned int z) {return (x&y)^((~x)&z);}
		static inline unsigned int Maj(unsigned int x,unsigned int y,unsigned int z) {return (x&y)^(x&z)^(y&z);}
		static inline unsigned int Parity(unsigned int x,unsigned int y,unsigned int z) {return x^y^z;}
		static inline unsigned int Sigma0(unsigned int x) {return ROTR(x,2)^ROTR(x,13)^ROTR(x,22);}
		static inline unsigned int Sigma1(unsigned int x) {return ROTR(x,6)^ROTR(x,11)^ROTR(x,25);}
		static inline unsigned int Gamma0(unsigned int x) {return ROTR(x,7)^ROTR(x,18)^SHR(x,3);}
		static inline unsigned int Gamma1(unsigned int x) {return ROTR(x,17)^ROTR(x,19)^SHR(x,10);}
		static inline unsigned int AddUnsigned(unsigned int lX,unsigned int lY) {return lX+lY;}
		static void _shainit(crtshactx* context,crtshatype type) {
			unsigned int hash1[5]={0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476,0xC3D2E1F0};
			unsigned int hash256[8]={0x6A09E667,0xBB67AE85,0x3C6EF372,0xA54FF53A,0x510E527F,0x9B05688C,0x1F83D9AB,0x5BE0CD19};
			context->type=type;
			if (context->type==crtshatype_sha1)
				memcpy(context->state,hash1,sizeof(hash1));
			else
				memcpy(context->state,hash256,sizeof(hash256));
			context->count[0]=0;
			context->count[1]=0;
		}
		static void _shaupdate(crtshactx* context, unsigned char* data, unsigned int len) {
			unsigned int i,j;
			j=context->count[0];
			context->count[1]+=len;
			if (j+len>63) {
				memcpy(&context->buffer[j],data,(i=64-j));
				_shatransform(context,context->buffer);
				for(;i+63<len;i+=64)
					_shatransform(context,&data[i]);
			}
			else i=0;
			if (len-i>0) {
				memcpy(&context->buffer[j],&data[i],len-i);
				context->count[0]+=len-i;
			}
		}
		static void _shatransform(crtshactx* context, unsigned char *buffer) {
			const unsigned int k1[4]={0x5A827999,0x6ED9EBA1,0x8F1BBCDC,0xCA62C1D6};
			const unsigned int k256[64]={0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,0x3956C25B,0x59F111F1,
				0x923F82A4,0xAB1C5ED5,0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,
				0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,0xE49B69C1,0xEFBE4786,
				0x0FC19DC6,0x240CA1CC,0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,
				0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,0xC6E00BF3,0xD5A79147,
				0x06CA6351,0x14292967,0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,
				0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,0xA2BFE8A1,0xA81A664B,
				0xC24B8B70,0xC76C51A3,0xD192E819,0xD6990624,0xF40E3585,0x106AA070,
				0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,0x391C0CB3,0x4ED8AA4A,
				0x5B9CCA4F,0x682E6FF3,0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,
				0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2};
			typedef unsigned int (*_crtsha_function)(unsigned int a,unsigned int b,unsigned int c);
			const _crtsha_function ft[4]={Ch,Parity,Maj,Parity};
			for(int i=0;i<64;i+=4)
			{
				unsigned char tmp=*(buffer+i);
				*(buffer+i)=*(buffer+i+3);
				*(buffer+i+3)=tmp;
				tmp=*(buffer+i+1);
				*(buffer+i+1)=*(buffer+i+2);
				*(buffer+i+2)=tmp;
			}
			unsigned int W[80];
			unsigned int a,b,c,d,e,f,g,h,j,lt,lt1,lt2,Round=79;
			a=context->state[0];b=context->state[1];
			c=context->state[2];d=context->state[3];
			e=context->state[4];
			if (context->type==crtshatype_sha256)
			{
				f=context->state[5];g=context->state[6];h=context->state[7];
				Round=63;
			}
			for (j=0;j<=Round;j++)
			{
				if (j<16)
					W[j]=*(((unsigned int *)buffer)+j);
				else
				{
					if (context->type==crtshatype_sha1)
						W[j]=ROTL(W[j-3]^W[j-8]^W[j-14]^W[j-16],1);
					else if (context->type==crtshatype_sha256)
						W[j]=AddUnsigned(AddUnsigned(AddUnsigned(Gamma1(W[j-2]),W[j-7]),Gamma0(W[j-15])),W[j-16]);
				}
				if (context->type==crtshatype_sha1)
				{
					lt=AddUnsigned(AddUnsigned(AddUnsigned(AddUnsigned(ROTL(a,5),(ft[j/20])(b,c,d)),e),k1[j/20]),W[j]);
					e=d;d=c;
					c=ROTL(b,30);
					b=a;a=lt;
				}
				else if (context->type==crtshatype_sha256)
				{
					lt1=AddUnsigned(AddUnsigned(AddUnsigned(AddUnsigned(h,Sigma1(e)),Ch(e,f,g)),k256[j]),W[j]);
					lt2=AddUnsigned(Sigma0(a),Maj(a,b,c));
					h=g;g=f;f=e;
					e=AddUnsigned(d,lt1);
					d=c;c=b;b=a;
					a=AddUnsigned(lt1,lt2);
				}
			}
			context->state[0]=AddUnsigned(a,context->state[0]);
			context->state[1]=AddUnsigned(b,context->state[1]);
			context->state[2]=AddUnsigned(c,context->state[2]);
			context->state[3]=AddUnsigned(d,context->state[3]);
			context->state[4]=AddUnsigned(e,context->state[4]);
			if (context->type==crtshatype_sha256)
			{
				context->state[5]=AddUnsigned(f,context->state[5]);
				context->state[6]=AddUnsigned(g,context->state[6]);
				context->state[7]=AddUnsigned(h,context->state[7]);
			}
		}
		static void _shafinal(crtshactx* context) {
			unsigned int count[2];
			count[0]=context->count[1]>>29;
			count[1]=context->count[1]<<3;
			unsigned char buf[4];
			_shaupdate(context,(unsigned char *)"\200", 1);
			while(context->count[0]!=64-8) _shaupdate(context,(unsigned char *)"\0",1);
			memcpy(buf,(char *)&count[0],4);
			_shaupdate(context,(unsigned char *)&buf[3],1);
			_shaupdate(context,(unsigned char *)&buf[2],1);
			_shaupdate(context,(unsigned char *)&buf[1],1);
			_shaupdate(context,(unsigned char *)&buf[0],1);
			memcpy(buf,(char *)&count[1],4);
			_shaupdate(context,(unsigned char *)&buf[3],1);
			_shaupdate(context,(unsigned char *)&buf[2],1);
			_shaupdate(context,(unsigned char *)&buf[1],1);
			_shaupdate(context,(unsigned char *)&buf[0],1);
		}
		static void _shagetresult(crtshactx* context,char *buffer) {
			*buffer='\0';
			if (context->type==crtshatype_sha1)
				sprintf(buffer,"%08x%08x%08x%08x%08x",
					context->state[0],context->state[1],context->state[2],context->state[3],context->state[4]);
			else if (context->type==crtshatype_sha256)
				sprintf(buffer,"%08x%08x%08x%08x%08x%08x%08x%08x",
					context->state[0],context->state[1],context->state[2],context->state[3],
					context->state[4],context->state[5],context->state[6],context->state[7]);
		}

	};
};

