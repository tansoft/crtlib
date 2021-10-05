#pragma once

#include "crtlib.h"
#include "crtstring.h"

/**
* @brief 正则表达式处理
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用nettlelib类库实现
  使用Sample：
</pre>*/

namespace crtfun {
	typedef enum _crtshatype{
		crtshatype_sha1,
		crtshatype_sha256
	}crtshatype;
	typedef struct _crtshactx{
		crtshatype type;
		uint32_t state[8];//SHA1:5 SHA256:8
		uint32_t count_low,count_high;
		uint8_t block[64];
		unsigned int index;
	}crtshactx;
	#define SHA_DIGEST_LENGTH 32	//SHA1:20 SHA256:32

	class crtsha{
	public:
		static unsigned int shadigestsize(crtshatype type) {return type==crtshatype_sha1?20:32;}
		static string shastr2str(const char *str,crtshatype type)
		{
			crtshactx context;
			unsigned char buffer[SHA_DIGEST_LENGTH];
			_shainit(&context,type);
			_shaupdate(&context,(unsigned char *)str,(int)strlen(str));
			_shafinal(&context,buffer);
			return crtbin2str(buffer,shadigestsize(type));
		}
		static string sha2str(const void *buf,int len,crtshatype type)
		{
			crtshactx context;
			unsigned char buffer[SHA_DIGEST_LENGTH];
			_shainit(&context,type);
			_shaupdate(&context,(unsigned char *)buf,len);
			_shafinal(&context,buffer);
			return crtbin2str(buffer,shadigestsize(type));
		}
		static string shafile2str(const char *path,crtshatype type)
		{
			int i;
			crtshactx context;
			unsigned char filebuf[16384];
			unsigned char buffer[SHA_DIGEST_LENGTH];
			FILE* file;
			if (!(file=fopen(path,"rb"))) return "";
			_shainit(&context,type);
			while (!feof(file))
			{
				i=(int)fread(filebuf,1,16384,file);
				_shaupdate(&context,filebuf,i);
			}
			_shafinal(&context,buffer);
			fclose(file);
			return crtbin2str(buffer,shadigestsize(type));
		}
	protected:
		#define _crtshaf1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )           /* Rounds  0-19 */
		#define _crtshaf2(x,y,z)   ( x ^ y ^ z )                       /* Rounds 20-39 */
		#define _crtshaf3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )   /* Rounds 40-59 */
		#define _crtshaf4(x,y,z)   ( x ^ y ^ z )                       /* Rounds 60-79 */
		#define _crtsharotl(n,X)  ( ( (X) << (n) ) | ( (X) >> ( 32 - (n) ) ) )
		#define _crtshaexpand(W,i) ( W[ i & 15 ] = \
					_crtsharotl( 1, ( W[ i & 15 ] ^ W[ (i - 14) & 15 ] ^ \
						W[ (i - 8) & 15 ] ^ W[ (i - 3) & 15 ] ) ) )
		#define _crtshasubround(a, b, c, d, e, f, k, data) \
			( e += _crtsharotl( 5, a ) + f( b, c, d ) + k + data, b = _crtsharotl( 30, b ) )
		#define _crtsha256_rotr(n,x) ((x)>>(n) | ((x)<<(32-(n))))
		#define _crtsha256_shr(n,x) ((x)>>(n))
		#define _crtsha256_choice(x,y,z)   ( (z) ^ ( (x) & ( (y) ^ (z) ) ) ) 
		#define _crtsha256_majority(x,y,z) ( ((x) & (y)) ^ ((z) & ((x) ^ (y))) )
		#define _crtsha256_bs0(x) (_crtsha256_rotr(2,(x)) ^ _crtsha256_rotr(13,(x)) ^ _crtsha256_rotr(22,(x))) 
		#define _crtsha256_bs1(x) (_crtsha256_rotr(6,(x)) ^ _crtsha256_rotr(11,(x)) ^ _crtsha256_rotr(25,(x)))
		#define _crtsha256_s0(x) (_crtsha256_rotr(7,(x)) ^ _crtsha256_rotr(18,(x)) ^ _crtsha256_shr(3,(x)))
		#define _crtsha256_s1(x) (_crtsha256_rotr(17,(x)) ^ _crtsha256_rotr(19,(x)) ^ _crtsha256_shr(10,(x)))
		#define _crtsha256_expand(W,i) \
			( W[(i) & 15 ] += (_crtsha256_s1(W[((i)-2) & 15]) + W[((i)-7) & 15] + _crtsha256_s0(W[((i)-15) & 15])) )
		#define _crtsha256_round(a,b,c,d,e,f,g,h,k,data) do {		\
			uint32_t T = h + _crtsha256_bs1(e) + _crtsha256_choice(e,f,g) + k + data;	\
			d += T;						\
			h = T + _crtsha256_bs0(a) + _crtsha256_majority(a,b,c);			\
		} while (0)

		static void sha256_block(crtshactx *ctx, const uint8_t *block)
		{
			uint32_t data[16];
			int i;
			if (!++ctx->count_low) ++ctx->count_high;
			for (i = 0; i<16; i++, block += 4)
				data[i] = crtreadu32(block);
			sha256_transform(ctx->state, data);
		}

		static void _nettle_sha1_compress(uint32_t *state, const uint8_t *input)
		{
			uint32_t data[16];
			uint32_t A, B, C, D, E;
			int i;
			for (i = 0; i < 16; i++, input+= 4) data[i] = crtreadu32(input);
			A = state[0];B = state[1];C = state[2];D = state[3];E = state[4];
			_crtshasubround( A, B, C, D, E, _crtshaf1, 0x5A827999L, data[ 0] );
			_crtshasubround( E, A, B, C, D, _crtshaf1, 0x5A827999L, data[ 1] );
			_crtshasubround( D, E, A, B, C, _crtshaf1, 0x5A827999L, data[ 2] );
			_crtshasubround( C, D, E, A, B, _crtshaf1, 0x5A827999L, data[ 3] );
			_crtshasubround( B, C, D, E, A, _crtshaf1, 0x5A827999L, data[ 4] );
			_crtshasubround( A, B, C, D, E, _crtshaf1, 0x5A827999L, data[ 5] );
			_crtshasubround( E, A, B, C, D, _crtshaf1, 0x5A827999L, data[ 6] );
			_crtshasubround( D, E, A, B, C, _crtshaf1, 0x5A827999L, data[ 7] );
			_crtshasubround( C, D, E, A, B, _crtshaf1, 0x5A827999L, data[ 8] );
			_crtshasubround( B, C, D, E, A, _crtshaf1, 0x5A827999L, data[ 9] );
			_crtshasubround( A, B, C, D, E, _crtshaf1, 0x5A827999L, data[10] );
			_crtshasubround( E, A, B, C, D, _crtshaf1, 0x5A827999L, data[11] );
			_crtshasubround( D, E, A, B, C, _crtshaf1, 0x5A827999L, data[12] );
			_crtshasubround( C, D, E, A, B, _crtshaf1, 0x5A827999L, data[13] );
			_crtshasubround( B, C, D, E, A, _crtshaf1, 0x5A827999L, data[14] );
			_crtshasubround( A, B, C, D, E, _crtshaf1, 0x5A827999L, data[15] );
			_crtshasubround( E, A, B, C, D, _crtshaf1, 0x5A827999L, _crtshaexpand( data, 16 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf1, 0x5A827999L, _crtshaexpand( data, 17 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf1, 0x5A827999L, _crtshaexpand( data, 18 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf1, 0x5A827999L, _crtshaexpand( data, 19 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 20 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 21 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 22 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 23 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 24 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 25 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 26 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 27 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 28 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 29 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 30 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 31 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 32 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 33 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 34 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 35 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 36 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 37 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 38 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf2, 0x6ED9EBA1L, _crtshaexpand( data, 39 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 40 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 41 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 42 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 43 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 44 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 45 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 46 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 47 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 48 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 49 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 50 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 51 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 52 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 53 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 54 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 55 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 56 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 57 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 58 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf3, 0x8F1BBCDCL, _crtshaexpand( data, 59 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 60 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 61 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 62 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 63 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 64 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 65 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 66 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 67 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 68 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 69 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 70 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 71 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 72 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 73 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 74 ) );
			_crtshasubround( A, B, C, D, E, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 75 ) );
			_crtshasubround( E, A, B, C, D, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 76 ) );
			_crtshasubround( D, E, A, B, C, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 77 ) );
			_crtshasubround( C, D, E, A, B, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 78 ) );
			_crtshasubround( B, C, D, E, A, _crtshaf4, 0xCA62C1D6L, _crtshaexpand( data, 79 ) );
			state[0] += A;state[1] += B;state[2] += C;state[3] += D;state[4] += E;
		}

		static void _shainit(crtshactx* context,crtshatype type) {
			uint32_t hash1[5]={0x67452301UL,0xEFCDAB89UL,0x98BADCFEUL,0x10325476UL,0xC3D2E1F0UL};
			uint32_t hash256[8]={0x6A09E667UL,0xBB67AE85UL,0x3C6EF372UL,0xA54FF53AUL,0x510E527FUL,0x9B05688CUL,0x1F83D9ABUL,0x5BE0CD19UL};
			context->type=type;
			if (context->type==crtshatype_sha1)
				memcpy(context->state,hash1,sizeof(hash1));
			else
				memcpy(context->state,hash256,sizeof(hash256));
			context->count_low=0;
			context->count_high=0;
			context->index=0;
		}
		static void _shaupdate(crtshactx* ctx, unsigned char* buffer, unsigned int length) {
			if (ctx->index){
				unsigned left = 64 - ctx->index;
				if (length < left) {
					memcpy(ctx->block + ctx->index, buffer, length);
					ctx->index += length;
					return;
				} else {
					memcpy(ctx->block + ctx->index, buffer, left);
					if (ctx->type==crtshatype_sha1) {
						_nettle_sha1_compress(ctx->state, ctx->block);
						ctx->count_high += !++ctx->count_low;
					} else
						sha256_block(ctx, ctx->block);
					buffer += left;
					length -= left;
				}
			}
			if (ctx->type==crtshatype_sha1) {
				while (length >= 64) {
					_nettle_sha1_compress(ctx->state, buffer);
					ctx->count_high += !++ctx->count_low;
					buffer += 64;
					length -= 64;
				}
				if ((ctx->index = length))
					memcpy(ctx->block, buffer, length);
			} else {
				while (length >= 64) {
					sha256_block(ctx, buffer);
					buffer += 64;
					length -= 64;
				}
				memcpy(ctx->block, buffer, length);
				ctx->index = length;
			}
		}
		static void sha256_transform(uint32_t *state, uint32_t *data)
		{
			uint32_t A, B, C, D, E, F, G, H;
			unsigned i;
			const uint32_t *k;
			uint32_t *d;
			static const uint32_t _crtsha256_k[64] = {
					0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 
					0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL, 
					0xe49b69c1UL, 0xefbe4786UL, 0xfc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 
					0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x6ca6351UL, 0x14292967UL, 
					0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL, 
					0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 
					0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL, 
					0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
			};
			A = state[0];B = state[1];C = state[2];D = state[3];
			E = state[4];F = state[5];G = state[6];H = state[7];
			for (i = 0, k = _crtsha256_k, d = data; i<16; i+=8, k += 8, d+= 8) {
				_crtsha256_round(A, B, C, D, E, F, G, H, k[0], d[0]);
				_crtsha256_round(H, A, B, C, D, E, F, G, k[1], d[1]);
				_crtsha256_round(G, H, A, B, C, D, E, F, k[2], d[2]);
				_crtsha256_round(F, G, H, A, B, C, D, E, k[3], d[3]);
				_crtsha256_round(E, F, G, H, A, B, C, D, k[4], d[4]);
				_crtsha256_round(D, E, F, G, H, A, B, C, k[5], d[5]);
				_crtsha256_round(C, D, E, F, G, H, A, B, k[6], d[6]);
				_crtsha256_round(B, C, D, E, F, G, H, A, k[7], d[7]);
			}
			for (; i<64; i += 16, k+= 16) {
				_crtsha256_round(A, B, C, D, E, F, G, H, k[ 0], _crtsha256_expand(data,  0));
				_crtsha256_round(H, A, B, C, D, E, F, G, k[ 1], _crtsha256_expand(data,  1));
				_crtsha256_round(G, H, A, B, C, D, E, F, k[ 2], _crtsha256_expand(data,  2));
				_crtsha256_round(F, G, H, A, B, C, D, E, k[ 3], _crtsha256_expand(data,  3));
				_crtsha256_round(E, F, G, H, A, B, C, D, k[ 4], _crtsha256_expand(data,  4));
				_crtsha256_round(D, E, F, G, H, A, B, C, k[ 5], _crtsha256_expand(data,  5));
				_crtsha256_round(C, D, E, F, G, H, A, B, k[ 6], _crtsha256_expand(data,  6));
				_crtsha256_round(B, C, D, E, F, G, H, A, k[ 7], _crtsha256_expand(data,  7));
				_crtsha256_round(A, B, C, D, E, F, G, H, k[ 8], _crtsha256_expand(data,  8));
				_crtsha256_round(H, A, B, C, D, E, F, G, k[ 9], _crtsha256_expand(data,  9));
				_crtsha256_round(G, H, A, B, C, D, E, F, k[10], _crtsha256_expand(data, 10));
				_crtsha256_round(F, G, H, A, B, C, D, E, k[11], _crtsha256_expand(data, 11));
				_crtsha256_round(E, F, G, H, A, B, C, D, k[12], _crtsha256_expand(data, 12));
				_crtsha256_round(D, E, F, G, H, A, B, C, k[13], _crtsha256_expand(data, 13));
				_crtsha256_round(C, D, E, F, G, H, A, B, k[14], _crtsha256_expand(data, 14));
				_crtsha256_round(B, C, D, E, F, G, H, A, k[15], _crtsha256_expand(data, 15));
			}
			state[0] += A;state[1] += B;state[2] += C;state[3] += D;
			state[4] += E;state[5] += F;state[6] += G;state[7] += H;
		}
		static void _shafinal(crtshactx* ctx,uint8_t *digest) {
			unsigned i,words,leftover;
			unsigned length;
			uint32_t bitcount_high,bitcount_low;
			uint32_t data[16];
			i = ctx->index;
			ctx->block[i++] = 0x80;
			if (ctx->type==crtshatype_sha1) {
				length = 20;
				if (i > (64 - 8)) {
					memset(ctx->block + i, 0, 64 - i);
					_nettle_sha1_compress(ctx->state, ctx->block);
					i = 0;
				}
				if (i < (64 - 8)) memset(ctx->block + i, 0, (64 - 8) - i);
				bitcount_high = (ctx->count_high << 9) | (ctx->count_low >> 23);
				bitcount_low = (ctx->count_low << 9) | (ctx->index << 3);
				crtwriteu32(ctx->block + (64 - 8), bitcount_high);
				crtwriteu32(ctx->block + (64 - 4), bitcount_low);
				_nettle_sha1_compress(ctx->state, ctx->block);
			} else {
				length = 32;
				for( ; i & 3; i++) ctx->block[i] = 0;
				words = i >> 2;
				for (i = 0; i < words; i++) data[i] = crtreadu32(ctx->block + 4*i);
				if (words > (16-2)) {
					for (i = words ; i < 16; i++) data[i] = 0;
					sha256_transform(ctx->state, data);
					for (i = 0; i < (16-2); i++) data[i] = 0;
				}
				else
					for (i = words ; i < 16 - 2; i++) data[i] = 0;
				data[16-2] = (ctx->count_high << 9) | (ctx->count_low >> 23);
				data[16-1] = (ctx->count_low << 9) | (ctx->index << 3);
				sha256_transform(ctx->state, data);
			}
			words = length / 4;
			leftover = length % 4;
			for (i = 0; i < words; i++, digest += 4)
				crtwriteu32(digest, ctx->state[i]);
			if (leftover) {
				uint32_t word;
				unsigned j = leftover;
				word = ctx->state[i];
				switch (leftover) {
					case 3:digest[--j] = (word >> 8) & 0xff;
					case 2:digest[--j] = (word >> 16) & 0xff;
					case 1:digest[--j] = (word >> 24) & 0xff;
				}
			}
		}
	};
};

