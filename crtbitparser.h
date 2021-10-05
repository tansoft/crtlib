#pragma once

#include "crtlib.h"

#undef setbit

namespace crtfun {
	/**
	* @brief 位处理类
	* @author Barry(barrytan@21cn.com,QQ:20962493)
	*/
	/**<pre>
	仅处理1字节的数据
	使用Sample：
	</pre>*/
	class crtbitreader
	{
	public:
		crtbitreader():byte(0),bitpos(0) {}
		crtbitreader& reset() {
			bitpos = byte = (unsigned char) (0);
			return *this;
		}
		crtbitreader& fillzero(unsigned char const count) {return read((unsigned char)0,(unsigned char)0,count);}	
		crtbitreader& fill(unsigned char const count) {return read((unsigned char)0xFF,(unsigned char)0,count);}
		crtbitreader& read(unsigned char bits, unsigned char beg, unsigned char count) {
			for (unsigned char i = 0 ;i < count; ++i) {
				byte <<= 1;
				++bitpos;
				unsigned char bit = ( (bits >> (7 - beg - i)) & 0x01);
				byte |= bit;
			}
			return *this;
		}
		unsigned char getbyte() const {return byte;}
	private:
		unsigned char byte;
		unsigned char bitpos;
	};
	/**
	* @brief 位处理类
	* @author Barry(barrytan@21cn.com,QQ:20962493)
	* @2008-02-23 新建类
	* @2008-11-30 增加网络字节序转换参数，增加rollbuffer操作等函数
	*/
	/**<pre>
	使用Sample：
	</pre>*/
	class crtbitparser
	{
	public:
		crtbitparser() {
			m_pbuffer=NULL;
			m_nbufferlen=0;
			m_nbitlen=0;
			m_ncachedfristnosetbit=0;
		}
		virtual ~crtbitparser() {
			if (m_pbuffer)
			{
				delete [] m_pbuffer;
				m_pbuffer=NULL;
			}
			m_nbufferlen=0;
			m_nbitlen=0;
			m_ncachedfristnosetbit=0;
		}
		//all buffer is empty
		bool assignbitlen(unsigned int nbitlen){
			if (!m_pbuffer || nbitlen > m_nbufferlen*8) {
				if (m_pbuffer) {
					delete [] m_pbuffer;
					m_nbufferlen=0;
					m_nbitlen=0;
				}
				unsigned int size=max(nbitlen/4,1024);
				m_pbuffer=new unsigned char[size];//init twice length of setbuffer
				if (!m_pbuffer) return false;
				m_nbufferlen=size;
			}
			memset(m_pbuffer,0,m_nbufferlen);
			m_nbitlen=nbitlen;
			m_ncachedfristnosetbit=0;//reflush it
			return true;
		}
		bool setbuffer(const void *buffer,unsigned int nbitlen){
			if (!m_pbuffer || nbitlen > m_nbufferlen*8) {
				if (m_pbuffer) {
					delete [] m_pbuffer;
					m_nbufferlen=0;
					m_nbitlen=0;
				}
				unsigned int size=max(nbitlen/4,1024);
				m_pbuffer=new unsigned char[size];//init twice length of setbuffer
				if (!m_pbuffer) return false;
				memset(m_pbuffer,0,size);
				m_nbufferlen=size;
			}
			if (nbitlen>0) {
				unsigned int copy=(nbitlen+7)/8;
				//may be copy to it self,memmove is safey
				memmove(m_pbuffer,buffer,copy);
				if (m_nbufferlen>copy) memset(m_pbuffer+copy,0,m_nbufferlen-copy);
			} else if (m_nbufferlen>0)
				memset(m_pbuffer,0,m_nbufferlen);
			m_nbitlen=nbitlen;
			m_ncachedfristnosetbit=0;//reflush it
			return true;
		}
		//返回的大小是字节数大小，因为函数返回的内容如果进行更改，将会可能使m_ncachedfristnosetbit失效
		const void *getbuffer(unsigned int *pbufferlen) const
		{
			if (pbufferlen)
				*pbufferlen=(m_nbitlen+7)/8;
			return (const uint8_t *)m_pbuffer;
		}

		//bH为本地顺序还是网络顺序
		inline bool getbit(unsigned int nbitoffest,bool hostorder=false) const
		{
			const unsigned char maskmap[]={0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80};
			const unsigned char maskmapn[]={0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};
			if (nbitoffest>=m_nbitlen) return false;
			unsigned int off=nbitoffest/8;
			unsigned char mask;
			if (hostorder)
				mask=maskmap[nbitoffest%8];
			else
				mask=maskmapn[nbitoffest%8];
			return ((*(m_pbuffer+off))&mask)!=0;
		}

