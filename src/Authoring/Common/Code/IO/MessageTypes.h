/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

#ifndef _MESSAGE_TYPES_H
#define _MESSAGE_TYPES_H 1

/** All of these need to be changed to defined numbers to avoid future versions changing
 * the types and creating incompatibilities.
 */

/** The request types in StudioMessages.
 */
class CElementTypes
{
public:
    enum EElementTypes {
        SEND_FILE = 1,
        CHUNKED_SEND_FILE = 2,
        LAME_SEND_FILE = 3,
        CACHED_SEND_FILE = 4,
        LAME_CACHED_SEND_FILE = 5,
        STATISTICS = 6,
        FILE_STATISTICS_LIST = 7,
        FILE_STATISTICS = 8,
    };
};

/** The attributes types for the elements in StudioMessages.
 */
class CAttributeTypes
{
public:
    enum EAttributeTypes {
        FILENAME = 1,
        ERROR_MESSAGE = 2,
        FILE_START_LOCATION = 3,
        FILE_END_LOCATION = 4,
        CHUNK_SIZE = 5,
        PRIORITY = 6,
        URGENCY = 7,

        // Statistics Attributes
        CONNECTED_COUNT = 1000,
        BITRATE = 1001,
        STREAM_COUNT = 1002,
        TOTAL_REQUEST_COUNT = 1003,

    };
};

/** The urgencies of the incoming messages.
 */
class CMessageUrgencies
{
public:
    enum EMessageUrgencies {
        URGENT = 1,
        NORMAL = 0,
    };
};

class CMessagePriorities
{
public:
    enum EMessagePriorities {
        LOW_PRIORITY = 0,
        MED_PRIORITY = 1,
        HIGH_PRIORITY = 2,
        ULTRA_PRIORITY = 3,
    };
};
#endif
