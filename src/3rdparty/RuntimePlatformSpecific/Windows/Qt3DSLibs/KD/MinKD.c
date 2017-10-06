#include "KD/kd.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include <malloc.h>


KDFile* kdFopen(const KDchar* pathname, const KDchar* mode)
{
	FILE* retval;
	retval = fopen( pathname, mode );
	return (KDFile*)retval;
}


KDsize kdFread(void *buffer, KDsize size, KDsize count, KDFile *file)
{
	return fread( buffer, size, count, (FILE*)file );
}

/* kdFwrite: Write to a file. */
KDsize kdFwrite(const void *buffer, KDsize size, KDsize count, KDFile *file)
{
	return fwrite( buffer, size, count, (FILE*)file );
}

KDint kdFclose(KDFile *file)
{
	return fclose( (FILE*)file );
}

/* kdCosf: Cosine function. */
KDfloat32 kdCosf(KDfloat32 x) { return cosf( x ); }

/* kdSinf: Sine function. */
KDfloat32 kdSinf(KDfloat32 x) { return sinf( x ); }

KDfloat32 kdSqrtf(KDfloat32 x ) { return sqrtf( x ); }

KDfloat32 kdTanf( KDfloat32 x ) { return tanf( x ); }

KDint kdAbs(KDint i)
{
	return i < 0 ? -i : i;
}


KDfloat32 kdFabsf(KDfloat32 i) { return fabs( i ); }

/* kdStrcpy_s: Copy a string with an overrun check. */

KDint kdStrcpy_s(KDchar* buf, KDsize buflen, const KDchar* src)
{
    return kdStrncpy_s(buf, buflen, src, -1);
}

/* kdStrncpy_s: Copy a string with an overrun check. */
KDint kdStrncpy_s(KDchar* buf, KDsize buflen, const KDchar* src, KDsize srclen)
{
	const char* term;
	if ( srclen == (KDsize)-1)
		srclen = buflen;

    term = (const char*)memchr(src, 0, srclen);
    if (term)
        srclen = term - src;
    if (srclen >= buflen) {
        if (buflen)
            *buf = 0;
        return KD_EINVAL;
    }
    memcpy(buf, src, srclen);
    buf[srclen] = 0;
    return 0;
}

/* kdStrncat_s: Concatenate two strings. */
KDint kdStrncat_s(KDchar* buf, KDsize buflen, const KDchar* src, KDsize srcmaxlen)
{
    size_t origlen = strlen(buf);
    const char* p = (const char*)memchr(src, 0, srcmaxlen);
    if (p)
        srcmaxlen = p - src;
    if (origlen + srcmaxlen >= buflen)
    {
        /* spec says "buf[0] is set to 0". But that's bad if buflen == 0!
         * kdStrncpy_s's spec is better here. */
        if (buflen)
            buf[0] = 0;
        return KD_ERANGE;
    }
    memcpy(buf + origlen, src, srcmaxlen);
    buf[origlen + srcmaxlen] = 0;
    return 0;
}
/* kdMemset: Set bytes in memory to a value. */
void* kdMemset(void *buf, KDint byte, KDsize len) { return memset( buf, byte, len ); }

/* kdStrcmp: Compares two strings. */
KDint kdStrcmp(const KDchar *str1, const KDchar *str2) { return strcmp( str1, str2); }

/* kdStrlen: Determine the length of a string. */
KDsize kdStrlen(const KDchar *str) { return strlen( str ); }

void* kdMalloc( KDsize size ) { return malloc(size); }

void kdFree( void* ptr ) { free( ptr ); }

void* kdMemcpy(void *buf, const void *src, KDsize len) { return memcpy( buf, src, len ); }

struct KDThreadMutex
{
    CRITICAL_SECTION p_mutex;
};

/* kdThreadMutexCreate: Create a mutex. */
KD_API KDThreadMutex* kdThreadMutexCreate(const void *mutexattr)
{
	KDThreadMutex* mutex = (KDThreadMutex*)malloc(sizeof(KDThreadMutex));
	if (mutex)
	{
		InitializeCriticalSection(&mutex->p_mutex);
		return mutex;
	}

	return KD_NULL;
}

/* kdThreadMutexFree: Free a mutex. */
KDint kdThreadMutexFree(KDThreadMutex *mutex)
{
    DeleteCriticalSection(&mutex->p_mutex);
	free(mutex);
    return 0;
}

/* kdThreadMutexLock: Lock a mutex. */
KDint kdThreadMutexLock(KDThreadMutex *mutex)
{
    (void)EnterCriticalSection(&mutex->p_mutex);
    return 0;
}

/* kdThreadMutexUnlock: Unlock a mutex. */
KDint kdThreadMutexUnlock(KDThreadMutex *mutex)
{
    LeaveCriticalSection(&mutex->p_mutex);
    return 0;
}

struct KDThreadAttr
{
    SECURITY_ATTRIBUTES p_attr;
};

