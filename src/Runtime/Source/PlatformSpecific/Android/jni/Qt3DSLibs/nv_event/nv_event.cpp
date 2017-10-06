/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define MODULE "NVEvent"
#define DBG_DETAILED 0

#include "nv_event.h"

#include <stdlib.h>
#include <jni.h>
#include <pthread.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include "../nv_time/nv_time.h"
#include "../nv_thread/nv_thread.h"
#include "../nv_debug.h"
#include "scoped_profiler.h"
#include "nv_keycode_mapping.h"
#include "nv_event_queue.h"

// TODO TBD - this should be done in NVTimeInit(), but we use a different
// class than most apps.  Need to clean this up, as it is fragile w.r.t.
// changes in nv_time
extern void nvAcquireTimeExtensionJNI(JNIEnv *, jobject);
extern jlong nvGetSystemTimeJNI(JNIEnv *, jobject);

#define CT_ASSERT(tag, cond) enum { COMPILE_TIME_ASSERT__##tag = 1 / (cond) }

enum {
    // Android lifecycle status flags.  Not app-specific
    // Set between onCreate and onDestroy
    NVEVENT_STATUS_RUNNING = 0x00000001,
    // Set between onResume and onPause
    NVEVENT_STATUS_ACTIVE = 0x00000002,
    // Set between onWindowFocusChanged(true) and (false)
    NVEVENT_STATUS_FOCUSED = 0x00000004,
    // Set when the app's SurfaceHolder points to a
    // valid, nonzero-sized surface
    NVEVENT_STATUS_HAS_REAL_SURFACE = 0x00000008,

    // Mask of all app lifecycle status flags, useful for checking when is it
    // a reasonable time to be setting up EGL and rendering
    NVEVENT_STATUS_INTERACTABLE = 0x0000000f,

    // NvEvent EGL status flags.  Not app-specific
    // Set between calls to NVEventInitEGL and NVEventCleanupEGL
    NVEVENT_STATUS_EGL_INITIALIZED = 0x00000010,
    // Set when the EGL surface is allocated
    NVEVENT_STATUS_EGL_HAS_SURFACE = 0x00000020,
    // Set when a surface and context are available and bound
    NVEVENT_STATUS_EGL_BOUND = 0x00000040,
};

static unsigned int s_appStatus = 0;

static void ZeroAppFlags()
{
    s_appStatus = 0;
}

static void SetAppFlag(unsigned int status)
{
    s_appStatus |= status;
}

static void ClearAppFlag(unsigned int status)
{
    s_appStatus &= ~status;
}

static bool QueryAppFlag(unsigned int status)
{
    return (s_appStatus & status) ? true : false;
}

static bool QueryAppFlagsEqualMasked(unsigned int status, unsigned int mask)
{
    return ((s_appStatus & mask) == status) ? true : false;
}

static NVKeyCodeMapping s_keyMapping;
static NVEventQueue s_eventQueue;
static jobject s_globalThiz;
static jfieldID s_lengthId;
static jfieldID s_dataId;
static jfieldID s_widthId;
static jfieldID s_heightId;
static jfieldID s_texDataId;
static pthread_t s_mainThread;
static bool s_appThreadExited = false;
static bool s_javaPostedQuit = false;

static int NVEVENT_ACTION_DOWN = 0;
static int NVEVENT_ACTION_UP = 0;
static int NVEVENT_ACTION_CANCEL = 0;
static int NVEVENT_ACTION_POINTER_INDEX_MASK = 0;
static int NVEVENT_ACTION_POINTER_INDEX_SHIFT = 0;
static int NVEVENT_ACTION_KEY_UP = 0;

class MethodRef
{
public:
    MethodRef(const char *name, const char *signature)
        : m_name(name)
        , m_signature(signature)
        , m_index(NULL)
    {
    }

    bool QueryID(JNIEnv *env, jclass k)
    {
        m_index = env->GetMethodID(k, m_name, m_signature);
        return true;
    }

