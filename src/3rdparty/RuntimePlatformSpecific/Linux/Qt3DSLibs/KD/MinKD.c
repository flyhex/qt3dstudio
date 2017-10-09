#include "kd.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#ifndef __INTEGRITY
#include <memory.h>
#endif
#include <pthread.h>
#include <dirent.h>

#ifdef __INTEGRITY
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 4
#endif
#endif

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
    term = memchr(src, 0, srclen);
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
    const char* p = memchr(src, 0, srcmaxlen);
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
    pthread_mutex_t p_mutex;
};

/* kdThreadMutexCreate: Create a mutex. */
KD_API KDThreadMutex* kdThreadMutexCreate(const void *mutexattr)
{
	KDThreadMutex* mutex = malloc(sizeof(KDThreadMutex));
	if (mutex)
	{
		if (pthread_mutex_init(&mutex->p_mutex, NULL) == 0)
		{
			return mutex;
		}
		free(mutex);
	}

	return KD_NULL;
}

/* kdThreadMutexFree: Free a mutex. */
KDint kdThreadMutexFree(KDThreadMutex *mutex)
{
    int res;
    res = pthread_mutex_destroy(&mutex->p_mutex);
    // If the mutex destroy failed, don't free the memory.
    // Per OpenKODE spec, this is undefined behaviour, and
    // we can leak the memory. The reason for this is that
    // later allocations may fail if they happen to go
    // to the same memory
    if (!res)
        free(mutex);
    return 0;
}

/* kdThreadMutexLock: Lock a mutex. */
KDint kdThreadMutexLock(KDThreadMutex *mutex)
{
    (void)pthread_mutex_lock(&mutex->p_mutex);
    return 0;
}

/* kdThreadMutexUnlock: Unlock a mutex. */
KDint kdThreadMutexUnlock(KDThreadMutex *mutex)
{
    pthread_mutex_unlock(&mutex->p_mutex);
    return 0;
}

struct KDThreadAttr
{
    pthread_attr_t p_attr;
};

struct KDThread
{
    pthread_t           Handle;
    uint8_t             Detached;
    uint8_t             IsRunning;
    uint8_t             UserThread;
    void*               (*UserThreadProc)(void*);
    void*               UserArg;
};
/* kdThreadCreate: Create a new thread. */
KDThread * kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
	pthread_t thread;
	int retval;
	const pthread_attr_t* p_attr;
    KDThread* newThread;
	if ( attr )
		p_attr = &attr->p_attr;
	else
		p_attr = KD_NULL;

	retval = pthread_create( &thread, p_attr, start_routine, arg );
	if ( retval != 0 )
	{
		return KD_NULL;
	}
	newThread = (KDThread*)malloc( sizeof( KDThread ) );
	if ( newThread == KD_NULL ) return KD_NULL;
	newThread->Handle = thread;
	newThread->Detached = 0;
	newThread->IsRunning = 1;
	newThread->UserThread = 1;
	newThread->UserThreadProc = start_routine;
	newThread->UserArg = arg;
	return newThread;
}

/* kdThreadExit: Terminate this thread. */
void kdThreadExit(void *retval)
{
	pthread_exit( retval );
}

/* kdThreadJoin: Wait for termination of another thread. */
KDint kdThreadJoin(KDThread *thread, void **retval)
{
	int join_retval;
	join_retval = pthread_join( thread->Handle, retval );
	if ( join_retval == 0 ) free( thread );
	return join_retval;
}

typedef struct KDDirInternal {
    DIR *dir;
    KDDirent ent;
} KDDirInternal;

/* kdOpenDir: Open a directory ready for listing. */
KDDir *kdOpenDir(const KDchar *pathname)
{
    KDDirInternal* retval;
	DIR* dir = opendir( pathname );
	if ( dir == KD_NULL ) return KD_NULL;
	retval = (KDDirInternal*)malloc( sizeof( KDDirInternal ));
	if ( retval == KD_NULL ) return KD_NULL;
	memset( retval, 0, sizeof( *retval ) );
	retval->dir = dir;
	return (KDDir*)retval;
}

/* kdReadDir: Return the next file in a directory. */
KDDirent* kdReadDir(KDDir *dir)
{
    struct dirent *sysDirEnt;
    KDDirent *dirent = NULL;

    sysDirEnt = readdir(((KDDirInternal *)dir)->dir);

    if (sysDirEnt) {
        dirent = &((KDDirInternal *)dir)->ent;
        dirent->d_name = sysDirEnt->d_name;
    }
    return dirent;
}

/* kdCloseDir: Close a directory. */
KDint kdCloseDir(KDDir *dir)
{
    closedir(((KDDirInternal *)dir)->dir);
    free(dir);
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

KDust kdGetTimeUST(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);

    return (NvS64)tp.tv_sec * (NvS64)1000000000 +
           (NvS64)tp.tv_nsec;
}

static KDTm* gmlocaltime(
    const KDtime *timep,
    KDTm *result,
    struct tm* (* func)(const time_t* timep, struct tm* result))
{
    struct tm tm;
    time_t t = *timep;
    (*func)(&t, &tm);
    result->tm_sec = tm.tm_sec;
    result->tm_min = tm.tm_min;
    result->tm_hour = tm.tm_hour;
    result->tm_mday = tm.tm_mday;
    result->tm_mon = tm.tm_mon;
    result->tm_year = tm.tm_year;
    result->tm_wday = tm.tm_wday;
    result->tm_yday = tm.tm_yday;
    return result;
}

/* kdGmtime_r, kdLocaltime_r: Convert a seconds-since-epoch time into broken-down time. */
KDTm* kdGmtime_r(const KDtime* timep, KDTm* result)
{
    return gmlocaltime(timep, result, &gmtime_r);
}

KDTm* kdLocaltime_r(const KDtime* timep, KDTm* result)
{
    return gmlocaltime(timep, result, &localtime_r);
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
