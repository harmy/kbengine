/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __KBE_PLATFORM_H__
#define __KBE_PLATFORM_H__
// common include	
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <math.h>
#include <assert.h> 
#include <iostream>  
#include <sstream>
#include <stdexcept>
#include <string>  
#include <cstring>  
#include <vector>
#include <map>
#include <list>
#include <set>
#include <deque>
#include <limits>
#include <algorithm>
#include <utility>
#include <functional>
#include <cctype>
#include "cstdkbe/strutil.hpp"
// windows include	
#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#pragma warning(disable:4996)
#pragma warning(disable:4819)
#pragma warning(disable:4049)
#pragma warning(disable:4217)
#include <io.h>
#include <time.h> 
#include <winsock2.h>		// ������windows.h֮ǰ������ ��������ģ���������
#include <mswsock.h> 
#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <unordered_map>
#include <functional>
#else
// linux include
#include <errno.h>
#include <pthread.h>	
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <langinfo.h>   /* CODESET */
#include <stdint.h>
#include <signal.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <tr1/unordered_map>
#include <tr1/functional>
#include <tr1/memory>
#include <linux/types.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/resource.h> 
#include <linux/errqueue.h>
#include <iconv.h>
#endif

#if !defined( PLAYSTATION3 )
#include <signal.h>
#endif

#if !defined( _WIN32 ) && !defined( PLAYSTATION3 )
# include <pwd.h>
#else
#define SIGHUP	1
#define SIGINT	2
#define SIGQUIT 3
#define SIGUSR1 10
#define SIGSYS	32
#endif