    bool CallBoolean()
    {
        JNIEnv *jniEnv = NVThreadGetCurrentJNIEnv();

        if (!jniEnv || !s_globalThiz) {
            __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Error: No valid JNI env in %s", m_name);
            return false;
        }
        if (!m_index) {
            __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Error: No valid function pointer in %s",
                                m_name);
            return false;
        }
        //        __android_log_print(ANDROID_LOG_DEBUG, MODULE,  "Calling JNI up to %s", m_name);
        return jniEnv->CallBooleanMethod(s_globalThiz, m_index);
    }

    bool CallInt()
    {
        JNIEnv *jniEnv = NVThreadGetCurrentJNIEnv();

        if (!jniEnv || !s_globalThiz) {
            __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Error: No valid JNI env in %s", m_name);
            return 0;
        }
        if (!m_index) {
            __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Error: No valid function pointer in %s",
                                m_name);
            return false;
        }
        return (int)jniEnv->CallIntMethod(s_globalThiz, m_index);
    }

    const char *m_name;
    const char *m_signature;
    jmethodID m_index;
};

static MethodRef s_InitEGL("InitEGL", "()Z");
static MethodRef s_CleanupEGL("CleanupEGL", "()Z");
static MethodRef s_CreateSurfaceEGL("CreateSurfaceEGL", "()Z");
static MethodRef s_DestroySurfaceEGL("DestroySurfaceEGL", "()Z");
static MethodRef s_SwapBuffersEGL("SwapBuffersEGL", "()Z");
static MethodRef s_BindSurfaceAndContextEGL("BindSurfaceAndContextEGL", "()Z");
static MethodRef s_UnbindSurfaceAndContextEGL("UnbindSurfaceAndContextEGL", "()Z");
static MethodRef s_GetErrorEGL("GetErrorEGL", "()I");
static MethodRef s_finish("finish", "()V");
static MethodRef
    s_loadFile("loadFile", "(Ljava/lang/String;)Lcom/nvidia/devtech/NvEventQueueActivity$RawData;");
static MethodRef
    s_loadTexture("loadTexture",
                  "(Ljava/lang/String;)Lcom/nvidia/devtech/NvEventQueueActivity$RawTexture;");

// True between onCreate and onDestroy
bool NVEventStatusIsRunning()
{
    // TBD - need to lock a mutex?
    return QueryAppFlag(NVEVENT_STATUS_RUNNING);
}

// True between onResume and onPause
bool NVEventStatusIsActive()
{
    // TBD - need to lock a mutex?
    return QueryAppFlag(NVEVENT_STATUS_ACTIVE);
}

// True between onWindowFocusChanged(true) and (false)
bool NVEventStatusIsFocused()
{
    // TBD - need to lock a mutex?
    return QueryAppFlag(NVEVENT_STATUS_FOCUSED);
}

// True when the app's SurfaceHolder points to a
// valid, nonzero-sized window
bool NVEventStatusHasRealSurface()
{
    // TBD - need to lock a mutex?
    return QueryAppFlag(NVEVENT_STATUS_HAS_REAL_SURFACE);
}

// True when all of IsRunning, IsActive, IsFocused, HasRealSurface are true
// useful for checking when is it a reasonable time to be setting up EGL and rendering
bool NVEventStatusIsInteractable()
{
    // TBD - need to lock a mutex?
    return QueryAppFlagsEqualMasked(NVEVENT_STATUS_INTERACTABLE, NVEVENT_STATUS_INTERACTABLE);
}

// True between calls to NVEventInitEGL and NVEventCleanupEGL
bool NVEventStatusEGLInitialized()
{
    // TBD - need to lock a mutex?
    return QueryAppFlag(NVEVENT_STATUS_EGL_INITIALIZED);
}

// True when the EGL surface is allocated
bool NVEventStatusEGLHasSurface()
{
    // TBD - need to lock a mutex?
    return QueryAppFlag(NVEVENT_STATUS_EGL_HAS_SURFACE);
}

// True when a surface and context are available and bound
bool NVEventStatusEGLIsBound()
{
    // TBD - need to lock a mutex?
    return QueryAppFlag(NVEVENT_STATUS_EGL_BOUND);
}

