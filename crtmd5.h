/*
  Copyright (C) 1999, 2002 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com

 */
/* $Id: md5.h,v 1.4 2002/04/13 19:20:28 lpd Exp $ */
/*
  Independent implementation of MD5 (RFC 1321).

  This code implements the MD5 Algorithm defined in RFC 1321, whose
  text is available at
	http://www.ietf.org/rfc/rfc1321.txt
  The code is derived from the text of the RFC, including the test suite
  (section A.5) but excluding the rest of Appendix A.  It does not include
  any code or documentation that is identified in the RFC as being
  copyrighted.

  The original and principal author of md5.h is L. Peter Deutsch
  <ghost@aladdin.com>.  Other authors are noted in the change history
  that follows (in reverse chronological order):

  2002-04-13 lpd Removed support for non-ANSI compilers; removed
	references to Ghostscript; clarified derivation from RFC 1321;
	now handles byte order either statically or dynamically.
  1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
  1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5);
	added conditionalization for C++ compilation from Martin
	Purschke <purschke@bnl.gov>.
  1999-05-03 lpd Original version.
 */

#pragma once
#include "crtlib.h"

namespace crtfun{
	class crtmd5{
	public:
		#define MD5_DIGEST_LENGTH 16
		typedef struct _md5buffer
		{
			_md5buffer()
			{
				memset(m,0,sizeof(m));
			}
			unsigned char m[MD5_DIGEST_LENGTH];
			bool operator < (const _md5buffer buf) const
			{
				return memcmp(m,buf.m,MD5_DIGEST_LENGTH)<0;
			}
		}md5buffer,*pmd5buffer;

		static void md5(const void *data,unsigned int datalen,pmd5buffer ret)
		{
			memset(ret,0,sizeof(md5buffer));
			md5_state_t state;
			md5_init(&state);
			md5_append(&state,(const unsigned char *)data,datalen);
			md5_finish(&state,(md5_byte_t *)ret);
		}

