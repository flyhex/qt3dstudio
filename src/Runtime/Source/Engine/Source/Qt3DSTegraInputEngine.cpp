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

#include "EnginePrefix.h"
#include "Qt3DSTegraInputEngine.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 * XXX
 */
CTegraInputEngine::CTegraInputEngine()
{
}

//==============================================================================
/**
 * XXX
 */
CTegraInputEngine::~CTegraInputEngine()
{
}

//==============================================================================
/**
 * Returns the structure that contains the input information for the current frame.
 */
SInputFrame &CTegraInputEngine::GetInputFrame()
{
    return m_InputFrame;
}

//==============================================================================
/**
 * Handles the input message.
 */
void CTegraInputEngine::HandleMessage(const KDEvent *inEvent,
                                      ITegraApplicationRenderEngine &inRenderEngine,
                                      CPresentation *inPresentation)
{
    static KDboolean s_PointerWasDown = KD_FALSE;

    if (NULL == inPresentation)
        return;

    switch (inEvent->type) {
    // we still want to preserve the mouse events support, hence process this event as usual.
    case KD_EVENT_INPUT_POINTER: {
        const KDEventInputPointer *ptr = &(inEvent->data.inputpointer);

        KDfloat32 x = static_cast<KDfloat32>(ptr->x);
        KDfloat32 y = static_cast<KDfloat32>(ptr->y);

        if (inRenderEngine.IsPickValid(x, y, *inPresentation)) {
            // printf( "INPUT x %ld y %ld\n", (int)x, (int)y );
            SetPickInput(x, y, (ptr->select || s_PointerWasDown) ? true : false);

            if (ptr->select) {
                if (s_PointerWasDown)
                    SetPickFlags(LMOUSE_DOWN);
                else
                    SetPickFlags(LMOUSE_PRESSED);

                s_PointerWasDown = KD_TRUE;
            } else {
                if (s_PointerWasDown)
                    SetPickFlags(LMOUSE_RELEASED);
                else
                    SetPickFlags(LMOUSE_UP);

                s_PointerWasDown = KD_FALSE;
            }
        } else {
            if (s_PointerWasDown)
                SetPickFlags(LMOUSE_RELEASED);
            else
                SetPickFlags(LMOUSE_UP);

            s_PointerWasDown = KD_FALSE;
        }
    } break;
#if !defined(Q_OS_MACOS)
    case KD_EVENT_WINDOW_FOCUS:
        /* On loss of focus, we simulate a pointer-up event */
        if (!inEvent->data.windowfocus.focusstate) {
            /*if ( s_PointerWasDown )
                SetPickFlags( LMOUSE_RELEASED );
            else
                SetPickFlags( LMOUSE_UP );*/
            s_PointerWasDown = KD_FALSE;
        }
        break;
#endif
    }
}

} // namespace Q3DStudio