///////////////////////////////////////////////////////////////////////////////
// Init and shutdown functions

static void NVEventInitFileFunctions(JNIEnv *env)
{
    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "file methods");

    jclass activity_class = env->FindClass("com/nvidia/devtech/NvEventQueueActivity");
    s_loadFile.QueryID(env, activity_class);
    jclass RawData_class = env->FindClass("com/nvidia/devtech/NvEventQueueActivity$RawData");
    s_lengthId = env->GetFieldID(RawData_class, "length", "I");
    s_dataId = env->GetFieldID(RawData_class, "data", "[B");

    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "texture methods");

    s_loadTexture.QueryID(env, activity_class);
    jclass RawTexture_class = env->FindClass("com/nvidia/devtech/NvEventQueueActivity$RawTexture");
    s_widthId = env->GetFieldID(RawTexture_class, "width", "I");
    s_heightId = env->GetFieldID(RawTexture_class, "height", "I");
    s_texDataId = env->GetFieldID(RawTexture_class, "data", "[B");
}

static void NVEventInitInputFields(JNIEnv *env)
{
    jclass Motion_class = env->FindClass("android/view/MotionEvent");
    jfieldID ACTION_DOWN_id = env->GetStaticFieldID(Motion_class, "ACTION_DOWN", "I");
    jfieldID ACTION_UP_id = env->GetStaticFieldID(Motion_class, "ACTION_UP", "I");
    jfieldID ACTION_CANCEL_id = env->GetStaticFieldID(Motion_class, "ACTION_CANCEL", "I");
    jfieldID ACTION_POINTER_INDEX_SHIFT_id =
        env->GetStaticFieldID(Motion_class, "ACTION_POINTER_ID_SHIFT", "I");
    jfieldID ACTION_POINTER_INDEX_MASK_id =
        env->GetStaticFieldID(Motion_class, "ACTION_POINTER_ID_MASK", "I");
    NVEVENT_ACTION_DOWN = env->GetStaticIntField(Motion_class, ACTION_DOWN_id);
    NVEVENT_ACTION_UP = env->GetStaticIntField(Motion_class, ACTION_UP_id);
    NVEVENT_ACTION_CANCEL = env->GetStaticIntField(Motion_class, ACTION_CANCEL_id);
    NVEVENT_ACTION_POINTER_INDEX_MASK =
        env->GetStaticIntField(Motion_class, ACTION_POINTER_INDEX_MASK_id);
    NVEVENT_ACTION_POINTER_INDEX_SHIFT =
        env->GetStaticIntField(Motion_class, ACTION_POINTER_INDEX_SHIFT_id);

    jclass KeyCode_class = env->FindClass("android/view/KeyEvent");
    jfieldID ACTION_KEY_UP_id = env->GetStaticFieldID(KeyCode_class, "ACTION_UP", "I");
    NVEVENT_ACTION_KEY_UP = env->GetStaticIntField(KeyCode_class, ACTION_KEY_UP_id);
}

static void *NVEventMainLoopThreadFunc(void *)
{
    NVEventAppMain(0, NULL);
    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "NvEvent native app Main returned");

    // signal the condition variable to unblock
    // java from waiting on pause or quit
    s_eventQueue.UnblockProducer();

    s_appThreadExited = true;

    // IF that app main returned because we posted a QUIT, then Java knows what to
    // do regarding lifecycle.  But, if the app returned from main of its own accord,
    // we need to call finish.
    if (!s_javaPostedQuit) {
        JNIEnv *env = NVThreadGetCurrentJNIEnv();
        env->CallVoidMethod(s_globalThiz, s_finish.m_index);
    }

    return NULL;
}

NVEventPlatformAppHandle NVEventGetPlatformAppHandle()
{
    return s_globalThiz;
}

///////////////////////////////////////////////////////////////////////////////
// Native event-handling functions