/** �����������ֿռ� */
namespace KBEngine{ 
/** ���������ֽ��� */
#define KBENGINE_LITTLE_ENDIAN							0
#define KBENGINE_BIG_ENDIAN								1
#if !defined(KBENGINE_ENDIAN)
#  if defined (ACE_BIG_ENDIAN)
#    define KBENGINE_ENDIAN KBENGINE_BIG_ENDIAN
#  else 
#    define KBENGINE_ENDIAN KBENGINE_LITTLE_ENDIAN
#  endif 
#endif


// current platform and compiler
#define PLATFORM_WIN32 0
#define PLATFORM_UNIX  1
#define PLATFORM_APPLE 2

#define UNIX_FLAVOUR_LINUX 1
#define UNIX_FLAVOUR_BSD 2
#define UNIX_FLAVOUR_OTHER 3
#define UNIX_FLAVOUR_OSX 4

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#  define KBE_PLATFORM PLATFORM_WIN32
#elif defined( __INTEL_COMPILER )
#  define KBE_PLATFORM PLATFORM_INTEL
#elif defined( __APPLE_CC__ )
#  define KBE_PLATFORM PLATFORM_APPLE
#else
#  define KBE_PLATFORM PLATFORM_UNIX
#endif

#define COMPILER_MICROSOFT 0
#define COMPILER_GNU	   1
#define COMPILER_BORLAND   2
#define COMPILER_INTEL     3

#ifdef _MSC_VER
#  define KBE_COMPILER COMPILER_MICROSOFT
#elif defined( __INTEL_COMPILER )
#  define KBE_COMPILER COMPILER_INTEL
#elif defined( __BORLANDC__ )
#  define KBE_COMPILER COMPILER_BORLAND
#elif defined( __GNUC__ )
#  define KBE_COMPILER COMPILER_GNU
#else
#  pragma error "FATAL ERROR: Unknown compiler."
#endif

#if KBE_PLATFORM == PLATFORM_UNIX || KBE_PLATFORM == PLATFORM_APPLE
#ifdef HAVE_DARWIN
#define KBE_PLATFORM_TEXT "MacOSX"
#define UNIX_FLAVOUR UNIX_FLAVOUR_OSX
#else
#ifdef USE_KQUEUE
#define KBE_PLATFORM_TEXT "FreeBSD"
#define UNIX_FLAVOUR UNIX_FLAVOUR_BSD
#else
#ifdef USE_KQUEUE_DFLY
#define KBE_PLATFORM_TEXT "DragonFlyBSD"
#define UNIX_FLAVOUR UNIX_FLAVOUR_BSD
#else
#define KBE_PLATFORM_TEXT "Linux"
#define UNIX_FLAVOUR UNIX_FLAVOUR_LINUX
#endif
#endif
#endif
#endif

#if KBE_PLATFORM == PLATFORM_WIN32
#define KBE_PLATFORM_TEXT "Win32"
#endif

#ifndef KBE_CONFIG
#ifdef _DEBUG
#define KBE_CONFIG "Debug"
#else
#define KBE_CONFIG "Release"
#endif
#endif

#ifdef X64
#define KBE_ARCH "X64"
#else
#define KBE_ARCH "X86"
#endif

/*---------------------------------------------------------------------------------
	���Ͷ���
---------------------------------------------------------------------------------*/
#ifndef TCHAR
#ifdef _UNICODE
	typedef wchar_t												TCHAR;
#else
	typedef char												TCHAR;
#endif
#endif

typedef unsigned char											uchar;
typedef unsigned short											ushort;
typedef unsigned int											uint;
typedef unsigned long											ulong;

#define charptr													char*
#define const_charptr											const char*
#define PyObject_ptr											PyObject*


/* Use correct types for x64 platforms, too */
#if KBE_COMPILER != COMPILER_GNU
typedef signed __int64											int64;
typedef signed __int32											int32;
typedef signed __int16											int16;
typedef signed __int8											int8;
typedef unsigned __int64										uint64;
typedef unsigned __int32										uint32;
typedef unsigned __int16										uint16;
typedef unsigned __int8											uint8;
typedef INT_PTR													intptr;
typedef UINT_PTR        										uintptr;
#define PRI64													"lld"
#define PRIu64													"llu"
#define PRIx64													"llx"
#define PRIX64													"llX"
#define PRIzu													"lu"
#define PRIzd													"ld"
#define PRTime													PRI64
#else
typedef int64_t													int64;
typedef int32_t													int32;
typedef int16_t													int16;
typedef int8_t													int8;
typedef uint64_t												uint64;
typedef uint32_t												uint32;
typedef uint16_t												uint16;
typedef uint8_t													uint8;
typedef uint16_t												WORD;
typedef uint32_t												DWORD;

#ifdef _LP64
typedef int64													intptr;
typedef uint64													uintptr;
#define PRI64													"ld"
#define PRIu64													"lu"
#define PRIx64													"lx"
#define PRIX64													"lX"
#define PRTime													PRI64
#else
typedef int32													intptr;
typedef uint32													uintptr;
#define PRI64													"lld"
#define PRIu64													"llu"
#define PRIx64													"llx"
#define PRIX64													"llX"
#define PRTime													"ld"
#endif

#ifndef PRIzd
#define PRIzd													"zd"
#endif

#ifndef PRIzu
#define PRIzu													"zu"
#endif

#endif

#define PRAppID													PRIu64
#define PRDBID													PRIu64

typedef uint16													ENTITY_TYPE;											// entity��������Ͷ���֧��0-65535�����
typedef int32													ENTITY_ID;												// entityID������
typedef uint32													SPACE_ID;												// һ��space��id
typedef uint32													CALLBACK_ID;											// һ��callback��CallbackMgr�����id
typedef uint64													COMPONENT_ID;											// һ�������������id
typedef int8													COMPONENT_ORDER;										// һ�����������˳��
typedef	uint32													TIMER_ID;												// һ��timer��id����
typedef uint8													MAIL_TYPE;												// mailbox ��Ͷ�ݵ�mail�������
typedef uint32													GAME_TIME;
typedef uint32													GameTime;
typedef int32													ScriptID;
typedef uint32													ArraySize;												// �κ�����Ĵ�С�����������
typedef uint64													DBID;													// һ�������ݿ��е�������������ĳID

#if KBE_PLATFORM == PLATFORM_WIN32
	#define IFNAMSIZ											16
	typedef SOCKET												KBESOCKET;
#ifndef socklen_t
	typedef	int													socklen_t;
#endif
	typedef u_short												u_int16_t;
	typedef u_long												u_int32_t;
	
#ifndef IFF_UP
	enum
	{
		IFF_UP													= 0x1,
		IFF_BROADCAST											= 0x2,
		IFF_DEBUG												= 0x4,
		IFF_LOOPBACK											= 0x8,
		IFF_POINTOPOINT											= 0x10,
		IFF_NOTRAILERS											= 0x20,
		IFF_RUNNING												= 0x40,
		IFF_NOARP												= 0x80,
		IFF_PROMISC												= 0x100,
		IFF_MULTICAST											= 0x1000
	};
#endif
#else
	typedef int													KBESOCKET;
#endif

/*---------------------------------------------------------------------------------
	�������ƽ̨�ϵĶ��߳����
---------------------------------------------------------------------------------*/
#if KBE_PLATFORM == PLATFORM_WIN32
	#define THREAD_ID											HANDLE
	#define THREAD_SINGNAL										HANDLE
	#define THREAD_SINGNAL_INIT(x)								x = CreateEvent(NULL, TRUE, FALSE, NULL)
	#define THREAD_SINGNAL_DELETE(x)							CloseHandle(x)
	#define THREAD_SINGNAL_SET(x)								SetEvent(x)
	#define THREAD_MUTEX										CRITICAL_SECTION
	#define THREAD_MUTEX_INIT(x)								InitializeCriticalSection(&x)
	#define THREAD_MUTEX_DELETE(x)								DeleteCriticalSection(&x)
	#define THREAD_MUTEX_LOCK(x)								EnterCriticalSection(&x)
	#define THREAD_MUTEX_UNLOCK(x)								LeaveCriticalSection(&x)	
#else
	#define THREAD_ID											pthread_t
	#define THREAD_SINGNAL										pthread_cond_t
	#define THREAD_SINGNAL_INIT(x)								pthread_cond_init(&x, NULL)
	#define THREAD_SINGNAL_DELETE(x)							pthread_cond_destroy(&x)
	#define THREAD_SINGNAL_SET(x)								pthread_cond_signal(&x);
	#define THREAD_MUTEX										pthread_mutex_t
	#define THREAD_MUTEX_INIT(x)								pthread_mutex_init (&x, NULL)
	#define THREAD_MUTEX_DELETE(x)								pthread_mutex_destroy(&x)
	#define THREAD_MUTEX_LOCK(x)								pthread_mutex_lock(&x)
	#define THREAD_MUTEX_UNLOCK(x)								pthread_mutex_unlock(&x)		
#endif

/*---------------------------------------------------------------------------------
	��ƽ̨�궨��
---------------------------------------------------------------------------------*/
#if 0
#define ARRAYCLR(v)					memset((v), 0x0, sizeof(v))
#define MEMCLR(v)					memset(&(v), 0x0, sizeof(v))
#define MEMCLRP(v)					memset((v), 0x0, sizeof(*v))
#endif

#define ARRAYSZ(v)					(sizeof(v) / sizeof(v[0]))
#define ARRAY_SIZE(v)				(sizeof(v) / sizeof(v[0]))

#if 0
#define offsetof(type, field)		((uint32)&(((type *)NULL)->field))
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)	offsetof(type, field)
#endif
#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field)		(sizeof(((type *)NULL)->field))
#endif
#endif

