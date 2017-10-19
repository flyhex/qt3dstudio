/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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
#include "Qt3DSStateLuaEngine.h"
#include "Qt3DSFileStream.h"
#include "Qt3DSDataLogger.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSLogging.h"
#include "foundation/Qt3DSSystem.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSStateLuaScriptContext.h"
#include "Qt3DSStateDebugger.h"
#include "EventPollingSystemLuaBinding.h"
#include "Qt3DSHash.h"

#include "EASTL/vector.h"
#include <sys/stat.h>

#include "Qt3DSStateDebugStreams.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "Qt3DSLuaDebugger.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSParametersSystem.h"
#include "Qt3DSTypes.h"

#include "Qt3DSStateInputStreamFactory.h"
#include "Qt3DSStateApplication.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace {

using qt3ds::foundation::atomicIncrement;
using qt3ds::foundation::atomicDecrement;
using qt3ds::foundation::CFileTools;
using qt3ds::QT3DSI32;
using qt3ds::QT3DSU32;
using Q3DStudio::BOOL;
using Q3DStudio::INT32;
using Q3DStudio::INT8;
using Q3DStudio::CHAR;
using Q3DStudio::CHash;

#if defined(_DEBUG) || (_PROFILE)
#define Q3DStudio_LOG_EVENT(S) // something
#else
#define Q3DStudio_LOG_EVENT(S)
#endif

// forward declaration of exported lua global functions
namespace __SLuaEngineImpl_Static_Global_Lua_Exporter_Declaration__ {
    static int Output(lua_State *inLuaState);
    static int GetElapsedTime(lua_State *inLuaState);
    static int GetDirectory(lua_State *inLuaState);
    static int ParseSCXML(lua_State *inLuaState);
    static int GetSCXMLInterpreter(lua_State *inLuaState);
    static int GetEventSystem(lua_State *inLuaState);
    static int GetMillisecondsSinceLastFrame(lua_State *inLuaState);
}

using namespace __SLuaEngineImpl_Static_Global_Lua_Exporter_Declaration__;

//==============================================================================
//	Constants
//==============================================================================

// for unique addresses to be used as keys for Lua Registry
const Q3DStudio::INT32 KEY_PRESENTATION = 1;
const Q3DStudio::INT32 KEY_BEHAVIORS = 2;
const Q3DStudio::INT32 KEY_EVENTDATA_LIST = 4;
const Q3DStudio::INT32 KEY_CHANGEDATA_LIST = 5;
const Q3DStudio::INT32 KEY_CALLBACKS = 6;
const Q3DStudio::INT32 KEY_REGISTEREDCALLBACKS = 7;

const char *KEY_ELEMENT = "element";
const char *KEY_SELF = "self";
const char *KEY_SCRIPT_INDEX = "__scriptIndex";
const char *KEY_ONINITIALIZE = "onInitialize";
const char *KEY_ONACTIVATE = "onActivate";
const char *KEY_ONDEACTIVATE = "onDeactivate";
const char *KEY_ONUPDATE = "onUpdate";

struct SLuaEngineImpl;
namespace __SLuaEngineImpl_Basic_Structs__ {
    struct SLuaEngineListener : public qt3ds::state::debugger::ILuaSideListener
    {
        qt3ds::NVFoundationBase &m_Foundation;
        SLuaEngineImpl &m_Engine;
        qt3ds::QT3DSI32 m_RefCount;
        SLuaEngineListener(qt3ds::NVFoundationBase &fnd, SLuaEngineImpl &eng)
            : m_Foundation(fnd)
            , m_Engine(eng)
            , m_RefCount(0)
        {
        }

        void addRef() { qt3ds::foundation::atomicIncrement(&m_RefCount); }
        void release()
        {
            qt3ds::foundation::atomicDecrement(&m_RefCount);
            if (m_RefCount <= 0)
                qt3ds::foundation::NVDelete(m_Foundation, this);
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
        FILE *m_File;
        bool m_EOF;
        qt3ds::foundation::NVScopedRefCounted<qt3ds::state::IRefCountedInputStream> m_InputStream;
        eastl::vector<char8_t> m_Buffer;

        SUserFile()
            : m_File(0)
            , m_EOF(true)
        {
        }
        SUserFile(FILE *f)
            : m_File(f)
            , m_EOF(false)
        {
        }
        SUserFile(qt3ds::state::IRefCountedInputStream &stream)
            : m_File(0)
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
                qt3ds::QT3DSU32 amount = qt3ds::NVMin((qt3ds::QT3DSU32)m_Buffer.size(), (qt3ds::QT3DSU32)len);
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
                    amountRead = (qt3ds::QT3DSU32)fread(writeBuffer, 1, len, m_File);

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
                double d = strtod(m_Buffer.data(), 0);
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

// int CScriptEngineCallFunctionArgRetriever::RetrieveArgument( lua_State *inState )
//{
//	int	theRet = 0;
//	if ( m_ArgumentString && *m_ArgumentString )
//	{
//		eastl::string temp( m_ArgumentString );
//		temp.insert( 0, "return " );
//		if ( luaL_loadstring( inState,  temp.c_str() ) != 0 )
//		{
//			theRet = -1;
//		}
//		else
//		{
//			theRet = lua_pcall( inState, 0, 1, 0 ) ? -1 : 1;
//		}
//	}
//	return theRet;
//}
//
// eastl::string CScriptEngineCallFunctionArgRetriever::GetArgDescription()
//{
//	eastl::string temp( m_ArgumentString );
//	temp.insert( 0, "return " );
//	return temp;
//}

namespace {
    qt3ds::state::IInputStreamFactory *g_InputStreamFactory;

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
                        return 0;
                }
        }*/
        qt3ds::state::IRefCountedInputStream *stream =
            g_InputStreamFactory->GetStreamForFile(filename, true);
        if (stream) {
            if (absfilename) {
                eastl::string tempStr;
                if (g_InputStreamFactory->GetPathForFile(filename, tempStr)) {
                    strcpy(absfilename, tempStr.c_str());
                }
            }
            return new SUserFile(*stream);
        }

