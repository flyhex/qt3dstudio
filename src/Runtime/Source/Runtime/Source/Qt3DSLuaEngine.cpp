/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#include "RuntimePrefix.h"

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSLuaEngine.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEngine.h"
#include "Qt3DSSceneManager.h"
#include "Qt3DSVector3.h"
#include "Qt3DSColor.h"
#include "Qt3DSLuaElementHelper.h"
#include "Qt3DSLuaEventHelper.h"
#include "Qt3DSLuaCommandHelper.h"
#include "Qt3DSLuaSceneHelper.h"
#include "Qt3DSLuaVector.h"
#include "Qt3DSLuaColor.h"
#include "Qt3DSLuaRotation.h"
#include "Qt3DSLuaMatrix.h"
#include "Qt3DSAttributeHashes.h"
#include "Qt3DSFileStream.h"
#include "Qt3DSDataLogger.h"
#include "Qt3DSLuaKeyboard.h"
#include "Qt3DSLuaButton.h"
#include "Qt3DSLuaAxis.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSSystem.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSStateLuaScriptContext.h"
#include "Qt3DSStateDebugger.h"
#include "EventPollingSystemLuaBinding.h"
#include "Qt3DSRenderBufferLoader.h"
#include "Qt3DSRenderInputStreamFactory.h"

#include "EASTL/vector.h"
#include <sys/stat.h>
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRuntimeFactory.h"
#include "Qt3DSStateDebugStreams.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "Qt3DSLuaDebugger.h"
#include "Qt3DSRenderThreadPool.h"
#include "Qt3DSRenderImageBatchLoader.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSAudioPlayer.h"
#include "Qt3DSActivationManager.h"
#include "Qt3DSParametersSystem.h"

#include <QtCore/QFileInfo>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

#if defined(_DEBUG) || (_PROFILE)
#define Q3DStudio_LOG_EVENT(S) // something
#else
#define Q3DStudio_LOG_EVENT(S)
#endif

using qt3ds::runtime::IApplicationCore;
using qt3ds::runtime::IApplication;
using namespace qt3ds;

// forward declaration of exported lua global functions
namespace __SLuaEngineImpl_Static_Global_Lua_Exporter_Declaration__ {
    static int Output(lua_State *inLuaState);
    static int GetScreenInformation(lua_State *inLuaState);
    static int GetElapsedTime(lua_State *inLuaState);
    static int GetMousePosition(lua_State *inLuaState);
    static int GetMultitouch(lua_State *inLuaState);
    static int GetLuaTable(lua_State *inLuaState);
    static int GetDirectory(lua_State *inLuaState);
    static int SetTextureData(lua_State *inLuaState);
    static int TestSetTextureData(lua_State *inLuaState);
    static int LoadImages(lua_State *inLuaState);
    static int MeasureText(lua_State *inLuaState);
    static int PlayToTime(lua_State *inLuaState);
    static int GetSCXMLInterpreter(lua_State *inLuaState);
    static int GetEventSystem(lua_State *inLuaState);
    static int IsPresentationActive(lua_State *inLuaState);
    static int SetPresentationActive(lua_State *inLuaState);
    static int GetPresentationSize(lua_State *inLuaState);
    static int GetDisplayedSize(lua_State *inLuaState);
    static int GetCameraBounds(lua_State *inLuaState);
    static int PositionToScreen(lua_State *inLuaState);
    static int ScreenToPosition(lua_State *inLuaState);
    static int ElementAt(lua_State *inLuaState);
    static int FacePosition(lua_State *inLuaState);
    static int GetMillisecondsSinceLastFrame(lua_State *inLuaState);
    static int PlaySounds(lua_State *inLuaState);
    static int CreateMeshBuffer(lua_State *inLuaState);
}

using namespace __SLuaEngineImpl_Static_Global_Lua_Exporter_Declaration__;

//==============================================================================
//	Constants
//==============================================================================

// for unique addresses to be used as keys for Lua Registry
const INT32 KEY_PRESENTATION = 1;
const INT32 KEY_BEHAVIORS = 2;
const INT32 KEY_EVENTDATA_LIST = 4;
const INT32 KEY_CHANGEDATA_LIST = 5;
const INT32 KEY_CALLBACKS = 6;
const INT32 KEY_REGISTEREDCALLBACKS = 7;

const char *KEY_ELEMENT = "element";
const char *KEY_SELF = "self";
const char *KEY_SCRIPT_INDEX = "__scriptIndex";
const char *KEY_ONINITIALIZE = "onInitialize";
const char *KEY_ONACTIVATE = "onActivate";
const char *KEY_ONDEACTIVATE = "onDeactivate";
const char *KEY_ONUPDATE = "onUpdate";
// extern const char* KEY_ONSLIDEEXIT			= "onSlideExit";
// extern const char* KEY_ONSLIDEENTER			= "onSlideEnter";

const THashValue HASH_ONINITIALIZE = CHash::HashEventCommand(KEY_ONINITIALIZE);
const THashValue HASH_ONACTIVATE = CHash::HashEventCommand(KEY_ONACTIVATE);
const THashValue HASH_ONDEACTIVATE = CHash::HashEventCommand(KEY_ONDEACTIVATE);
const THashValue HASH_ONUPDATE = CHash::HashEventCommand(KEY_ONUPDATE);
// extern const THashValue HASH_ONSLIDEEXIT	= CHash::HashEventCommand( KEY_ONSLIDEEXIT );
// extern const THashValue HASH_ONSLIDEENTER	= CHash::HashEventCommand( KEY_ONSLIDEENTER );

const UINT32 LENGTH_KEY_ELEMENT = (UINT32)::strlen(KEY_ELEMENT);
const UINT32 LENGTH_KEY_SELF = (UINT32)::strlen(KEY_SELF);
const UINT32 LENGTH_KEY_SCRIPT_INDEX = (UINT32)::strlen(KEY_SCRIPT_INDEX);
const UINT32 LENGTH_KEY_ONINITIALIZE = (UINT32)::strlen(KEY_ONINITIALIZE);
const UINT32 LENGTH_KEY_ONACTIVATE = (UINT32)::strlen(KEY_ONACTIVATE);
const UINT32 LENGTH_KEY_ONDEACTIVATE = (UINT32)::strlen(KEY_ONDEACTIVATE);
const UINT32 LENGTH_KEY_ONUPDDATE = (UINT32)::strlen(KEY_ONUPDATE);
// extern const UINT32 LENGTH_KEY_ONSLIDEEXIT	= ::strlen( KEY_ONSLIDEEXIT );
// extern const UINT32 LENGTH_KEY_ONSLIDEENTER	= ::strlen( KEY_ONSLIDEENTER );

const char *SCALE_MODE_FREE = "Free";
const char *SCALE_MODE_FIXED = "Fixed";
const char *SCALE_MODE_ASPECT = "Aspect";
const char *SCALE_MODE_UNINIT = "Unknown";

const THashValue HASH_PROPERTYNAME_FLOAT = CHash::HashString("float");
const THashValue HASH_PROPERTYNAME_FLOAT3 = CHash::HashString("float3");
const THashValue HASH_PROPERTYNAME_LONG = CHash::HashString("long");
const THashValue HASH_PROPERTYNAME_COLOR4 = CHash::HashString("color4");
const THashValue HASH_PROPERTYNAME_BOOL = CHash::HashString("bool");
const THashValue HASH_PROPERTYNAME_STRING = CHash::HashString("string");

struct SLuaEngineImpl;
namespace __SLuaEngineImpl_Basic_Structs__ {
    struct SLuaTimeCallback : public IComponentTimeOverrideFinishedCallback
    {
        SLuaEngineImpl &m_Engine;
        CPresentation &m_Presentation;
        TElement *m_Element;
        UVariant m_CallbackVariant;
        INT32 m_CallbackId;
        bool m_Fired;
        SLuaTimeCallback(SLuaEngineImpl &inEngine, INT32 inCbackId, CPresentation &inPresentation,
                         TElement *inElement)
            : m_Engine(inEngine)
            , m_CallbackId(inCbackId)
            , m_Presentation(inPresentation)
            , m_Element(inElement)
            , m_Fired(false)
        {
        }
        ~SLuaTimeCallback() {}

        void OnTimeFinished() override
        {
            m_CallbackVariant.m_INT32 = m_CallbackId;
            m_Presentation.FireCommand(COMMAND_CUSTOMCALLBACK, m_Element, &m_CallbackVariant);
            m_Fired = true;
        }

        void Release() override;
    };

    struct SImageLoadCallback : public qt3ds::render::IImageLoadListener
    {
        NVFoundationBase &m_Foundation;
        SLuaEngineImpl *m_Engine; // Note the engine may be null in the case of system shutdown.
        QT3DSU32 m_CallbackId;
        QT3DSI32 m_RefCount;
        SImageLoadCallback(NVFoundationBase &fnd, SLuaEngineImpl &inEngine, QT3DSU32 inId)
            : m_Foundation(fnd)
            , m_Engine(&inEngine)
            , m_CallbackId(inId)
            , m_RefCount(0)
        {
        }

        void OnImageLoadComplete(CRegisteredString inPath,
                                         qt3ds::render::ImageLoadResult::Enum inResult) override;

        void OnImageBatchComplete(QT3DSU64) override {}

        void addRef() override { qt3ds::foundation::atomicIncrement(&m_RefCount); }

        void release() override;
    };

    struct SLuaEngineListener : public qt3ds::state::debugger::ILuaSideListener
    {
        NVFoundationBase &m_Foundation;
        SLuaEngineImpl &m_Engine;
        QT3DSI32 m_RefCount;
        SLuaEngineListener(NVFoundationBase &fnd, SLuaEngineImpl &eng)
            : m_Foundation(fnd)
            , m_Engine(eng)
            , m_RefCount(0)
        {
        }

        void addRef() { atomicIncrement(&m_RefCount); }
        void release()
        {
            atomicDecrement(&m_RefCount);
            if (m_RefCount <= 0)
                NVDelete(m_Foundation, this);
        }

        void OnBreak() override;
    };

    struct SLuaPerfEvent
    {
        qt3ds::QT3DSU64 m_Time;
        eastl::string m_Name;
    };

    struct SLuaFunctionResult
    {
        BOOL m_HasFunction;
        BOOL m_PCallError;
        SLuaFunctionResult()
            : m_HasFunction(false)
            , m_PCallError(false)
        {
        }
        SLuaFunctionResult(BOOL inHS, BOOL inPC)
            : m_HasFunction(inHS)
            , m_PCallError(inPC)
        {
        }
    };

    struct SLuaStackScope
    {
        lua_State *m_LuaState;
        int m_Top;
        SLuaStackScope(lua_State *s)
            : m_LuaState(s)
            , m_Top(lua_gettop(s))
        {
        }
        ~SLuaStackScope() { lua_settop(m_LuaState, m_Top); }
    };

    struct LuaScopedLock
    {
        qt3ds::foundation::Mutex *m_Mutex;

        LuaScopedLock(qt3ds::foundation::Mutex *mtx)
            : m_Mutex(mtx)
        {
            if (m_Mutex)
                m_Mutex->lock();
        }

        ~LuaScopedLock()
        {
            if (m_Mutex)
                m_Mutex->unlock();
        }
    };

    struct SUserFile
    {
        bool m_EOF;
        FILE *m_File;
        NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> m_InputStream;
        eastl::vector<char8_t> m_Buffer;

        SUserFile()
            : m_File(NULL)
            , m_EOF(true)
        {
        }
        SUserFile(FILE *f)
            : m_File(f)
            , m_EOF(false)
        {
        }
        SUserFile(qt3ds::render::IRefCountedInputStream &stream)
            : m_File(NULL)
            , m_InputStream(stream)
            , m_EOF(false)
        {
        }
        ~SUserFile()
        {
            if (m_File)
                fclose(m_File);
        }

        size_t read(void *buffer, size_t len)
        {
            size_t retval = 0;
            char *writeBuffer = (char *)buffer;
            if (m_Buffer.size()) {
                QT3DSU32 amount = qt3ds::NVMin((QT3DSU32)m_Buffer.size(), (QT3DSU32)len);
                memcpy(writeBuffer, m_Buffer.data(), amount);
                m_Buffer.erase(m_Buffer.begin(), m_Buffer.begin() + amount);
                writeBuffer += amount;
                len -= amount;
                retval += amount;
            }
            if (len) {
                qt3ds::QT3DSU32 amountRead = 0;
                if (m_InputStream)
                    amountRead = m_InputStream->Read(
                        qt3ds::foundation::toDataRef((qt3ds::QT3DSU8 *)writeBuffer, (qt3ds::QT3DSU32)len));
                else if (m_File)
                    amountRead = (QT3DSU32)fread(writeBuffer, 1, len, m_File);

                retval += amountRead;
                m_EOF = amountRead == 0;
            }
            return retval;
        }

        void unget(int c)
        {
            if (c != EOF)
                m_Buffer.push_back((char8_t)c);
        }

        bool eof() const { return m_EOF; }

        static bool is_white(char d) { return d == '\n' || d == ' ' || d == '\r' || d == '\t'; }

        // read till not white, discarding everything that is white
        void read_till_not_white()
        {
            char data = ' ';
            while (!eof() && is_white(data)) {
                read(&data, 1);
            }
            if (!eof())
                m_Buffer.push_back(data);
        }

        // read till we find whitespace or eof
        void read_till_white()
        {
            char data = '1';
            while (!eof() && is_white(data) == false) {
                read(&data, 1);
                m_Buffer.push_back(data);
            }
        }

        int read_number(float &outNum)
        {
            outNum = 0;
            // read in whitespace till we have data
            // then read till we get whitespace or eof
            read_till_not_white();
            read_till_white();
            if (m_Buffer.size() > 0) {
                // now add a null character
                m_Buffer.push_back(0);
                double d = strtod(m_Buffer.data(), NULL);
                // get rid of null character
                m_Buffer.resize(m_Buffer.size() - 1);
                if (!eof())
                    m_Buffer.erase(m_Buffer.begin(), m_Buffer.begin() + m_Buffer.size() - 1);
                else
                    m_Buffer.clear();
                outNum = static_cast<float>(d);
                return 1;
            }
            return 0;
        }
    };
}
using namespace __SLuaEngineImpl_Basic_Structs__;
#define LUA_ENGINE_MULTITHREAD_PROTECT_METHOD LuaScopedLock __locker(m_MultithreadedMutex);

int CScriptEngineCallFunctionArgRetriever::RetrieveArgument(lua_State *inState)
{
    int theRet = 0;
    if (m_ArgumentString && *m_ArgumentString) {
        eastl::string temp(m_ArgumentString);
        temp.insert(0, "return ");
        if (luaL_loadstring(inState, temp.c_str()) != 0) {
            theRet = -1;
        } else {
            theRet = lua_pcall(inState, 0, 1, 0) ? -1 : 1;
        }
    }
    return theRet;
}

eastl::string CScriptEngineCallFunctionArgRetriever::GetArgDescription()
{
    eastl::string temp(m_ArgumentString);
    temp.insert(0, "return ");
    return temp;
}

namespace {
    qt3ds::render::IInputStreamFactory *g_InputStreamFactory;

    void *lua_user_fopen(const char *filename, const char *mode, char *absfilename)
    {
        Q3DStudio_UNREFERENCED_PARAMETER(mode);
        // Ensure this isn't a shared library of sorts.
        /*size_t len = strlen( filename );
        if ( len > 2 )
        {
                size_t end = len - 3;
                const char* ext = filename + end;
                //Do not use the abstraction for shared libraries or dlls.
                if ( strcmp( ext, "dll" ) == 0
                        || strcmp( ext, ".so" ) == 0
                        || strcmp( ext, "lib" ) == 0 )
                {
                        FILE* testFile = fopen( filename, mode );
                        if ( testFile )
                        {
                                return new SUserFile( testFile );
                        }
                        return NULL;
                }
        }*/
        qt3ds::render::IRefCountedInputStream *stream =
            g_InputStreamFactory->GetStreamForFile(filename, true);
        if (stream) {
            if (absfilename) {
                QString tempStr;
                if (g_InputStreamFactory->GetPathForFile(filename, tempStr)) {
                    strcpy(absfilename, tempStr.toUtf8().data());
                }
            }
            return new SUserFile(*stream);
        }

        return NULL;
    }

    int lua_user_fclose(void *file)
    {
        SUserFile *theFile = reinterpret_cast<SUserFile *>(file);
        if (theFile)
            delete theFile;
        return 0;
    }