#define KBE_LITTLE_ENDIAN
/*#define KBE_BIG_ENDIAN*/

#ifdef KBE_LITTLE_ENDIAN
/* accessing individual bytes (int8) and words (int16) within
 * words and long words (int32).
 * Macros ending with W deal with words, L macros deal with longs
 */
/// Returns the high byte of a word.
#define HIBYTEW(b)		(((b) & 0xff00) >> 8)
/// Returns the low byte of a word.
#define LOBYTEW(b)		( (b) & 0xff)

/// Returns the high byte of a long.
#define HIBYTEL(b)		(((b) & 0xff000000L) >> 24)
/// Returns the low byte of a long.
#define LOBYTEL(b)		( (b) & 0xffL)

/// Returns byte 0 of a long.
#define BYTE0L(b)		( (b) & 0xffL)
/// Returns byte 1 of a long.
#define BYTE1L(b)		(((b) & 0xff00L) >> 8)
/// Returns byte 2 of a long.
#define BYTE2L(b)		(((b) & 0xff0000L) >> 16)
/// Returns byte 3 of a long.
#define BYTE3L(b)		(((b) & 0xff000000L) >> 24)

/// Returns the high word of a long.
#define HIWORDL(b)		(((b) & 0xffff0000L) >> 16)
/// Returns the low word of a long.
#define LOWORDL(b)		( (b) & 0xffffL)

