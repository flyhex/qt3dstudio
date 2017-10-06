/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef UIC_STATE_SHARED_IMPL_H
#define UIC_STATE_SHARED_IMPL_H
#include "UICState.h"
#include "foundation/Utils.h"

namespace uic {
namespace state {
    namespace impl {

        inline bool NameMatchesInternal(const char8_t *inTransEvent, QT3DSU32 transLen,
                                        const char8_t *inEventName, QT3DSU32 eventLen)
        {
            QT3DSU32 idx, end;
            // Empty loop intentional to find first nonmatching character
            for (idx = 0, end = NVMin(transLen, eventLen);
                 idx < end && inTransEvent[idx] == inEventName[idx]; ++idx) {
            }
            // Note that in this case we point to the first nonmatching character which may be off
            // the end of either
            // eventStr or transStr
            bool match = false;
            if (idx == transLen) {
                if (idx == eventLen)
                    match = true;
                else if (inEventName[idx] == '.')
                    match = true;
                else if (inTransEvent[idx - 1] == '.')
                    match = true;
            } else if (idx == eventLen) {
                if ((transLen - idx) == 1)
                    match = inTransEvent[idx] == '*' || inTransEvent[idx] == '.';

                else if ((transLen - idx) == 2)
                    match = inTransEvent[idx] == '.' && inTransEvent[idx + 1] == '*';
            } else {
                if (inTransEvent[idx] == '*')
                    match = true;
            }
            return match;
        }

        inline const char8_t *FindNextNonSpace(const char8_t *inPtr)
        {
            for (; *inPtr == ' '; ++inPtr) {
            }
            return inPtr;
        }

        inline const char8_t *FindNextSpaceOrNull(const char8_t *inPtr)
        {
            for (; *inPtr && *inPtr != ' '; ++inPtr) {
            }
            return inPtr;
        }

        inline bool NameMatches(const char8_t *inTransEvent, const char8_t *inEventName)
        {
            inTransEvent = nonNull(inTransEvent);
            inEventName = nonNull(inEventName);

            QT3DSU32 transLen = StrLen(inTransEvent);

            QT3DSU32 eventLen = StrLen(inEventName);
            if (transLen == 0) {
                QT3DS_ASSERT(false);
                return false;
            }
            if (eventLen == 0) {
                QT3DS_ASSERT(false);
                return false;
            }

            for (inTransEvent = FindNextNonSpace(inTransEvent); inTransEvent && *inTransEvent;
                 inTransEvent = FindNextNonSpace(inTransEvent)) {
                const char8_t *end = FindNextSpaceOrNull(inTransEvent);
                QT3DSU32 len = (QT3DSU32)(end - inTransEvent);

                if (len && NameMatchesInternal(inTransEvent, len, inEventName, eventLen))
                    return true;
                inTransEvent = end;
            }

            return false;
        }
    }
}
}
#endif