		//获取nbitoffest开始ncount位组成的二进制值,最多支持获取sizeof(uint32_t)*8位,bH为本地顺序还是网络顺序
		uint32_t getbit2u32(unsigned int nbitoffest,unsigned int ncount,bool hostorder=false) {
			if (ncount>sizeof(uint32_t)*8) return 0;
			uint32_t ret=0;
			for(unsigned int i=0;i<ncount;i++) {
				if (getbit(nbitoffest+i,hostorder)) {
					if (hostorder) ret|=(1<<i);
					else ret|=(1<<(ncount-1-i));
				}
			}
			return ret;
		}
		bool setbit(unsigned int nbitoffest,bool bbitvalue,bool hostorder=false){
			const unsigned char maskmap[]={0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80};
			const unsigned char maskmapn[]={0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};
			if (nbitoffest>=m_nbufferlen*8) {
				//Realloc buffer
				unsigned int size=max(nbitoffest/4,1024);
				unsigned char *tmp=new unsigned char[size];
				if (!tmp) return false;
				memset(tmp,0,size);
				memcpy(tmp,m_pbuffer,m_nbufferlen);
				delete [] m_pbuffer;
				m_pbuffer=tmp;
				m_nbufferlen=size;
			}
			unsigned int off=nbitoffest/8;
			unsigned char mask;
			if (hostorder)
				mask=maskmap[nbitoffest%8];
			else
				mask=maskmapn[nbitoffest%8];
			if (bbitvalue)
				*(m_pbuffer+off)=(*(m_pbuffer+off))|mask;
			else
				*(m_pbuffer+off)=(*(m_pbuffer+off))&(~mask);
			if (m_nbitlen<nbitoffest+1) m_nbitlen=nbitoffest+1;
			m_ncachedfristnosetbit=0;//reflush it
			return true;
		}
		//bool setbit(unsigned int nbitoffest,bool bbitvalue){return setbit(nbitoffest,bbitvalue,false);}
		inline bool setbit(bool bbitvalue) {return setbit(m_nbitlen,bbitvalue,false);}
		inline bool setbits(unsigned int nbitoffest,bool *bbitvalue,unsigned int ncount){
			for(unsigned int i=0;i<ncount;i++)
				if (!setbit(nbitoffest+i,*(bbitvalue+i),false)) return false;
			return true;
		}
		//滚动bm，取消前面的n个字节数据，使用字节对齐是为了加速
		void rollbuffer(unsigned int nbytes){
			if (m_nbitlen>0) {
				if (nbytes*8>=m_nbitlen)
					clear();
				else {
					m_nbitlen-=nbytes*8;
					unsigned int copyoff=(m_nbitlen+7)/8;
					memmove(m_pbuffer,m_pbuffer+nbytes,copyoff);
					if (m_nbufferlen>copyoff) memset(m_pbuffer+copyoff,0,m_nbufferlen-copyoff);
				}
			}
		}
		string print() const {
			string ret;
			unsigned int plen=(m_nbitlen+7)/8;
			char buf[5];
			for(unsigned int i=0;i<plen;i++) {
				sprintf(buf,"%02x",*(m_pbuffer+i));
				ret+=buf;
			}
			return ret;
		}

		inline void clear() {
			m_nbitlen=0;
			if (m_nbufferlen>0) memset(m_pbuffer,0,m_nbufferlen);
			m_ncachedfristnosetbit=0;
		}

		//把bitset传到某buffer中，以buflen为准，缓冲不够只返回前面的，多出的地方memset为0
		bool copytobuffer(void *outbuf,unsigned int buflen) {
			unsigned int copybuf=(m_nbitlen+7)/8;
			//如果buffer过小，则只返回前面的
			if (copybuf>buflen) copybuf=buflen;
			memcpy(outbuf,m_pbuffer,copybuf);
			if (buflen>copybuf) memset((char *)outbuf+copybuf,0,buflen-copybuf);
			return true;
		}
		inline unsigned int getbitlen(){return m_nbitlen;}
		bool isallbitset(unsigned int nbitlen) const {
			unsigned int fastcheck=nbitlen/8;
			unsigned int nReMain=nbitlen%8;
			unsigned int i;
			if (m_ncachedfristnosetbit!=0)
				i=(m_ncachedfristnosetbit-1)/8;//在m_ncachedfristnosetbit之前的肯定是已完成的
			for(i=0;i<fastcheck;i++)
				if (*(m_pbuffer+i)!=0xff) return false;
			fastcheck*=8;
			for(i=0;i<nReMain;i++)
				if (!getbit(fastcheck+i)) return false;
			return true;
		}
		bool setmaxbit(unsigned int nbitlen) {
			if (m_nbitlen!=nbitlen) {
				if (m_nbitlen<nbitlen) {
					if (nbitlen<m_nbufferlen*8)
						//buffer is enough,just set m_nbitlen
						m_nbitlen=nbitlen;
					else {
						//inc the buffer
						for(unsigned int i=m_nbitlen;i<nbitlen;i++) setbit(i,0);
					}
				} else {
					//reduce the buffer,just reset the bytes which may be reuse
					for(unsigned int i=nbitlen;i<nbitlen+8 && i<m_nbitlen;i++) setbit(i,0);
					unsigned int copy=(nbitlen+7)/8;
					if (m_nbufferlen>copy) memset(m_pbuffer+copy,0,m_nbufferlen-copy);
					m_nbitlen=nbitlen;
				}
				return true;
			}
			return false;
		}
		unsigned int getfirstnosetbit(unsigned int nstartbit) const {
			if (m_ncachedfristnosetbit!=0 && nstartbit<m_ncachedfristnosetbit) return m_ncachedfristnosetbit;
			unsigned int nupbit=nstartbit;
			while(nupbit<m_nbitlen) {
				if (nupbit%8==0) {
					//if at the bytes start
					if (*(m_pbuffer+nupbit/8)==0xff)
						nupbit+=8;
					else {
						if (!getbit(nupbit)) break;
						nupbit++;
					}
				} else {
					if (!getbit(nupbit)) break;
					nupbit++;
				}
			}
			if (nstartbit==0) m_ncachedfristnosetbit=nupbit;
			return nupbit;
		}
	private:
		unsigned char *m_pbuffer;
		unsigned int m_nbitlen;
		unsigned int m_nbufferlen;
		mutable volatile unsigned int m_ncachedfristnosetbit;		///<用于加速获取最后没有设置位
	};

};