const char *NVEventGetEventStr(NVEventType eventType)
{
    switch (eventType) {
    case QT3DS_EVENT_KEY:
        return "QT3DS_EVENT_KEY";
    case QT3DS_EVENT_CHAR:
        return "QT3DS_EVENT_CHAR";
    case QT3DS_EVENT_TOUCH:
        return "QT3DS_EVENT_TOUCH";
    case QT3DS_EVENT_MULTITOUCH:
        return "QT3DS_EVENT_MULTITOUCH";
    case QT3DS_EVENT_ACCEL:
        return "QT3DS_EVENT_ACCEL";
    case QT3DS_EVENT_START:
        return "QT3DS_EVENT_START";
    case QT3DS_EVENT_RESTART:
        return "QT3DS_EVENT_RESTART";
    case QT3DS_EVENT_RESUME:
        return "QT3DS_EVENT_RESUME";
    case QT3DS_EVENT_FOCUS_GAINED:
        return "QT3DS_EVENT_FOCUS_GAINED";
    case QT3DS_EVENT_SURFACE_CREATED:
        return "QT3DS_EVENT_SURFACE_CREATED";
    case QT3DS_EVENT_SURFACE_SIZE:
        return "QT3DS_EVENT_SURFACE_SIZE";
    case QT3DS_EVENT_SURFACE_DESTROYED:
        return "QT3DS_EVENT_SURFACE_DESTROYED";
    case QT3DS_EVENT_FOCUS_LOST:
        return "QT3DS_EVENT_FOCUS_LOST";
    case QT3DS_EVENT_PAUSE:
        return "QT3DS_EVENT_PAUSE";
    case QT3DS_EVENT_STOP:
        return "QT3DS_EVENT_STOP";
    case QT3DS_EVENT_QUIT:
        return "QT3DS_EVENT_QUIT";
    case QT3DS_EVENT_USER:
        return "QT3DS_EVENT_USER";
    }
    // update this if you end up having to edit something.
    CT_ASSERT(NEED_TO_ADD_STRING_HERE, QT3DS_EVENT_NUM_EVENTS == 17);
    return "unknown event type!";
}

const NVEvent *NVEventGetNextEvent(int waitMSecs)
{
    return s_eventQueue.RemoveOldest(waitMSecs);
}

void NVEventDoneWithEvent(bool handled)
{
    return s_eventQueue.DoneWithEvent(handled);
}

static void NVEventInsert(NVEvent *ev)
{
    if (!s_appThreadExited)
        s_eventQueue.Insert(ev);
}

