/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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

#ifndef INCLUDED_IBREADCRUMBPROVIDER_H
#define INCLUDED_IBREADCRUMBPROVIDER_H 1

#pragma once

#include <QColor>
#include <QString>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QPixmap)

struct SBreadCrumb
{
    QColor m_Color; /// Color for text of the bread crumb
    QString m_String; /// Text to be displayed for the bread crumb
};

//=============================================================================
/**
 * A interface class for the breadcrumb control, to walk down the breadcrumb trail, without having
 * to know any underlying implementations.
 */
class IBreadCrumbProvider : public QObject
{
    Q_OBJECT
public:
    typedef std::vector<SBreadCrumb> TTrailList;

public:
    virtual ~IBreadCrumbProvider() {}

    virtual TTrailList GetTrail(bool inRefresh = true) = 0;
    virtual void OnBreadCrumbClicked(long inTrailIndex) = 0;

    virtual QPixmap GetRootImage() const = 0;
    virtual QPixmap GetBreadCrumbImage() const = 0;
    virtual QPixmap GetSeparatorImage() const = 0;
    virtual QPixmap GetActiveBreadCrumbImage() const = 0;
Q_SIGNALS:
    void SigBreadCrumbUpdate();
};

#endif // INCLUDED_IBREADCRUMBPROVIDER_H