    int lua_user_getc(void *file)
    {
        SUserFile *theFile = reinterpret_cast<SUserFile *>(file);
        if (theFile) {
            char8_t temp;
            int numRead = (int)theFile->read(&temp, 1);
            if (numRead == 1)
                return temp;
        }
        return -1;
    }
    int lua_user_ungetc(int c, void *file)
    {
        SUserFile *theFile = reinterpret_cast<SUserFile *>(file);
        if (theFile)
            theFile->unget(c);
        return 0;
    }

    char *lua_user_fgets(char *ptr, int count, void *file)
    {
        SUserFile *theFile = reinterpret_cast<SUserFile *>(file);
        if (theFile && theFile->read(ptr, count))
            return ptr;
        return NULL;
    }

    size_t lua_user_fread(void *ptr, size_t size, size_t count, void *file)
    {
        SUserFile *theFile = reinterpret_cast<SUserFile *>(file);
        size_t amount = size * count;
        if (theFile)
            return theFile->read(ptr, amount);
        return 0;
    }

    int lua_user_ferror(void *) { return 0; }

    int lua_user_feof(void *file)
    {
        SUserFile *theFile = reinterpret_cast<SUserFile *>(file);
        if (theFile)
            return theFile->eof();
        return 0;
    }

    int lua_user_fread_number(void *file, lua_Number *d)
    {
        SUserFile *theFile = reinterpret_cast<SUserFile *>(file);
        if (theFile)
            return theFile->read_number(*d);
        return 0;
    }

    void lua_user_clearerr(void *) { (void)0; }
}

namespace {

    enum EPathType { eLuaPath, eCPath };
    const char *const sm_PathType[] = { "path", "cpath" };
    const char *const sm_PathTypeExtension[] = {
        "lua",
#ifdef _WIN32
        "dll",
#else
        "so"
#endif
    };
    static void AppendLuaPackagePath(lua_State *state, const eastl::string &inPath,
                                     EPathType inPathType)
    {
        if (state == NULL)
            return;
        SLuaStackScope __selfScope(state);
        lua_getglobal(state, "package");
        lua_getfield(state, -1, sm_PathType[inPathType]);
        eastl::string cur_path(lua_tostring(state, -1));
        cur_path.append(";");
        cur_path.append(inPath.c_str());
        cur_path.append("/?.");
        cur_path.append(sm_PathTypeExtension[inPathType]);
        lua_pop(state, 1);
        lua_pushstring(state, cur_path.c_str());
        lua_setfield(state, -2, sm_PathType[inPathType]);
    }

    static void AppendAdditionalLuaPackagePath(lua_State *state, const eastl::string &inPath)
    {
        AppendLuaPackagePath(state, inPath, eCPath);

        eastl::string scriptsDir;
        CFileTools::CombineBaseAndRelative(inPath.c_str(), "scripts", scriptsDir);
        AppendLuaPackagePath(state, scriptsDir, eCPath);

        // Add in additional search directory for specific platform
        eastl::string scriptsPlatformDir;
        CFileTools::CombineBaseAndRelative(scriptsDir.c_str(), System::getPlatformStr(),
                                           scriptsPlatformDir);
        AppendLuaPackagePath(state, scriptsPlatformDir, eCPath);
    }
}

namespace {

    static void MapPresentationToScriptIndex(lua_State *state, IPresentation &pres, int idx)
    {
        SLuaStackScope __scope(state);
        lua_getfield(state, LUA_REGISTRYINDEX, "__script_index_to_presentation");
        if (lua_isnil(state, -1)) {
            lua_pop(state, 1);
            lua_newtable(state);
            lua_pushvalue(state, -1);
            lua_setfield(state, LUA_REGISTRYINDEX, "__script_index_to_presentation");
        }
        // table is on the top of the stack
        lua_pushinteger(state, idx); // table key
        lua_pushlightuserdata(state, &pres); // table value
        lua_settable(state, -3);
    }

    static IPresentation *GetPresentationFromScriptIndex(lua_State *state, int idx)
    {
        SLuaStackScope __scope(state);
        lua_getfield(state, LUA_REGISTRYINDEX, "__script_index_to_presentation");
        if (lua_istable(state, -1)) {
            lua_pushinteger(state, idx);
            lua_gettable(state, -2);
            if (lua_islightuserdata(state, -1))
                return reinterpret_cast<IPresentation *>(lua_touserdata(state, -1));
        }
        return NULL;
    }

    static int WriteLuaData(lua_State *, const void *p, size_t sz, void *ud)
    {
        qt3ds::foundation::MemoryBuffer<> &theWriter =
            *reinterpret_cast<qt3ds::foundation::MemoryBuffer<> *>(ud);
        theWriter.write((const QT3DSU8 *)p, (QT3DSU32)sz);
        return 0;
    }

    static void *MemoryAllocator(void *inUserData, void *inPointer, size_t inOldSize,
                                 size_t inNewSize)
    {
        Q3DStudio_UNREFERENCED_PARAMETER(inUserData);

        if (inNewSize == 0) {
            Q3DStudio_free(inPointer, INT8, inOldSize);
            return NULL;
        } else
            return Q3DStudio_reallocate_desc(inPointer, INT8, inOldSize, inNewSize, "Lua");
    }

    static void GetLoadedBuffersTable(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "loadedbuffers");
        if (lua_isnil(inLuaState, -1)) {
            lua_pop(inLuaState, 1);
            lua_newtable(inLuaState);
            lua_pushvalue(inLuaState, -1);
            lua_setfield(inLuaState, LUA_REGISTRYINDEX, "loadedbuffers");
        }
    }

    static int ErrorChannel(lua_State *inLuaState)
    {
        qCCritical(qt3ds::INVALID_OPERATION) << "Lua: ErrorChannel: " << lua_tostring(inLuaState, -1);
        lua_pop(inLuaState, 1);
        return 0;
    }

    static int EnsureScriptIsLoaded(lua_State *inLuaState, const char *inRelativePath,
                                    const char *inBasePath, eastl::string &scriptBuffer,
                                    qt3ds::foundation::IPerfTimer &inPerfTimer,
                                    qt3ds::render::IInputStreamFactory &inStreamFactory,
                                    eastl::vector<char8_t> &inLoadVec)
    {
        qt3ds::foundation::CFileTools::CombineBaseAndRelative(inBasePath, inRelativePath,
                                                           scriptBuffer);
        qt3ds::foundation::CFileTools::ToPlatformPath(scriptBuffer);
        int theError = 0;
        // The lua system expects file to begin with @.
        scriptBuffer.insert(scriptBuffer.begin(), '@');
        const char *theFullPath = scriptBuffer.c_str();
        GetLoadedBuffersTable(inLuaState);
        lua_getfield(inLuaState, -1, inRelativePath);
        if (lua_isnil(inLuaState, -1)) {
            SStackPerfTimer __perfTimer(inPerfTimer, "Load Lua File");
            lua_pop(inLuaState, 1);
            qt3ds::render::IInputStreamFactory &theFactory(inStreamFactory);
            NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream =
                theFactory.GetStreamForFile(inRelativePath, true);

            inLoadVec.clear();
            if (theStream) {
                qt3ds::QT3DSU8 buf[1024];
                for (size_t amount = theStream->Read(toDataRef(buf, 1024)); amount;
                     amount = theStream->Read(toDataRef(buf, 1024)))
                    inLoadVec.insert(inLoadVec.end(), (char8_t *)buf, (char8_t *)buf + amount);
            }
            if (inLoadVec.empty() == false) {
                // Load the Lua script
                theError = luaL_loadbuffer(inLuaState, static_cast<const char *>(&inLoadVec[0]),
                                           static_cast<size_t>(inLoadVec.size()), theFullPath);
                lua_pushvalue(inLuaState, -1);
                lua_setfield(inLuaState, -3, inRelativePath);
                lua_insert(inLuaState, -2);
                lua_pop(inLuaState, 1);
            } else {
                lua_pop(inLuaState, 1);
                theError = -1;
            }
        }
        return theError;
    }
}

eastl::vector<eastl::string> g_AdditionalSharedLibraryPaths;
TLuaLibraryLoader LoadLuaLibraries = luaL_openlibs; ///< Defaults to open all lua libraries
lua_user_file_io g_engine_file_io;
lua_Alloc g_engine_alloc = MemoryAllocator;

struct SLuaEngineImpl : public CLuaEngine
{
    typedef eastl::vector<eastl::string> TStringList;
    qt3ds::NVFoundationBase &m_Foundation;
    lua_State *m_LuaState;

    IApplication *m_Application;
    IApplicationCore *m_ApplicationCore;

    eastl::vector<SImageLoadCallback *> m_ImageLoadingCallbacks;
    qt3ds::foundation::Pool<SLuaTimeCallback> m_TimeCallbackPool;
    TCallbackDataList m_EventCallbackDataList; ///< The list of event callback data
    TCallbackDataList m_ChangeCallbackDataList; ///< The list of change callback data
    INT32 m_CallbackIndex;

    INT32 m_ServerPort; ///<port number of remote debugger
    eastl::string m_ScriptString;
    eastl::string m_ServerIP;
    NVScopedRefCounted<qt3ds::state::debugger::ILuaDebugger> m_LuaDebugger;
    NVScopedRefCounted<SLuaEngineListener> m_LuaListener;

    lua_State *m_PreloadState;
    TStringList m_PreloadScripts;
    qt3ds::foundation::Sync m_PreloadSync;
    eastl::string m_PreloadProjectDir;
    qt3ds::foundation::Mutex m_PreloadMutex;
    qt3ds::foundation::Mutex *m_MultithreadedMutex;

    eastl::vector<char8_t> m_LoadVec;
    bool m_LuaPathInitialized;
    QT3DSI32 mRefCount;

    qt3ds::QT3DSU32 m_PerfStackIndex;
    eastl::string m_PerfNameBuilder;
    bool m_ProfileActive;
    eastl::vector<SLuaPerfEvent> m_PerfStack;

    SLuaEngineImpl(NVFoundationBase &fnd, ITimeProvider &inTimeProvider);

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    // functions from IScriptBridge
    void EnableMultithreadedAccess() override;
    void DisableMultithreadedAccess() override;

    void SetApplicationCore(qt3ds::runtime::IApplicationCore &inApplication) override;
    void SetApplication(qt3ds::runtime::IApplication &inApplication) override;

    void BeginPreloadScripts(const eastl::vector<const char *> &inScripts,
                                     qt3ds::render::IThreadPool &inThreadPool,
                                     const char *inProjectDir) override;
    void EndPreloadScripts() override;
    eastl::vector<eastl::string> GetLoadedScripts() override;

    void LoadScript(IPresentation *inPresentation, TElement *inOwner, const CHAR *inName) override;
    Q3DStudio::INT32 InitializeApplicationBehavior(const char *inProjectRelativePath) override;

    void ProcessFrameCallbacks(IPresentation *inPresentation) override;
    void ExecuteApplicationScriptFunction(Q3DStudio::INT32 inApp, const char *inFnName) override;
    void CallFunction(const char *behavior, const char *handler,
                              CScriptEngineCallFunctionArgRetriever &inArgRetriever) override;

    void ProcessSignal(IPresentation *inPresentation,
                       const SEventCommand &inCommand) override {}
    void ProcessCustomActions(IPresentation *inPresentation,
                                      const SEventCommand &inCommand) override;
    void ProcessCustomCallback(IPresentation *inPresentation,
                                       const SEventCommand &inCommand) override;

    void SetTableForElement(TElement &inElement, ILuaScriptTableProvider &inProvider) override;
    void SetAttribute(const char *element, const char *attName, const char *value) override;
    void FireEvent(const char *element, const char *evtName) override;

    void GotoSlide(const char *component, const char *slideName,
                           const SScriptEngineGotoSlideArgs &inArgs) override;
    void GotoSlideRelative(const char *component, bool inNextSlide, bool inWrap,
                                   const SScriptEngineGotoSlideArgs &inArgs) override;

    void SetPresentationAttribute(const char *presId, const char *attName,
                                          const char *attValue) override;

    bool PlaySoundFile(const char *soundPath) override;

    void AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction) override;
    void EnableDebugging(qt3ds::state::debugger::IMultiProtocolSocket &socket) override;
    void EnableProfiling() override;
    void StepGC() override;

    // functions from CLuaEngine
    qt3ds::state::IStateInterpreter *CreateStateMachine(const char8_t *inPath,
                                                              const char8_t *inId,
                                                              const char8_t *inDatamodelFunction) override;
    void PreInitialize() override;
    void Initialize() override;
    void Shutdown(qt3ds::NVFoundationBase &inFoundation) override;

    // local functions
    void ProcessFrameCallbacksHelper(TElementList &inElementList, const CHAR *inFunctionName);
    SLuaFunctionResult CallFunction(const INT32 inScriptIndex, const CHAR *inFunctionName);
    void CallElementCallback(TElement &inElement, const CHAR *inFunctionName);
    INT32 RegisterCallback(lua_State *inLuaState);
    void ReleaseCallback(INT32 incback);
    void ReleaseCallback(SLuaTimeCallback &inCallbkac);
    void PrepareToLoadFile();
    int PCall(int argcount, int retcount, int errorIndex);
    void OnBreak();
    void CallImageLoadComplete(const char *inPath, bool inSucceeded, qt3ds::QT3DSU32 inCallbackId);
    void CallbackFinished(SImageLoadCallback &inCallback);
};

namespace __SLuaEngineImpl_Static_Calls__ {
    static void PreloadFunctionCall(void *inEngine)
    {
        SLuaEngineImpl *theEngine = reinterpret_cast<SLuaEngineImpl *>(inEngine);
        eastl::vector<char8_t> loadBuffer;
        eastl::string pathBuffer;
        SStackPerfTimer __perfTimer(
            theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer(),
            "Preload Lua Data");
        {
            // First attempt to load all compiled lua scripts.
            eastl::string theLuaBinFile = theEngine->m_PreloadProjectDir;
            CFileTools::CombineBaseAndRelative(theEngine->m_PreloadProjectDir.c_str(),
                                               "binary/compiledlua.bin", theLuaBinFile);
            NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theBinFile =
                theEngine->m_ApplicationCore->GetRuntimeFactoryCore()
                    .GetInputStreamFactory()
                    .GetStreamForFile(theLuaBinFile.c_str());
            if (theBinFile) {
                qCInfo(TRACE_INFO, "Lua - Found binary file, loading scripts");
                eastl::vector<eastl::pair<eastl::string, QT3DSU32>> theDirectory;
                QT3DSU32 numDirFiles = 0;
                theBinFile->Read(numDirFiles);
                theDirectory.resize(numDirFiles);
                for (QT3DSU32 idx = 0, end = numDirFiles; idx < end; ++idx) {
                    QT3DSU32 strLen = 0;
                    theBinFile->Read(strLen);
                    theDirectory[idx].first.resize(strLen);
                    theBinFile->Read((QT3DSU8 *)&theDirectory[idx].first[0], strLen);
                    theBinFile->Read(theDirectory[idx].second);
                }
                QT3DSU32 binSectionSize = 0;
                theBinFile->Read(binSectionSize);
                eastl::vector<char8_t> theData;
                theData.resize(binSectionSize);
                theBinFile->Read(theData.data(), binSectionSize);
                SLuaStackScope theScope(theEngine->m_PreloadState);
                GetLoadedBuffersTable(theEngine->m_PreloadState);
                for (QT3DSU32 idx = 0, end = numDirFiles; idx < end; ++idx) {
                    eastl::pair<eastl::string, QT3DSU32> dirEntry = theDirectory[idx];
                    QT3DSU32 entrySize = 0;
                    if (idx < end - 1)
                        entrySize = theDirectory[idx + 1].second - dirEntry.second;
                    else
                        entrySize = binSectionSize - dirEntry.second;
                    qt3ds::foundation::CFileTools::CombineBaseAndRelative(
                        theEngine->m_PreloadProjectDir.c_str(), dirEntry.first.c_str(), pathBuffer);
                    qt3ds::foundation::CFileTools::ToPlatformPath(pathBuffer);
                    pathBuffer.insert(0, "@");

                    luaL_loadbuffer(theEngine->m_PreloadState,
                                    static_cast<const char *>(&theData[dirEntry.second]), entrySize,
                                    pathBuffer.c_str());
                    lua_setfield(theEngine->m_PreloadState, -2, dirEntry.first.c_str());
                }
            }
        }

        for (QT3DSU32 idx = 0, end = theEngine->m_PreloadScripts.size(); idx < end; ++idx) {
            SLuaStackScope theScope(theEngine->m_PreloadState);
            EnsureScriptIsLoaded(
                theEngine->m_PreloadState, theEngine->m_PreloadScripts[idx].c_str(),
                theEngine->m_PreloadProjectDir.c_str(), pathBuffer,
                theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer(),
                theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetInputStreamFactory(),
                loadBuffer);
        }
        theEngine->m_PreloadSync.set();
    }