static bool NVEventInsertBlocking(NVEvent *ev)
{
    if (!s_appThreadExited)
        return s_eventQueue.InsertBlocking(ev);

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Native to Java EGL call-up functions

bool NVEventInitEGL()
{
    if (s_InitEGL.CallBoolean()) {
        SetAppFlag(NVEVENT_STATUS_EGL_INITIALIZED);
        return true;
    } else
        return false;
}

bool NVEventCleanupEGL()
{
    ClearAppFlag(NVEVENT_STATUS_EGL_BOUND);
    ClearAppFlag(NVEVENT_STATUS_EGL_HAS_SURFACE);
    ClearAppFlag(NVEVENT_STATUS_EGL_INITIALIZED);
    return s_CleanupEGL.CallBoolean();
}

bool NVEventCreateSurfaceEGL()
{
    if (s_CreateSurfaceEGL.CallBoolean()) {
        SetAppFlag(NVEVENT_STATUS_EGL_HAS_SURFACE);
        return true;
    } else
        return false;
}

bool NVEventDestroySurfaceEGL()
{
    if (!QueryAppFlag(NVEVENT_STATUS_EGL_HAS_SURFACE))
        return true;

    if (QueryAppFlag(NVEVENT_STATUS_EGL_BOUND))
        NVEventUnbindSurfaceAndContextEGL();

    ClearAppFlag(NVEVENT_STATUS_EGL_HAS_SURFACE);
    return s_DestroySurfaceEGL.CallBoolean();
}

bool NVEventBindSurfaceAndContextEGL()
{
    if (s_BindSurfaceAndContextEGL.CallBoolean()) {
        SetAppFlag(NVEVENT_STATUS_EGL_BOUND);
        return true;
    } else
        return false;
}

bool NVEventUnbindSurfaceAndContextEGL()
{
    ClearAppFlag(NVEVENT_STATUS_EGL_BOUND);
    return s_UnbindSurfaceAndContextEGL.CallBoolean();
}

bool NVEventSwapBuffersEGL()
{
    if (!s_SwapBuffersEGL.CallBoolean())
        return false;
    RESET_PROFILING();
    return true;
}

EGLint NVEventGetErrorEGL()
{
    return s_GetErrorEGL.CallInt();
}

bool NVEventReadyToRenderEGL(bool allocateIfNeeded)
{
    // If we have a bound context and surface, then EGL is ready
    if (!NVEventStatusEGLIsBound()) {
        if (!allocateIfNeeded)
            return false;

        // If we have not bound the context and surface, do we even _have_ a surface?
        if (!NVEventStatusEGLHasSurface()) {
            // No surface, so we need to check if EGL is set up at all
            if (!NVEventStatusEGLInitialized()) {
                if (!NVEventInitEGL())
                    return false;
            }

            // Create the rendering surface now that we have a context
            if (!NVEventCreateSurfaceEGL())
                return false;
        }

        // We have a surface and context, so bind them
        if (NVEventBindSurfaceAndContextEGL())
            return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Input event-related Java to Native callback functions

static jboolean NVEventTouchEvent(JNIEnv *env, jobject thiz, jint action, jint mx, jint my)
{
    {
        NVEvent ev;
        ev.m_type = QT3DS_EVENT_TOUCH;
        ev.m_data.m_touch.m_action = (NVEVENT_ACTION_UP == action)
            ? QT3DS_TOUCHACTION_UP
            : ((NVEVENT_ACTION_DOWN == action) ? QT3DS_TOUCHACTION_DOWN : QT3DS_TOUCHACTION_MOVE);
        ev.m_data.m_touch.m_x = mx;
        ev.m_data.m_touch.m_y = my;
        NVEventInsert(&ev);
    }

    return JNI_TRUE;
}

static jboolean NVEventMultiTouchEvent(JNIEnv *env, jobject thiz, jint action, jint count, jint mx1,
                                       jint my1, jint mx2, jint my2)
{
    {
        NVEvent ev;

        int actionOnly = action & (~NVEVENT_ACTION_POINTER_INDEX_MASK);
        int maskOnly = (count >= 2) ? 0x3 : ((count == 0) ? 0x0 : 0x1);

        ev.m_type = QT3DS_EVENT_MULTITOUCH;

        if (actionOnly == NVEVENT_ACTION_UP) {
            ev.m_data.m_multi.m_action = QT3DS_MULTITOUCH_UP;
        } else if (actionOnly == NVEVENT_ACTION_DOWN) {
            ev.m_data.m_multi.m_action = QT3DS_MULTITOUCH_DOWN;
        } else if (actionOnly == NVEVENT_ACTION_CANCEL) {
            ev.m_data.m_multi.m_action = QT3DS_MULTITOUCH_CANCEL;
        } else {
            ev.m_data.m_multi.m_action = QT3DS_MULTITOUCH_MOVE;
        }
        ev.m_data.m_multi.m_action = (NVMultiTouchEventType)(
            ev.m_data.m_multi.m_action | (maskOnly << QT3DS_MULTITOUCH_POINTER_SHIFT));
        ev.m_data.m_multi.m_x1 = mx1;
        ev.m_data.m_multi.m_y1 = my1;
        ev.m_data.m_multi.m_x2 = mx2;
        ev.m_data.m_multi.m_y2 = my2;
        NVEventInsert(&ev);
    }

    return JNI_TRUE;
}

static jboolean NVEventKeyEvent(JNIEnv *env, jobject thiz, jint action, jint keycode, jint unichar)
{
    // TBD - remove these or make them resettable for safety...
    static int lastKeyAction = 0;
    static int lastKeyCode = 0;
    bool ret = false;

    NVKeyCode code = QT3DS_KEYCODE_NULL;
    if (s_keyMapping.MapKey((int)keycode, code)) {
        if ((code != QT3DS_KEYCODE_NULL) && ((code != lastKeyCode) || (action != lastKeyAction))) {
            NVEvent ev;
            ev.m_type = QT3DS_EVENT_KEY;
            ev.m_data.m_key.m_action =
                (NVEVENT_ACTION_UP == action) ? QT3DS_KEYACTION_UP : QT3DS_KEYACTION_DOWN;
            ev.m_data.m_key.m_code = code;
            ret = NVEventInsertBlocking(&ev);
        }

        lastKeyAction = action;
        lastKeyCode = code;
    }

    if (unichar && (NVEVENT_ACTION_UP != action)) {
        NVEvent ev;
        ev.m_type = QT3DS_EVENT_CHAR;
        ev.m_data.m_char.m_unichar = unichar;
        NVEventInsert(&ev);
    }

    return ret;
}

static jboolean NVEventAccelerometerEvent(JNIEnv *env, jobject thiz, jfloat values0, jfloat values1,
                                          jfloat values2)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_ACCEL;
    ev.m_data.m_accel.m_x = values0;
    ev.m_data.m_accel.m_y = values1;
    ev.m_data.m_accel.m_z = values2;
    NVEventInsert(&ev);
    return JNI_TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Java to Native app lifecycle callback functions

static jboolean onCreateNative(JNIEnv *env, jobject thiz)
{
    ZeroAppFlags();

    if (!s_globalThiz) {
        s_globalThiz = env->NewGlobalRef(thiz);
        if (!s_globalThiz) {
            __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Error: Thiz NewGlobalRef failed!");
        }

        __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Thiz NewGlobalRef: 0x%p", s_globalThiz);
    }

    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Init KeyCode Map");
    s_keyMapping.Init(env, thiz);

    NVEventInitFileFunctions(env);
    NVEventInitInputFields(env);

    s_eventQueue.Init();

    s_javaPostedQuit = false;

    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Calling NVEventAppInit");

    if (NVEventAppInit(0, NULL)) {
        __android_log_print(ANDROID_LOG_DEBUG, MODULE, "NVEventAppInit error");
        return JNI_FALSE;
    }

    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "spawning thread");

    s_appThreadExited = false;
    SetAppFlag(NVEVENT_STATUS_RUNNING);

    /* Launch thread with main loop */
    NVThreadSpawnJNIThread(&s_mainThread, NULL, NVEventMainLoopThreadFunc, NULL);

    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "thread spawned");

    return JNI_TRUE;
}

static jboolean onStartNative(JNIEnv *env, jobject thiz)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_START;
    return NVEventInsertBlocking(&ev);
}

