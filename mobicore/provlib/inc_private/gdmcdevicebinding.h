#ifndef _INC_GDMCDEVICEBINDING_H
#define _INC_GDMCDEVICEBINDING_H

#include <gdmcprovlib.h>
#include <gdmcprovprotocol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KSoCAuthSNTS  KSoCAuthSNTS;

struct _KSoCAuthSNTS
{
  mcSymmetricKey_t  kSoCAuth;
  _u64              serialNumber;
  _u64              timeStamp;
};

#ifdef __cplusplus
}
#endif

gderror GDMCComposeErrorMessage ( gdmcinst   *inst, 
                                  gderror     error, 
                                  _u8        *msgout, 
                                  _u32       *msgout_size, 
                                  _u32        initial_msgout_size, 
                                  const char *pszmsg, ... );

gderror GDPROVAPI GDMCValidateProvMessage ( const _u8        *msg, 
                                            _u32              msgsize,
                                            gdmc_msgheader  **ppheader,
                                            _u8             **ppbody,
                                            gdmc_msgtrailer **pptrailer );

gderror GDPROVAPI GDMCHandleGetSUID ( gdmcinst *inst,
                                      _u8      *msgout,
                                      _u32     *msgout_size,
                                      _u32      initial_msgout_size );

gderror GDPROVAPI GDMCHandleGenAuthToken ( gdmcinst          *inst,
                                           gdmc_actmsg_req   *req,
                                          _u8                *msgout,
                                          _u32               *msgout_size,
                                          _u32                initial_msgout_size );

gderror GDPROVAPI GDMCHandleValidateAuthToken ( gdmcinst         *inst,
                                                gdmc_so_authtok  *validateSoAuthTok,
                                                _u8              *msgout,
                                                _u32             *msgout_size,
                                                _u32              initial_msgout_size );

#define ERRMSG_0001       "Unable to access memory region at %p (size: %u byte(s)) for READ."
#define ERRMSG_0002       "Unable to access memory region at %p (size: %u byte(s)) for WRITE."
#define ERRMSG_0003       "First in-message must be empty."
#define ERRMSG_0004       "Insufficient memory available."
#define ERRMSG_0005       "Message output buffer too small (%u but %u required to store message)."
#define ERRMSG_0006       "Message validation failed."
#define ERRMSG_0007       "Unexpected message received. Cannot evaluate message (ignored)."
#define ERRMSG_0008       "SUID of returned SO.AuthToken mismatches (my) internal SUID. SO.AuthToken discarded."
#define ERRMSG_0009       "Unable to generate SD.Receipt."
#define ERRMSG_000A       "Expecting MC_GETSUID_REQ message from Production Station."
#define ERRMSG_000B       "Unable to retrieve SUID from SoC (MobiCore)."
#define ERRMSG_000C       "Unable to dump SO.AuthToken (MobiCore)."
#define ERRMSG_000D       "Unable to retrieve SUID from SoC."
#define ERRMSG_000E       "Unable to generate SO.AuthToken."
#define ERRMSG_000F       "Validation of SO.AuthToken failed because no SO.AuthToken available."

#endif // _INC_GDMCDEVICEBINDING_H