        return 0;
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
        return 0;
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
        if (state == 0)
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
        qt3ds::foundation::CFileTools::CombineBaseAndRelative(inPath.c_str(), "scripts", scriptsDir);
        AppendLuaPackagePath(state, scriptsDir, eCPath);

        // Add in additional search directory for specific platform
        eastl::string scriptsPlatformDir;
        qt3ds::foundation::CFileTools::CombineBaseAndRelative(
            scriptsDir.c_str(), qt3ds::foundation::System::getPlatformStr(), scriptsPlatformDir);
        AppendLuaPackagePath(state, scriptsPlatformDir, eCPath);
    }
}

namespace {

    static int WriteLuaData(lua_State *, const void *p, size_t sz, void *ud)
    {
        qt3ds::foundation::MemoryBuffer<> &theWriter =
            *reinterpret_cast<qt3ds::foundation::MemoryBuffer<> *>(ud);
        theWriter.write((const qt3ds::QT3DSU8 *)p, (qt3ds::QT3DSU32)sz);
        return 0;
    }

    static void *MemoryAllocator(void *inUserData, void *inPointer, size_t inOldSize,
                                 size_t inNewSize)
    {
        Q3DStudio_UNREFERENCED_PARAMETER(inUserData);

        if (inNewSize == 0) {
            Q3DStudio_free(inPointer, INT8, inOldSize);
            return 0;
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

    // static int ErrorChannel( lua_State* inLuaState )
    //{
    //	NVLog( NVLOGLEVEL_ERROR, "Lua", "ErrorChannel", "%s", lua_tostring( inLuaState, -1 ) );
    //	lua_pop( inLuaState, 1 );
    //	return 0;
    //}

    static int EnsureScriptIsLoaded(lua_State *inLuaState, const char *inRelativePath,
                                    const char *inBasePath, eastl::string &scriptBuffer
                                    //, qt3ds::foundation::IPerfTimer& inPerfTimer
                                    ,
                                    qt3ds::state::IInputStreamFactory &inStreamFactory,
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
            // SStackPerfTimer __perfTimer( inPerfTimer, "Load Lua File" );
            lua_pop(inLuaState, 1);
            qt3ds::state::IInputStreamFactory &theFactory(inStreamFactory);
            qt3ds::foundation::NVScopedRefCounted<qt3ds::state::IRefCountedInputStream> theStream =
                theFactory.GetStreamForFile(inRelativePath, true);

            inLoadVec.clear();
            if (theStream) {
                qt3ds::QT3DSU8 buf[1024];
                for (size_t amount = theStream->Read(qt3ds::foundation::toDataRef(buf, 1024)); amount;
                     amount = theStream->Read(qt3ds::foundation::toDataRef(buf, 1024)))
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
qt3ds::state::TLuaLibraryLoader LoadLuaLibraries =
    luaL_openlibs; ///< Defaults to open all lua libraries
lua_user_file_io g_engine_file_io;
lua_Alloc g_engine_alloc = MemoryAllocator;

struct SLuaEngineImpl : public qt3ds::state::INDDStateLuaEngine
{
    typedef eastl::vector<eastl::string> TStringList;
    qt3ds::state::SNDDStateContext &m_Context;
    lua_State *m_LuaState;

    qt3ds::state::INDDStateApplication *m_Application;

    INT32 m_CallbackIndex;

    INT32 m_ServerPort; ///<port number of remote debugger
    eastl::string m_ScriptString;
    eastl::string m_ServerIP;
    qt3ds::foundation::NVScopedRefCounted<qt3ds::state::debugger::ILuaDebugger> m_LuaDebugger;
    qt3ds::foundation::NVScopedRefCounted<SLuaEngineListener> m_LuaListener;

    lua_State *m_PreloadState;
    TStringList m_PreloadScripts;
    qt3ds::foundation::Sync m_PreloadSync;
    eastl::string m_PreloadProjectDir;
    qt3ds::foundation::Mutex m_PreloadMutex;
    qt3ds::foundation::Mutex *m_MultithreadedMutex;

    eastl::vector<char8_t> m_LoadVec;
    bool m_LuaPathInitialized;
    qt3ds::QT3DSI32 mRefCount;

    qt3ds::QT3DSU32 m_PerfStackIndex;
    eastl::string m_PerfNameBuilder;
    bool m_ProfileActive;
    eastl::vector<SLuaPerfEvent> m_PerfStack;

    SLuaEngineImpl(qt3ds::state::SNDDStateContext &inContext);
    virtual ~SLuaEngineImpl();

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Context.GetAllocator())

    // functions from IScriptBridge
    void EnableMultithreadedAccess() override;
    void DisableMultithreadedAccess() override;

    void SetApplication(qt3ds::state::INDDStateApplication &inApplication) override;

    //	virtual void		BeginPreloadScripts( const eastl::vector<const char*>& inScripts,
    //uic::render::IThreadPool& inThreadPool, const char* inProjectDir );
    //	virtual void		EndPreloadScripts();
    eastl::vector<eastl::string> GetLoadedScripts() override;

    Q3DStudio::INT32 InitializeApplicationBehavior(const char *inProjectRelativePath) override;

    void ExecuteApplicationScriptFunction(Q3DStudio::INT32 inApp, const char *inFnName) override;
    //	virtual void		CallFunction( const char* behavior, const char* handler,
    //CScriptEngineCallFunctionArgRetriever &inArgRetriever );

    void AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction) override;
    //	virtual void		EnableDebugging(uic::state::debugger::IMultiProtocolSocket& socket
    //);
    //	virtual void		EnableProfiling();
    void StepGC() override;

    // functions from CLuaEngine
    qt3ds::state::IStateInterpreter *CreateStateMachine(const char8_t *inPath,
                                                              const char8_t *inId,
                                                              const char8_t *inDatamodelFunction) override;
    void PreInitialize() override;
    void Initialize() override;
    void Shutdown(qt3ds::NVFoundationBase &inFoundation) override;

    // local functions
    // SLuaFunctionResult	CallFunction( const Q3DStudio::INT32 inScriptIndex, const CHAR*
    // inFunctionName );
    void PrepareToLoadFile();
    int PCall(int argcount, int retcount, int errorIndex);
    void OnBreak();
};

namespace __SLuaEngineImpl_Static_Calls__ {
    // static void	PreloadFunctionCall( void* inEngine )
    //{
    //	SLuaEngineImpl* theEngine = reinterpret_cast<SLuaEngineImpl*>( inEngine );
    //	eastl::vector<char8_t> loadBuffer;
    //	eastl::string pathBuffer;
    //	SStackPerfTimer __perfTimer(
    //theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer(), "Preload Lua Data" );
    //	{
    //		//First attempt to load all compiled lua scripts.
    //		eastl::string theLuaBinFile = theEngine->m_PreloadProjectDir;
    //		CFileTools::CombineBaseAndRelative( theEngine->m_PreloadProjectDir.c_str(),
    //"binary/compiledlua.bin", theLuaBinFile );
    //		NVScopedRefCounted<uic::render::IRefCountedInputStream> theBinFile
    //			=
    //theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetInputStreamFactory().GetStreamForFile(
    //theLuaBinFile.c_str() );
    //		if ( theBinFile )
    //		{
    //			theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetFoundation().error( QT3DS_INFO,
    //"Lua - Found binary file, loading scripts" );
    //			eastl::vector<eastl::pair<eastl::string, QT3DSU32> > theDirectory;
    //			QT3DSU32 numDirFiles = 0;
    //			theBinFile->Read( numDirFiles );
    //			theDirectory.resize( numDirFiles );
    //			for ( QT3DSU32 idx = 0, end = numDirFiles; idx < end; ++idx )
    //			{
    //				QT3DSU32 strLen = 0;
    //				theBinFile->Read( strLen );
    //				theDirectory[idx].first.resize( strLen );
    //				theBinFile->Read( (QT3DSU8*)&theDirectory[idx].first[0], strLen );
    //				theBinFile->Read( theDirectory[idx].second );
    //			}
    //			QT3DSU32 binSectionSize = 0;
    //			theBinFile->Read( binSectionSize );
    //			eastl::vector<char8_t> theData;
    //			theData.resize( binSectionSize );
    //			theBinFile->Read( theData.data(), binSectionSize );
    //			SLuaStackScope theScope( theEngine->m_PreloadState );
    //			GetLoadedBuffersTable(theEngine->m_PreloadState);
    //			for ( QT3DSU32 idx = 0, end = numDirFiles; idx < end; ++idx )
    //			{
    //				eastl::pair<eastl::string,QT3DSU32> dirEntry = theDirectory[idx];
    //				QT3DSU32 entrySize = 0;
    //				if ( idx < end - 1 )
    //					entrySize = theDirectory[idx+1].second - dirEntry.second;
    //				else
    //					entrySize = binSectionSize - dirEntry.second;
    //				qt3ds::foundation::CFileTools::CombineBaseAndRelative(
    //theEngine->m_PreloadProjectDir.c_str(), dirEntry.first.c_str(),  pathBuffer );
    //				qt3ds::foundation::CFileTools::ToPlatformPath( pathBuffer );
    //				pathBuffer.insert( 0, "@" );
    //
    //				luaL_loadbuffer( theEngine->m_PreloadState, static_cast<const char*>(
    //&theData[dirEntry.second] ),
    //					entrySize, pathBuffer.c_str() );
    //				lua_setfield( theEngine->m_PreloadState, -2, dirEntry.first.c_str()
    //);
    //			}
    //		}
    //	}

    //	for ( QT3DSU32 idx = 0, end = theEngine->m_PreloadScripts.size(); idx < end; ++idx )
    //	{
    //		SLuaStackScope theScope( theEngine->m_PreloadState );
    //		EnsureScriptIsLoaded( theEngine->m_PreloadState,
    //theEngine->m_PreloadScripts[idx].c_str()
    //							, theEngine->m_PreloadProjectDir.c_str()
    //							, pathBuffer
    //							,
    //theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer()
    //							,
    //theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetInputStreamFactory()
    //							, loadBuffer );
    //	}
    //	theEngine->m_PreloadSync.set();
    //}

    // static void ProfileHook( lua_State* inLuaState, lua_Debug* ar )
    //{
    //	//First check if valid function
    //	//We only profile pure lua code.
    //	lua_Debug stackInfo;
    //	if ( lua_getstack( inLuaState, 0, &stackInfo ) && lua_getinfo( inLuaState, "nSl", &stackInfo
    //) )
    //	{
    //		bool validFunction = ( ar->event == LUA_HOOKCALL || ar->event == LUA_HOOKRET )
    //								&& !qt3ds::foundation::isTrivial( stackInfo.source
    //)
    //								&& stackInfo.source[0] == '@';

    //		if ( !validFunction ) return;
    //
    //		lua_getfield( inLuaState, LUA_REGISTRYINDEX, "qt3ds_lua_engine" );
    //		SLuaEngineImpl* theEngine = (SLuaEngineImpl*)( lua_touserdata( inLuaState, -1 ) );
    //		lua_pop( inLuaState, 1 );
    //		theEngine->m_PerfNameBuilder.assign( stackInfo.source + 1 );
    //		eastl::string::size_type slashPos = theEngine->m_PerfNameBuilder.find_last_of( "\\/"
    //);
    //		if ( slashPos != eastl::string::npos )
    //			theEngine->m_PerfNameBuilder.erase( theEngine->m_PerfNameBuilder.begin(),
    //theEngine->m_PerfNameBuilder.begin() + slashPos + 1 );
    //		theEngine->m_PerfNameBuilder.append( ":" );
    //		char buf[24];
    //		sprintf( buf, "%d", stackInfo.linedefined );
    //		theEngine->m_PerfNameBuilder.append( buf );
    //		size_t currentPerfStackSize = theEngine->m_PerfStack.size();
    //		if ( ar->event == LUA_HOOKCALL )
    //		{
    //			while ( theEngine->m_PerfStackIndex >= currentPerfStackSize )
    //			{
    //				theEngine->m_PerfStack.push_back( SLuaPerfEvent() );
    //				++currentPerfStackSize;
    //			}

    //			SLuaPerfEvent& backEvent =
    //theEngine->m_PerfStack[theEngine->m_PerfStackIndex];
    //			++theEngine->m_PerfStackIndex;
    //			backEvent.m_Time = qt3ds::foundation::Time::getCurrentCounterValue();
    //			backEvent.m_Name.assign( theEngine->m_PerfNameBuilder );
    //		}
    //		else
    //		{

    //			if ( theEngine->m_ApplicationCore != 0
    //				&& theEngine->m_PerfStackIndex > 0
    //				&& theEngine->m_PerfStackIndex <= currentPerfStackSize )
    //			{
    //				qt3ds::foundation::IPerfTimer& theTimer =
    //theEngine->m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
    //				SLuaPerfEvent& backEvent = theEngine->m_PerfStack[theEngine->m_PerfStackIndex -
    //1];
    //				-- theEngine->m_PerfStackIndex;
    //				theTimer.Update( backEvent.m_Name.c_str(),
    //qt3ds::foundation::Time::getCurrentCounterValue() - backEvent.m_Time );
    //			}
    //		}
    //	}
    //}
}
using namespace __SLuaEngineImpl_Static_Calls__;

namespace __SLuaEngineImpl_Basic_Structs__ {
    // void SLuaEngineListener::OnBreak()
    //{
    //	m_Engine.OnBreak();
    //}
}
//==============================================================================
/**
 *	Constructor
 */
SLuaEngineImpl::SLuaEngineImpl(qt3ds::state::SNDDStateContext &inContext)
    : m_Context(inContext)
    , m_LuaState(0)
    , m_Application(0)
    , m_ServerPort(0)
    , m_CallbackIndex(0)
    , m_LuaPathInitialized(false)
    , m_PreloadState(0)
    , m_PreloadSync(m_Context.GetAllocator())
    , m_PreloadMutex(m_Context.GetAllocator())
    , m_MultithreadedMutex(0)
    , m_PerfStackIndex(0)
    , m_ProfileActive(false)
    , mRefCount(0)
{
    m_LuaState = lua_newstate(g_engine_alloc, 0);
    Q3DStudio_ASSERT(m_LuaState);

    lua_gc(m_LuaState, LUA_GCSTOP, 0);

    // Override this function pointer for customization of loading of lua libraries.
    // Default uses luaL_openlibs which opens all libraries found in linit.c
    LoadLuaLibraries(m_LuaState);

    lua_newtable(m_LuaState);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // Reg(1) = BehaviorsT

    // Reg(5) = Global functions temporary parking lot for registerForEvent and registerForChange
    lua_newtable(m_LuaState);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS);

    // Callbacks table for callbacks sent from script.
    lua_newtable(m_LuaState);
    lua_rawseti(m_LuaState, LUA_REGISTRYINDEX, KEY_REGISTEREDCALLBACKS);

    lua_pushlightuserdata(m_LuaState, this);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "qt3ds_lua_engine");

    lua_getglobal(m_LuaState, "math"); // 2 = "math" table
    lua_pushstring(m_LuaState, "randomseed"); // 3 = "randomseed"
    lua_rawget(m_LuaState, -2); // 3 = math.randomseed function
    lua_pushinteger(m_LuaState,
                    static_cast<INT32>(m_Context.m_TimeProvider.GetCurrentTimeMicroSeconds()));
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

SLuaEngineImpl::~SLuaEngineImpl()
{
    qCDebug (qt3ds::TRACE_INFO) << "SLuaEngineImpl: destructing";
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
    m_MultithreadedMutex = 0;
}

void SLuaEngineImpl::SetApplication(qt3ds::state::INDDStateApplication &inApplication)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_Application = &inApplication;
    lua_pushlightuserdata(m_LuaState, m_Application);
    lua_setglobal(m_LuaState, "Qt3DSApplication");
}

// Starts preloading scripts offline.  This sets m_LuaState to NULL until after EndPreloadScripts to
// avoid multithreaded lua state access.  Some calls may be queued till EndPreloadScripts
// void SLuaEngineImpl::BeginPreloadScripts( const eastl::vector<const char*>& inScripts,
// uic::render::IThreadPool& inThreadPool, const char* inProjDir )
//{
//	m_PreloadScripts.assign( inScripts.begin(), inScripts.end() );
//	m_PreloadState = m_LuaState;
//	m_LuaState = 0;
//	m_PreloadProjectDir.assign( inProjDir );
//	m_PreloadSync.reset();
//	inThreadPool.AddTask( this, PreloadFunctionCall, 0 );
//}

// Ends preload, restores m_LuaState.
// void SLuaEngineImpl::EndPreloadScripts()
//{
//	qt3ds::foundation::Mutex::ScopedLock __locker( m_PreloadMutex );
//	if ( m_LuaState == 0 )
//	{
//		m_PreloadSync.wait();
//		m_LuaState = m_PreloadState;
//		m_PreloadState = 0;
//	}
//}

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
    qt3ds::foundation::MemoryBuffer<> theBuffer(
        qt3ds::foundation::ForwardingAllocator(m_Context.GetAllocator(), "LuaBinaryData"));
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
    qt3ds::foundation::CFileTools::CombineBaseAndRelative(m_Context.GetProjectDir(), "binary",
                                                       tempStr);
    qt3ds::foundation::CFileTools::CreateDir(tempStr.c_str());
    tempStr.append("/compiledlua.bin");
    qt3ds::foundation::CFileSeekableIOStream theStream(tempStr.c_str(),
                                                    qt3ds::foundation::FileWriteFlags());
    if (theStream.IsOpen() == false) {
        qCCritical(qt3ds::INTERNAL_ERROR, "Unable to open lua binary file");
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

Q3DStudio::INT32 SLuaEngineImpl::InitializeApplicationBehavior(const char *inRelativePath)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope __topScope(m_LuaState);
    PrepareToLoadFile();
    eastl::string scriptBuffer;
    int loadError =
        EnsureScriptIsLoaded(m_LuaState, inRelativePath, m_Context.GetProjectDir(), scriptBuffer
                             //, m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer()
                             ,
                             *m_Context.m_InputStreamFactory, m_LoadVec);
    if (loadError) {
        const char *errorMsg = lua_tostring(m_LuaState, -1);
        qCCritical(qt3ds::INVALID_PARAMETER, "Failed to load lua file: %s: %s",
                                      inRelativePath, errorMsg);
        return 0;
    }
    Q3DStudio::INT32 theScriptIndex = 0;
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
        const char *errorMsg = lua_tostring(m_LuaState, -1);
        qCCritical(qt3ds::INVALID_PARAMETER, "Failed to load lua file: %s: %s",
            inRelativePath, errorMsg);
        return 0;
    }
    return theScriptIndex;
}