static jboolean onRestartNative(JNIEnv *env, jobject thiz)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_RESTART;
    return NVEventInsertBlocking(&ev);
}

static jboolean onResumeNative(JNIEnv *env, jobject thiz)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_RESUME;
    SetAppFlag(NVEVENT_STATUS_ACTIVE);
    return NVEventInsertBlocking(&ev);
}

static jboolean onSurfaceCreatedNative(JNIEnv *env, jobject thiz, int w, int h)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_SURFACE_CREATED;
    ev.m_data.m_size.m_w = w;
    ev.m_data.m_size.m_h = h;
    if ((w > 0) && (h > 0))
        SetAppFlag(NVEVENT_STATUS_HAS_REAL_SURFACE);
    else
        ClearAppFlag(NVEVENT_STATUS_HAS_REAL_SURFACE);
    return NVEventInsertBlocking(&ev);
}

static jboolean onFocusChangedNative(JNIEnv *env, jobject thiz, jboolean focused)
{
    NVEvent ev;
    ev.m_type = (focused == JNI_TRUE) ? QT3DS_EVENT_FOCUS_GAINED : QT3DS_EVENT_FOCUS_LOST;
    if (focused)
        SetAppFlag(NVEVENT_STATUS_FOCUSED);
    else
        ClearAppFlag(NVEVENT_STATUS_FOCUSED);
    return NVEventInsertBlocking(&ev);
}

