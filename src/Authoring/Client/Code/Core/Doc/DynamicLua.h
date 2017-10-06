/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "foundation/Qt3DSSimpleTypes.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSOption.h"
namespace qt3ds {
class NVFoundationBase;
}

namespace Q3DStudio {

class IDynamicLua
{
protected:
    virtual ~IDynamicLua() {}
public:
    struct Scope
    {
        IDynamicLua &m_Lua;
        int m_Top;
        Scope(IDynamicLua &inLua)
            : m_Lua(inLua)
            , m_Top(inLua.GetTop())
        {
        }
        ~Scope() { m_Lua.SetTop(m_Top); }
    };
    enum PathType {
        LuaModule = 0,
        CModule,
    };

    virtual void AddLibraryPath(const char8_t *inPath, PathType inType) = 0;
    // Load a file, storing result into global var named 'name'
    virtual bool LoadLuaFile(const char8_t *inFname, const char8_t *inGlobalVarName) = 0;
    virtual int GetTop() = 0;
    virtual void SetTop(int inTop) = 0;
    // Assuming a table is top of stack.
    virtual qt3ds::foundation::Option<qt3ds::QT3DSF32>
    NumberFromTopOfStackTable(const char8_t *inKeyName) = 0;
    virtual qt3ds::foundation::Option<qt3ds::QT3DSVec3>
    Vec3FromTopOfStackTable(const char8_t *inKeyName) = 0;
    virtual qt3ds::foundation::Option<bool> BooleanFromTopOfStackTable(const char8_t *inKeyName) = 0;
    virtual eastl::string StringFromTopOfStackTable(const char8_t *inKeyName) = 0;
    // inIndex is 1 based, not zero based.  Uses rawgeti so don't expect metadata to work.
    // Returns true if the child is nonnull.  If it returns false, it pops the null off the stack
    // leaving the table.
    virtual bool GetChildFromTopOfStackTable(qt3ds::QT3DSI32 inIndex) = 0;
    virtual int GetNumChildrenFromTopOfStackTable() = 0;
    // Execute a function assumed to be found from a global table.
    // Expects one result and leaves it on the stack.
    virtual bool ExecuteFunction(const char8_t *inGlobalVarName, const char8_t *fnName,
                                 const char8_t *fnArg) = 0;

    // Assumes the arguments are TOS already
    virtual bool ExecuteFunction(const char8_t *inGlobalVarName, const char8_t *fnName, int numArgs,
                                 int numResults) = 0;
    // Begin table iteration, assuming table is TOS
    virtual bool BeginTableIteration() = 0;
    // pop the value off the stack, and call lua_next again
    virtual bool NextTableIteration(int numPops) = 0;
    // This is specifically to handle parsing an array of floating point pairs
    //{ {x,y}, {x1,y1}, {x2,y2} }
    virtual void ParseFloatingPointPairArray(eastl::vector<qt3ds::QT3DSVec2> &outFloats) = 0;
    virtual void Release() = 0;

    static IDynamicLua *CreateDynamicLua(qt3ds::NVFoundationBase &fnd, const char8_t *dllPath);
};
}