    static void ProfileHook(lua_State *inLuaState, lua_Debug *ar)
    {
        // First check if valid function
        // We only profile pure lua code.
        lua_Debug stackInfo;
        if (lua_getstack(inLuaState, 0, &stackInfo) && lua_getinfo(inLuaState, "nSl", &stackInfo)) {
            bool validFunction = (ar->event == LUA_HOOKCALL || ar->event == LUA_HOOKRET)
                && !qt3ds::foundation::isTrivial(stackInfo.source) && stackInfo.source[0] == '@';

            if (!validFunction)
                return;

            lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
            SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
            lua_pop(inLuaState, 1);
            theEngine->m_PerfNameBuilder.assign(stackInfo.source + 1);
            eastl::string::size_type slashPos = theEngine->m_PerfNameBuilder.find_last_of("\\/");
            if (slashPos != eastl::string::npos)
                theEngine->m_PerfNameBuilder.erase(theEngine->m_PerfNameBuilder.begin(),
                                                   theEngine->m_PerfNameBuilder.begin() + slashPos
                                                       + 1);
            theEngine->m_PerfNameBuilder.append(":");
            char buf[24];
            sprintf(buf, "%d", stackInfo.linedefined);
            theEngine->m_PerfNameBuilder.append(buf);
            size_t currentPerfStackSize = theEngine->m_PerfStack.size();
            if (ar->event == LUA_HOOKCALL) {
                while (theEngine->m_PerfStackIndex >= currentPerfStackSize) {
                    theEngine->m_PerfStack.push_back(SLuaPerfEvent());
                    ++currentPerfStackSize;
                }

                SLuaPerfEvent &backEvent = theEngine->m_PerfStack[theEngine->m_PerfStackIndex];
                ++theEngine->m_PerfStackIndex;
                backEvent.m_Time = qt3ds::foundation::Time::getCurrentCounterValue();
                backEvent.m_Name.assign(theEngine->m_PerfNameBuilder);
            } else {

                if (theEngine->m_ApplicationCore != NULL && theEngine->m_PerfStackIndex > 0
                    && theEngine->m_PerfStackIndex <= currentPerfStackSize) {
                    qt3ds::foundation::IPerfTimer &theTimer =
                        theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
                    SLuaPerfEvent &backEvent =
                        theEngine->m_PerfStack[theEngine->m_PerfStackIndex - 1];
                    --theEngine->m_PerfStackIndex;
                    theTimer.Update(backEvent.m_Name.c_str(),
                                    qt3ds::foundation::Time::getCurrentCounterValue()
                                        - backEvent.m_Time);
                }
            }
        }
    }

    static int ParseSCXML(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));

        qt3ds::state::debugger::IDebugger *theDebugger = NULL;
        if (theEngine) {
            theDebugger = &theEngine->m_Application->GetStateDebugger();
        }
        return qt3ds::state::ILuaScriptContext::ParseSCXML(
            inLuaState, theDebugger, theEngine->m_Foundation,
            theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetStringTable());
    }
}
using namespace __SLuaEngineImpl_Static_Calls__;

namespace __SLuaEngineImpl_Basic_Structs__ {
    void SLuaTimeCallback::Release()
    {
        if (!m_Fired)
            m_Engine.ReleaseCallback(m_CallbackId);
        m_Engine.ReleaseCallback(*this);
    }

    void SImageLoadCallback::OnImageLoadComplete(CRegisteredString inPath,
                                                 qt3ds::render::ImageLoadResult::Enum inResult)
    {
        if (m_Engine) {
            m_Engine->CallImageLoadComplete(
                inPath, inResult == qt3ds::render::ImageLoadResult::Succeeded, m_CallbackId);
        }
    }

    void SImageLoadCallback::release()
    {
        qt3ds::foundation::atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            if (m_Engine) {
                m_Engine->ReleaseCallback(m_CallbackId);
                m_Engine->CallbackFinished(*this);
            }
            NVDelete(m_Foundation, this);
        }
    }

    void SLuaEngineListener::OnBreak() { m_Engine.OnBreak(); }
}
//==============================================================================
/**
 *	Constructor
 */
SLuaEngineImpl::SLuaEngineImpl(NVFoundationBase &fnd, ITimeProvider &inTimeProvider)
    : m_Foundation(fnd)
    , m_LuaState(NULL)
    , m_ServerPort(0)
    , m_EventCallbackDataList(0, 0, "EventCallbackList")
    , m_ChangeCallbackDataList(0, 0, "ChangeCallbackList")
    , m_CallbackIndex(0)
    , m_Application(NULL)
    , m_ApplicationCore(NULL)
    , m_LuaPathInitialized(false)
    , m_PreloadState(NULL)
    , m_PreloadSync(fnd.getAllocator())
    , m_PreloadMutex(fnd.getAllocator())
    , m_MultithreadedMutex(NULL)
    , m_PerfStackIndex(0)
    , m_ProfileActive(false)
    , mRefCount(0)
{
    m_LuaState = lua_newstate(g_engine_alloc, NULL);
    Q3DStudio_ASSERT(m_LuaState);

    lua_gc(m_LuaState, LUA_GCSTOP, 0);

    // Override this function pointer for customization of loading of lua libraries.
    // Default uses luaL_openlibs which opens all libraries found in linit.c
    LoadLuaLibraries(m_LuaState);

    lua_newtable(m_LuaState);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // Reg(1) = BehaviorsT

    lua_pushlightuserdata(m_LuaState, &m_EventCallbackDataList);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX,
                KEY_EVENTDATA_LIST); // Reg(3) = &m_EventCallbackDataList

    lua_pushlightuserdata(m_LuaState, &m_ChangeCallbackDataList);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX,
                KEY_CHANGEDATA_LIST); // Reg(4) = &m_ChangeCallbackDataList

    // Reg(5) = Global functions temporary parking lot for registerForEvent and registerForChange
    lua_newtable(m_LuaState);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS);

    // Callbacks table for callbacks sent from script.
    lua_newtable(m_LuaState);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX, KEY_REGISTEREDCALLBACKS);

    lua_pushlightuserdata(m_LuaState, this);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "uic_lua_engine");

    lua_getglobal(m_LuaState, "math"); // 2 = "math" table
    lua_pushstring(m_LuaState, "randomseed"); // 3 = "randomseed"
    lua_rawget(m_LuaState, -2); // 3 = math.randomseed function
    lua_pushinteger(m_LuaState, static_cast<INT32>(inTimeProvider.GetCurrentTimeMicroSeconds()));
    lua_pcall(m_LuaState, 1, 0, 0); // [size = 2]

    lua_settop(m_LuaState, 0);

    memset(&g_engine_file_io, 0, sizeof(_lua_user_file_io));

    g_engine_file_io.user_fopen = lua_user_fopen;
    g_engine_file_io.user_fclose = lua_user_fclose;
    g_engine_file_io.user_getc = lua_user_getc;
    g_engine_file_io.user_ungetc = lua_user_ungetc;
    g_engine_file_io.user_fgets = lua_user_fgets;
    g_engine_file_io.user_fread = lua_user_fread;
    g_engine_file_io.user_ferror = lua_user_ferror;
    g_engine_file_io.user_feof = lua_user_feof;
    g_engine_file_io.user_fread_double = lua_user_fread_number;
    g_engine_file_io.user_clearerr = lua_user_clearerr;
}

//==============================================================================
/**
 * ****************Inherited virtual functions from IScriptBridge **************
 */
//=============================================================================
void SLuaEngineImpl::EnableMultithreadedAccess()
{
    m_MultithreadedMutex = &m_PreloadMutex;
}

void SLuaEngineImpl::DisableMultithreadedAccess()
{
    m_MultithreadedMutex = NULL;
}

//==============================================================================
/**
 *	Setting package.loaders[1] = LoadFileForRequire
 *  Changed default Lua behavior and may confusing scriptors, disable it!
 */
/*static int	LoadFileForRequire( lua_State* inLuaState )
{
        lua_getfield( inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine" );
        CLuaEngine* theEngine = (CLuaEngine*)( lua_touserdata( inLuaState, -1 ) );
        lua_pop( inLuaState, 1 );

        if ( theEngine )
        {
                const char* modname = lua_tostring( inLuaState, -1 );
                theEngine->m_PreloadProjectDir.assign( "./scripts/" );
                theEngine->m_PreloadProjectDir.append( nonNull( modname ) );
                theEngine->m_PreloadProjectDir.append( ".lua" );
                int error = theEngine->EnsureScriptIsLoaded( inLuaState
                        , theEngine->m_PreloadProjectDir.c_str()
                        , theEngine->m_ApplicationCore->GetProjectDirectory()
                        , theEngine->m_ScriptString
                        , theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer()
                        ,
theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetInputStreamFactory()
                        , theEngine->m_LoadVec );
                if ( error == 0 )
                        return 1;
        }

        lua_pushnil( inLuaState );
        return 1;
}*/

void SLuaEngineImpl::SetApplicationCore(qt3ds::runtime::IApplicationCore &inApplication)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_ApplicationCore = &inApplication;
    lua_pushlightuserdata(m_LuaState, m_Application);
    lua_setglobal(m_LuaState, "UICApplicationCore");
    g_InputStreamFactory = &m_ApplicationCore->GetRuntimeFactoryCore()
                                .GetUICRenderContextCore()
                                .GetInputStreamFactory();
    // re-route lua through our file handling system.  This allows us to load data from an APK file
    // directly without needing
    // some unpack step.
    lua_set_file_io(&g_engine_file_io);
    /* See comments for LoadFileForRequire
    SLuaStackScope __scope( m_LuaState );
    lua_getglobal( m_LuaState, "table" );
    lua_getfield( m_LuaState, -1, "insert" );
    lua_insert( m_LuaState, -2 );
    lua_pop( m_LuaState, 1 );
    lua_getglobal( m_LuaState, "package" );
    if ( lua_istable( m_LuaState, -1 ) )
    {
            lua_getfield( m_LuaState, -1, "loaders" );
            if ( lua_istable( m_LuaState, -1 ) )
            {
                    int error;
                    lua_insert( m_LuaState, -2 );
                    lua_pop( m_LuaState, 1 );
                    lua_pushnumber( m_LuaState, 1 );
                    lua_pushcfunction( m_LuaState, LoadFileForRequire );
                    error = lua_pcall( m_LuaState, 3, 0, 0 );
                    QT3DS_ASSERT( error == 0 );
            }
    }*/
}

void SLuaEngineImpl::SetApplication(qt3ds::runtime::IApplication &inApplication)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_Application = &inApplication;
    lua_pushlightuserdata(m_LuaState, m_Application);
    lua_setglobal(m_LuaState, "UICApplication");
}

// Starts preloading scripts offline.  This sets m_LuaState to NULL until after EndPreloadScripts to
// avoid multithreaded lua state access.  Some calls may be queued till EndPreloadScripts
void SLuaEngineImpl::BeginPreloadScripts(const eastl::vector<const char *> &inScripts,
                                         qt3ds::render::IThreadPool &inThreadPool,
                                         const char *inProjDir)
{
    m_PreloadScripts.assign(inScripts.begin(), inScripts.end());
    m_PreloadState = m_LuaState;
    m_LuaState = NULL;
    m_PreloadProjectDir.assign(inProjDir);
    m_PreloadSync.reset();
    inThreadPool.AddTask(this, PreloadFunctionCall, NULL);
}

// Ends preload, restores m_LuaState.
void SLuaEngineImpl::EndPreloadScripts()
{
    qt3ds::foundation::Mutex::ScopedLock __locker(m_PreloadMutex);
    if (m_LuaState == NULL) {
        m_PreloadSync.wait();
        m_LuaState = m_PreloadState;
        m_PreloadState = NULL;
    }
}

// Fast loading support; on save get the set of loaded scripts
eastl::vector<eastl::string> SLuaEngineImpl::GetLoadedScripts()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    eastl::vector<eastl::string> retval;
    SLuaStackScope theScope(m_LuaState);
    GetLoadedBuffersTable(m_LuaState);
    QT3DS_ASSERT(lua_type(m_LuaState, -1) == LUA_TTABLE);
    lua_pushnil(m_LuaState);
    eastl::vector<eastl::pair<eastl::string, QT3DSU32>> theDirectory;
    qt3ds::foundation::MemoryBuffer<> theBuffer(ForwardingAllocator(
        m_ApplicationCore->GetRuntimeFactoryCore().GetFoundation().getAllocator(),
        "LuaBinaryData"));
    while (lua_next(m_LuaState, -2)) {
        // dup the key
        lua_pushvalue(m_LuaState, -2);
        const char *relativeFileName = lua_tostring(m_LuaState, -1);
        retval.push_back(relativeFileName);
        // pop the key
        lua_pop(m_LuaState, 1);
        // Dump the function
        theDirectory.push_back(eastl::make_pair(retval.back(), (QT3DSU32)theBuffer.size()));
        lua_dump(m_LuaState, WriteLuaData, &theBuffer);
        // pop the value
        lua_pop(m_LuaState, 1);
    }
    eastl::string tempStr;
    CFileTools::CombineBaseAndRelative(m_ApplicationCore->GetProjectDirectory().c_str(), "binary",
                                       tempStr);
    CFileTools::CreateDir(tempStr.c_str());
    tempStr.append("/compiledlua.bin");
    qt3ds::foundation::CFileSeekableIOStream theStream(tempStr.c_str(), FileWriteFlags());
    if (theStream.IsOpen() == false) {
        qCritical(qt3ds::INTERNAL_ERROR, "Unable to open lua binary file");
        QT3DS_ASSERT(false);
    }
    qt3ds::foundation::IOutStream &theOutStream(theStream);
    theOutStream.Write((QT3DSU32)theDirectory.size());
    for (QT3DSU32 idx = 0, end = theDirectory.size(); idx < end; ++idx) {
        eastl::pair<eastl::string, QT3DSU32> &theEntry(theDirectory[idx]);
        QT3DSU32 theStrLen = (QT3DSU32)theEntry.first.size();
        if (theStrLen)
            ++theStrLen;
        theOutStream.Write(theStrLen);
        if (theStrLen)
            theOutStream.Write(theEntry.first.c_str(), theStrLen);
        theOutStream.Write(theEntry.second);
    }
    theOutStream.Write((QT3DSU32)theBuffer.size());
    theOutStream.Write(theBuffer.begin(), (QT3DSU32)theBuffer.size());
    return retval;
}

//==============================================================================
/**
 *	Load the lua scripts from the Lua file
 */