static jboolean onSurfaceChangedNative(JNIEnv *env, jobject thiz, int w, int h)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_SURFACE_SIZE;
    ev.m_data.m_size.m_w = w;
    ev.m_data.m_size.m_h = h;
    if (w * h)
        SetAppFlag(NVEVENT_STATUS_HAS_REAL_SURFACE);
    else
        ClearAppFlag(NVEVENT_STATUS_HAS_REAL_SURFACE);
    return NVEventInsertBlocking(&ev);
}

static jboolean onSurfaceDestroyedNative(JNIEnv *env, jobject thiz)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_SURFACE_DESTROYED;
    ClearAppFlag(NVEVENT_STATUS_HAS_REAL_SURFACE);
    return NVEventInsertBlocking(&ev);
}

static jboolean onPauseNative(JNIEnv *env, jobject thiz)
{
    // TODO: we could selectively flush here to
    //       improve responsiveness to the pause
    s_eventQueue.Flush();
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_PAUSE;
    ClearAppFlag(NVEVENT_STATUS_ACTIVE);
    return NVEventInsertBlocking(&ev);
}

static jboolean onStopNative(JNIEnv *env, jobject thiz)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_STOP;
    return NVEventInsertBlocking(&ev);
}

static jboolean onDestroyNative(JNIEnv *env, jobject thiz)
{
    if (!env || !s_globalThiz) {
        __android_log_print(ANDROID_LOG_DEBUG, MODULE,
                            "Error: DestroyingRegisteredObjectInstance no TLS data!");
    }

    if (!s_appThreadExited) {
        __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Posting quit event");

        // flush ALL events
        s_eventQueue.Flush();

        NVEvent ev;
        ev.m_type = QT3DS_EVENT_QUIT;
        ClearAppFlag(NVEVENT_STATUS_RUNNING);

        // We're posting quit, so we need to mark that; when the main loop
        // thread exits, we check this flag to ensure that we only call "finish"
        // if the app returned of its own accord, not if we posted it
        s_javaPostedQuit = true;
        NVEventInsert(&ev);

        // ensure that the native side
        // isn't blocked waiting for an event -- since we've flushed
        // all the events save quit, we must artificially unblock native
        s_eventQueue.UnblockConsumer();

        __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Waiting for main loop exit");
        pthread_join(s_mainThread, NULL);
        __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Main loop exited");
    }

    env->DeleteGlobalRef(s_globalThiz);
    s_globalThiz = NULL;

    __android_log_print(ANDROID_LOG_DEBUG, MODULE, "Released global thiz!");

    s_eventQueue.Shutdown();

    return JNI_TRUE;
}

static jboolean postUserEvent(JNIEnv *env, jobject thiz, jint u0, jint u1, jint u2, jint u3,
                              jboolean blocking)
{
    NVEvent ev;
    ev.m_type = QT3DS_EVENT_USER;
    ev.m_data.m_user.m_u0 = u0;
    ev.m_data.m_user.m_u1 = u1;
    ev.m_data.m_user.m_u2 = u2;
    ev.m_data.m_user.m_u3 = u3;
    if (blocking == JNI_TRUE) {
        return NVEventInsertBlocking(&ev);
    } else {
        NVEventInsert(&ev);
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
// File and APK handling functions

char *NVEventLoadFile(const char *file)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    jstring test = env->NewStringUTF(file);
    jobject rawData = env->CallObjectMethod(s_globalThiz, s_loadFile.m_index, test);

    jbyteArray data = (jbyteArray)env->GetObjectField(rawData, s_dataId);
    int size = env->GetIntField(rawData, s_lengthId);

    unsigned char *data2 = (unsigned char *)env->GetByteArrayElements(data, NULL);
    char *buffer = new char[size + 1];
    memcpy(buffer, data2, size);
    buffer[size] = '\0';

    env->ReleaseByteArrayElements(data, (jbyte *)data2, 0);
    env->DeleteLocalRef(rawData);

    return buffer;
}

void *NVEventGetTextureData(const char *filename, unsigned char *&pixels, unsigned int &width,
                            unsigned int &height, unsigned int &format, unsigned int &type)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    jstring test = env->NewStringUTF(filename);
    jobject rawTexture = env->CallObjectMethod(s_globalThiz, s_loadTexture.m_index, test);

    jbyteArray data = (jbyteArray)env->GetObjectField(rawTexture, s_texDataId);
    width = env->GetIntField(rawTexture, s_widthId);
    height = env->GetIntField(rawTexture, s_heightId);

    format = GL_RGBA;
    type = GL_UNSIGNED_BYTE;

    pixels = (unsigned char *)env->GetByteArrayElements(data, NULL);
    env->DeleteLocalRef(rawTexture);

    return (void *)data;
}