/**
 *	This macro takes a dword ordered 0123 and reorder it to 3210.
 */
#define SWAP_DW(a)	  ( (((a) & 0xff000000)>>24) |	\
						(((a) & 0xff0000)>>8) |		\
						(((a) & 0xff00)<<8) |		\
						(((a) & 0xff)<<24) )

#else
/* big endian macros go here */
#endif

#if defined(_WIN32)

#undef min
#define min min
#undef max
#define max max

template <class T>
inline const T & min( const T & a, const T & b )
{
	return b < a ? b : a;
}

template <class T>
inline const T & max( const T & a, const T & b )
{
	return a < b ? b : a;
}

#define KBE_MIN min
#define KBE_MAX max

#define NOMINMAX

#else

#define KBE_MIN std::min
#define KBE_MAX std::max

#endif

// ���������ַ�������󳤶�
#define MAX_NAME 256	
// ip�ַ�������󳤶�
#define MAX_IP 50
// �����buf����
#define MAX_BUF 256

// ���ϵͳ���������һ�δ�������
inline char* kbe_strerror(int ierrorno = 0)
{
#if KBE_PLATFORM == PLATFORM_WIN32
	if(ierrorno == 0)
		ierrorno = GetLastError();

	static char lpMsgBuf[256] = {0};
	
	/*
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		ierrorno,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		1024,
		NULL
	); 
	*/
	kbe_snprintf(lpMsgBuf, 256, "errorno=%d",  ierrorno);
	return lpMsgBuf;
#else
	if(ierrorno != 0)
		return strerror(ierrorno);
	return strerror(errno);
#endif
}

inline int kbe_lasterror()
{
#if KBE_PLATFORM == PLATFORM_WIN32
	return GetLastError();
#else
	return errno;
#endif
}

/** ��ȡ�û�UID */
inline int32 getUserUID()
{
#if KBE_PLATFORM == PLATFORM_WIN32
	// VS2005:
	#if _MSC_VER >= 1400
		char uid[16];
		size_t sz;
		return getenv_s( &sz, uid, sizeof( uid ), "UID" ) == 0 ? atoi( uid ) : 0;

	// VS2003:
	#elif _MSC_VER < 1400
		char * uid = getenv( "UID" );
		return uid ? atoi( uid ) : 0;
	#endif
#else
// Linux:
	char * uid = getenv( "UID" );
	return uid ? atoi( uid ) : getuid();
#endif
}

/** ��ȡ�û��� */
inline const char * getUsername()
{
#if KBE_PLATFORM == PLATFORM_WIN32
	DWORD dwSize = MAX_NAME;
	static char username[MAX_NAME];
	memset(username, 0, MAX_NAME);
	::GetUserNameA(username, &dwSize);	
	return username;
#else
	char * pUsername = cuserid( NULL );
	return pUsername ? pUsername : "";
#endif
}

