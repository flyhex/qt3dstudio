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

#ifndef _DATA_LOGGER_ENUMS_H_
#define _DATA_LOGGER_ENUMS_H_

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

// TODO ah - for now, these are all specific to paired events.  Later we will
// extend this to support single events which may or may not have timestamps
// for example, every matrix multiple would log a single event with no time
enum EDataLoggerEvents {
    //
    DATALOGGER_FRAME, /// Time spent in a application frame ( UpdatePresentations, UpdateScenes, and
                      /// Render )

    // DATALOGGER_FRAME
    // CRuntime
    DATALOGGER_UPDATEPRESENTATIONS, /// Time spent updating all presentations
    DATALOGGER_UPDATESCENES, /// Time spent updating all scenes
    DATALOGGER_RENDERSCENES, /// Time spent rendering all scenes

    // DATALOGGER_UPDATEPRESENTATIONS
    // CPresentation::Update
    DATALOGGER_PROCESSEVENTS, /// Time spent in ProcessEventCommandQueue
    DATALOGGER_ELEMENTMANAGERUPDATE, /// Time spent in CElementManager::Update doing activate and
                                     /// deactivate scans
    DATALOGGER_ANIMATIONMANAGERUPDATE, /// Time spent in CAnimationManager::Update
    DATALOGGER_PROCESSFRAMECALLBACKS, /// Time spent in CLuaEngine::ProcessFrameCallbacks

    // More detailed presentation update
    DATALOGGER_PROCESSCOMMAND, /// xxx
    DATALOGGER_PROCESSEVENT, /// xxx
    DATALOGGER_PROCESSEVENTBUBBLING, /// xxx

    // CLuaEngine
    DATALOGGER_SLIDECALLBACKHELPER, /// xxx
    DATALOGGER_FRAMECALLBACKSHELPER, /// xxx
    DATALOGGER_CALLLUAFUNCTION, /// xxx
    DATALOGGER_CALLLUAFUNCTIONONUPDATE, /// xxx

    // DATALOGGER_UPDATESCENES
    // CScene::Update
    DATALOGGER_PROCESSDIRTYLIST, /// Time spent in CScene::ProcessDirtyList
    DATALOGGER_CALCULATETRANSFORM, /// Time spent in CScene::CalculateGlobalTransform
    DATALOGGER_SCENEUPDATE, /// Time spent in CScene::Update recursion

    // More detailed scene update
    DATALOGGER_SCANNODEATTRIBUTES, /// Time
    DATALOGGER_PUSHNODE, /// Time

    // DATALOGGER_RENDERSCENES
    // CRenderEngine
    DATALOGGER_RENDERLAYER, /// Time spent rendering the contents of a layer
    DATALOGGER_RENDEROPAQUE, /// Time spent rendering all opaque items
    DATALOGGER_RENDERTRANSPARENT, /// Time spent rendering all transparent items
    DATALOGGER_FINALIZEDRAWLIST, /// Time spent finalizing the draw list
    DATALOGGER_PICK, /// Time spent in pick code

    // More detailed rendering
    DATALOGGER_RENDER_SORTDISTANCE, /// asdf
    DATALOGGER_RENDER_SETOPACITY, /// asdf
    DATALOGGER_RENDER_DRAWMODEL, /// asdf
    DATALOGGER_RENDER_DRAWTEXT, /// asdf
    DATALOGGER_RENDER_CLEARBUFFER, /// asdf
    DATALOGGER_RENDER_ZWRITEENABLE, /// asdf
    DATALOGGER_RENDER_CHECKRESIZE, /// asdf
    DATALOGGER_RENDER_SETATTRIBUTES, /// asdf
    DATALOGGER_RENDER_TEXTUPDATE, /// asdf
    DATALOGGER_RENDER_CAMERAUPDATE, /// asdf

    // super specific low level gl calls
    DATALOGGER_TEXTURESTATE_APPLY, /// asdf
    DATALOGGER_GEOMETRY_DRAW, /// asdf
    DATALOGGER_MATERIAL_APPLY, /// asdf
    DATALOGGER_SETSHADER, /// asdf

    // Lua vector math
    DATALOGGER_VECTOR, /// asdf
    DATALOGGER_MATRIX, /// asdf
    DATALOGGER_CUBICROOT, /// asdf

    // Misc
    DATALOGGER_LUA_CALCULATEGLOBALTRANSFORM, /// asdf
    DATALOGGER_LUA_SETLOCALTRANSFORMMATRIX, /// asdf
    DATALOGGER_LUA_CLONE, /// asdf
    DATALOGGER_LUA_EVENTREGISTRATION, /// asdf
    DATALOGGER_LUA_CHANGEREGISTRATION, /// asdf
    DATALOGGER_LUA_EVENTCALLBACK, /// asdf
    DATALOGGER_LUA_CHANGECALLBACK, /// asdf
    DATALOGGER_LUA_FINDCALLBACKDATA, /// asdf
    DATALOGGER_LUA_SETATTRIBUTE, /// asdf
    DATALOGGER_LUA_GETATTRIBUTE, /// asdf
    DATALOGGER_LUA_HASATTRIBUTE, /// asdf
    DATALOGGER_LUA_GETELEMENT, /// asdf

    DATALOGGER_LUA_GETCURRENTSLIDE, /// asdf
    DATALOGGER_LUA_GETTIME, /// asdf

    DATALOGGER_LUA_FIRECOMMAND, /// Play, pause, goto time etc
    DATALOGGER_LUA_GOTOTIME, /// Play
    DATALOGGER_LUA_GOTOSLIDE, /// Play

    // Lua vector math
    DATALOGGER_LUA_VECTOR, /// asdf
    DATALOGGER_LUA_MATRIX, /// asdf

    // Data binding stuff
    DATALOGGER_LUA_GETRECORD, /// asdf
    DATALOGGER_LUA_GETRECORDS, /// asdf
    DATALOGGER_LUA_GETINTERFACE, /// asdf
    DATALOGGER_LUA_GETRECORDINTERFACE, /// asdf
    DATALOGGER_LUA_GETFIELDS, /// asdf
    DATALOGGER_LUA_GETRECORDFIELD, /// asdf

    DATALOGGER_LUA_SCRIPTA, /// asdf
    DATALOGGER_LUA_SCRIPTB, /// asdf
    DATALOGGER_LUA_SCRIPTC, /// asdf
    DATALOGGER_LUA_SCRIPTD, /// asdf
    DATALOGGER_LUA_SCRIPTE, /// asdf
    DATALOGGER_LUA_SCRIPTF, /// asdf
    DATALOGGER_LUA_SCRIPTG, /// asdf
    DATALOGGER_LUA_SCRIPTH, /// asdf
    DATALOGGER_LUA_SCRIPTI, /// asdf
    DATALOGGER_LUA_SCRIPTJ, /// asdf
    //
    DATALOGGER_COUNTERTEST, /// Time spent in counter test code
    DATALOGGER_COUNTERTESTX, /// Time spent in counter test code

    DATALOGGEREVENTCOUNT /// Event count
};

// CPerfLogPairedEventWrapperx thePerfLog( xxx );

} // namespace Q3DStudio

#endif // _DATA_LOGGER_ENUMS_H_