void SLuaEngineImpl::LoadScript(IPresentation *inPresentation, TElement *inBehavior,
                                const CHAR *inFileName)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    qt3ds::foundation::IPerfTimer &thePerfTimer(
        m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer());
    SStackPerfTimer __perfTimer(thePerfTimer, "CLuaEngine: LoadScript");
    QFileInfo fileInfo(inPresentation->GetFilePath());
    qt3ds::foundation::CFileTools::CombineBaseAndRelative(
        fileInfo.path().toLatin1().constData(), inFileName, m_ScriptString);
    qt3ds::foundation::CFileTools::ToPlatformPath(m_ScriptString);

    {
        SStackPerfTimer __perfTimer(thePerfTimer, "CLuaEngine: Prepare to load file");
        PrepareToLoadFile();
    }
    int theTop = lua_gettop(m_LuaState);
    int theError = 0;
    INT32 theScriptIndex = 0;
    {
        SStackPerfTimer __perfTimer(thePerfTimer, "CLuaEngine: Setup Load Script");

        // The behavior table will stay at index 1 on the stack

        lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // 1 = BehaviorsT

        // Create a new table for each behavior and set it as the "self" global entry
        lua_pushlstring(m_LuaState, KEY_SELF, LENGTH_KEY_SELF); // 2 = "self"
        lua_newtable(m_LuaState); // 3 = new ScriptT
        lua_rawset(m_LuaState, LUA_GLOBALSINDEX); // [size = 1]

        // Add "self.element" as a valid variable on each table
        lua_pushlstring(m_LuaState, KEY_SELF, LENGTH_KEY_SELF); // 2 = "self"
        lua_rawget(m_LuaState, LUA_GLOBALSINDEX); // 2 = ScriptT
        lua_pushlstring(m_LuaState, KEY_ELEMENT, LENGTH_KEY_ELEMENT); // 3 = "element"
        lua_pushlightuserdata(m_LuaState, inBehavior); // 4 = ElementRef
        lua_rawset(m_LuaState, -3); // [size = 2]
        // Get a reference to our new table and put that table into the behavior table
        theScriptIndex = luaL_ref(m_LuaState, -2); // [size = 1]

        // puts the script index into the table
        lua_pushlstring(m_LuaState, KEY_SELF, LENGTH_KEY_SELF); // 2 = "self"
        lua_rawget(m_LuaState, LUA_GLOBALSINDEX); // 2 = ScriptT
        lua_pushlstring(m_LuaState, KEY_SCRIPT_INDEX,
                        LENGTH_KEY_SCRIPT_INDEX); // 3 = "__scriptIndex"
        lua_pushinteger(m_LuaState, theScriptIndex); // 4 = ScriptIndex
        lua_rawset(m_LuaState, -3); // [size = 2]

        lua_pushstring(m_LuaState, inFileName);
        lua_setfield(m_LuaState, -2, "__scriptpath__");

        lua_pop(m_LuaState, 1); // [size = 1]
    }

    MapPresentationToScriptIndex(m_LuaState, *inPresentation, theScriptIndex);

    {
        SStackPerfTimer __perfTimer(thePerfTimer, "CLuaEngine: Ensure Lua File Is Loaded");

        QFileInfo fileInfo(inPresentation->GetFilePath());
        theError = EnsureScriptIsLoaded(
            m_LuaState, inFileName, fileInfo.path().toLatin1().constData(),
            m_ScriptString, m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer(),
            m_ApplicationCore->GetRuntimeFactoryCore().GetInputStreamFactory(), m_LoadVec);
    }

    const char *theFullPath = m_ScriptString.c_str();

    if (theError != 0) {
        qCCritical(qt3ds::INVALID_OPERATION) << "Lua: loadfile failed: " << theFullPath;
        lua_settop(m_LuaState, 0);
        return;
    }

    {
        SStackPerfTimer __perfTimer(thePerfTimer, "CLuaEngine: Execute Lua File");
        // Execute the chunk on the stack with "self" pointing to our new table
        theError = lua_pcall(m_LuaState, 0, 0, 0); // [size = 1]
    }
    if (theError != 0) {
        qCCritical(qt3ds::INVALID_OPERATION) << "pcall failed loading file: "
                << nonNull(theFullPath) << " " << nonNull(lua_tostring(m_LuaState, -1));
        lua_settop(m_LuaState, 0);
        return;
    }

    lua_settop(m_LuaState, theTop);

    inBehavior->m_ScriptID = theScriptIndex;
    inBehavior->Flags().clearOrSet(true, ELEMENTFLAG_SCRIPTCALLBACKS);
    inBehavior->Flags().clearOrSet(true, ELEMENTFLAG_SCRIPTINITIALIZE);
}

Q3DStudio::INT32 SLuaEngineImpl::InitializeApplicationBehavior(const char *inRelativePath)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope __topScope(m_LuaState);
    PrepareToLoadFile();
    eastl::string scriptBuffer;
    int loadError = EnsureScriptIsLoaded(
        m_LuaState, inRelativePath, m_ApplicationCore->GetProjectDirectory(), scriptBuffer,
        m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer(),
        m_ApplicationCore->GetRuntimeFactoryCore().GetInputStreamFactory(), m_LoadVec);
    if (loadError) {
        qCCritical(qt3ds::INVALID_PARAMETER, "Failed to load lua file: %s: %s", inRelativePath,
            lua_tostring(m_LuaState, -1));
        return 0;
    }
    INT32 theScriptIndex = 0;
    {
        SLuaStackScope __selfScope(m_LuaState);

        lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // 1 = BehaviorsT
        lua_newtable(m_LuaState); // 2 = self

        lua_pushvalue(m_LuaState, -1); // dup the table
        lua_setfield(m_LuaState, LUA_GLOBALSINDEX, KEY_SELF); // 2 = self

        lua_pushvalue(m_LuaState, -1); // dup the table
        theScriptIndex = luaL_ref(m_LuaState, -3); // Add a new script into the behaviors table.

        lua_pushinteger(m_LuaState, theScriptIndex);
        lua_setfield(m_LuaState, -2, KEY_SCRIPT_INDEX);

        lua_pushstring(m_LuaState, inRelativePath);
        lua_setfield(m_LuaState, -2, "__scriptpath__");
    }

    int pcallError = PCall(0, 0, 0);
    if (pcallError) {
        qCCritical(INVALID_PARAMETER, "Failed to load lua file: %s: %s", inRelativePath,
            lua_tostring(m_LuaState, -1));
        return 0;
    }
    return theScriptIndex;
}

//==============================================================================
/**
 *	Signal from the associated presentation that it's time to fire off
 *	any frame callbacks such as onActivation and onUpdate.
 */
void SLuaEngineImpl::ProcessFrameCallbacks(IPresentation *inPresentation)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    PerfLogGeneralEvent2(DATALOGGER_PROCESSFRAMECALLBACKS);

    CPresentationFrameData &theFrameData = inPresentation->GetFrameData();

    // Call onSlideExit functions
    // ProcessSlideCallbacksHelper( theElementManager, theFrameData.GetSlideExitList( ),
    // KEY_ONSLIDEEXIT );

    // Call onDeactivate functions
    ProcessFrameCallbacksHelper(theFrameData.GetDeactivationList(), KEY_ONDEACTIVATE);

    // Call onInitialize functions and remove the ELEMENTFLAG_SCRIPTINITIALIZE flag
    TElementList &theUpdateList = theFrameData.GetScriptsList();
    FOR_ARRAY(TElement *, theElement, theUpdateList)
    {
        if ((*theElement)->GetFlag(ELEMENTFLAG_SCRIPTINITIALIZE)) {
            CallElementCallback(**theElement, KEY_ONINITIALIZE);
            (*theElement)->SetFlag(ELEMENTFLAG_SCRIPTINITIALIZE, false);
        }
    }

    // Call onActivate functions
    ProcessFrameCallbacksHelper(theFrameData.GetActivationList(), KEY_ONACTIVATE);

    // Call onUpdate functions
    ProcessFrameCallbacksHelper(theUpdateList, KEY_ONUPDATE);
}

void SLuaEngineImpl::ExecuteApplicationScriptFunction(Q3DStudio::INT32 inApp, const char *inFnName)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope __topScope(m_LuaState);
    lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS);
    lua_rawgeti(m_LuaState, -1, inApp);
    int selfIndex = lua_gettop(m_LuaState);
    if (!lua_istable(m_LuaState, -1)) {
        qCCritical(INVALID_OPERATION, "Lua object index appears to be invalid");
        return;
    }
    lua_getfield(m_LuaState, -1, inFnName);
    if (lua_isfunction(m_LuaState, -1)) {
        IPerfTimer *theTimer = NULL;
        const char *thePerfName = "";
        if (m_ApplicationCore) {
            lua_getfield(m_LuaState, -2, "__scriptpath__");
            if (lua_isstring(m_LuaState, -1)) {
                m_ScriptString.clear();
                m_ScriptString.assign(lua_tostring(m_LuaState, -1));
                m_ScriptString.append(" - ");
                m_ScriptString.append(inFnName);
                thePerfName = m_ScriptString.c_str();
                theTimer = &m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
            }
            lua_pop(m_LuaState, 1);
        }
        SStackPerfTimer __scriptTimer(theTimer, thePerfName);
        // push the self element
        lua_pushvalue(m_LuaState, -2);
        lua_pushvalue(m_LuaState, -1); // dup that
        lua_setglobal(m_LuaState, KEY_SELF); // set the global self element (WAT)

        int pcallError = PCall(1, 0, 0);
        if (pcallError) {
            qCCritical(INVALID_OPERATION,
                "Lua object call %s failed: %s.  disabling further calls", inFnName,
                lua_tostring(m_LuaState, -1));
            lua_pushnil(m_LuaState);
            lua_setfield(m_LuaState, selfIndex, inFnName);
        }
    }
}

void SLuaEngineImpl::CallFunction(const char *behavior, const char *handler,
                                  CScriptEngineCallFunctionArgRetriever &inArgRetriever)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    int theTop = lua_gettop(m_LuaState);
    lua_getglobal(m_LuaState, "UICApplication");
    qt3ds::runtime::IApplication *theApp =
        static_cast<qt3ds::runtime::IApplication *>(lua_touserdata(m_LuaState, -1));
    lua_pop(m_LuaState, 1);

    TElement *theBehavior = CLuaElementHelper::GetElement(*theApp, NULL, behavior, NULL);
    if (theBehavior) {
        // Ensure that onInitialize has been called.
        if (theBehavior->GetFlag(ELEMENTFLAG_SCRIPTINITIALIZE)) {
            CallElementCallback(*theBehavior, KEY_ONINITIALIZE);
            theBehavior->SetFlag(ELEMENTFLAG_SCRIPTINITIALIZE, false);
        }

        INT32 theScriptIndex = theBehavior->m_ScriptID;
        if (theScriptIndex < 1) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "LuaEngine::CallFunction: Failed to find script for item: "
                    << behavior << ":" << handler;
        } else {
            lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS);
            lua_rawgeti(m_LuaState, -1, theScriptIndex); // Top + 1 = ScriptT
            lua_pushvalue(m_LuaState, -1);
            lua_setfield(m_LuaState, LUA_GLOBALSINDEX, KEY_SELF);

            lua_getfield(m_LuaState, -1, handler); // Top + 2 = custom function, if there's one
            if (lua_isfunction(m_LuaState, -1)) // if it is a function, call it
            {
                const int theFunctionTop = lua_gettop(m_LuaState);
                int theArgCount = 0;
                // Setup the global 'self'.  Very annoying.

                QT3DS_ASSERT(lua_istable(m_LuaState, -2));
                lua_pushvalue(m_LuaState, -2);
                ++theArgCount;
                QT3DS_ASSERT(lua_istable(m_LuaState, theFunctionTop + theArgCount));
                int theArgumentStatus = inArgRetriever.RetrieveArgument(m_LuaState);
                if (theArgumentStatus < 0) {
                    qCCritical(qt3ds::INVALID_OPERATION)
                            << "LuaEngine::CallFunction: failed to evaluate behavior "
                            << "function arguments: " << behavior << ":" << handler
                            << ":" << inArgRetriever.GetArgDescription().c_str();
                } else if (theArgumentStatus > 0) {
                    if (lua_istable(m_LuaState, -1)) {
                        // iterate through the keys and push each value.  This allows people to
                        // provide more than
                        // one argument.
                        lua_pushnil(m_LuaState);
                        while (lua_next(m_LuaState, -2) != 0) {
                            ++theArgCount;
                            lua_insert(m_LuaState, theFunctionTop + theArgCount);
                            // lua_pop( m_LuaState, 1 );
                        }
                    } else {
                        ++theArgCount;
                        lua_insert(m_LuaState, theFunctionTop + theArgCount);
                        QT3DS_ASSERT(lua_istable(m_LuaState, theFunctionTop + 1));
                    }
                }
                QT3DS_ASSERT(lua_istable(m_LuaState, theFunctionTop + 1));
                lua_settop(m_LuaState, theFunctionTop + theArgCount);
                int failure = PCall(theArgCount, 0, 0);
                if (failure) {
                    const char *error = lua_tostring(m_LuaState, -1);
                    qCCritical(qt3ds::INVALID_OPERATION)
                            << "LuaEngine::CallFunction: failed to call handler: "
                            << behavior << ":" << handler << ":"
                            << inArgRetriever.GetArgDescription().c_str() << " - " << error;
                }
            } else {
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "LuaEngine::CallFunction: behavior property is not a function"
                        << behavior << ":" << handler;
            }
        }
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CLuaEngine::CallFunction: Unable to find behavior "<< behavior;
    }
    lua_settop(m_LuaState, theTop);
}

//==============================================================================
/**
 *	Handle custom actions
 *	@param inCommand				carrier of behavior and parameter
 *information
 */
void SLuaEngineImpl::ProcessCustomActions(IPresentation *inPresentation,
                                          const SEventCommand &inCommand)
{
    using qt3ds::runtime::TIdValuePair;
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    TElement *theBehavior = inCommand.m_Target; // the element that is a behavior

    CPresentation *thePresentation = (CPresentation *)inPresentation;
    IParametersSystem &theParametersManager = thePresentation->GetParametersSystem();
    qt3ds::foundation::IStringTable &theStrTable(thePresentation->GetStringTable());

    INT32 theScriptIndex = theBehavior->m_ScriptID;
    if (theScriptIndex < 1) {
        UVariant theLuaScript;
        QString theWarning = "Attempted to execute unloaded Lua: ";

        if (theBehavior->GetAttribute(ATTRIBUTE_BEHAVIORSCRIPTS, theLuaScript)) {
            qt3ds::foundation::CRegisteredString theString =
                thePresentation->GetStringTable().HandleToStr(theLuaScript.m_StringHandle);
            theWarning.append(theString.c_str());
        }
        qCWarning(qt3ds::INVALID_OPERATION)
                << "Lua: Unknown script index " << theWarning.toLatin1().constData();
        return;
    }

    INT32 theArgumentCount = 1;

    // The start index points to the function name, which is string
    INT32 theGroupId = inCommand.m_Arg1.m_INT32; // the group id of the parameters group
    UINT32 theNumParams = theParametersManager.GetNumParameters(theGroupId);
    if (theNumParams == 0) {
        QT3DS_ASSERT(false);
        return;
    }

    TIdValuePair tempData = theParametersManager.GetParameter(theGroupId, 0);

    if (tempData.first != HASH_PROPERTYNAME_STRING) {
        Q3DStudio_ASSERT(false);
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Lua: ProcessCustomActions failed to get a param by index!";
        return;
    }

    int theTop = lua_gettop(m_LuaState);
    lua_pushcfunction(m_LuaState, ErrorChannel); // Top + 1 = error handler
    lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // Top + 2 = BehaviorsT
    lua_rawgeti(m_LuaState, -1, theScriptIndex); // Top + 3 = ScriptT
    if (lua_istable(m_LuaState, -1)) {
        lua_getfield(
            m_LuaState, -1,
            theStrTable.HandleToStr(
                tempData.second.m_StringHandle)); // Top + 4 = custom function, if there's one
        if (lua_isfunction(m_LuaState, -1)) // if it is a function, call it
        {
            lua_pushlstring(m_LuaState, KEY_SELF, LENGTH_KEY_SELF); // Top + 5 = 'self'
            lua_pushvalue(m_LuaState, -3); // Top + 6 = ScriptT
            lua_rawset(m_LuaState, LUA_GLOBALSINDEX); // [size = Top + 4]

            lua_pushvalue(m_LuaState, -2); // Top + 5 = ScriptT

            for (UINT32 theIndex = 1; theIndex < theNumParams; ++theIndex) {
                tempData = theParametersManager.GetParameter(theGroupId, theIndex);
                if (tempData.first == HASH_PROPERTYNAME_FLOAT) {
                    lua_pushnumber(m_LuaState, tempData.second.m_FLOAT);
                } else if (tempData.first == HASH_PROPERTYNAME_FLOAT3) {
                    RuntimeVector3 &theVector = CLuaVector::CreateVector(m_LuaState);

                    theVector.m_X = tempData.second.m_FLOAT;
                    theVector.m_Y =
                        theParametersManager.GetParameter(theGroupId, ++theIndex).second.m_FLOAT;
                    theVector.m_Z =
                        theParametersManager.GetParameter(theGroupId, ++theIndex).second.m_FLOAT;
                } else if (tempData.first == HASH_PROPERTYNAME_LONG) {
                    lua_pushinteger(m_LuaState, static_cast<INT32>(tempData.second.m_FLOAT));
                } else if (tempData.first == HASH_PROPERTYNAME_COLOR4) {
                    CColor &theColor = CLuaColor::CreateColor(m_LuaState);

                    theColor.m_Red = static_cast<UINT8>(tempData.second.m_FLOAT);
                    theColor.m_Green = static_cast<UINT8>(
                        theParametersManager.GetParameter(theGroupId, ++theIndex).second.m_FLOAT);
                    theColor.m_Blue = static_cast<UINT8>(
                        theParametersManager.GetParameter(theGroupId, ++theIndex).second.m_FLOAT);
                    theColor.m_Alpha = static_cast<UINT8>(
                        theParametersManager.GetParameter(theGroupId, ++theIndex).second.m_FLOAT);
                } else if (tempData.first == HASH_PROPERTYNAME_BOOL) {
                    lua_pushboolean(m_LuaState, tempData.second.m_INT32 != 0);
                } else if (tempData.first == HASH_PROPERTYNAME_STRING) {
                    lua_pushstring(m_LuaState,
                                   theStrTable.HandleToStr(tempData.second.m_StringHandle));
                } else {
                    qCCritical(qt3ds::INVALID_OPERATION)
                            << "Lua: ProcessCustomAction: Unknown data type: " << tempData.first;
                }
                ++theArgumentCount;
            }

            PCall(theArgumentCount, 0, theTop + 1);
        }
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "Lua: Unknown script index: Attempt to execute action on object with no "
                << "script table registered";
    }

    lua_settop(m_LuaState, theTop);
}