/** ��ȡ����ID */
inline int32 getProcessPID()
{
#if KBE_PLATFORM != PLATFORM_WIN32
	return getpid();
#else
	return (int32) GetCurrentProcessId();
#endif
}

/** ��ȡϵͳʱ�� */
#if KBE_PLATFORM == PLATFORM_WIN32
	inline uint32 getSystemTime() 
	{ 
		// ע���������windows��ֻ����ȷά��49�졣
		return ::GetTickCount(); 
	};
#else
	inline uint32 getSystemTime()
	{
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	};
#endif

/** ��ȡ2��ϵͳʱ��� */
inline uint32 getSystemTimeDiff(uint32 oldTime, uint32 newTime)
{
    // getSystemTime() have limited data range and this is case when it overflow in this tick
    if (oldTime > newTime)
        return (0xFFFFFFFF - oldTime) + newTime;
    else
        return newTime - oldTime;
}

/* ����һ��64λ��uuid 
*/
extern COMPONENT_ORDER g_componentOrder;

inline uint64 genUUID64()
{
	srand(getSystemTime());
	
	if(g_componentOrder <= 0)
	{
		// ʱ���32λ�� 16λ������� 16λ������
		uint64 tv = getSystemTime();
		static uint16 lastNum0 = rand() % 65535;
		uint32 rnd = rand() % 65535;
		return (tv << 32) + (rnd << (24)) + lastNum0++;
	}
	else
	{
		// app˳����8λ��ʱ���32λ�� 8λ������� 16λ������
		uint32 tv = getSystemTime();
		uint64 appOrder = g_componentOrder;
		static uint16 lastNum1 = rand() % 65535;
		uint32 rnd = rand() % 255;
		return (appOrder << 56) + (tv << (24)) + (rnd << (8)) + lastNum1++;
	}
}

/** sleep ��ƽ̨ */
#if KBE_PLATFORM == PLATFORM_WIN32
	inline void sleep(uint32 ms)
	{ 
		::Sleep(ms); 
	}
#else
	inline void sleep(uint32 ms)
	{ 
	  struct timeval tval;
	  tval.tv_sec	= ms / 1000;
	  tval.tv_usec	= ( ms * 1000) % 1000000;
	  select(0, NULL, NULL, NULL, &tval);
	}	
#endif

/** �ж�ƽ̨�Ƿ�ΪС���ֽ��� */
inline bool isPlatformLittleEndian()
{
   int n = 1;
   return *((char*)&n)? true:false;
}

/** �������Ƚ� */
#define floatEqual(v1, v3) (abs(v1 - v2) < std::numeric_limits<float>::epsilon())
inline bool almostEqual(const float f1, const float f2, const float epsilon = 0.0004f)
{
	return fabsf( f1 - f2 ) < epsilon;
}

inline bool almostEqual(const double d1, const double d2, const double epsilon = 0.0004)
{
	return fabs( d1 - d2 ) < epsilon;
}

inline bool almostZero(const float f, const float epsilon = 0.0004f)
{
	return f < epsilon && f > -epsilon;
}

inline bool almostZero(const double d, const double epsilon = 0.0004)
{
	return d < epsilon && d > -epsilon;
}

template<typename T>
inline bool almostEqual(const T& c1, const T& c2, const float epsilon = 0.0004f)
{
	if( c1.size() != c2.size() )
		return false;
	typename T::const_iterator iter1 = c1.begin();
	typename T::const_iterator iter2 = c2.begin();
	for( ; iter1 != c1.end(); ++iter1, ++iter2 )
		if( !almostEqual( *iter1, *iter2, epsilon ) )
			return false;
	return true;
}

/** �����������Ż� */
#if KBE_COMPILER == COMPILER_MICROSOFT && _MSC_VER >= 1400
#pragma float_control(push)
#pragma float_control(precise, on)
#endif

// fast int abs
static inline int int32abs( const int value )
{
	return (value ^ (value >> 31)) - (value >> 31);
}

