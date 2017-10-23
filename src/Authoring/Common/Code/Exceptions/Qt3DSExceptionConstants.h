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

//==============================================================================
//	Prefix
//==============================================================================

#ifndef __QT3DS_EXCEPTION_CONSTANTS_H__
#define __QT3DS_EXCEPTION_CONSTANTS_H__

const unsigned long ERROR_TYPE_PLAYER = 0x00000001;
const unsigned long ERROR_TYPE_LIBRARY = 0x00000002;
const unsigned long ERROR_TYPE_CONTROLLER = 0x00000004;
const unsigned long ERROR_TYPE_ICELIB = 0x00000008;
const unsigned long ERROR_TYPE_IOLIBRARY = 0x00000010;
const unsigned long ERROR_TYPE_OBJECT_MODEL = 0x00000020;
const unsigned long ERROR_TYPE_GLOBAL_INTERFACE_TABLE = 0x00000040;
const unsigned long ERROR_TYPE_COMPONENT = 0x00000080;
const unsigned long ERROR_TYPE_RENDER = 0x00000100;
const unsigned long ERROR_TYPE_SCRIPT_ENGINE = 0x00000200;
const unsigned long ERROR_TYPE_STREAM = 0x00000400;
const unsigned long ERROR_TYPE_TYPES = 0x00000800;
const unsigned long ERROR_TYPE_UTILITY = 0x00001000;
const unsigned long ERROR_TYPE_TRANSM = 0x00002000;
const unsigned long ERROR_TYPE_INSTALLER = 0x00004000;

#endif // #ifndef __QT3DS_EXCEPTION_CONSTANTS_H__
