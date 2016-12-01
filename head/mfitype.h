#ifndef _MFITYPE_HEADER_
#define _MFITYPE_HEADER_

#define _MFI_ERROR           (-2147483647L-1)  /* 0x80000000 */

/*- Mfi Types --------------------------------------------------------------*/

#ifndef _MFI_INT64_UINT64_DEFINED
#if defined(_WIN64) || ((defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)) && !defined(_NI_mswin16_))
#if (defined(_MSC_VER) && (_MSC_VER >= 1200)) || (defined(_CMfi_) && (_CMfi_ >= 700)) || (defined(__BORLANDC__) && (__BORLANDC__ >= 0x0520))
typedef unsigned   __int64  MfiUInt64;
typedef signed __int64  MfiInt64;
#define _MFI_INT64_UINT64_DEFINED
#if defined(_WIN64)
#define _MFI_ENV_IS_64_BIT
#else
/* This is a 32-bit OS, not a 64-bit OS */
#endif
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 3)
#include <limits.h>
#include <sys/types.h>
typedef u_int64_t           MfiUInt64;
typedef int64_t             MfiInt64;
#define _MFI_INT64_UINT64_DEFINED
#if defined(LONG_MAX) && (LONG_MAX > 0x7FFFFFFFL)
#define _MFI_ENV_IS_64_BIT
#else
/* This is a 32-bit OS, not a 64-bit OS */
#endif
#else
/* This platform does not support 64-bit types */
#endif
#endif

#if defined(_MFI_INT64_UINT64_DEFINED)
typedef MfiUInt64    * MfiPUInt64;
typedef MfiUInt64    * MfiAUInt64;
typedef MfiInt64     * MfiPInt64;
typedef MfiInt64     * MfiAInt64;
#endif

#if defined(LONG_MAX) && (LONG_MAX > 0x7FFFFFFFL)
typedef unsigned int        MfiUInt32;
typedef signed int      MfiInt32;
#else
typedef unsigned long       MfiUInt32;
typedef signed long     MfiInt32;
#endif

typedef MfiUInt32    * MfiPUInt32;
typedef MfiUInt32    * MfiAUInt32;
typedef MfiInt32     * MfiPInt32;
typedef MfiInt32     * MfiAInt32;

typedef unsigned short      MfiUInt16;
typedef MfiUInt16         * MfiPUInt16;
typedef MfiUInt16         * MfiAUInt16;

typedef signed short       MfiInt16;
typedef MfiInt16         * MfiPInt16;
typedef MfiInt16         * MfiAInt16;

typedef unsigned char       MfiUInt8;
typedef MfiUInt8          * MfiPUInt8;
typedef MfiUInt8          * MfiAUInt8;

typedef signed char    MfiInt8;
typedef MfiInt8      * MfiPInt8;
typedef MfiInt8      * MfiAInt8;

typedef char           MfiChar;
typedef MfiChar      * MfiPChar;
typedef MfiChar      * MfiAChar;

typedef unsigned char  MfiByte;
typedef MfiByte      * MfiPByte;
typedef MfiByte      * MfiAByte;

typedef void         * MfiAddr;
typedef MfiAddr      * MfiPAddr;
typedef MfiAddr      * MfiAAddr;

typedef float          MfiReal32;
typedef MfiReal32    * MfiPReal32;
typedef MfiReal32    * MfiAReal32;

typedef double         MfiReal64;
typedef MfiReal64    * MfiPReal64;
typedef MfiReal64    * MfiAReal64;

typedef MfiPByte       MfiBuf;
typedef MfiPByte       MfiPBuf;
typedef MfiPByte     * MfiABuf;

typedef MfiPChar       MfiString;
typedef MfiPChar       MfiPString;
typedef MfiPChar     * MfiAString;

typedef MfiString      MfiRsrc;
typedef MfiString      MfiPRsrc;
typedef MfiString    * MfiARsrc;

typedef MfiUInt16      MfiBoolean;
typedef MfiBoolean   * MfiPBoolean;
typedef MfiBoolean   * MfiABoolean;

typedef MfiInt32       MfiStatus;
typedef MfiStatus    * MfiPStatus;
typedef MfiStatus    * MfiAStatus;

typedef MfiUInt32      MfiVersion;
typedef MfiVersion   * MfiPVersion;
typedef MfiVersion   * MfiAVersion;

typedef MfiInt32       MfiObject;
typedef MfiObject    * MfiPObject;
typedef MfiObject    * MfiAObject;

typedef MfiObject      MfiSession;
typedef MfiSession   * MfiPSession;
typedef MfiSession   * MfiASession;

typedef MfiUInt32      MfiAttr;
typedef MfiUInt32      MfiCommand;

#ifndef _MFI_CONST_STRING_DEFINED
typedef const MfiChar * MfiConstString;
#define _MFI_CONST_STRING_DEFINED
#endif

/*- Completion and Error Codes ----------------------------------------------*/

#define MFI_SUCCESS          (0L)

/*- Other Mfi Definitions --------------------------------------------------*/

#define MFI_NULL             (0)

#define MFI_TRUE             (1)
#define MFI_FALSE            (0)


#endif