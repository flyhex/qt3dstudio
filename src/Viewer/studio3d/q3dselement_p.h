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

#ifndef Q3DSSCENE_P_H
#define Q3DSSCENE_P_H

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

#include "q3dselement.h"
#include "q3dscommandqueue_p.h"
#include "UICViewerApp.h"

QT_BEGIN_NAMESPACE

class Q3DSPresentationPrivate;

class Q_STUDIO3D_EXPORT Q3DSElementPrivate
{
    Q_DECLARE_PUBLIC(Q3DSElement)
public:
    explicit Q3DSElementPrivate(Q3DSElement *parent);
    virtual ~Q3DSElementPrivate();

    void setElementPath(const QString &elementPath);

    virtual void setViewerApp(UICViewer::UICViewerApp *app);
    virtual void setCommandQueue(CommandQueue *queue);
    virtual void setPresentation(Q3DSPresentationPrivate *presentation);

    virtual void requestResponseHandler(CommandType commandType, void *requestData);

protected:
    Q3DSElement *q_ptr;
    UICViewer::UICViewerApp *m_viewerApp; // Not owned
    CommandQueue *m_commandQueue; // Not owned
    Q3DSPresentationPrivate *m_presentation; // Not owned
    QString m_elementPath;
};

QT_END_NAMESPACE

#endif // Q3DSSCENE_P_H
