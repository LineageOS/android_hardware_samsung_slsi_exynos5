#ifndef _SBL_VERSION // SBL = Secondary Bootloader Version

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <gdmcprovlib.h>
#include <gdmcprovprotocol.h>
#include <gdmcinstance.h>

typedef struct tagMCCM        MCCM;

struct tagMCCM
{
  cmp_t                *cmp;          ///< World Shared Memory (WSM) to the TCI buffer
  mcSessionHandle_t     sess;         ///< session handle
  mcResult_t            lasterror;    ///< last MC driver error
  cmpReturnCode_t       lastcmperr;   ///< last Content Management Protocol error
  uint32_t              lastmccmerr;  ///< error code from MCCM (MobiCore Content Management) library
};

static MCCM g_mccm;

// Copied from MCCM library not to have this additional dependency!

// returns 1 if successful, 0 otherwise
bool mccmOpen ( void )
{
  const mcUuid_t      UUID = TL_CM_UUID;
  mcResult_t          result;

  memset(&g_mccm,0,sizeof(MCCM));

  result = mcOpenDevice(MC_DEVICE_ID_DEFAULT);

  if (MC_DRV_OK != result) 
    return false;

  result = mcMallocWsm(MC_DEVICE_ID_DEFAULT, 0, sizeof(cmp_t), (uint8_t **)&g_mccm.cmp, 0);
  if (MC_DRV_OK != result) 
  {
    mcCloseDevice(MC_DEVICE_ID_DEFAULT);
    return false;
  }

  result = mcOpenSession(&g_mccm.sess,(const mcUuid_t *)&UUID,(uint8_t *)g_mccm.cmp,(uint32_t)sizeof(cmp_t));
  if (MC_DRV_OK != result)
  {
    mcFreeWsm(MC_DEVICE_ID_DEFAULT,(uint8_t*)g_mccm.cmp);
    mcCloseDevice(MC_DEVICE_ID_DEFAULT);
    return false;
  }

  return true;
}

void mccmClose ( void )
{
  mcCloseSession(&g_mccm.sess);

  if (NULL!=g_mccm.cmp)
    mcFreeWsm(MC_DEVICE_ID_DEFAULT,(uint8_t*)g_mccm.cmp);

  mcCloseDevice(MC_DEVICE_ID_DEFAULT);

  memset(&g_mccm,0,sizeof(MCCM));
}

static bool mccmTransmit ( int32_t timeout )
{
  // Send CMP message to content management trustlet.

  g_mccm.lasterror = mcNotify(&g_mccm.sess);

  if (unlikely( MC_DRV_OK!=g_mccm.lasterror ))
    return false;

  // Wait for trustlet response.

  g_mccm.lasterror = mcWaitNotification(&g_mccm.sess, timeout);

  if (unlikely( MC_DRV_OK!=g_mccm.lasterror )) 
    return false;

  return true;
}

static bool mccmGetSuid ( mcSuid_t *suid )
{
  g_mccm.lastcmperr = SUCCESSFUL;

  memset(g_mccm.cmp,0,sizeof(cmp_t));
  g_mccm.cmp->msg.cmpCmdGetSuid.cmdHeader.commandId = MC_CMP_CMD_GET_SUID;

  if (unlikely( !mccmTransmit(MC_INFINITE_TIMEOUT) ))
    return false;

  if (unlikely( (MC_CMP_CMD_GET_SUID|RSP_ID_MASK)!=g_mccm.cmp->msg.cmpRspGetSuid.rspHeader.responseId ))
  {
    g_mccm.lasterror = MC_DRV_ERR_UNKNOWN;
    return false;
  }

  g_mccm.lastcmperr = g_mccm.cmp->msg.cmpRspGetSuid.rspHeader.returnCode;
  
  if (unlikely( SUCCESSFUL!=g_mccm.lastcmperr ))
  {
    g_mccm.lasterror = MC_DRV_ERR_UNKNOWN;
    return false;
  }

  memcpy(suid,&g_mccm.cmp->msg.cmpRspGetSuid.suid,sizeof(mcSuid_t));

  return true;
}

