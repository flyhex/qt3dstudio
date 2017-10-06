/****************************************************************************
**
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

#ifndef Q3DSPRESENTATION_P_H
#define Q3DSPRESENTATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtStudio3D API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "q3dspresentation.h"
#include "UICViewerApp.h"
#include <QtCore/QHash>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

class CommandQueue;
class ViewerQmlStreamProxy;
class QKeyEvent;

class Q_STUDIO3D_EXPORT Q3DSPresentationPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Q3DSPresentation)

public:
    typedef QHash<QString, Q3DSElement *> ElementMap;

    explicit Q3DSPresentationPrivate(Q3DSPresentation *parent);
    ~Q3DSPresentationPrivate();

    void setSource(const QUrl &source);
    void setViewerApp(UICViewer::UICViewerApp *app, bool connectApp = true);
    void setCommandQueue(CommandQueue *queue);

    void registerElement(Q3DSElement *element);
    void unregisterElement(Q3DSElement *element);
    void unregisterAllElements();

    ViewerQmlStreamProxy *streamProxy();
    Q3DStudio::EKeyCode getScanCode(QKeyEvent *e);

public Q_SLOTS:
    void handleSlideEntered(const QString &elementPath, unsigned int index, const QString &name);

public:
    Q3DSPresentation *q_ptr;

private:
    UICViewer::UICViewerApp *m_viewerApp; // Not owned
    CommandQueue *m_commandQueue; // Not owned
    ElementMap m_elements;
    QUrl m_source;
    ViewerQmlStreamProxy *m_streamProxy;
};

QT_END_NAMESPACE

#endif // Q3DSPRESENTATION_P_H