// fast int abs and recast to unsigned
static inline uint32 int32abs2uint32( const int value )
{
	return (uint32)(value ^ (value >> 31)) - (value >> 31);
}

/// Fastest Method of float2int32
static inline int float2int32(const float value)
{
#if !defined(X64) && KBE_COMPILER == COMPILER_MICROSOFT 
	int i;
	__asm {
		fld value
		frndint
		fistp i
	}
	return i;
#else
	union { int asInt[2]; double asDouble; } n;
	n.asDouble = value + 6755399441055744.0;

	return n.asInt [0];
#endif
}

/// Fastest Method of long2int32
static inline int long2int32(const double value)
{
#if !defined(X64) && KBE_COMPILER == COMPILER_MICROSOFT
	int i;
	__asm {
		fld value
		frndint
		fistp i
	}
	return i;
#else
  union { int asInt[2]; double asDouble; } n;
  n.asDouble = value + 6755399441055744.0;
  return n.asInt [0];
#endif
}

/** �����������Ż����� */
#if KBE_COMPILER == COMPILER_MICROSOFT && _MSC_VER >= 1400
#pragma float_control(pop)
#endif


inline char* wchar2char(const wchar_t* ts)
{
	int len = (wcslen(ts) + 1) * 4;
	char* ccattr =(char *)malloc(len);
	memset(ccattr, 0, len);
    wcstombs(ccattr, ts, len);
	return ccattr;
};

inline wchar_t* char2wchar(const char* cs)
{
	int len = (strlen(cs) + 1) * 4;
	wchar_t* ccattr =(wchar_t *)malloc(len);
	memset(ccattr, 0, len);
    mbstowcs(ccattr, cs, len);
	return ccattr;
};

inline int wchar2utf8(const wchar_t* in, int in_len, char* out, int out_max)   
{   
#ifdef WIN32   
    BOOL use_def_char;   
    use_def_char = FALSE;   
    return ::WideCharToMultiByte(CP_UTF8, 0, in,in_len / sizeof(wchar_t), out, out_max, NULL, NULL);   
#else   
    size_t result;   
    iconv_t env;   
   
    env = iconv_open("UTF8", "WCHAR_T");   
    result = iconv(env,(char**)&in,(size_t*)&in_len,(char**)&out,(size_t*)&out_max);        
    iconv_close(env);   
    return (int) result;   
#endif   
}   
   
inline int wchar2utf8(const std::wstring& in, std::string& out)   
{   
    int len = in.length() + 1;   
    int result;   

    char* pBuffer = new char[len * 4];   

    memset(pBuffer,0,len * 4);               

    result = wchar2utf8(in.c_str(), in.length() * sizeof(wchar_t), pBuffer,len * 4);   

    if(result >= 0)   
    {   
        out = pBuffer;   
    }   
    else   
    {   
        out = "";   
    }   

    delete[] pBuffer;   
    return result;   
}   
   
inline int utf82wchar(const char* in, int in_len, wchar_t* out, int out_max)   
{   
#ifdef WIN32   
    return ::MultiByteToWideChar(CP_UTF8, 0, in, in_len, out, out_max);   
#else   
    size_t result;   
    iconv_t env;   
    env = iconv_open("WCHAR_T", "UTF8");   
    result = iconv(env,(char**)&in, (size_t*)&in_len, (char**)&out,(size_t*)&out_max);   
    iconv_close(env);   
    return (int) result;   
#endif   
}   
   
inline int utf82wchar(const std::string& in, std::wstring& out)   
{   
    int len = in.length() + 1;   
    int result;   
 
    wchar_t* pBuffer = new wchar_t[len];   
    memset(pBuffer,0,len * sizeof(wchar_t));   
    result = utf82wchar(in.c_str(), in.length(), pBuffer, len*sizeof(wchar_t));   

    if(result >= 0)   
    {   
        out = pBuffer;   
    }   
    else   
    {   
        out.clear();         
    }   

    delete[] pBuffer;   
    return result;   
}   

}
#endif