static bool mccmGenerateAuthToken ( 
                      const cmpCmdGenAuthToken_t *cmd, 
                      cmpRspGenAuthToken_t       *rsp )
{
  g_mccm.lastcmperr = SUCCESSFUL;

  memset(g_mccm.cmp,0,sizeof(cmp_t));
  
  memcpy(g_mccm.cmp,cmd,sizeof(*cmd));

  if (unlikely( !mccmTransmit(MC_INFINITE_TIMEOUT) ))
    return false;

  if (unlikely( (cmd->cmd.sdata.cmdHeader.commandId|RSP_ID_MASK)!=g_mccm.cmp->msg.cmpRspGenAuthToken.rsp.rspHeader.responseId ))
  {
    g_mccm.lasterror = MC_DRV_ERR_UNKNOWN;
    return false;
  }

  g_mccm.lastcmperr = g_mccm.cmp->msg.cmpRspGenAuthToken.rsp.rspHeader.returnCode;
  
  if (unlikely( SUCCESSFUL!=g_mccm.lastcmperr ))
  {
    g_mccm.lasterror = MC_DRV_ERR_UNKNOWN;
    return false;
  }

  memcpy(rsp,g_mccm.cmp,sizeof(*rsp));

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Convenience functions
///////////////////////////////////////////////////////////////////////////////////////////

gderror MCGetSUID ( _u8 *suid )
{
  if (unlikely( NULL==suid ))
    return GDERROR_PARAMETER;

  memset(suid,0,SUID_LENGTH);

  if (!mccmGetSuid((mcSuid_t*)suid))
    return GDERROR_CANT_GET_SUID;

  return GDERROR_OK;
}

gderror MCGenerateAuthToken ( gdmcinst *inst, const gdmc_actmsg_req *req, gdmc_so_authtok *authtok )
{
  cmpRspGenAuthToken_t    rsp;

  if (unlikely( NULL==inst || NULL==req || NULL==authtok ))
    return GDERROR_PARAMETER;

  memset(authtok,0,sizeof(gdmc_so_authtok));

  if (MC_CMP_CMD_GENERATE_AUTH_TOKEN!=req->msg_type)
    return GDERROR_MESSAGE_FORMAT;

  if (!mccmGenerateAuthToken((const cmpCmdGenAuthToken_t *)req,&rsp))
    return GDERROR_CANT_BUILD_AUTHTOKEN;

  memcpy(authtok,&rsp.soAuthCont,sizeof(*authtok));

  return GDERROR_OK;
}

#else // Secondary Bootloader Version

#define _NO_OPENSSL_INCLUDES

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <gdmcprovlib.h>
#include <gdmcprovprotocol.h>
#include <gdmcinstance.h>

#define SHA256_BLOCK_LENGTH		64
#define SHA256_DIGEST_LENGTH		32
#define SHA256_DIGEST_STRING_LENGTH	(SHA256_DIGEST_LENGTH * 2 + 1)

typedef unsigned char u_int8_t;		/* 1-byte  (8-bits)  */
typedef unsigned int u_int32_t;		/* 4-bytes (32-bits) */
typedef unsigned long long u_int64_t;	/* 8-bytes (64-bits) */

typedef unsigned char  sha2_byte;	/* Exactly 1 byte */
typedef unsigned int sha2_word32;	/* Exactly 4 bytes */
typedef unsigned long long sha2_word64;	/* Exactly 8 bytes */

typedef struct _SHA256_CTX {
	u_int32_t	state[8];
	u_int64_t	bitcount;
	u_int8_t	buffer[SHA256_BLOCK_LENGTH];
} SHA256_CTX;

static void SHA256(const sha2_byte* data, size_t len, char digest[SHA256_DIGEST_STRING_LENGTH]);

gderror MCGetSUID ( gdmcinst *inst, _u8 suid[SUID_LENGTH] )
{
  inst->suid[ 0] = 0x11;
  inst->suid[ 1] = 0x22;
  inst->suid[ 2] = 0x33;
  inst->suid[ 3] = 0x44;
  inst->suid[ 4] = 0x55;
  inst->suid[ 5] = 0x66;
  inst->suid[ 6] = 0x77;
  inst->suid[ 7] = 0x88;
  inst->suid[ 8] = 0x99;
  inst->suid[ 9] = 0xAA;
  inst->suid[10] = 0xBB;
  inst->suid[11] = 0xCC;
  inst->suid[12] = 0xDD;
  inst->suid[13] = 0xEE;
  inst->suid[14] = 0xFF;
  inst->suid[15] = 0xFE;

  memcpy(suid,inst->suid,SUID_LENGTH);

  return GDERROR_OK;
}

gderror MCGenerateAuthToken ( gdmcinst *inst, const gdmc_actmsg_req *req, gdmc_so_authtok *authtok )
{
  _u8                 md[SHA256_DIGEST_LENGTH];
  mcSoAuthTokenCont_t tok;

  memset(&tok,0,sizeof(tok));
  memset(authtok,0,sizeof(gdmc_so_authtok));

  if (MC_CMP_CMD_GENERATE_AUTH_TOKEN!=req->msg_type)
    return GDERROR_MESSAGE_FORMAT;

  SHA256((const unsigned char *)req,offsetof(gdmc_actmsg_req,md),md);  // hash it...

  if (memcmp(md,req->md,SHA256_DIGEST_LENGTH))
    return GDERROR_MESSAGE_DIGEST;

  if (memcmp(inst->suid,req->suid,SUID_LENGTH))
    return GDERROR_SUID_MISMATCH;

  // Header:

  tok.soHeader.type         = MC_SO_TYPE_REGULAR;
  tok.soHeader.version      = 1;
  tok.soHeader.context      = MC_SO_CONTEXT_DEVICE;
  tok.soHeader.plainLen     = sizeof(tok.coSoc.type)+
                              sizeof(tok.coSoc.attribs)+
                              sizeof(tok.coSoc.suid);
  tok.soHeader.encryptedLen = sizeof(tok.coSoc.co);

  // Plain data:

  tok.coSoc.type          = CONT_TYPE_SOC;
  tok.coSoc.attribs.state = MC_CONT_STATE_ACTIVATED;

  memcpy(&tok.coSoc.suid,inst->suid,sizeof(tok.coSoc.suid));

  // Secret:

  memcpy(&tok.coSoc.co.kSocAuth,&inst->kSoCAuth,sizeof(tok.coSoc.co.kSocAuth));

  SHA256((const unsigned char *)&tok,
         offsetof(mcSoAuthTokenCont_t,hashAndPad),
         (char *)&tok.hashAndPad);

  tok.hashAndPad[MC_SO_HASH_SIZE] = 0x80; // ISO-padding

  memcpy(authtok,&tok,sizeof(gdmc_so_authtok));
  
  return GDERROR_OK;
}

#undef SHA2_UNROLL_TRANSFORM

#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN 

#if !defined(BYTE_ORDER) || (BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN)
#error Define BYTE_ORDER to be equal to either LITTLE_ENDIAN or BIG_ENDIAN
#endif

#define SHA256_SHORT_BLOCK_LENGTH	(SHA256_BLOCK_LENGTH - 8)
#define SHA384_SHORT_BLOCK_LENGTH	(SHA384_BLOCK_LENGTH - 16)
#define SHA512_SHORT_BLOCK_LENGTH	(SHA512_BLOCK_LENGTH - 16)


/*** ENDIAN REVERSAL MACROS *******************************************/
#if BYTE_ORDER == LITTLE_ENDIAN
#define REVERSE32(w,x)	{ \
	sha2_word32 tmp = (w); \
	tmp = (tmp >> 16) | (tmp << 16); \
	(x) = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); \
}
#define REVERSE64(w,x)	{ \
	sha2_word64 tmp = (w); \
	tmp = (tmp >> 32) | (tmp << 32); \
	tmp = ((tmp & 0xff00ff00ff00ff00ULL) >> 8) | \
	      ((tmp & 0x00ff00ff00ff00ffULL) << 8); \
	(x) = ((tmp & 0xffff0000ffff0000ULL) >> 16) | \
	      ((tmp & 0x0000ffff0000ffffULL) << 16); \
}
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

