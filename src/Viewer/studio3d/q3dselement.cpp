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

#include "q3dselement_p.h"
#include "q3dspresentation_p.h"
#include "q3dscommandqueue_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

Q3DSElement::Q3DSElement(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSElementPrivate(this))
{
}

Q3DSElement::Q3DSElement(const QString &elementPath, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSElementPrivate(this))
{
    d_ptr->m_elementPath = elementPath;
}

Q3DSElement::Q3DSElement(Q3DSPresentation *presentation, const QString &elementPath,
                         QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSElementPrivate(this))
{
    d_ptr->m_elementPath = elementPath;
    if (presentation)
        presentation->registerElement(this);
}

Q3DSElement::Q3DSElement(Q3DSElementPrivate *d, Q3DSPresentation *presentation,
                         const QString &elementPath, QObject *parent)
    : QObject(parent)
    , d_ptr(d)
{
    d_ptr->m_elementPath = elementPath;
    if (presentation)
        presentation->registerElement(this);
}

Q3DSElement::~Q3DSElement()
{
    // Private class isn't QObject, so we need to delete it explicitly
    delete d_ptr;
}

QString Q3DSElement::elementPath() const
{
    return d_ptr->m_elementPath;
}

void Q3DSElement::setElementPath(const QString &elementPath)
{
    if (d_ptr->m_elementPath != elementPath) {
        d_ptr->setElementPath(elementPath);
        Q_EMIT elementPathChanged(d_ptr->m_elementPath);
    }
}

void Q3DSElement::setAttribute(const QString &attributeName, const QVariant &value)
{
    if (d_ptr->m_presentation)
        d_ptr->m_presentation->q_ptr->setAttribute(d_ptr->m_elementPath, attributeName, value);
    else
        qWarning() << __FUNCTION__ << "Element is not registered to any presentation!";
}

void Q3DSElement::fireEvent(const QString &eventName)
{
    if (d_ptr->m_presentation)
        d_ptr->m_presentation->q_ptr->fireEvent(d_ptr->m_elementPath, eventName);
    else
        qWarning() << __FUNCTION__ << "Element is not registered to any presentation!";
}

Q3DSElementPrivate::Q3DSElementPrivate(Q3DSElement *parent)
    : q_ptr(parent)
    , m_viewerApp(nullptr)
    , m_commandQueue(nullptr)
    , m_presentation(nullptr)
{
}

Q3DSElementPrivate::~Q3DSElementPrivate()
{
    if (m_presentation)
        m_presentation->unregisterElement(q_ptr);
}

void Q3DSElementPrivate::setElementPath(const QString &elementPath)
{
    m_elementPath = elementPath;

    if (m_presentation)
        m_presentation->registerElement(q_ptr);
}

void Q3DSElementPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;
}

void Q3DSElementPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;
}

void Q3DSElementPrivate::setPresentation(Q3DSPresentationPrivate *presentation)
{
    m_presentation = presentation;
}

void Q3DSElementPrivate::requestResponseHandler(CommandType commandType, void *requestData)
{
    Q_UNUSED(commandType)
    Q_UNUSED(requestData)

    // Base element doesn't handle any request command types yet
    qWarning() << __FUNCTION__ << "Unknown command type.";
}

QT_END_NAMESPACE
