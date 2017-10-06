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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_DROP_PROXY
#define INCLUDED_DROP_PROXY 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include <QObject>

//==============================================================================
//	Forwards
//==============================================================================

class CDropContainer;
class CStudioApp;
class CDropSource;

class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;

class CDropProxy : public QObject
{
    Q_OBJECT
public:
    CDropProxy(CDropContainer *inParent);
    virtual ~CDropProxy();

    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

    CDropSource *GetDropSource(const QMimeData *inDataObject, long inFlavor, long inItem);
    long GetDragItemCount(const QMimeData *inDataObject);
    bool HasMainFlavor(const QMimeData *inDataObject, long inFlavor);
    Qt::KeyboardModifiers ReflectMouse(long inX, long inY);

    void Register(QWidget* widget);

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    CDropContainer *m_Parent;
};

#endif // INCLUDED_DROP_PROXY