void SLuaEngineImpl::ProcessCustomCallback(IPresentation * /*inPresentation*/,
                                           const SEventCommand &inCommand)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    int theTop = lua_gettop(m_LuaState);
    INT32 cbackIdx = inCommand.m_Arg1.m_INT32;
    lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_REGISTEREDCALLBACKS);
    lua_pushcfunction(m_LuaState, ErrorChannel);
    lua_rawgeti(m_LuaState, -2, cbackIdx);
    if (lua_isnil(m_LuaState, -1)) {
        lua_settop(m_LuaState, theTop);
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Lua: ProcessCustomCallback: Callback id was nil " << cbackIdx;
    } else {
        PCall(0, 0, -2);
    }
    ReleaseCallback(cbackIdx);
    lua_settop(m_LuaState, theTop);
}

void SLuaEngineImpl::SetTableForElement(TElement &inElement, ILuaScriptTableProvider &inProvider)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    // Called when something that isn't a lua script needs to get set on the element.
    SLuaStackScope __stackScope(m_LuaState);
    inProvider.CreateTable(m_LuaState);
    int new_top = lua_gettop(m_LuaState);
    if (new_top > __stackScope.m_Top && lua_istable(m_LuaState, -1)) {
        IPresentation *thePresentation = inElement.GetBelongedPresentation();
        lua_pushlightuserdata(m_LuaState, &inElement);
        lua_setfield(m_LuaState, -2, KEY_ELEMENT);
        lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS);
        // swap the behaviors table and the provided table so the provided table is TOS.
        lua_insert(m_LuaState, -2);
        // dup the provided table so there are two of them
        lua_pushvalue(m_LuaState, -1);
        // Get a reference to our new table and put that table into the behavior table
        INT32 theScriptIndex = luaL_ref(m_LuaState, -3); // [size = 1]
        // Note that the duplicated table should still be TOS until this
        lua_pushinteger(m_LuaState, theScriptIndex);
        lua_setfield(m_LuaState, -2, KEY_SCRIPT_INDEX);
        MapPresentationToScriptIndex(m_LuaState, *thePresentation, theScriptIndex);

        inElement.m_ScriptID = theScriptIndex;
        inElement.SetFlag(ELEMENTFLAG_SCRIPTCALLBACKS, true);
        inElement.SetFlag(ELEMENTFLAG_SCRIPTINITIALIZE, true);
    }
}

void SLuaEngineImpl::SetAttribute(const char *element, const char *attName, const char *value)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope theScope(m_LuaState);

    TElement *theTarget = CLuaElementHelper::GetElement(*m_Application, NULL, element, NULL);
    if (theTarget) {
        eastl::string temp(value);
        temp.insert(0, "return ");
        luaL_loadstring(m_LuaState, temp.c_str());
        int failure = PCall(0, 1, 0);
        if (failure) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "LuaEngine::SetAttribute: failed to evaluate element setAttribute arguments: "
                    << element << ":" << attName << ":" << temp.c_str();
        } else {
            bool success = CLuaElementHelper::SetAttribute(m_LuaState, theTarget, attName, true);
            if (!success) {
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "LuaEngine::SetAttribute: failed to set attribute on element: "
                        << element << ":" << attName << ":" << temp.c_str();
            }
        }
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "LuaEngine::SetAttribute: failed to find element "
                << element << " (" << attName << ":" << value << ")";
    }
}

void SLuaEngineImpl::FireEvent(const char *element, const char *evtName)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope theScope(m_LuaState);
    TElement *theElement = CLuaElementHelper::GetElement(*m_Application, NULL, element, NULL);
    if (theElement && theElement->GetActive() == true) {
        IPresentation *thePresentation = theElement->GetBelongedPresentation();
        thePresentation->FireEvent(CHash::HashEventCommand(evtName), theElement);
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "LuaEngine::FireEvent: failed to find element: "
                << element << "(" << evtName << ")";
    }
}

void SLuaEngineImpl::GotoSlide(const char *component, const char *slideName,
                               const SScriptEngineGotoSlideArgs &inArgs)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    int theTop = lua_gettop(m_LuaState);
    lua_getglobal(m_LuaState, "UICApplication");
    qt3ds::runtime::IApplication *theApp =
        static_cast<qt3ds::runtime::IApplication *>(lua_touserdata(m_LuaState, -1));
    lua_pop(m_LuaState, 1);

    TElement *theTarget = CLuaElementHelper::GetElement(*theApp, NULL, component, NULL);
    if (theTarget) {
        CLuaCommandHelper::SetupGotoSlideCommand(*theTarget, slideName, inArgs);
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CLuaEngine::GotoSlide: Unable to find component: " << component;
    }
    lua_settop(m_LuaState, theTop);
}

void SLuaEngineImpl::GotoSlideRelative(const char *component, bool inNextSlide, bool inWrap,
                                       const SScriptEngineGotoSlideArgs &inArgs)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope theScope(m_LuaState);
    TElement *theTarget = CLuaElementHelper::GetElement(*m_Application, NULL, component, NULL);
    if (theTarget) {
        theTarget = &theTarget->GetComponentParent();
        if (theTarget && theTarget->GetActive()) {
            TComponent *theComponent = static_cast<TComponent *>(theTarget);
            Q3DStudio::INT32 theSlide = theComponent->GetCurrentSlide();
            Q3DStudio::INT32 theSlideCount = theComponent->GetSlideCount();
            theSlide = inNextSlide ? theSlide + 1 : theSlide - 1;
            if (theSlide < 1) {
                if (inWrap)
                    theSlide = theSlideCount - 1;
                else
                    theSlide = 1;
            } else if (theSlide == theSlideCount) {
                if (inWrap)
                    theSlide = 1;
                else
                    theSlide = theSlideCount - 1;
            }
            if (theSlide != theComponent->GetCurrentSlide()) {
                CLuaCommandHelper::SetupGotoSlideCommand(*theTarget, theSlide, inArgs);
            }
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "LuaEngine::GotoSlideRelative: Component is not active: "<< component;
        }
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "LuaEngine::GotoSlideRelative: failed to find component: "<< component;
    }
}

void SLuaEngineImpl::SetPresentationAttribute(const char *presId, const char *, const char *value)
{
    if (isTrivial(presId))
        return;
    if (presId[0] == '#')
        ++presId;
    CPresentation *thePresentation = m_Application->GetPresentationById(presId);
    if (thePresentation) {
        bool active = AreEqualCaseless(nonNull(value), "True");
        thePresentation->SetActive(active);
    }
}

bool SLuaEngineImpl::PlaySoundFile(const char *soundPath)
{
    return m_Application->GetAudioPlayer().PlaySoundFile(soundPath);
}

//==============================================================================
/**
 *	Add a new global function to my lua_State. This is useful to call from the outside.
 */
void SLuaEngineImpl::AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction)
{
    if (m_LuaState) {
        lua_register(m_LuaState, inFunctionName, inFunction);
    }
}

void SLuaEngineImpl::EnableDebugging(qt3ds::state::debugger::IMultiProtocolSocket &socket)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    using namespace qt3ds::state::debugger;
    QT3DS_ASSERT(m_ApplicationCore);
    NVScopedRefCounted<qt3ds::state::debugger::IMultiProtocolSocketStream> theStream =
        socket.CreateProtocol(qt3ds::state::debugger::ILuaArchitectDebugServer::LuaProtocolName(),
                              NULL);
    m_LuaListener = QT3DS_NEW(m_Foundation.getAllocator(), SLuaEngineListener)(m_Foundation, *this);

    m_LuaDebugger = qt3ds::state::debugger::ILuaDebugger::CreateLuaSideDebugger(
        m_ApplicationCore->GetRuntimeFactoryCore().GetFoundation(), *theStream, m_LuaState,
        m_ApplicationCore->GetRuntimeFactoryCore().GetStringTable(),
        m_ApplicationCore->GetProjectDirectory().c_str(), m_LuaListener.mPtr);
}

void SLuaEngineImpl::EnableProfiling()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    lua_sethook(m_LuaState, ProfileHook, LUA_MASKCALL | LUA_MASKRET, 0);
}

void SLuaEngineImpl::StepGC()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;

    IPerfTimer *theTimer = NULL;
    if (m_ApplicationCore)
        theTimer = &m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
    SStackPerfTimer __timer(theTimer, "CLuaEngine::StepGC");
    lua_gc(m_LuaState, LUA_GCSTEP, 5);
}
//==============================================================================
/**
 * ****************End of IScriptBridge functions **************
 */
//=============================================================================

//==============================================================================
/**
 * ****************Inherited virtual functions from CLuaEngine **************
 */
//=============================================================================
qt3ds::state::IStateInterpreter *
SLuaEngineImpl::CreateStateMachine(const char8_t *inPath, const char8_t *inId,
                                   const char8_t *inDatamodelFunction)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope __topScope(m_LuaState);
    // This assertion is key.  Without it, somewhere in below a setField function gets called on
    // something that isn't
    // a table and the program blinks out of existance.
    if (__topScope.m_Top != 0) {
        qFatal("CLuaEngine::CreateStateMachine called when lua stack is not at zero; aborting");
        QT3DS_ASSERT(false);
        return NULL;
    }

    lua_pushcfunction(m_LuaState, ParseSCXML);
    eastl::string fullPath;
    qt3ds::foundation::CFileTools::CombineBaseAndRelative(m_Application->GetProjectDirectory().c_str(),
                                                       inPath, fullPath);
    lua_pushstring(m_LuaState, fullPath.c_str());
    qt3ds::state::IStateInterpreter *theInterpreter = NULL;
    int error = PCall(1, 1, 0);
    if (error) {
        qCCritical(INVALID_OPERATION, "Failed to load scxml file: %s", lua_tostring(m_LuaState, -1));
    } else {
        lua_insert(m_LuaState, 1);
        lua_settop(m_LuaState, 1);
        int isTable = lua_istable(m_LuaState, 1);
        QT3DS_ASSERT(isTable);
        (void)isTable;
        // shove it in the global table under its path.  We will get to this again in a second
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "uic_state_machines");
        if (lua_isnil(m_LuaState, -1)) {
            lua_pop(m_LuaState, 1);
            lua_newtable(m_LuaState);
            lua_pushvalue(m_LuaState, -1);
            lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "uic_state_machines");
        }
        lua_pushvalue(m_LuaState, -2);
        lua_setfield(m_LuaState, -2, inId);
        lua_pop(m_LuaState, 1); // pop the machine table

        // The state machine should still be top of the stack.
        // if ( !qt3ds::foundation::isTrivial( inDatamodelFunction ) )
        {
            int theNewTop = lua_gettop(m_LuaState);
            eastl::string evalStr("return ");
            evalStr.append(inDatamodelFunction);
            luaL_loadstring(m_LuaState, evalStr.c_str());
            int failure = PCall(0, 1, 0);
            if (failure) {
                const char *error = lua_tostring(m_LuaState, -1);
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "LuaEngine: Unable to initialize datamodel: "
                        << inDatamodelFunction << ":" << error;
                lua_settop(m_LuaState, theNewTop);
            } else {
                // Hmm, now to get the item the function returned to be below the string in the
                // stack.
                // And clear off the stack from there.
                lua_insert(m_LuaState, theNewTop + 1);
                lua_settop(m_LuaState, theNewTop + 1);
            }
        }
        // If all that worked then we should have the state machine and the datatable on the stack
        // and only those two on the stack.
        // At which point we can call start, which will pop everything off the stack
        qt3ds::state::ILuaScriptContext::Initialize(m_LuaState, inId);
        // Now get the interpreter from the map so we can get its value
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "uic_state_machines");
        lua_getfield(m_LuaState, -1, inId);
        theInterpreter = qt3ds::state::ILuaScriptContext::GetInterpreterFromBindings(m_LuaState);
    }
    return theInterpreter;
}

// Functions that will not crash if the lua engine has no application or if the presentations
// are in an indeterminate state.
void SLuaEngineImpl::PreInitialize()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    lua_register(m_LuaState, "output", Output);
    lua_register(m_LuaState, "getEventSystem", GetEventSystem);
    CLuaVector::RegisterLibrary(m_LuaState);
    CLuaColor::RegisterLibrary(m_LuaState);
    CLuaRotation::RegisterLibrary(m_LuaState);
    CLuaMatrix::RegisterLibrary(m_LuaState);
    CLuaKeyboard::RegisterLibrary(m_LuaState);
    CLuaButton::RegisterLibrary(m_LuaState);
    CLuaAxis::RegisterLibrary(m_LuaState);
}
//==============================================================================
/**
 *	Register the set of global Lua functions
 */
void SLuaEngineImpl::Initialize()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    lua_register(m_LuaState, "getElement", CLuaElementHelper::GetElement);
    lua_register(m_LuaState, "getElementType", CLuaElementHelper::GetElementType);
    lua_register(m_LuaState, "getAttribute", CLuaElementHelper::GetAttribute);
    lua_register(m_LuaState, "setAttribute", CLuaElementHelper::SetAttribute);
    lua_register(m_LuaState, "hasAttribute", CLuaElementHelper::HasAttribute);
    lua_register(m_LuaState, "getChildren", CLuaElementHelper::GetChildren);
    lua_register(m_LuaState, "getCurrentSlide", CLuaElementHelper::GetCurrentSlide);
    lua_register(m_LuaState, "getTime", CLuaElementHelper::GetTime);

    lua_register(m_LuaState, "calculateGlobalOpacity", CLuaSceneHelper::CalculateGlobalOpacity);
    lua_register(m_LuaState, "calculateBoundingBox", CLuaSceneHelper::CalculateBoundingBox);
    lua_register(m_LuaState, "calculateLocalBoundingBox",
                 CLuaSceneHelper::CalculateLocalBoundingBox);
    lua_register(m_LuaState, "calculateGlobalTransform", CLuaSceneHelper::CalculateGlobalTransform);
    lua_register(m_LuaState, "getImageInfo", CLuaSceneHelper::GetImageInfo);
    lua_register(m_LuaState, "setLocalTransformMatrix", CLuaSceneHelper::SetLocalTransformMatrix);

    lua_register(m_LuaState, "fireEvent", CLuaEventHelper::FireEvent);
    lua_register(m_LuaState, "registerForEvent", CLuaEventHelper::RegisterForEvent);
    lua_register(m_LuaState, "unregisterForEvent", CLuaEventHelper::UnRegisterForEvent);
    // lua_register( m_LuaState, "registerForChange",			CLuaEventHelper::RegisterForChange
    // );
    // lua_register( m_LuaState, "unregisterForChange",		CLuaEventHelper::UnRegisterForChange
    // );

    lua_register(m_LuaState, "play", CLuaCommandHelper::Play);
    lua_register(m_LuaState, "pause", CLuaCommandHelper::Pause);
    lua_register(m_LuaState, "goToTime", CLuaCommandHelper::GoToTime);
    lua_register(m_LuaState, "goToSlide", CLuaCommandHelper::GoToSlide);
    lua_register(m_LuaState, "goToNextSlide", CLuaCommandHelper::GoToNextSlide);
    lua_register(m_LuaState, "goToPreviousSlide", CLuaCommandHelper::GoToPreviousSlide);
    lua_register(m_LuaState, "goToBackSlide", CLuaCommandHelper::GoToBackSlide);

    lua_register(m_LuaState, "getScreenInfo", GetScreenInformation);
    lua_register(m_LuaState, "getElapsedTime", GetElapsedTime);
    lua_register(m_LuaState, "getMousePosition", GetMousePosition);
    lua_register(m_LuaState, "getMultitouch", GetMultitouch);
    lua_register(m_LuaState, "getLuaTable", GetLuaTable);
    lua_register(m_LuaState, "getDirectory", GetDirectory);
    lua_register(m_LuaState, "setTextureData", SetTextureData);
    lua_register(m_LuaState, "testSetTextureData", TestSetTextureData);
    lua_register(m_LuaState, "measureText", MeasureText);
    lua_register(m_LuaState, "loadImages", LoadImages);
    lua_register(m_LuaState, "playToTime", PlayToTime);
    lua_register(m_LuaState, "getStateMachine", GetSCXMLInterpreter);
    lua_register(m_LuaState, "getPresentationSize", GetPresentationSize);
    lua_register(m_LuaState, "getDisplayedSize", GetDisplayedSize);
    lua_register(m_LuaState, "getCameraBounds", GetCameraBounds);
    lua_register(m_LuaState, "positionToScreen", PositionToScreen);
    lua_register(m_LuaState, "screenToPosition", ScreenToPosition);
    lua_register(m_LuaState, "elementAt", ElementAt);
    lua_register(m_LuaState, "facePosition", FacePosition);
    lua_register(m_LuaState, "isPresentationActive", IsPresentationActive);
    lua_register(m_LuaState, "setPresentationActive", SetPresentationActive);
    lua_register(m_LuaState, "getMillisecondsSinceLastFrame", GetMillisecondsSinceLastFrame);
    lua_register(m_LuaState, "playSound", PlaySounds);

    lua_register(m_LuaState, "createMeshBuffer", CreateMeshBuffer);
}