		static string md52str(const void *data,unsigned int datalen)
		{
			md5buffer ret;
			md5(data,datalen,&ret);
			return md52str(&ret);
		}
		static string md5str2str(const char *data)
		{
			return md52str(data,(unsigned int)strlen(data));
		}
		static void md5file(const char *filename,pmd5buffer ret)
		{
			memset(ret,0,sizeof(md5buffer));
			FILE *fp=fopen(filename,"rb");
			int nRead=0;
			if (!fp) return;
			unsigned char fbuf[4096];
			md5_state_t state;
			md5_init(&state);
			while((nRead = (int)fread(fbuf,1,sizeof(fbuf),fp))>0)
				md5_append(&state,fbuf,nRead);
			md5_finish(&state,(md5_byte_t *)ret);
			fclose(fp);
		}
		static string md5file2str(const char *filename)
		{
			md5buffer ret;
			md5file(filename,&ret);
			return md52str(&ret);
		}
		static string md52str(const pmd5buffer m)
		{
			pmd5buffer md=(pmd5buffer)m;
			char buf[33];
			sprintf(buf,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			md->m[0],md->m[1],md->m[2],md->m[3],md->m[4],md->m[5],md->m[6],md->m[7],
			md->m[8],md->m[9],md->m[10],md->m[11],md->m[12],md->m[13],md->m[14],md->m[15]);
			*(buf+32)='\0'; //last must be zero
			return buf;
		}
		static bool issame(const pmd5buffer buf1,const pmd5buffer buf2)
		{
			return memcmp(buf1,buf2,MD5_DIGEST_LENGTH)==0;
		}
		static bool issamestr(const char *buf1,const char *buf2)
		{
#ifdef _WIN32
			return stricmp(buf1,buf2)==0;
#else
			return strcasecmp(buf1,buf2)==0;
#endif
		}
		static bool isempty(const pmd5buffer md)
		{
			if (!md) return true;
			return (memcmp(md,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",MD5_DIGEST_LENGTH)==0);
		}
		static bool isemptystr(const char *md)
		{
			if (!md) return true;
			return (memcmp(md,"0000000000000000",MD5_DIGEST_LENGTH)==0);
		}
	protected:
		/*
		 * This package supports both compile-time and run-time determination of CPU
		 * byte order.  If ARCH_IS_BIG_ENDIAN is defined as 0, the code will be
		 * compiled to run only on little-endian CPUs; if ARCH_IS_BIG_ENDIAN is
		 * defined as non-zero, the code will be compiled to run only on big-endian
		 * CPUs; if ARCH_IS_BIG_ENDIAN is not defined, the code will be compiled to
		 * run on either big- or little-endian CPUs, but will run slightly less
		 * efficiently on either one than if ARCH_IS_BIG_ENDIAN is defined.
		 */
		#undef BYTE_ORDER	/* 1 = big-endian, -1 = little-endian, 0 = unknown */
		#ifdef ARCH_IS_BIG_ENDIAN
		#  define BYTE_ORDER (ARCH_IS_BIG_ENDIAN ? 1 : -1)
		#else
		#  define BYTE_ORDER 0
		#endif

		#define T_MASK ((md5_word_t)~0)
		#define md5T1 /* 0xd76aa478 */ (T_MASK ^ 0x28955b87)
		#define md5T2 /* 0xe8c7b756 */ (T_MASK ^ 0x173848a9)
		#define md5T3    0x242070db
		#define md5T4 /* 0xc1bdceee */ (T_MASK ^ 0x3e423111)
		#define md5T5 /* 0xf57c0faf */ (T_MASK ^ 0x0a83f050)
		#define md5T6    0x4787c62a
		#define md5T7 /* 0xa8304613 */ (T_MASK ^ 0x57cfb9ec)
		#define md5T8 /* 0xfd469501 */ (T_MASK ^ 0x02b96afe)
		#define md5T9    0x698098d8
		#define md5T10 /* 0x8b44f7af */ (T_MASK ^ 0x74bb0850)
		#define md5T11 /* 0xffff5bb1 */ (T_MASK ^ 0x0000a44e)
		#define md5T12 /* 0x895cd7be */ (T_MASK ^ 0x76a32841)
		#define md5T13    0x6b901122
		#define md5T14 /* 0xfd987193 */ (T_MASK ^ 0x02678e6c)
		#define md5T15 /* 0xa679438e */ (T_MASK ^ 0x5986bc71)
		#define md5T16    0x49b40821
		#define md5T17 /* 0xf61e2562 */ (T_MASK ^ 0x09e1da9d)
		#define md5T18 /* 0xc040b340 */ (T_MASK ^ 0x3fbf4cbf)
		#define md5T19    0x265e5a51
		#define md5T20 /* 0xe9b6c7aa */ (T_MASK ^ 0x16493855)
		#define md5T21 /* 0xd62f105d */ (T_MASK ^ 0x29d0efa2)
		#define md5T22    0x02441453
		#define md5T23 /* 0xd8a1e681 */ (T_MASK ^ 0x275e197e)
		#define md5T24 /* 0xe7d3fbc8 */ (T_MASK ^ 0x182c0437)
		#define md5T25    0x21e1cde6
		#define md5T26 /* 0xc33707d6 */ (T_MASK ^ 0x3cc8f829)
		#define md5T27 /* 0xf4d50d87 */ (T_MASK ^ 0x0b2af278)
		#define md5T28    0x455a14ed
		#define md5T29 /* 0xa9e3e905 */ (T_MASK ^ 0x561c16fa)
		#define md5T30 /* 0xfcefa3f8 */ (T_MASK ^ 0x03105c07)
		#define md5T31    0x676f02d9
		#define md5T32 /* 0x8d2a4c8a */ (T_MASK ^ 0x72d5b375)
		#define md5T33 /* 0xfffa3942 */ (T_MASK ^ 0x0005c6bd)
		#define md5T34 /* 0x8771f681 */ (T_MASK ^ 0x788e097e)
		#define md5T35    0x6d9d6122
		#define md5T36 /* 0xfde5380c */ (T_MASK ^ 0x021ac7f3)
		#define md5T37 /* 0xa4beea44 */ (T_MASK ^ 0x5b4115bb)
		#define md5T38    0x4bdecfa9
		#define md5T39 /* 0xf6bb4b60 */ (T_MASK ^ 0x0944b49f)
		#define md5T40 /* 0xbebfbc70 */ (T_MASK ^ 0x4140438f)
		#define md5T41    0x289b7ec6
		#define md5T42 /* 0xeaa127fa */ (T_MASK ^ 0x155ed805)
		#define md5T43 /* 0xd4ef3085 */ (T_MASK ^ 0x2b10cf7a)
		#define md5T44    0x04881d05
		#define md5T45 /* 0xd9d4d039 */ (T_MASK ^ 0x262b2fc6)
		#define md5T46 /* 0xe6db99e5 */ (T_MASK ^ 0x1924661a)
		#define md5T47    0x1fa27cf8
		#define md5T48 /* 0xc4ac5665 */ (T_MASK ^ 0x3b53a99a)
		#define md5T49 /* 0xf4292244 */ (T_MASK ^ 0x0bd6ddbb)
		#define md5T50    0x432aff97
		#define md5T51 /* 0xab9423a7 */ (T_MASK ^ 0x546bdc58)
		#define md5T52 /* 0xfc93a039 */ (T_MASK ^ 0x036c5fc6)
		#define md5T53    0x655b59c3
		#define md5T54 /* 0x8f0ccc92 */ (T_MASK ^ 0x70f3336d)
		#define md5T55 /* 0xffeff47d */ (T_MASK ^ 0x00100b82)
		#define md5T56 /* 0x85845dd1 */ (T_MASK ^ 0x7a7ba22e)
		#define md5T57    0x6fa87e4f
		#define md5T58 /* 0xfe2ce6e0 */ (T_MASK ^ 0x01d3191f)
		#define md5T59 /* 0xa3014314 */ (T_MASK ^ 0x5cfebceb)
		#define md5T60    0x4e0811a1
		#define md5T61 /* 0xf7537e82 */ (T_MASK ^ 0x08ac817d)
		#define md5T62 /* 0xbd3af235 */ (T_MASK ^ 0x42c50dca)
		#define md5T63    0x2ad7d2bb
		#define md5T64 /* 0xeb86d391 */ (T_MASK ^ 0x14792c6e)

		typedef unsigned char md5_byte_t; /* 8-bit byte */
		typedef unsigned int md5_word_t; /* 32-bit word */

		/* Define the state of the MD5 Algorithm. */
		typedef struct md5_state_s {
			md5_word_t count[2];	/* message length in bits, lsw first */
			md5_word_t abcd[4];		/* digest buffer */
			md5_byte_t buf[64];		/* accumulate block */
		} md5_state_t;

		static void md5_process(md5_state_t *pms, const md5_byte_t *data /*[64]*/)
		{
			md5_word_t
			a = pms->abcd[0], b = pms->abcd[1],
			c = pms->abcd[2], d = pms->abcd[3];
			md5_word_t t;
		#if BYTE_ORDER > 0
			/* Define storage only for big-endian CPUs. */
			md5_word_t X[16];
		#else
			/* Define storage for little-endian or both types of CPUs. */
			md5_word_t xbuf[16];
			const md5_word_t *X;
		#endif

			{
		#if BYTE_ORDER == 0
			/*
			 * Determine dynamically whether this is a big-endian or
			 * little-endian machine, since we can use a more efficient
			 * algorithm on the latter.
			 */
			static const int w = 1;

			if (*((const md5_byte_t *)&w)) /* dynamic little-endian */
		#endif
		#if BYTE_ORDER <= 0		/* little-endian */
			{
				/*
				 * On little-endian machines, we can process properly aligned
				 * data without copying it.
				 */
				if (!((data - (const md5_byte_t *)0) & 3)) {
				/* data are properly aligned */
				X = (const md5_word_t *)data;
				} else {
				/* not aligned */
				memcpy(xbuf, data, 64);
				X = xbuf;
				}
			}
		#endif
		#if BYTE_ORDER == 0
			else			/* dynamic big-endian */
		#endif
		#if BYTE_ORDER >= 0		/* big-endian */
			{
				/*
				 * On big-endian machines, we must arrange the bytes in the
				 * right order.
				 */
				const md5_byte_t *xp = data;
				int i;

		#  if BYTE_ORDER == 0
				X = xbuf;		/* (dynamic only) */
		#  else
		#    define xbuf X		/* (static only) */
		#  endif
				for (i = 0; i < 16; ++i, xp += 4)
				xbuf[i] = xp[0] + (xp[1] << 8) + (xp[2] << 16) + (xp[3] << 24);
			}
		#endif
			}

		#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

			/* Round 1. */
			/* Let [abcd k s i] denote the operation
			   a = b + ((a + F(b,c,d) + X[k] + T[i]) <<< s). */
		#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
		#define SET(a, b, c, d, k, s, Ti)\
		  t = a + F(b,c,d) + X[k] + Ti;\
		  a = ROTATE_LEFT(t, s) + b
			/* Do the following 16 operations. */
			SET(a, b, c, d,  0,  7,  md5T1);
			SET(d, a, b, c,  1, 12,  md5T2);
			SET(c, d, a, b,  2, 17,  md5T3);
			SET(b, c, d, a,  3, 22,  md5T4);
			SET(a, b, c, d,  4,  7,  md5T5);
			SET(d, a, b, c,  5, 12,  md5T6);
			SET(c, d, a, b,  6, 17,  md5T7);
			SET(b, c, d, a,  7, 22,  md5T8);
			SET(a, b, c, d,  8,  7,  md5T9);
			SET(d, a, b, c,  9, 12, md5T10);
			SET(c, d, a, b, 10, 17, md5T11);
			SET(b, c, d, a, 11, 22, md5T12);
			SET(a, b, c, d, 12,  7, md5T13);
			SET(d, a, b, c, 13, 12, md5T14);
			SET(c, d, a, b, 14, 17, md5T15);
			SET(b, c, d, a, 15, 22, md5T16);
		#undef SET

			 /* Round 2. */
			 /* Let [abcd k s i] denote the operation
				  a = b + ((a + G(b,c,d) + X[k] + T[i]) <<< s). */
		#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
		#define SET(a, b, c, d, k, s, Ti)\
		  t = a + G(b,c,d) + X[k] + Ti;\
		  a = ROTATE_LEFT(t, s) + b
			 /* Do the following 16 operations. */
			SET(a, b, c, d,  1,  5, md5T17);
			SET(d, a, b, c,  6,  9, md5T18);
			SET(c, d, a, b, 11, 14, md5T19);
			SET(b, c, d, a,  0, 20, md5T20);
			SET(a, b, c, d,  5,  5, md5T21);
			SET(d, a, b, c, 10,  9, md5T22);
			SET(c, d, a, b, 15, 14, md5T23);
			SET(b, c, d, a,  4, 20, md5T24);
			SET(a, b, c, d,  9,  5, md5T25);
			SET(d, a, b, c, 14,  9, md5T26);
			SET(c, d, a, b,  3, 14, md5T27);
			SET(b, c, d, a,  8, 20, md5T28);
			SET(a, b, c, d, 13,  5, md5T29);
			SET(d, a, b, c,  2,  9, md5T30);
			SET(c, d, a, b,  7, 14, md5T31);
			SET(b, c, d, a, 12, 20, md5T32);
		#undef SET

			 /* Round 3. */
			 /* Let [abcd k s t] denote the operation
				  a = b + ((a + H(b,c,d) + X[k] + T[i]) <<< s). */
		#define H(x, y, z) ((x) ^ (y) ^ (z))
		#define SET(a, b, c, d, k, s, Ti)\
		  t = a + H(b,c,d) + X[k] + Ti;\
		  a = ROTATE_LEFT(t, s) + b
			 /* Do the following 16 operations. */
			SET(a, b, c, d,  5,  4, md5T33);
			SET(d, a, b, c,  8, 11, md5T34);
			SET(c, d, a, b, 11, 16, md5T35);
			SET(b, c, d, a, 14, 23, md5T36);
			SET(a, b, c, d,  1,  4, md5T37);
			SET(d, a, b, c,  4, 11, md5T38);
			SET(c, d, a, b,  7, 16, md5T39);
			SET(b, c, d, a, 10, 23, md5T40);
			SET(a, b, c, d, 13,  4, md5T41);
			SET(d, a, b, c,  0, 11, md5T42);
			SET(c, d, a, b,  3, 16, md5T43);
			SET(b, c, d, a,  6, 23, md5T44);
			SET(a, b, c, d,  9,  4, md5T45);
			SET(d, a, b, c, 12, 11, md5T46);
			SET(c, d, a, b, 15, 16, md5T47);
			SET(b, c, d, a,  2, 23, md5T48);
		#undef SET

			 /* Round 4. */
			 /* Let [abcd k s t] denote the operation
				  a = b + ((a + I(b,c,d) + X[k] + T[i]) <<< s). */
		#define I(x, y, z) ((y) ^ ((x) | ~(z)))
		#define SET(a, b, c, d, k, s, Ti)\
		  t = a + I(b,c,d) + X[k] + Ti;\
		  a = ROTATE_LEFT(t, s) + b
			 /* Do the following 16 operations. */
			SET(a, b, c, d,  0,  6, md5T49);
			SET(d, a, b, c,  7, 10, md5T50);
			SET(c, d, a, b, 14, 15, md5T51);
			SET(b, c, d, a,  5, 21, md5T52);
			SET(a, b, c, d, 12,  6, md5T53);
			SET(d, a, b, c,  3, 10, md5T54);
			SET(c, d, a, b, 10, 15, md5T55);
			SET(b, c, d, a,  1, 21, md5T56);
			SET(a, b, c, d,  8,  6, md5T57);
			SET(d, a, b, c, 15, 10, md5T58);
			SET(c, d, a, b,  6, 15, md5T59);
			SET(b, c, d, a, 13, 21, md5T60);
			SET(a, b, c, d,  4,  6, md5T61);
			SET(d, a, b, c, 11, 10, md5T62);
			SET(c, d, a, b,  2, 15, md5T63);
			SET(b, c, d, a,  9, 21, md5T64);
		#undef SET

			 /* Then perform the following additions. (That is increment each
				of the four registers by the value it had before this block
				was started.) */
			pms->abcd[0] += a;
			pms->abcd[1] += b;
			pms->abcd[2] += c;
			pms->abcd[3] += d;
		}

		/* Initialize the algorithm. */
		static void md5_init(md5_state_t *pms)
		{
			pms->count[0] = pms->count[1] = 0;
			pms->abcd[0] = 0x67452301;
			pms->abcd[1] = /*0xefcdab89*/ T_MASK ^ 0x10325476;
			pms->abcd[2] = /*0x98badcfe*/ T_MASK ^ 0x67452301;
			pms->abcd[3] = 0x10325476;
		}

		/* Append a string to the message. */
		static void md5_append(md5_state_t *pms, const md5_byte_t *data, int nbytes)
		{
			const md5_byte_t *p = data;
			int left = nbytes;
			int offset = (pms->count[0] >> 3) & 63;
			md5_word_t nbits = (md5_word_t)(nbytes << 3);

			if (nbytes <= 0)
			return;

			/* Update the message length. */
			pms->count[1] += nbytes >> 29;
			pms->count[0] += nbits;
			if (pms->count[0] < nbits)
			pms->count[1]++;

			/* Process an initial partial block. */
			if (offset) {
			int copy = (offset + nbytes > 64 ? 64 - offset : nbytes);

			memcpy(pms->buf + offset, p, copy);
			if (offset + copy < 64)
				return;
			p += copy;
			left -= copy;
			md5_process(pms, pms->buf);
			}

			/* Process full blocks. */
			for (; left >= 64; p += 64, left -= 64) md5_process(pms, p);

			/* Process a final partial block. */
			if (left) memcpy(pms->buf, p, left);
		}

		/* Finish the message and return the digest. */
		static void md5_finish(md5_state_t *pms, md5_byte_t digest[16])
		{
			static const md5_byte_t pad[64] = {
				0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			};
			md5_byte_t data[8];
			int i;

			/* Save the length before padding. */
			for (i = 0; i < 8; ++i)
			data[i] = (md5_byte_t)(pms->count[i >> 2] >> ((i & 3) << 3));
			/* Pad to 56 bytes mod 64. */
			md5_append(pms, pad, ((55 - (pms->count[0] >> 3)) & 63) + 1);
			/* Append the length. */
			md5_append(pms, data, 8);
			for (i = 0; i < 16; ++i) digest[i] = (md5_byte_t)(pms->abcd[i >> 2] >> ((i & 3) << 3));
		}
	};
}