typedef unsigned char uint8_t;
typedef HANDLE pthread_t;

struct KDThread
{
    HANDLE				Handle;
    uint8_t             Detached;
    uint8_t             IsRunning;
    INT					UserThread;
    void*               (*UserThreadProc)(void*);
    void*               UserArg;
};


static DWORD WINAPI ThreadProc( __in  LPVOID lpParameter )
{
	struct KDThread* threadPtr = (struct KDThread*)lpParameter;
	return (DWORD)threadPtr->UserThreadProc( threadPtr->UserArg );
}
/* kdThreadCreate: Create a new thread. */
KDThread * kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
	pthread_t thread;
	int retval;
	SECURITY_ATTRIBUTES* p_attr;
	struct KDThread* newThread;
	if ( attr )
		p_attr = (const SECURITY_ATTRIBUTES*)( &attr->p_attr );
	else
		p_attr = KD_NULL;
	newThread = (struct KDThread*) malloc( sizeof(struct KDThread ) );
	if ( newThread == KD_NULL ) return KD_NULL;
	newThread->Handle;
	newThread->Detached = 0;
	newThread->IsRunning = 1;
	newThread->UserThreadProc = start_routine;
	newThread->UserArg = arg;

	newThread->Handle = CreateThread( p_attr, 0, ThreadProc, newThread, 0, &newThread->UserThread );
	if ( newThread->Handle == 0 )
	{
		free( newThread );
		return KD_NULL;
	}
	return newThread;
}

/* kdThreadExit: Terminate this thread. */
void kdThreadExit(void *retval)
{
	ExitThread( (DWORD)retval );
}

/* kdThreadJoin: Wait for termination of another thread. */
KDint kdThreadJoin(KDThread *thread, void **retval)
{
	WaitForSingleObject( thread->Handle, INFINITE );
	CloseHandle( thread->Handle );
	free( thread );
	return 0;
}

typedef struct KDDirInternal {
	WIN32_FIND_DATAA ffd;
	HANDLE hFind;
	KDDirent dirEnt;
	char pathBuffer[MAX_PATH];
} KDDirInternal;

/* kdOpenDir: Open a directory ready for listing. */
KDDir *kdOpenDir(const KDchar *pathname)
{
	KDDirInternal* retval = (KDDirInternal*)malloc( sizeof( KDDirInternal ));
	if ( retval == NULL ) { return NULL; }

	memset( retval, 0, sizeof( KDDirInternal ) );

	
	retval->hFind = FindFirstFileA( pathname, &retval->ffd );

	if ( retval->hFind ) { free(retval ); return KD_NULL; };
	if ( retval == KD_NULL ) return KD_NULL;
	memset( retval, 0, sizeof( *retval ) );
	return (KDDir*)retval;
}

/* kdReadDir: Return the next file in a directory. */
KDDirent* kdReadDir(KDDir *dir)
{
	KDDirInternal* theDir = (KDDirInternal *)dir;
	INT success;
	if ( *theDir->ffd.cFileName == 0 
		|| theDir->hFind == INVALID_HANDLE_VALUE )
		return NULL;

	strcpy_s( theDir->pathBuffer, MAX_PATH, theDir->ffd.cFileName );
	theDir->dirEnt.d_name = theDir->pathBuffer;

	success = FindNextFile(theDir->hFind, &theDir->ffd);
	if ( success == 0 )
		theDir->ffd.cFileName[0] = 0;

    return &theDir->dirEnt;
}

/* kdCloseDir: Close a directory. */
KDint kdCloseDir(KDDir *dir)
{
	KDDirInternal* theDir = (KDDirInternal *)dir;
	FindClose( theDir->hFind );
    free(theDir);
    return 0;
}

KDint kdSetWindowPropertyiv(KDWindow * win, KDint prop, const KDint32 * val)
{
	return 0;
}

void kdDefaultEvent(const KDEvent *event )
{

}

typedef long long NvS64;

//Stubbed and not implemented.

KDust kdGetTimeUST(void)
{
	KDust retval;
	memset( &retval, 0, sizeof( retval ) );
	return retval;
}

/* kdGmtime_r, kdLocaltime_r: Convert a seconds-since-epoch time into broken-down time. */
KDTm* kdGmtime_r(const KDtime* timep, KDTm* result)
{
	return NULL;
}

KDTm* kdLocaltime_r(const KDtime* timep, KDTm* result)
{
	return NULL;
}

//===========================================================================
// kdCreateEvent: Create an event for posting.
//===========================================================================
KDEvent* kdCreateEvent(void)
{
	//Memory leak and there isn't much I am going to do about this now
	KDEvent* newEvent = (KDEvent*)malloc( sizeof( KDEvent ));
	memset( newEvent, 0, sizeof( KDEvent ));
	return newEvent;
}

KDfloat32 kdInvsqrtf(KDfloat32 x)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = x * 0.5F;
	y  = x;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration

	return y;
}
