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

//==============================================================================
//	Includes
//==============================================================================
#include "RuntimePrefix.h"
#include "Qt3DSInputFrame.h"
#include "Qt3DSLuaButton.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEngine.h"

//==============================================================================
//	Namespace
//==============================================================================
using namespace Q3DStudio;

//==============================================================================
//	Statics
//==============================================================================

static const char *s_ButtonCodeIdentifier[] = {
    "BUTTON_A",      "BUTTON_B",      "BUTTON_X",      "BUTTON_Y",      "BUTTON_L1",   "BUTTON_R1",
    "BUTTON_THUMBL", "BUTTON_THUMBR", "BUTTON_SELECT", "BUTTON_START",  "BUTTON_MODE", "BUTTON_UP",
    "BUTTON_DOWN",   "BUTTON_LEFT",   "BUTTON_RIGHT",  "BUTTON_CENTER", "BUTTON_ENTER"
};

//==============================================================================
/**
 *	Registers global button codes
 */
void CLuaButton::RegisterLibrary(lua_State *inLuaState)
{
    for (long theCount = 0; theCount < BUTTON_TOTAL_COUNT; ++theCount) {
        lua_pushinteger(inLuaState, theCount);
        lua_setglobal(inLuaState, s_ButtonCodeIdentifier[theCount]);
    }
}