void NVEventReleaseTextureData(void *data, unsigned char *pixels)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    env->ReleaseByteArrayElements((jbyteArray)data, (jbyte *)pixels, JNI_ABORT);
    env->DeleteLocalRef((jbyteArray)data);
}

///////////////////////////////////////////////////////////////////////////////
// JVM Initialization functions

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;

    NVThreadInit(vm);

    DEBUG("JNI_OnLoad called");
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        DEBUG("Failed to get the environment using GetEnv()");
        return -1;
    }
    JNINativeMethod methods[] = {
        { "onCreateNative", "()Z", (void *)onCreateNative },
        { "onStartNative", "()Z", (void *)onStartNative },
        { "onRestartNative", "()Z", (void *)onRestartNative },
        { "onResumeNative", "()Z", (void *)onResumeNative },
        { "onSurfaceCreatedNative", "(II)Z", (void *)onSurfaceCreatedNative },
        { "onFocusChangedNative", "(Z)Z", (void *)onFocusChangedNative },
        { "onSurfaceChangedNative", "(II)Z", (void *)onSurfaceChangedNative },
        { "onSurfaceDestroyedNative", "()Z", (void *)onSurfaceDestroyedNative },
        { "onPauseNative", "()Z", (void *)onPauseNative },
        { "onStopNative", "()Z", (void *)onStopNative },
        { "onDestroyNative", "()Z", (void *)onDestroyNative },
        { "postUserEvent", "(IIIIZ)Z", (void *)postUserEvent },
        { "touchEvent", "(IIILandroid/view/MotionEvent;)Z", (void *)NVEventTouchEvent

        },
        { "multiTouchEvent", "(IIIIIILandroid/view/MotionEvent;)Z", (void *)NVEventMultiTouchEvent

        },
        { "keyEvent", "(IIILandroid/view/KeyEvent;)Z", (void *)NVEventKeyEvent },
        { "accelerometerEvent", "(FFF)Z", (void *)NVEventAccelerometerEvent },
        // TODO TBD - this should be done in NVTimeInit(), but we use a different
        // class than most apps.  Need to clean this up, as it is fragile w.r.t.
        // changes in nv_time
        { "nvAcquireTimeExtension", "()V", (void *)nvAcquireTimeExtensionJNI },
        { "nvGetSystemTime", "()J", (void *)nvGetSystemTimeJNI },
    };
    jclass k;
    k = (env)->FindClass("com/nvidia/devtech/NvEventQueueActivity");
    (env)->RegisterNatives(k, methods, dimof(methods));

    s_InitEGL.QueryID(env, k);
    s_CleanupEGL.QueryID(env, k);
    s_CreateSurfaceEGL.QueryID(env, k);
    s_DestroySurfaceEGL.QueryID(env, k);
    s_SwapBuffersEGL.QueryID(env, k);
    s_BindSurfaceAndContextEGL.QueryID(env, k);
    s_UnbindSurfaceAndContextEGL.QueryID(env, k);
    s_GetErrorEGL.QueryID(env, k);
    s_finish.QueryID(env, k);

    return JNI_VERSION_1_4;
}
