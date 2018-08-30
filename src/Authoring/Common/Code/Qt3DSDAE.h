/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QT3DSDAE_H
#define QT3DSDAE_H

// Disable warnings from 3rd party code
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wheader-guard"
#pragma clang diagnostic ignored "-Wnull-conversion"
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif // __clang__

#include "dae.h"
#include "dom/domCOLLADA.h"
#include "dom/domConstants.h"
#include "dom/domCommon_float_or_param_type.h"
#include "dom/domCommon_transparent_type.h"
#include "dom/domElements.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domCommon_color_or_texture_type.h"

// Re-enable warnings
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

#endif // QT3DSDAE_H