void SLuaEngineImpl::ExecuteApplicationScriptFunction(Q3DStudio::INT32 inApp, const char *inFnName)
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
    SLuaStackScope __topScope(m_LuaState);
    lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS);
    lua_rawgeti(m_LuaState, -1, inApp);
    int selfIndex = lua_gettop(m_LuaState);
    if (!lua_istable(m_LuaState, -1)) {
        qCCritical(qt3ds::INVALID_OPERATION, "Lua object index appears to be invalid");
        return;
    }
    lua_getfield(m_LuaState, -1, inFnName);
    if (lua_isfunction(m_LuaState, -1)) {
        // IPerfTimer* theTimer = 0;
        // const char* thePerfName = "";
        // if ( m_ApplicationCore )
        //{
        //	lua_getfield( m_LuaState, -2, "__scriptpath__" );
        //	if ( lua_isstring( m_LuaState, -1 ) )
        //	{
        //		m_ScriptString.clear();
        //		m_ScriptString.assign( lua_tostring( m_LuaState, -1 ) );
        //		m_ScriptString.append( " - " );
        //		m_ScriptString.append( inFnName );
        //		thePerfName = m_ScriptString.c_str();
        //		theTimer = &m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
        //	}
        //	lua_pop( m_LuaState, 1 );
        //}
        // SStackPerfTimer __scriptTimer( theTimer, thePerfName );
        // push the self element
        lua_pushvalue(m_LuaState, -2);
        lua_pushvalue(m_LuaState, -1); // dup that
        lua_setglobal(m_LuaState, KEY_SELF); // set the global self element (WAT)

        int pcallError = PCall(1, 0, 0);
        if (pcallError) {
            const char *errorStr = lua_tostring(m_LuaState, -1);
            qCCritical(qt3ds::INVALID_OPERATION,
                "Lua object call %s failed: %s.  disabling further calls",
                inFnName, errorStr);
            lua_pushnil(m_LuaState);
            lua_setfield(m_LuaState, selfIndex, inFnName);
        }
    }
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

