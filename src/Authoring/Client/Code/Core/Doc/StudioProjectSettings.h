/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

#ifndef INCLUDED_STUDIO_PROJECT_SETTINGS_H
#define INCLUDED_STUDIO_PROJECT_SETTINGS_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "Pt.h"
#include "CColor.h"

//==============================================================================
//	Forwards
//==============================================================================
class CCore;
class CStudioProjectVariables;

class CStudioProjectSettings
{

    // Construction
public:
    CStudioProjectSettings(CCore *inCore = NULL);
    ~CStudioProjectSettings();

    QString GetAuthor() { return m_Author; }
    void SetAuthor(const QString &inAuthor);

    QString GetCompany() { return m_Company; }
    void SetCompany(const QString &inCompany);

    CPt GetPresentationSize() { return m_PresentationSize; }
    void SetPresentationSize(CPt inSize);

    long GetFSAAMode() const { return m_FSAAMode; }
    void SetFSAAMode(long inFSAAMode);

    bool GetMaintainAspect();
    void SetMaintainAspect(bool inFlag);

    bool GetEmbedFonts();
    void SetEmbedFonts(bool inFlag);

    bool GetRotatePresentation();
    void SetRotatePresentation(bool inFlag);

    void Reset();
    void RestoreDefaults();

    // Implementation
protected:
    QString m_Author;
    QString m_Company;

    // TODO : remove m_EmbedFonts, m_FSAAMode
    CPt m_PresentationSize;
    long m_FSAAMode; ///< Fullscreen anti-aliasing mode
    bool m_MaintainAspect;
    bool m_RotatePresentation;
    CCore *m_Core;

    bool m_EmbedFonts; ///< Whether to embed fonts in am files
};

#endif // INCLUDED_STUDIO_PROJECT_SETTINGS_H