void SLuaEngineImpl::Shutdown(qt3ds::NVFoundationBase &inFoundation)
{
    qCInfo(TRACE_INFO, "Lua engine Begin Exit");
    EndPreloadScripts();
    for (int idx = 0, end = m_ImageLoadingCallbacks.size(); idx < end; ++idx) {
        m_ImageLoadingCallbacks[idx]->m_Engine = NULL;
    }
    m_ImageLoadingCallbacks.clear();
    m_ApplicationCore = NULL;
    m_Application = NULL;
    {
        Mutex::ScopedLock __locker(m_PreloadMutex);
        DisableMultithreadedAccess();
    }
    lua_close(m_LuaState);
    m_LuaState = NULL;

    g_InputStreamFactory = NULL;
    lua_set_file_io(NULL);

    FOR_ARRAY(SCallbackData *, theData, m_EventCallbackDataList)
    FreeCallbackData(*theData);

    FOR_ARRAY(SCallbackData *, theData, m_ChangeCallbackDataList)
    FreeCallbackData(*theData);

    qCInfo(TRACE_INFO, "Lua engine End Exit");
}

//==============================================================================
/**
 * ************************End of CLuaEngine functions ************************
 */
//=============================================================================

//==============================================================================
/**
 * *******************Local functions  ************************
 */
//=============================================================================
/**
 *	Helper function to call the appropriate frame callbacks
 *	@param inElementManager		the manager to get the Script ID from the element
 *	@param inElementList		the list of elements to interate through
 *	@param inFunctionName		the name of the frame callback function to call
 */
void SLuaEngineImpl::ProcessFrameCallbacksHelper(TElementList &inElementList,
                                                 const CHAR *inFunctionName)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;

    qt3ds::foundation::IPerfTimer &thePerfTimer(
        m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer());
    SStackPerfTimer __perfTimer(thePerfTimer, "CLuaEngine::ProcessFrameCallbacksHelper");

    FOR_ARRAY(TElement *, theElement, inElementList)
    {
        if ((*theElement)->GetFlag(ELEMENTFLAG_SCRIPTCALLBACKS))
            CallElementCallback(**theElement, inFunctionName);
    }
}

//==============================================================================
/**
 *	Helper function to call the lua functions in the self table
 *	@param inScriptIndex		the script index to get the self table
 *	@param inFunctionName		the lua function to call
 *	@return true if the function is called, false if otherwise
 */
SLuaFunctionResult SLuaEngineImpl::CallFunction(INT32 inScriptIndex, const CHAR *inFunctionName)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    PerfLogLuaEvent1(DATALOGGER_CALLLUAFUNCTION);

    int theTop = lua_gettop(m_LuaState);

    lua_pushcfunction(m_LuaState, ErrorChannel); // Top + 1 = error handler

    lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // Top + 2 = BehaviorsT

    lua_rawgeti(m_LuaState, -1, inScriptIndex); // Top + 3 = ScriptT
    BOOL theFunctionFlag = FALSE;
    BOOL thePCallError = TRUE;
    if (lua_istable(m_LuaState, -1)) {
        thePCallError = FALSE;
        lua_getfield(m_LuaState, -1, inFunctionName); // Top + 4 = OnXXX function, if there's one

        theFunctionFlag = lua_isfunction(m_LuaState, -1);
        if (theFunctionFlag) // if it is a function, call it
        {
            const char *thePerfName = "";
            IPerfTimer *theTimer = NULL;
            if (m_ApplicationCore) {
                lua_getfield(m_LuaState, -2, "__scriptpath__");
                if (lua_isstring(m_LuaState, -1)) {
                    m_ScriptString.clear();
                    m_ScriptString.assign(lua_tostring(m_LuaState, -1));
                    m_ScriptString.append(" - ");
                    m_ScriptString.append(inFunctionName);
                    thePerfName = m_ScriptString.c_str();
                    theTimer = &m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
                }
            }
            SStackPerfTimer __scriptTimer(theTimer, thePerfName);
            lua_pop(m_LuaState, 1);
            lua_pushlstring(m_LuaState, KEY_SELF, LENGTH_KEY_SELF); // Top + 5 = 'self'
            lua_pushvalue(m_LuaState, -3); // Top + 6 = ScriptT
            lua_rawset(m_LuaState, LUA_GLOBALSINDEX); // [size = Top + 4]

            lua_pushvalue(m_LuaState, -2); // Top + 5 = ScriptT

            int error = PCall(1, 0, theTop + 1); // [size = Top + 3]
            if (error)
                thePCallError = TRUE;
        }
    }

    lua_settop(m_LuaState, theTop);

    return SLuaFunctionResult(theFunctionFlag, thePCallError);
}

void SLuaEngineImpl::CallElementCallback(TElement &inElement, const CHAR *inFunctionName)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    qt3ds::foundation::IPerfTimer &thePerfTimer(
        m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer());
    SStackPerfTimer __perfTimer(thePerfTimer, "CLuaEngine::CallElementCallback");
    SLuaFunctionResult theResult = CallFunction(inElement.m_ScriptID, inFunctionName);
    // If we have the function and there was an error, disable any further callbacks
    if (theResult.m_HasFunction == TRUE && theResult.m_PCallError == TRUE) {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Lua: Script error detected, disabling further callbacks";
        inElement.SetFlag(ELEMENTFLAG_SCRIPTINITIALIZE, false);
        inElement.SetFlag(ELEMENTFLAG_SCRIPTCALLBACKS, false);

    }
}

INT32 SLuaEngineImpl::RegisterCallback(lua_State *inLuaState)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    int argc = lua_gettop(inLuaState);
    luaL_checktype(inLuaState, -1, LUA_TFUNCTION);
    lua_rawgeti(inLuaState, LUA_REGISTRYINDEX, KEY_REGISTEREDCALLBACKS);
    ++m_CallbackIndex;
    lua_rawgeti(inLuaState, -1, m_CallbackIndex);
    // check if the value was valid or not.  If it was, increment callback idx again
    while (!lua_isnil(inLuaState, -1)) {
        lua_pop(inLuaState, 1);
        ++m_CallbackIndex;
        lua_rawgeti(inLuaState, -1, m_CallbackIndex);
    }
    // pop the nil-check off the stack.
    lua_pop(inLuaState, 1);
    // swap the table and the function on the stack.
    lua_insert(inLuaState, -2);
    lua_rawseti(inLuaState, -2, m_CallbackIndex);
    // Pop the table off the stack.
    lua_pop(inLuaState, 1);
    int check = lua_gettop(inLuaState);
    Q3DStudio_ASSERT(check == (argc - 1));
    (void)argc;
    (void)check;
    return m_CallbackIndex;
}

void SLuaEngineImpl::ReleaseCallback(INT32 inCallback)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    int argc = lua_gettop(m_LuaState);
    lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_REGISTEREDCALLBACKS);
    lua_pushnil(m_LuaState);
    lua_rawseti(m_LuaState, -2, inCallback);
    lua_pop(m_LuaState, 1);
    int check = lua_gettop(m_LuaState);
    Q3DStudio_ASSERT(check == argc);
    (void)argc;
    (void)check;
}

void SLuaEngineImpl::ReleaseCallback(SLuaTimeCallback &inCallback)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    inCallback.~SLuaTimeCallback();
    m_TimeCallbackPool.deallocate(&inCallback);
}

void SLuaEngineImpl::PrepareToLoadFile()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;

    if (m_LuaPathInitialized == false) {
        m_LuaPathInitialized = true;
        eastl::string applicationDir(m_ApplicationCore->GetApplicationDirectory());
        CFileTools::ToPlatformPath(applicationDir);

        eastl::string subdir = System::getPlatformStr();
        lua_State *state = m_LuaState != NULL ? m_LuaState : m_PreloadState;
        // this block of code so far is only used by android
        if (!g_AdditionalSharedLibraryPaths.empty()) {
            for (eastl::vector<eastl::string>::iterator iter =
                     g_AdditionalSharedLibraryPaths.begin();
                 iter != g_AdditionalSharedLibraryPaths.end(); iter++) {
                const eastl::string &additionalPathRef = *iter;
                AppendAdditionalLuaPackagePath(state, additionalPathRef);
            }
        }

        eastl::string dataCenterPluginDir;
        CFileTools::CombineBaseAndRelative(applicationDir.c_str(), "Runtime/Plugins/Bindings",
                                           dataCenterPluginDir);
        CFileTools::ToPlatformPath(dataCenterPluginDir);
        // AppendLuaPackagePath( state, scriptsDir, eLuaPath);
        AppendLuaPackagePath(state, dataCenterPluginDir, eCPath);

        // Add in additional search directory for specific platform
        eastl::string scriptsPlatformDir;
        CFileTools::CombineBaseAndRelative(".\\scripts", subdir.c_str(), scriptsPlatformDir);
        CFileTools::ToPlatformPath(scriptsPlatformDir);
        AppendLuaPackagePath(state, scriptsPlatformDir, eCPath);
    }
}

int SLuaEngineImpl::PCall(int argcount, int retcount, int errorIndex)
{
    bool wasProfiling = m_ProfileActive;
    m_ProfileActive = true;
    if (!wasProfiling)
        m_PerfStackIndex = 0;
    int retval = lua_pcall(m_LuaState, argcount, retcount, errorIndex);
    m_ProfileActive = wasProfiling;
    return retval;
}

void SLuaEngineImpl::OnBreak()
{
    qt3ds::state::debugger::IDebugger &theDebugger = m_ApplicationCore->GetStateDebugger();
    theDebugger.OnExternalBreak();
}

void SLuaEngineImpl::CallImageLoadComplete(const char *inPath, bool inSucceeded,
                                           qt3ds::QT3DSU32 inCallbackId)
{
    int top = lua_gettop(m_LuaState);
    lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_REGISTEREDCALLBACKS);
    lua_rawgeti(m_LuaState, -1, inCallbackId);
    lua_insert(m_LuaState, -2);
    // Pop the registeredcallbacks table off the stack.
    lua_pop(m_LuaState, 1);
    lua_pushstring(m_LuaState, nonNull(inPath));
    lua_pushboolean(m_LuaState, inSucceeded ? 1 : 0);
    int failure = PCall(2, 0, 0);
    if (failure) {
        qCCritical(INVALID_OPERATION, "Image loading callback failed: %s", lua_tostring(m_LuaState, -1));
    }
    lua_settop(m_LuaState, top);
}

void SLuaEngineImpl::CallbackFinished(SImageLoadCallback &inCallback)
{
    eastl::vector<SImageLoadCallback *>::iterator theFind =
        eastl::find(m_ImageLoadingCallbacks.begin(), m_ImageLoadingCallbacks.end(), &inCallback);
    if (theFind != m_ImageLoadingCallbacks.end()) {
        (*theFind)->m_Engine = NULL;
        m_ImageLoadingCallbacks.erase(theFind);
    }
}
//==============================================================================
/**
 * ************************End of local functions ************************
 */
//=============================================================================

//==============================================================================
/**
 * *******************static functions from CLuaEngine ************************
 */
//=============================================================================
//==============================================================================
/**
 *	Frees up the SCallbackData memory properly
 *	@param inData		the callback data to free
 */
void CLuaEngine::FreeCallbackData(SCallbackData *inData)
{
    Q3DStudio_free(inData, CLuaEngine::SCallbackData, 1);
}

//==============================================================================
/**
 *	Get the presentation pointer from the lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the Presentation pointer
 */
CPresentation *CLuaEngine::GetCurrentPresentation(lua_State *inLuaState)
{
    CPresentation *thePresentation = NULL;
    SLuaStackScope __topScope(inLuaState);
    lua_pushlstring(inLuaState, KEY_SELF, LENGTH_KEY_SELF); // Top + 1 = "self"
    lua_rawget(inLuaState, LUA_GLOBALSINDEX); // 2 = ScriptT
    lua_pushlstring(inLuaState, KEY_SCRIPT_INDEX,
                    LENGTH_KEY_SCRIPT_INDEX); // Top + 2 = "__scriptIndex"
    lua_rawget(inLuaState, -2); // Top + 2 = script index

    if (lua_isnumber(inLuaState, -1)) {
        INT32 theScriptIndex = static_cast<INT32>(lua_tointeger(inLuaState, -1));
        thePresentation = static_cast<CPresentation *>(
            GetPresentationFromScriptIndex(inLuaState, theScriptIndex));
    }

    if (thePresentation == NULL) {
        lua_getglobal(inLuaState, "UICApplication");
        IApplication *appInterface =
            reinterpret_cast<IApplication *>(lua_touserdata(inLuaState, -1));
        if (appInterface != NULL)
            thePresentation = appInterface->GetPrimaryPresentation();
    }

    return thePresentation;
}

void CLuaEngine::SetAdditionalSharedLibraryPath(const eastl::string &inAdditionalSharedLibraryPath)
{
    eastl::find(g_AdditionalSharedLibraryPaths.begin(), g_AdditionalSharedLibraryPaths.end(),
                inAdditionalSharedLibraryPath)
            == g_AdditionalSharedLibraryPaths.end()
        ? g_AdditionalSharedLibraryPaths.push_back(inAdditionalSharedLibraryPath)
        : void(0);
}

CLuaEngine *CLuaEngine::Create(qt3ds::NVFoundationBase &inFoundation, ITimeProvider &inTimeProvider)
{
    return QT3DS_NEW(inFoundation.getAllocator(), SLuaEngineImpl)(inFoundation, inTimeProvider);
}
//==============================================================================
/**
 * *******************end of static functions from CLuaEngine ******************
 */
//=============================================================================

//==============================================================================
/**
 * *******************exported global Lua functions definition********************
 */
//=============================================================================
namespace __SLuaEngineImpl_Static_Global_Lua_Exporter_Declaration__ {
    using namespace Q3DStudio;

    int Output(lua_State *inLuaState)
    {
        INT32 theTop = lua_gettop(inLuaState);
        QString theString = "";

        for (INT32 theCounter = 1; theCounter <= theTop; ++theCounter) {
            lua_getglobal(inLuaState, "tostring");
            lua_pushvalue(inLuaState, theCounter);
            if (lua_pcall(inLuaState, 1, 1, 0))
                theString.append("output: Error while calling 'tostring'.");
            else {
                const CHAR *theResult = lua_tostring(inLuaState, -1);
                if (theResult)
                    theString.append(theResult);
                else
                    theString.append("output: 'tostring' must return a string to 'output'");
            }

            if ((theCounter + 1) <= theTop)
                theString.append("\n");
        }

        qCInfo(qt3ds::TRACE_INFO) << "Lua: " << theString.toLatin1().constData();

        lua_settop(inLuaState, 0);

        return 0;
    }

