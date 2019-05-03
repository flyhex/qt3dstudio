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
#include "q3dscommandqueue_p.h"
#include "Qt3DSViewerApp.h"
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
    typedef QHash<QString, Q3DSDataInput *> DataInputMap;

    explicit Q3DSPresentationPrivate(Q3DSPresentation *parent);
    ~Q3DSPresentationPrivate();

    void setSource(const QUrl &source);
    void setVariantList(const QStringList &variantList);
    void setViewerApp(Q3DSViewer::Q3DSViewerApp *app, bool connectApp = true);
    void setCommandQueue(CommandQueue *queue);
    void setDelayedLoading(bool enable);

    void registerElement(Q3DSElement *element);
    void unregisterElement(Q3DSElement *element);
    void unregisterAllElements();

    void registerDataInput(Q3DSDataInput *dataInput);
    void unregisterDataInput(Q3DSDataInput *dataInput);
    void unregisterAllDataInputs();

    bool isValidDataInput(const Q3DSDataInput *dataInput) const;
    float dataInputMin(const QString &name) const;
    float dataInputMax(const QString &name) const;

    ViewerQmlStreamProxy *streamProxy();
    Q3DStudio::EKeyCode getScanCode(QKeyEvent *e);

    void requestResponseHandler(CommandType commandType, void *requestData);

public Q_SLOTS:
    void handleSlideEntered(const QString &elementPath, unsigned int index, const QString &name);

public:
    Q3DSPresentation *q_ptr;

private:
    Q3DSViewer::Q3DSViewerApp *m_viewerApp; // Not owned
    CommandQueue *m_commandQueue; // Not owned
    ElementMap m_elements;
    DataInputMap m_dataInputs;
    QUrl m_source;
    QStringList m_variantList;
    ViewerQmlStreamProxy *m_streamProxy;
    bool m_delayedLoading;
};

QT_END_NAMESPACE

#endif // Q3DSPRESENTATION_P_H