// void SLuaEngineImpl::EnableDebugging(uic::state::debugger::IMultiProtocolSocket& socket)
//{
//	LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
//	using namespace uic::state::debugger;
//	QT3DS_ASSERT( m_ApplicationCore );
//	NVScopedRefCounted<uic::state::debugger::IMultiProtocolSocketStream> theStream
//		= socket.CreateProtocol(
//uic::state::debugger::ILuaArchitectDebugServer::LuaProtocolName(), 0 );
//	m_LuaListener = QT3DS_NEW( m_Foundation.getAllocator(), SLuaEngineListener)( m_Foundation,
//*this );
//
//	m_LuaDebugger = uic::state::debugger::ILuaDebugger::CreateLuaSideDebugger(
//		  m_ApplicationCore->GetRuntimeFactoryCore().GetFoundation()
//		, *theStream
//		, m_LuaState
//		, m_ApplicationCore->GetRuntimeFactoryCore().GetStringTable()
//		, m_ApplicationCore->GetProjectDirectory().c_str()
//		, m_LuaListener.mPtr );
//}

// void SLuaEngineImpl::EnableProfiling()
//{
//	LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
//	lua_sethook( m_LuaState, ProfileHook,  LUA_MASKCALL | LUA_MASKRET, 0 );
//}