/*
 * Macro for incrementally adding the unsigned 64-bit integer n to the
 * unsigned 128-bit integer (represented using a two-element array of
 * 64-bit words):
 */
#define ADDINC128(w,n)	{ \
	(w)[0] += (sha2_word64)(n); \
	if ((w)[0] < (n)) { \
		(w)[1]++; \
	} \
}

#define R(b,x) 		((x) >> (b))
#define S32(b,x)	(((x) >> (b)) | ((x) << (32 - (b))))

/* Two of six logical functions used in SHA-256, SHA-384, and SHA-512: */
#define Ch(x,y,z)	(((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)	(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Four of six logical functions used in SHA-256: */
#define Sigma0_256(x)	(S32(2,  (x)) ^ S32(13, (x)) ^ S32(22, (x)))
#define Sigma1_256(x)	(S32(6,  (x)) ^ S32(11, (x)) ^ S32(25, (x)))
#define sigma0_256(x)	(S32(7,  (x)) ^ S32(18, (x)) ^ R(3 ,   (x)))
#define sigma1_256(x)	(S32(17, (x)) ^ S32(19, (x)) ^ R(10,   (x)))

void SHA256_Transform(SHA256_CTX*, const sha2_word32*);

const static sha2_word32 K256[64] = {
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
	0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
	0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
	0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
	0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
	0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
	0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
	0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
	0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

/* Initial hash value H for SHA-256: */
const static sha2_word32 sha256_initial_hash_value[8] = {
	0x6a09e667UL,
	0xbb67ae85UL,
	0x3c6ef372UL,
	0xa54ff53aUL,
	0x510e527fUL,
	0x9b05688cUL,
	0x1f83d9abUL,
	0x5be0cd19UL
};

static void SHA256_Init(SHA256_CTX* context) 
{
	if (context == (SHA256_CTX*)0) 
  {
		return;
	}
	memcpy(context->state,sha256_initial_hash_value, SHA256_DIGEST_LENGTH);
	memset(context->buffer, 0,SHA256_BLOCK_LENGTH);
	context->bitcount = 0;
}

static void SHA256_Transform(SHA256_CTX* context, const sha2_word32* data) {
	sha2_word32	a, b, c, d, e, f, g, h, s0, s1;
	sha2_word32	T1, T2, *W256;
	int		j;

	W256 = (sha2_word32*)context->buffer;

	/* Initialize registers with the prev. intermediate value */
	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];
	f = context->state[5];
	g = context->state[6];
	h = context->state[7];

	j = 0;
	do {
#if BYTE_ORDER == LITTLE_ENDIAN
		/* Copy data while converting to host byte order */
		REVERSE32(*data++,W256[j]);
		/* Apply the SHA-256 compression function to update a..h */
		T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + W256[j];
#else /* BYTE_ORDER == LITTLE_ENDIAN */
		/* Apply the SHA-256 compression function to update a..h with copy */
		T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + (W256[j] = *data++);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
		T2 = Sigma0_256(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 16);

	do {
		/* Part of the message block expansion: */
		s0 = W256[(j+1)&0x0f];
		s0 = sigma0_256(s0);
		s1 = W256[(j+14)&0x0f];	
		s1 = sigma1_256(s1);

		/* Apply the SHA-256 compression function to update a..h */
		T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + 
		     (W256[j&0x0f] += s1 + W256[(j+9)&0x0f] + s0);
		T2 = Sigma0_256(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 64);

	/* Compute the current intermediate hash value */
	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
	context->state[5] += f;
	context->state[6] += g;
	context->state[7] += h;

	/* Clean up */
	a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#define bcopy(s,d,len) memcpy(d,s,len)
#define bzero(d,len) memset(d,0,len)

static void SHA256_Update(SHA256_CTX* context, const sha2_byte *data, size_t len) {
	unsigned int	freespace, usedspace;

	if (len == 0) {
		/* Calling with no data is valid - we do nothing */
		return;
	}

	usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
	if (usedspace > 0) {
		/* Calculate how much free space is available in the buffer */
		freespace = SHA256_BLOCK_LENGTH - usedspace;

		if (len >= freespace) {
			/* Fill the buffer completely and process it */

			bcopy(data, &context->buffer[usedspace], freespace);
			context->bitcount += freespace << 3;
			len -= freespace;
			data += freespace;
			SHA256_Transform(context, (sha2_word32*)context->buffer);
		} else {
			/* The buffer is not yet full */
			bcopy(data, &context->buffer[usedspace], len);
			context->bitcount += len << 3;
			/* Clean up: */
			usedspace = freespace = 0;
			return;
		}
	}
	while (len >= SHA256_BLOCK_LENGTH) {
		/* Process as many complete blocks as we can */
		SHA256_Transform(context, (const sha2_word32*)data);
		context->bitcount += SHA256_BLOCK_LENGTH << 3;
		len -= SHA256_BLOCK_LENGTH;
		data += SHA256_BLOCK_LENGTH;
	}
	if (len > 0) {
		/* There's left-overs, so save 'em */
		bcopy(data, context->buffer, len);
		context->bitcount += len << 3;
	}
	/* Clean up: */
	usedspace = freespace = 0;
}

static void SHA256_Final(sha2_byte digest[], SHA256_CTX* context) {
	sha2_word32	*d = (sha2_word32*)digest;
	unsigned int	usedspace;

	/* If no digest buffer is passed, we don't bother doing this: */
	if (digest != (sha2_byte*)0) {
		usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
		/* Convert FROM host byte order */
		REVERSE64(context->bitcount,context->bitcount);
#endif
		if (usedspace > 0) {
			/* Begin padding with a 1 bit: */
			context->buffer[usedspace++] = 0x80;

			if (usedspace < SHA256_SHORT_BLOCK_LENGTH) {
				/* Set-up for the last transform: */
				bzero(&context->buffer[usedspace], SHA256_SHORT_BLOCK_LENGTH - usedspace);
			} else {
				if (usedspace < SHA256_BLOCK_LENGTH) {
					bzero(&context->buffer[usedspace], SHA256_BLOCK_LENGTH - usedspace);
				}
				/* Do second-to-last transform: */
				SHA256_Transform(context, (sha2_word32*)context->buffer);

				/* And set-up for the last transform: */
				bzero(context->buffer, SHA256_SHORT_BLOCK_LENGTH);
			}
		} else {
			/* Set-up for the last transform: */
			bzero(context->buffer, SHA256_SHORT_BLOCK_LENGTH);

			/* Begin padding with a 1 bit: */
			*context->buffer = 0x80;
		}
		/* Set the bit count: */
		*(sha2_word64*)&context->buffer[SHA256_SHORT_BLOCK_LENGTH] = context->bitcount;

		/* Final transform: */
		SHA256_Transform(context, (sha2_word32*)context->buffer);

#if BYTE_ORDER == LITTLE_ENDIAN
		{
			/* Convert TO host byte order */
			int	j;
			for (j = 0; j < 8; j++) {
				REVERSE32(context->state[j],context->state[j]);
				*d++ = context->state[j];
			}
		}
#else
		bcopy(context->state, d, SHA256_DIGEST_LENGTH);
#endif
	}

	/* Clean up state data: */
	bzero(context, sizeof(context));
	usedspace = 0;
}

static void SHA256(const sha2_byte* data, size_t len, char digest[SHA256_DIGEST_STRING_LENGTH]) 
{
	SHA256_CTX	context;

	SHA256_Init(&context);
	SHA256_Update(&context, data, len);
	SHA256_Final(digest,&context);
}

#endif