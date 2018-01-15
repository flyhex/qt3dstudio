/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef Q3DSDATAINPUT_P_P_H
#define Q3DSDATAINPUT_P_P_H

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

#include "q3dsdatainput.h"
#include "q3dscommandqueue_p.h"
#include "Qt3DSViewerApp.h"

QT_BEGIN_NAMESPACE

class Q3DSPresentationPrivate;

class Q_STUDIO3D_EXPORT Q3DSDataInputPrivate
{
    Q_DECLARE_PUBLIC(Q3DSDataInput)
public:
    explicit Q3DSDataInputPrivate(Q3DSDataInput *parent);
    virtual ~Q3DSDataInputPrivate();

    void setViewerApp(Q3DSViewer::Q3DSViewerApp *app);
    void setCommandQueue(CommandQueue *queue);
    void setPresentation(Q3DSPresentationPrivate *presentation);

protected:
    Q3DSDataInput *q_ptr;
    Q3DSViewer::Q3DSViewerApp *m_viewerApp = nullptr; // Not owned
    CommandQueue *m_commandQueue = nullptr; // Not owned
    Q3DSPresentationPrivate *m_presentation = nullptr; // Not owned
    QString m_name;
    QVariant m_value;
};

QT_END_NAMESPACE

#endif // Q3DSDATAINPUT_P_P_H