void SLuaEngineImpl::StepGC()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;

    // IPerfTimer* theTimer = 0;
    // if ( m_ApplicationCore )
    //	theTimer = &m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
    // SStackPerfTimer __timer( theTimer, "CLuaEngine::StepGC" );
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
        return 0;
    }

    lua_pushcfunction(m_LuaState, ParseSCXML);
    eastl::string fullPath;
    qt3ds::foundation::CFileTools::CombineBaseAndRelative(m_Context.GetProjectDir(), inPath, fullPath);
    lua_pushstring(m_LuaState, fullPath.c_str());
    qt3ds::state::IStateInterpreter *theInterpreter = 0;
    int error = PCall(1, 1, 0);
    if (error) {
        qCCritical(qt3ds::INVALID_OPERATION, "Failed to load scxml file: %s",
            lua_tostring(m_LuaState, -1));
    } else {
        lua_insert(m_LuaState, 1);
        lua_settop(m_LuaState, 1);
        int isTable = lua_istable(m_LuaState, 1);
        QT3DS_ASSERT(isTable);
        (void)isTable;
        // shove it in the global table under its path.  We will get to this again in a second
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "qt3ds_state_machines");
        if (lua_isnil(m_LuaState, -1)) {
            lua_pop(m_LuaState, 1);
            lua_newtable(m_LuaState);
            lua_pushvalue(m_LuaState, -1);
            lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "qt3ds_state_machines");
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
                qCCritical (qt3ds::INVALID_OPERATION)
                        << "LuaEngine: Unable to initialize datamodel, "
                        << inDatamodelFunction << " " << error;
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
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "qt3ds_state_machines");
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
}
//==============================================================================
/**
 *	Register the set of global Lua functions
 */
