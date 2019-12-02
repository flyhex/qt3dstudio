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

#include <QtCore/qstring.h>
#include <QtCore/qsize.h>

class CCore;

class CStudioProjectSettings
{
public:
    CStudioProjectSettings(CCore *inCore = nullptr);
    ~CStudioProjectSettings();

    QString getAuthor() const { return m_author; }
    void setAuthor(const QString &inAuthor);

    QString getCompany() const { return m_company; }
    void setCompany(const QString &inCompany);

    QSize getPresentationSize() const { return m_presentationSize; }
    void setPresentationSize(const QSize &inSize);

    bool getMaintainAspect() const { return m_maintainAspect; }
    void setMaintainAspect(bool inFlag);

    bool getRotatePresentation() const { return m_rotatePresentation; }
    void setRotatePresentation(bool inFlag);

    bool getPreferCompressedTextures() const { return m_preferCompressedTextures; }
    void setPreferCompressedTextures(bool inFlag);

    bool getFlipCompressedTextures() const { return m_flipCompressedTextures; }
    void setFlipCompressedTextures(bool inFlag);

    void reset();

protected:
    QString m_author;
    QString m_company;

    QSize m_presentationSize;
    bool m_maintainAspect;
    bool m_rotatePresentation;
    bool m_preferCompressedTextures;
    bool m_flipCompressedTextures;
    CCore *m_core;
};

#endif // INCLUDED_STUDIO_PROJECT_SETTINGS_H