    //==============================================================================
    /**
     *	Gets the screen infomation
     *	@param inLuaState		the Lua state, required for interaction with lua
     *	@return 3 - the width, height and scale mode of the presentation are pushed
     *				on the stack in that order
     */
    int GetScreenInformation(lua_State *inLuaState)
    {
        CPresentation *thePresentation = CLuaEngine::GetCurrentPresentation(inLuaState);
        qt3ds::runtime::IApplication &theApp = thePresentation->GetApplication();

        INT32 theViewWidth;
        INT32 theViewHeight;
        SPresentationSize thePresentationSize = thePresentation->GetSize();

        theApp.GetRuntimeFactory().GetSceneManager().GetViewSize(theViewWidth, theViewHeight);

        switch (thePresentationSize.m_ScaleMode) {
        case SCALEMODE_FREE:
            lua_pushinteger(inLuaState, theViewWidth);
            lua_pushinteger(inLuaState, theViewHeight);
            lua_pushstring(inLuaState, SCALE_MODE_FREE);
            break;
        case SCALEMODE_EXACT:
            lua_pushinteger(inLuaState, thePresentationSize.m_Width);
            lua_pushinteger(inLuaState, thePresentationSize.m_Height);
            lua_pushstring(inLuaState, SCALE_MODE_FIXED);
            break;
        case SCALEMODE_ASPECT: {
            FLOAT theAspectRatio = static_cast<FLOAT>(thePresentationSize.m_Width)
                / static_cast<FLOAT>(thePresentationSize.m_Height);
            FLOAT theStudioWidth = theAspectRatio * theViewHeight;
            FLOAT theStudioHeight = static_cast<FLOAT>(theViewHeight);
            if (theStudioWidth > theViewWidth) {
                theStudioWidth = static_cast<FLOAT>(theViewWidth);
                theStudioHeight = theViewWidth / theAspectRatio;
            }
            lua_pushinteger(inLuaState, static_cast<INT32>(theStudioWidth));
            lua_pushinteger(inLuaState, static_cast<INT32>(theStudioHeight));
            lua_pushstring(inLuaState, SCALE_MODE_ASPECT);
        } break;
        case SCALEMODE_UNKNOWN: // Pass thru
        default:
            lua_pushinteger(inLuaState, theViewWidth);
            lua_pushinteger(inLuaState, theViewHeight);
            lua_pushnil(inLuaState);
            break;
        }

        return 3;
    }

    //==============================================================================
    /**
     *	Gets the elapsed time since the start of the presentation. If element isn't
     *	a component, it walks up the tree to find one that is.
     *	@param inLuaState		the Lua state, required for interaction with lua
     *	@return 2 - the elapsed time in seconds and the real world time that were
     *				pushed on the stack
     */
    int GetElapsedTime(lua_State *inLuaState)
    {
        CPresentation *thePresentation = CLuaEngine::GetCurrentPresentation(inLuaState);
        IApplication &theApplication = thePresentation->GetApplication();

        TTimeUnit thePresentationTime = thePresentation->GetTime();
        TTimeUnit theRealWorldTime = theApplication.GetTimeMilliSecs();

        lua_pushnumber(inLuaState, thePresentationTime / 1000.0f);
        lua_pushnumber(inLuaState, theRealWorldTime / 1000.0f);

        return 2;
    }

    //==============================================================================
    /**
     *	Gets the current mouse position in the screen
     *	@param inLuaState		the Lua state, required for interaction with lua
     *	@return 2 - the mouse X and Y in screen pixels that are pushed on the stack
     */
    int GetMousePosition(lua_State *inLuaState)
    {
        CPresentation *thePresentation = CLuaEngine::GetCurrentPresentation(inLuaState);

        IApplication &theApplication = thePresentation->GetApplication();

        SInputFrame theInput = theApplication.GetInputEngine().GetInputFrame();
        if (theApplication.GetPrimaryPresentation()) {

            // Only the primary presentation knows the translation from window to screen
            // coordinates.
            Q3DStudio::SMousePosition thePresPos =
                theApplication.GetPrimaryPresentation()->GetScene()->WindowToPresentation(
                    Q3DStudio::SMousePosition(static_cast<INT32>(theInput.m_PickX),
                                              static_cast<INT32>(theInput.m_PickY)));

            lua_pushinteger(inLuaState, thePresPos.m_X);
            lua_pushinteger(inLuaState, thePresPos.m_Y);

            return 2;
        } else {
            lua_pushstring(inLuaState, "No primary presentation");
            lua_error(inLuaState);
            return 0;
        }
    }

    int GetMultitouch(lua_State *inLuaState)
    {
        CPresentation *thePresentation = CLuaEngine::GetCurrentPresentation(inLuaState);

        IApplication &theApplication = thePresentation->GetApplication();

        SInputFrame theInput = theApplication.GetInputEngine().GetInputFrame();
        if (theApplication.GetPrimaryPresentation()) {
            CInputEngine::TPickInputList theInputs = theApplication.GetInputEngine().GetPickInput();
            lua_newtable(inLuaState);
            for (QT3DSU32 idx = 0, end = theInputs.size(); idx < end; ++idx) {
                lua_newtable(inLuaState);
                const eastl::pair<float, float> &theInput(theInputs[idx]);
                // Only the primary presentation knows the translation from window to screen
                // coordinates.
                Q3DStudio::SMousePosition thePresPos =
                    theApplication.GetPrimaryPresentation()->GetScene()->WindowToPresentation(
                        Q3DStudio::SMousePosition(static_cast<INT32>(theInput.first),
                                                  static_cast<INT32>(theInput.second)));

                lua_pushnumber(inLuaState, thePresPos.m_X);
                lua_setfield(inLuaState, -2, "x");
                lua_pushinteger(inLuaState, thePresPos.m_Y);
                lua_setfield(inLuaState, -2, "y");
                lua_rawseti(inLuaState, -2, idx + 1);
            }

            return 1;
        } else {
            lua_pushstring(inLuaState, "No primary presentation");
            lua_error(inLuaState);
            return 0;
        }
    }