void SLuaEngineImpl::Initialize()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;

    lua_register(m_LuaState, "getElapsedTime", GetElapsedTime);
    lua_register(m_LuaState, "getDirectory", GetDirectory);
    lua_register(m_LuaState, "getStateMachine", GetSCXMLInterpreter);
    lua_register(m_LuaState, "getMillisecondsSinceLastFrame", GetMillisecondsSinceLastFrame);
}

void SLuaEngineImpl::Shutdown(qt3ds::NVFoundationBase &inFoundation)
{
    qCInfo(qt3ds::TRACE_INFO, "Lua engine Begin Exit");
    // EndPreloadScripts();
    {
        qt3ds::foundation::Mutex::ScopedLock __locker(m_PreloadMutex);
        DisableMultithreadedAccess();
    }
    lua_close(m_LuaState);
    m_LuaState = 0;

    g_InputStreamFactory = 0;
    lua_set_file_io(0);

    qCInfo(qt3ds::TRACE_INFO, "Lua engine End Exit");
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

//==============================================================================
/**
 *	Helper function to call the lua functions in the self table
 *	@param inScriptIndex		the script index to get the self table
 *	@param inFunctionName		the lua function to call
 *	@return true if the function is called, false if otherwise
 */
// SLuaFunctionResult SLuaEngineImpl::CallFunction( INT32 inScriptIndex, const CHAR* inFunctionName
// )
//{
//	LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;
//	PerfLogLuaEvent1( DATALOGGER_CALLLUAFUNCTION );
//
//	int theTop = lua_gettop( m_LuaState );
//
//	lua_pushcfunction( m_LuaState, ErrorChannel );						// Top + 1
//= error handler
//
//	lua_rawgeti( m_LuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS );		// Top + 2 =
//BehaviorsT
//
//	lua_rawgeti( m_LuaState, -1, inScriptIndex );						// Top + 3
//= ScriptT
//	BOOL theFunctionFlag = FALSE;
//	BOOL thePCallError = TRUE;
//	if ( lua_istable( m_LuaState, -1 ) )
//	{
//		thePCallError = FALSE;
//		lua_getfield( m_LuaState, -1, inFunctionName );						//
//Top + 4 = OnXXX function, if there's one
//
//	    theFunctionFlag = lua_isfunction( m_LuaState, -1 );
//		if ( theFunctionFlag )
//// if it is a function, call it
//		{
//			const char* thePerfName = "";
//			IPerfTimer* theTimer = 0;
//			if ( m_ApplicationCore )
//			{
//				lua_getfield( m_LuaState, -2, "__scriptpath__" );
//				if ( lua_isstring( m_LuaState, -1 ) )
//				{
//					m_ScriptString.clear();
//					m_ScriptString.assign( lua_tostring( m_LuaState, -1 ) );
//					m_ScriptString.append( " - " );
//					m_ScriptString.append( inFunctionName );
//					thePerfName = m_ScriptString.c_str();
//					theTimer =
//&m_ApplicationCore->GetRuntimeFactoryCore().GetPerfTimer();
//				}
//			}
//			SStackPerfTimer __scriptTimer( theTimer, thePerfName );
//			lua_pop( m_LuaState, 1 );
//			lua_pushlstring( m_LuaState, KEY_SELF, LENGTH_KEY_SELF );		// Top + 5
//= 'self'
//			lua_pushvalue( m_LuaState, -3 );
//// Top + 6 = ScriptT
//			lua_rawset( m_LuaState, LUA_GLOBALSINDEX );
//// [size = Top + 4]
//
//			lua_pushvalue( m_LuaState, -2 );// Top + 5 = ScriptT
//
//			int error = PCall( 1, 0, theTop + 1 );						//
//[size = Top + 3]
//			if ( error )
//				thePCallError = TRUE;
//		}
//	}
//
//	lua_settop( m_LuaState, theTop );
//
//	return SLuaFunctionResult( theFunctionFlag, thePCallError );
//}

void SLuaEngineImpl::PrepareToLoadFile()
{
    LUA_ENGINE_MULTITHREAD_PROTECT_METHOD;

    if (m_LuaPathInitialized == false) {
        m_LuaPathInitialized = true;
        eastl::string projectDir(m_Context.GetProjectDir());
        qt3ds::foundation::CFileTools::ToPlatformPath(projectDir);

        eastl::string subdir = qt3ds::foundation::System::getPlatformStr();
        lua_State *state = m_LuaState != 0 ? m_LuaState : m_PreloadState;
        // this block of code so far is only used by android
        if (!g_AdditionalSharedLibraryPaths.empty()) {
            for (eastl::vector<eastl::string>::iterator iter =
                     g_AdditionalSharedLibraryPaths.begin();
                 iter != g_AdditionalSharedLibraryPaths.end(); iter++) {
                const eastl::string &additionalPathRef = *iter;
                AppendAdditionalLuaPackagePath(state, additionalPathRef);
            }
        }

        // eastl::string scriptsDir;
        // CFileTools::CombineBaseAndRelative( projectDir.c_str(), "scripts", scriptsDir );
        // AppendLuaPackagePath( state, scriptsDir, eLuaPath);
        // AppendLuaPackagePath( state, scriptsDir, eCPath);

        // Add in additional search directory for specific platform
        eastl::string scriptsPlatformDir;
        qt3ds::foundation::CFileTools::CombineBaseAndRelative(".\\scripts", subdir.c_str(),
                                                           scriptsPlatformDir);
        qt3ds::foundation::CFileTools::ToPlatformPath(scriptsPlatformDir);
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

// void SLuaEngineImpl::OnBreak()
//{
//	uic::state::debugger::IDebugger& theDebugger = m_ApplicationCore->GetStateDebugger();
//	theDebugger.OnExternalBreak();
//}

//==============================================================================
/**
 * ************************End of local functions ************************
 */
//=============================================================================

//==============================================================================
/**
 * *******************exported global Lua functions definition********************
 */
//=============================================================================
namespace __SLuaEngineImpl_Static_Global_Lua_Exporter_Declaration__ {
    using namespace Q3DStudio;

    SLuaEngineImpl *GetLuaEngineFromLua(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);
        return theEngine;
    }

    int Output(lua_State *inLuaState)
    {
        Q3DStudio::INT32 theTop = lua_gettop(inLuaState);
        QString theString = "";

        for (Q3DStudio::INT32 theCounter = 1; theCounter <= theTop; ++theCounter) {
            lua_getglobal(inLuaState, "tostring");
            lua_pushvalue(inLuaState, theCounter);
            if (lua_pcall(inLuaState, 1, 1, 0))
                theString.append("output: Error while calling 'tostring'.");
            else {
                const Q3DStudio::CHAR *theResult = lua_tostring(inLuaState, -1);
                if (theResult)
                    theString.append(theResult);
                else
                    theString.append("output: 'tostring' must return a string to 'output'");
            }

            if ((theCounter + 1) <= theTop)
                theString.append("\n");
        }

        qCInfo (qt3ds::TRACE_INFO) << "Lua: " << theString.toLatin1().constData();

        lua_settop(inLuaState, 0);

        return 0;
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
        SLuaEngineImpl *theLuaEngine = GetLuaEngineFromLua(inLuaState);
        if (theLuaEngine) {
            Q3DStudio::INT64 theMicroSeconds =
                theLuaEngine->m_Context.m_TimeProvider.GetCurrentTimeMicroSeconds();
            double theSeconds = (double)theMicroSeconds / 1000.0 / 1000.0;
            lua_pushnumber(inLuaState, (lua_Number)(theSeconds));
            return 1;
        }
        return 0;
    }

    const THashValue HASH_DIRECTORY_PROJECT = CHash::HashString("project");
    const THashValue HASH_DIRECTORY_SCRIPTS = CHash::HashString("scripts");
    const THashValue HASH_DIRECTORY_PLUGINS = CHash::HashString("plugins");
    const THashValue HASH_DIRECTORY_SCRIPTS_PLATFORM = CHash::HashString("scripts-platform");
    const THashValue HASH_DIRECTORY_PLUGINS_PLATFORM = CHash::HashString("plugins-platform");

    int GetDirectory(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);
        if (theEngine) {
            eastl::string sub;
            int argc = lua_gettop(inLuaState);
            const Q3DStudio::CHAR *directoryName =
                argc > 0 ? luaL_checkstring(inLuaState, 1) : "project";
            THashValue directoryHash = CHash::HashString(directoryName);
            if (directoryHash == HASH_DIRECTORY_PROJECT)
                sub.clear();
            else if (directoryHash == HASH_DIRECTORY_SCRIPTS)
                sub.assign("scripts");
            else if (directoryHash == HASH_DIRECTORY_PLUGINS)
                sub.assign("plugins");
            else if (directoryHash == HASH_DIRECTORY_SCRIPTS_PLATFORM)
                sub.assign("scripts/").append(qt3ds::foundation::System::getPlatformStr());
            else {
                qCCritical (qt3ds::TRACE_INFO) << "getDirectory wasn't passed an recognized string";
                lua_pushnil(inLuaState);
                return 1;
            }
            eastl::string dir(theEngine->m_Context.GetProjectDir());
            if (!sub.empty())
                qt3ds::foundation::CFileTools::CombineBaseAndRelative(dir.c_str(), sub.c_str(), dir);
            CFileTools::ToPlatformPath(dir);
            lua_pushstring(inLuaState, dir.c_str());
        } else
            lua_pushnil(inLuaState);
        return 1;
    }

    int ParseSCXML(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));

        qt3ds::state::debugger::IDebugger *theDebugger = 0;
        // if ( theEngine )
        //{
        //	theDebugger = &theEngine->m_Application->GetStateDebugger();
        //}
        return qt3ds::state::ILuaScriptContext::ParseSCXML(inLuaState, theDebugger,
                                                         *theEngine->m_Context.m_Foundation,
                                                         theEngine->m_Context.GetStringTable());
    }

    int GetSCXMLInterpreter(lua_State *inLuaState)
    {
        const char *path = lua_tostring(inLuaState, -1);
        if (!path) {
            lua_pushnil(inLuaState);
            return 1;
        }
        // shove it in the global table under its path.  We will get to this again in a second
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_state_machines");
        if (lua_isnil(inLuaState, -1))
            return 1;

        lua_getfield(inLuaState, -1, path);
        return 1;
    }

    int GetEventSystem(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_event_system");
        if (!lua_istable(inLuaState, -1)) {
            lua_pop(inLuaState, 1);
            lua_getfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_lua_engine");
            SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
            lua_pop(inLuaState, 1);
            qt3ds::evt::IEventSystem &theEventSystem =
                theEngine->m_Context.m_Factory->GetEventSystem();
            qt3ds::evt::SLuaEventPollerBinding::WrapEventPoller(inLuaState, theEventSystem);
            lua_pushvalue(inLuaState, -1);
            lua_setfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_event_system");
        }
        return 1;
    }

    int GetMillisecondsSinceLastFrame(lua_State *inLuaState)
    {
        lua_getfield(inLuaState, LUA_REGISTRYINDEX, "qt3ds_lua_engine");
        SLuaEngineImpl *theEngine = (SLuaEngineImpl *)(lua_touserdata(inLuaState, -1));
        lua_pop(inLuaState, 1);
        if (theEngine) {
            lua_pushnumber(inLuaState, theEngine->m_Application->GetMillisecondsSinceLastFrame());
            return 1;
        }
        return 0;
    }
}
//==============================================================================
/**
 * *******************end of exported global Lua functions definitions************
 */
//=============================================================================
}

//==============================================================================
/**
 * *******************static functions from INDDStateLuaEngine ************************
 */
//=============================================================================
//==============================================================================
namespace qt3ds {
namespace state {
    INDDStateLuaEngine *INDDStateLuaEngine::Create(SNDDStateContext &inContext)
    {
        return QT3DS_NEW(inContext.GetAllocator(), SLuaEngineImpl)(inContext);
    }
}
}
//==============================================================================
/**
 * *******************end of static functions from CLuaEngine ******************
 */
//=============================================================================