    int GetLuaTable(lua_State *inLuaState)
    {
        luaL_checktype(inLuaState, -1, LUA_TLIGHTUSERDATA);
        TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, -1));

        if (theElement) {
            Q3DStudio::INT32 theScriptId = theElement->m_ScriptID;

            lua_rawgeti(inLuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS);
            lua_rawgeti(inLuaState, -1, theScriptId);
        } else
            lua_pushnil(inLuaState);
        return 1;
    }

    const THashValue HASH_DIRECTORY_PROJECT = CHash::HashString("project");
    const THashValue HASH_DIRECTORY_SCRIPTS = CHash::HashString("scripts");
    const THashValue HASH_DIRECTORY_PLUGINS = CHash::HashString("plugins");
    const THashValue HASH_DIRECTORY_SCRIPTS_PLATFORM = CHash::HashString("scripts-platform");
    const THashValue HASH_DIRECTORY_PLUGINS_PLATFORM = CHash::HashString("plugins-platform");

    int GetDirectory(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);
        if (theEngine && theEngine->m_ApplicationCore) {
            eastl::string sub;
            int argc = lua_gettop(inLuaState);
            const CHAR *directoryName = argc > 0 ? luaL_checkstring(inLuaState, 1) : "project";
            THashValue directoryHash = CHash::HashString(directoryName);
            if (directoryHash == HASH_DIRECTORY_PROJECT)
                sub.clear();
            else if (directoryHash == HASH_DIRECTORY_SCRIPTS)
                sub.assign("scripts");
            else if (directoryHash == HASH_DIRECTORY_PLUGINS)
                sub.assign("plugins");
            else if (directoryHash == HASH_DIRECTORY_SCRIPTS_PLATFORM)
                sub.assign("scripts/").append(System::getPlatformStr());
            else if (directoryHash == HASH_DIRECTORY_PLUGINS_PLATFORM)
                sub.assign("plugins/").append(System::getPlatformGLStr());
            else {
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "getDirectory wasn't passed an recognized string";
                lua_pushnil(inLuaState);
                return 1;
            }
            eastl::string dir(theEngine->m_ApplicationCore->GetProjectDirectory());
            if (!sub.empty())
                qt3ds::foundation::CFileTools::CombineBaseAndRelative(dir.c_str(), sub.c_str(), dir);
            CFileTools::ToPlatformPath(dir);
            lua_pushstring(inLuaState, dir.c_str());
        } else
            lua_pushnil(inLuaState);
        return 1;
    }

    const THashValue HASH_TEXTUREFORMAT_RGB = CHash::HashString("RGB");
    const THashValue HASH_TEXTUREFORMAT_RGBA = CHash::HashString("RGBA");
    const THashValue HASH_TEXTUREFORMAT_RGBA16 = CHash::HashString("RGBA16");
    const THashValue HASH_TEXTUREFORMAT_ALPHA = CHash::HashString("ALPHA");
    const THashValue HASH_TEXTUREFORMAT_LUMINANCE = CHash::HashString("LUMINANCE");
    const THashValue HASH_TEXTUREFORMAT_LUMINANCEALPHA = CHash::HashString("LUMINANCE_ALPHA");
    const THashValue HASH_TEXTUREFORMAT_DXT1 = CHash::HashString("DXT1");
    const THashValue HASH_TEXTUREFORMAT_DXT3 = CHash::HashString("DXT3");
    const THashValue HASH_TEXTUREFORMAT_DXT5 = CHash::HashString("DXT5");

    int SetTextureData(lua_State *inLuaState)
    {

        const INT32 ARG_ELEMENT = 1;
        const INT32 ARG_TEXTUREFORMAT = 2;
        const INT32 ARG_WIDTH = 3;
        const INT32 ARG_HEIGHT = 4;
        const INT32 ARG_TEXTURE_DATA = 5;
        const INT32 ARG_TEXTURE_LENGTH = 6;
        const INT32 HAS_TRANSPARENCY = 7;

        INT32 theWidth = 0;
        INT32 theHeight = 0;

        luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
        int argc = lua_gettop(inLuaState);
        const CHAR *formatName = luaL_checkstring(inLuaState, ARG_TEXTUREFORMAT);
        theWidth = static_cast<INT32>(luaL_checknumber(inLuaState, ARG_WIDTH));
        theHeight = static_cast<INT32>(luaL_checknumber(inLuaState, ARG_HEIGHT));
        TElement *theElement =
            reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));
        // luaL_checktype( inLuaState, ARG_TEXTURE_DATA, LUA_TUSERDATA ); //(userdata expected, got
        // userdata) wt?????
        int theLength =
            argc > 5 ? static_cast<INT32>(luaL_checknumber(inLuaState, ARG_TEXTURE_LENGTH)) : 0;
        int hasTransparency =
            argc > 6 ? static_cast<INT32>(luaL_checknumber(inLuaState, HAS_TRANSPARENCY)) : -1;

        if (theElement) {
            // ensure that format is correct
            THashValue formatHash = CHash::HashString(formatName);
            qt3ds::render::NVRenderTextureFormats::Enum theFormat =
                qt3ds::render::NVRenderTextureFormats::Unknown;
            if (formatHash == HASH_TEXTUREFORMAT_RGB)
                theFormat = qt3ds::render::NVRenderTextureFormats::RGB8;
            else if (formatHash == HASH_TEXTUREFORMAT_RGBA)
                theFormat = qt3ds::render::NVRenderTextureFormats::RGBA8;
            else if (formatHash == HASH_TEXTUREFORMAT_RGBA16)
                theFormat = qt3ds::render::NVRenderTextureFormats::RGBA16F;
            else if (formatHash == HASH_TEXTUREFORMAT_ALPHA)
                theFormat = qt3ds::render::NVRenderTextureFormats::Alpha8;
            else if (formatHash == HASH_TEXTUREFORMAT_LUMINANCE)
                theFormat = qt3ds::render::NVRenderTextureFormats::Luminance8;
            else if (formatHash == HASH_TEXTUREFORMAT_LUMINANCEALPHA)
                theFormat = qt3ds::render::NVRenderTextureFormats::LuminanceAlpha8;
            else if (formatHash == HASH_TEXTUREFORMAT_DXT1)
                theFormat = qt3ds::render::NVRenderTextureFormats::RGBA_DXT1;
            else if (formatHash == HASH_TEXTUREFORMAT_DXT3)
                theFormat = qt3ds::render::NVRenderTextureFormats::RGBA_DXT3;
            else if (formatHash == HASH_TEXTUREFORMAT_DXT5)
                theFormat = qt3ds::render::NVRenderTextureFormats::RGBA_DXT5;
            else {
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "SetTextureData wasn't passed an texture element";
                return 0;
            }

            // start to set the texture data
            const unsigned char *theBuffer = NULL;
            theBuffer =
                reinterpret_cast<unsigned char *>(lua_touserdata(inLuaState, ARG_TEXTURE_DATA));
            IPresentation *thePresentation = theElement->GetBelongedPresentation();
            if (thePresentation && thePresentation->GetScene())
                thePresentation->GetScene()->SetTextureData(theElement, theBuffer, theLength,
                                                            theWidth, theHeight, theFormat,
                                                            hasTransparency);
        } else {
            qCCritical(qt3ds::INVALID_OPERATION) << "SetTextureData wasn't passed an texture element";
        }
        // we pushed 0 numbers to lua
        return 0;
    }

    int TestSetTextureData(lua_State *inLuaState)
    {
        TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, 1));

        NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream =
            theElement->GetBelongedPresentation()
                ->GetApplication()
                .GetRuntimeFactoryCore()
                .GetInputStreamFactory()
                .GetStreamForFile("maps/IBL/AcroIBL.raw");
        if (theStream.mPtr) {
            QT3DSU32 theLength = 230400;
            QT3DSU8 theBuffer[230400];
            QT3DSU32 amountRead = theStream->Read(toDataRef(theBuffer, theLength));
            QT3DS_ASSERT(amountRead == theLength);
            (void)amountRead;
            IPresentation *thePresentation = theElement->GetBelongedPresentation();
            thePresentation->GetScene()->SetTextureData(theElement, theBuffer, theLength, 320, 240,
                                                        qt3ds::render::NVRenderTextureFormats::RGB8,
                                                        false);
        }
        return 0;
    }

    int LoadImages(lua_State *inLuaState)
    {
        const INT32 ARG_ELEMENT = 1;
        const INT32 ARG_IMAGES = 2;
        const INT32 ARG_DEFAULT_IMAGE = 3;
        const INT32 ARG_CALLBACK = 4;

        luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
        luaL_checktype(inLuaState, ARG_IMAGES, LUA_TTABLE);
        luaL_checktype(inLuaState, ARG_DEFAULT_IMAGE, LUA_TSTRING);
        if (lua_gettop(inLuaState) == 4)
            luaL_checktype(inLuaState, ARG_CALLBACK, LUA_TFUNCTION);

        const CHAR *defaultImageStr = luaL_checkstring(inLuaState, ARG_DEFAULT_IMAGE);

        TElement *theElement =
            reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));
        if (theElement == NULL)
            return 0;

        IPresentation *thePresentation = theElement->GetBelongedPresentation();
        IScene *theScene(thePresentation->GetScene());

        eastl::vector<qt3ds::foundation::CRegisteredString> theSourcePaths;

        qt3ds::foundation::CRegisteredString theDefaultImageStr(
            theScene->RegisterStr(defaultImageStr));
        lua_pushnil(inLuaState);
        while (lua_next(inLuaState, ARG_IMAGES) != 0) {
            const char *theValue = luaL_checkstring(inLuaState, -1);
            if (theValue && *theValue) {
                theSourcePaths.push_back(theScene->RegisterStr(theValue));
            }
            lua_pop(inLuaState, 1);
        }

        Q3DStudio::INT32 retval = 0;

        qt3ds::render::IImageLoadListener *theListener = NULL;
        if (lua_gettop(inLuaState) == 4) {
            SLuaEngineImpl *theEngine =
                static_cast<SLuaEngineImpl *>(thePresentation->GetScriptBridge());
            lua_pushvalue(inLuaState, ARG_CALLBACK);
            INT32 cbackId = theEngine->RegisterCallback(inLuaState);
            SImageLoadCallback *theTemp =
                QT3DS_NEW(theEngine->m_Foundation.getAllocator(),
                       SImageLoadCallback)(theEngine->m_Foundation, *theEngine, cbackId);
            theEngine->m_ImageLoadingCallbacks.push_back(theTemp);
            theListener = theTemp;
        }

        if (theSourcePaths.size())
            retval = theScene->LoadImageBatch(theSourcePaths.data(), theSourcePaths.size(),
                                              theDefaultImageStr, theListener);

        lua_pushinteger(inLuaState, retval);

        return 1;
    }

    int MeasureText(lua_State *inLuaState)
    {
        const INT32 ARG_ELEMENT = 1;
        const INT32 ARG_TEXT = 2;
        luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
        int argc = lua_gettop(inLuaState);
        TElement *theElement =
            reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

        const CHAR *textStr = NULL;
        if (argc > 1)
            textStr = luaL_checkstring(inLuaState, ARG_TEXT);

        if (theElement) {
            IPresentation *thePresentation = theElement->GetBelongedPresentation();
            Q3DStudio::STextSizes theDimensions;
            if (thePresentation && thePresentation->GetScene())
                theDimensions = thePresentation->GetScene()->MeasureText(theElement, textStr);

            lua_pushinteger(inLuaState, static_cast<INT32>(theDimensions.m_Width));
            lua_pushinteger(inLuaState, static_cast<INT32>(theDimensions.m_Height));
            return 2;
        } else {
            qCCritical(qt3ds::INVALID_OPERATION) << "SetTextureData wasn't passed an texture element";
        }
        return 0;
    }

    int PlayToTime(lua_State *inLuaState)
    {
        const INT32 ARG_ELEMENT = 1;
        const INT32 ARG_ENDTIME = 2;
        const INT32 ARG_INTERPOLATION = 3;
        const INT32 ARG_CALLBACK = 4;

        luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
        luaL_checktype(inLuaState, ARG_ENDTIME, LUA_TNUMBER);
        luaL_checktype(inLuaState, ARG_INTERPOLATION, LUA_TNUMBER);
        luaL_checktype(inLuaState, ARG_CALLBACK, LUA_TFUNCTION);

        TElement *theElement =
            reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));
        CPresentation *thePresentation =
            static_cast<CPresentation *>(theElement->GetBelongedPresentation());
        SLuaEngineImpl *theEngine =
            static_cast<SLuaEngineImpl *>(thePresentation->GetScriptBridge());
        // Push the callback
        lua_pushvalue(inLuaState, ARG_CALLBACK);
        INT32 cbackId = theEngine->RegisterCallback(inLuaState);
        lua_Number endTime = lua_tonumber(inLuaState, ARG_ENDTIME);
        lua_Number interp = lua_tonumber(inLuaState, ARG_INTERPOLATION);
        // convert time to milliseconds;
        endTime *= 1000;
        TTimeUnit theTime = static_cast<TTimeUnit>(endTime + .5f);
        SLuaTimeCallback *theCallback =
            (SLuaTimeCallback *)theEngine->m_TimeCallbackPool.allocate(__FILE__, __LINE__);
        new (theCallback) SLuaTimeCallback(*theEngine, cbackId, *thePresentation, theElement);
        thePresentation->GetComponentManager().SetComponentTimeOverride(theElement, theTime, interp,
                                                                        theCallback);
        return 0;
    }

    int GetSCXMLInterpreter(lua_State *inLuaState)
    {
        const char *path = lua_tostring(inLuaState, -1);
        if (!path) {
            lua_pushnil(inLuaState);
            return 1;
        }
        // shove it in the global table under its path.  We will get to this again in a second
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_state_machines");
        if (lua_isnil(inLuaState, -1))
            return 1;

        lua_getfield(inLuaState, -1, path);
        return 1;
    }

    int GetEventSystem(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_event_system");
        if (!lua_istable(inLuaState, -1)) {
            lua_pop(inLuaState, 1);
            lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
            SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
            lua_pop(inLuaState, 1);
            qt3ds::evt::IEventSystem &theEventSystem =
                theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetEventSystem();
            qt3ds::evt::SLuaEventPollerBinding::WrapEventPoller(inLuaState, theEventSystem);
            lua_pushvalue(inLuaState, -1);
            lua_setfield(inLuaState, LUA_REGISTRYINDEX, "uic_event_system");
        }
        return 1;
    }

    int IsPresentationActive(lua_State *inLuaState)
    {
        luaL_checktype(inLuaState, 1, LUA_TSTRING);
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        CPresentation *thePresentation =
            theEngine->m_Application->GetPresentationById(lua_tostring(inLuaState, 1));
        if (thePresentation) {
            lua_pushboolean(inLuaState, (int)thePresentation->GetActive());
            return 1;
        }
        lua_pushstring(inLuaState, "Failed to find presentation");
        lua_error(inLuaState);
        return 0;
    }

    int SetPresentationActive(lua_State *inLuaState)
    {
        luaL_checktype(inLuaState, 1, LUA_TSTRING);
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        CPresentation *thePresentation =
            theEngine->m_Application->GetPresentationById(lua_tostring(inLuaState, 1));
        if (thePresentation) {
            int activeBool = lua_toboolean(inLuaState, 2);
            thePresentation->SetActive(activeBool ? true : false);
            return 1;
        }
        lua_pushstring(inLuaState, "Failed to find presentation");
        lua_error(inLuaState);
        return 0;
    }

    int GetPresentationSize(lua_State *inLuaState)
    {
        luaL_checktype(inLuaState, 1, LUA_TLIGHTUSERDATA);
        TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, 1));
        if (theElement == NULL) {
            QT3DS_ASSERT(false);
            return 0;
        }
        IPresentation *thePresentation = theElement->GetBelongedPresentation();
        if (thePresentation->GetScene()) {
            STextSizes theSizes = thePresentation->GetScene()->GetPresentationDesignDimensions();
            lua_pushinteger(inLuaState, theSizes.m_Width);
            lua_pushinteger(inLuaState, theSizes.m_Height);
            return 2;
        }
        QT3DS_ASSERT(false);
        return 1;
    }

    int GetDisplayedSize(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);
        if (theEngine && theEngine->m_Application) {
            Q3DStudio::ISceneManager &theManager(
                theEngine->m_Application->GetRuntimeFactory().GetSceneManager());
            STextSizes theSizes =
                theManager.GetDisplayDimensions(theEngine->m_Application->GetPrimaryPresentation());
            lua_pushinteger(inLuaState, theSizes.m_Width);
            lua_pushinteger(inLuaState, theSizes.m_Height);
            return 2;
        }
        return 0;
    }

    int GetCameraBounds(lua_State *inLuaState)
    {
        luaL_checktype(inLuaState, 1, LUA_TLIGHTUSERDATA);
        TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, 1));
        if (theElement) {
            IPresentation *thePresentation = theElement->GetBelongedPresentation();
            IScene *theScene = thePresentation->GetScene();
            if (theScene) {
                Q3DStudio::SCameraRect theRect = theScene->GetCameraBounds(*theElement);
                if (theRect.IsValid()) {
                    lua_pushnumber(inLuaState, theRect.m_Left);
                    lua_pushnumber(inLuaState, theRect.m_Top);
                    lua_pushnumber(inLuaState, theRect.m_Right);
                    lua_pushnumber(inLuaState, theRect.m_Bottom);
                    return 4;
                }
            }
        }
        return 0;
    }

    int PositionToScreen(lua_State *inLuaState)
    {
        const INT32 ARG_POSITION = 2;
        const INT32 ARG_ELEMENT = 1;

        luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);

        QT3DSVec3 worldPos(0.0f);
        RuntimeVector3 &theVec = CLuaVector::CheckVector(inLuaState, ARG_POSITION);
        worldPos = QT3DSVec3(theVec.m_X, theVec.m_Y, theVec.m_Z);
        TElement *theElement =
            reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

        if (theElement) {
            IPresentation *thePresentation = theElement->GetBelongedPresentation();
            IScene *theScene = thePresentation->GetScene();
            if (theScene) {
                qt3ds::QT3DSVec3 outPos(0.0f);
                theScene->PositionToScreen(*theElement, worldPos, outPos);

                theVec.Set(outPos.x, outPos.y, outPos.z);

                // NOTE -- a zero output is what projection/unproject routines return if it could
                // not
                // acquire the appropriate scene data or camera data for this element.
                if (outPos.isZero()) {
                    qCCritical(qt3ds::INVALID_OPERATION)
                            << "PositionToScreen could not successfully project position";
                }
            }
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "PositionToScreen was not passed a Node element";
        }
        return 0;
    }

    int ScreenToPosition(lua_State *inLuaState)
    {
        const INT32 ARG_SCREEN = 2;
        const INT32 ARG_ELEMENT = 1;

        luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);

        QT3DSVec3 screenPos(0.0f);
        RuntimeVector3 &theVec = CLuaVector::CheckVector(inLuaState, ARG_SCREEN);
        screenPos = QT3DSVec3(theVec.m_X, theVec.m_Y, theVec.m_Z);
        TElement *theElement =
            reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

        if (theElement) {
            IPresentation *thePresentation = theElement->GetBelongedPresentation();
            IScene *theScene = thePresentation->GetScene();
            if (theScene) {
                qt3ds::QT3DSVec3 outPos(0.0f);
                theScene->ScreenToPosition(*theElement, screenPos, outPos);
                theVec.Set(outPos.x, outPos.y, outPos.z);
            }
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "ScreenToPosition was not passed a Node element";
        }
        return 0;
    }

    int ElementAt(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);

        if (theEngine && theEngine->m_Application) {
            luaL_checktype(inLuaState, 1, LUA_TNUMBER);
            luaL_checktype(inLuaState, 2, LUA_TNUMBER);

            float mouseX = lua_tonumber(inLuaState, 1);
            float mouseY = lua_tonumber(inLuaState, 2);

            Q3DStudio::ISceneManager &theManager =
                theEngine->m_Application->GetRuntimeFactory().GetSceneManager();
            Q3DStudio::TElement *retvalElem = theManager.UserPick(mouseX, mouseY);

            if (retvalElem)
                lua_pushlightuserdata(inLuaState, retvalElem);
            else
                lua_pushnil(inLuaState);
            return 1;
        }

        return 0;
    }

    int FacePosition(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);

        if (theEngine && theEngine->m_Application) {
            luaL_checktype(inLuaState, 1, LUA_TLIGHTUSERDATA);
            luaL_checktype(inLuaState, 2, LUA_TNUMBER);
            luaL_checktype(inLuaState, 3, LUA_TNUMBER);
            eastl::vector<Q3DStudio::TElement *> theMapperElements;
            if (lua_gettop(inLuaState) >= 4) {
                if (!lua_isnil(inLuaState, 4)) {
                    luaL_checktype(inLuaState, 4, LUA_TTABLE);
                    // iterate through the table as if it is an array.
                    int table_len = luaL_getn(inLuaState, 4);
                    for (int idx = 1; idx <= table_len; ++idx) {
                        lua_rawgeti(inLuaState, 4, idx);
                        Q3DStudio::TElement *mapperElem =
                            reinterpret_cast<Q3DStudio::TElement *>(lua_touserdata(inLuaState, -1));
                        lua_pop(inLuaState, 1);
                        theMapperElements.push_back(mapperElem);
                    }
                }
            }
            FacePositionPlanes::Enum theFacePos(FacePositionPlanes::XY);
            if (lua_gettop(inLuaState) >= 5) {
                luaL_checktype(inLuaState, 5, LUA_TSTRING);
                const char *theData = lua_tostring(inLuaState, 5);
                eastl::string theEData(theData);
                if (theEData.comparei("XZ") == 0)
                    theFacePos = FacePositionPlanes::XZ;
                else if (theEData.comparei("YZ") == 0)
                    theFacePos = FacePositionPlanes::YZ;
            }

            Q3DStudio::TElement *theElem =
                reinterpret_cast<Q3DStudio::TElement *>(lua_touserdata(inLuaState, 1));
            float mouseX = lua_tonumber(inLuaState, 2);
            float mouseY = lua_tonumber(inLuaState, 3);
            if (theElem == NULL)
                return 0;

            Q3DStudio::ISceneManager &theManager =
                theEngine->m_Application->GetRuntimeFactory().GetSceneManager();
            qt3ds::foundation::Option<qt3ds::QT3DSVec2> relativePos = theManager.FacePosition(
                *theElem, mouseX, mouseY,
                NVDataRef<TElement *>(theMapperElements.data(), (QT3DSU32)theMapperElements.size()),
                theFacePos);
            if (relativePos.hasValue()) {
                lua_pushnumber(inLuaState, relativePos->x);
                lua_pushnumber(inLuaState, relativePos->y);
                return 2;
            }
        }
        return 0;
    }

    int GetMillisecondsSinceLastFrame(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);
        if (theEngine->m_Application) {
            lua_pushnumber(inLuaState, theEngine->m_Application->GetMillisecondsSinceLastFrame());
            return 1;
        }
        return 0;
    }

    int PlaySounds(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "uic_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);
        bool theResult = false;
        if (theEngine->m_Application) {
            luaL_checktype(inLuaState, 1, LUA_TSTRING);
            const char *thePathC = lua_tostring(inLuaState, 1);
            eastl::string thePath(thePathC);
            theResult = theEngine->PlaySoundFile(thePath.c_str());
        }
        lua_pushboolean(inLuaState, theResult ? 1 : 0);
        return 1;
    }

    int CreateMeshBuffer(lua_State *inLuaState)
    {
        const INT32 ARG_MESHNAME = 1;
        const INT32 ARG_PARENT = 2;
        const INT32 ARG_VERTEX_DATA = 3;
        const INT32 ARG_NUM_VERTS = 4;
        const INT32 ARG_INDEX_DATA = 5;
        const INT32 ARG_NUM_INDICES = 6;
        const INT32 ARG_BOUNDS_MIN = 7;
        const INT32 ARG_BOUNDS_MAX = 8;

        // Get the full element name used for entry into the RenderBufferManager
        eastl::string theElemName("");

        luaL_checktype(inLuaState, ARG_MESHNAME, LUA_TSTRING);
        Q3DStudio::TElement *theElem =
            reinterpret_cast<Q3DStudio::TElement *>(lua_touserdata(inLuaState, ARG_PARENT));
        if (!theElem) {
            qCCritical(qt3ds::INVALID_OPERATION) << "CreateMeshBuffer was not passed a parent object";
        } else {
            theElemName.append(theElem->m_Path.c_str());
            theElemName.append(".#");
            theElemName.append(lua_tostring(inLuaState, ARG_MESHNAME));
        }

        // assumed that vertex format (for now) is Position + UV + Normal
        // so the vertex stride is always (3 + 2 + 3) * 4 = 32 bytes
        QT3DSU8 *vertData = NULL;
        QT3DSU32 *idxData = NULL;

        vertData = reinterpret_cast<QT3DSU8 *>(lua_touserdata(inLuaState, ARG_VERTEX_DATA));
        QT3DSU32 numVerts = static_cast<INT32>(luaL_checknumber(inLuaState, ARG_NUM_VERTS));

        idxData = reinterpret_cast<QT3DSU32 *>(lua_touserdata(inLuaState, ARG_INDEX_DATA));
        QT3DSU32 numIndices = static_cast<INT32>(luaL_checknumber(inLuaState, ARG_NUM_INDICES));

        if ((!vertData) || (!idxData)) {
            qCCritical(qt3ds::INVALID_OPERATION) << "CreateMeshBuffer was not provided a valid vertex or index buffer!";
        }

        RuntimeVector3 &theMinVec = CLuaVector::CheckVector(inLuaState, ARG_BOUNDS_MIN);
        RuntimeVector3 &theMaxVec = CLuaVector::CheckVector(inLuaState, ARG_BOUNDS_MAX);

        qt3ds::NVBounds3 theBounds;
        theBounds.minimum = qt3ds::QT3DSVec3(theMinVec.m_X, theMinVec.m_Y, theMinVec.m_Z);
        theBounds.maximum = qt3ds::QT3DSVec3(theMaxVec.m_X, theMaxVec.m_Y, theMaxVec.m_Z);

        bool success = theElem->GetBelongedPresentation()->GetScene()->CreateOrSetMeshData(
            theElemName.c_str(), vertData, numVerts, 32, idxData, numIndices, theBounds);
        if (!success)
            qCCritical(qt3ds::INVALID_OPERATION) << "CreateMeshBuffer failed!";

        return 0;
    }
}
//==============================================================================
/**
 * *******************end of exported global Lua functions definitions************
 */
//=============================================================================

} // namespace Q3DStudio
