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

#include "q3dsdatainput_p.h"
#include "q3dspresentation_p.h"
#include "q3dscommandqueue_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

Q3DSDataInput::Q3DSDataInput(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataInputPrivate(this))
{
}

Q3DSDataInput::Q3DSDataInput(const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataInputPrivate(this))
{
    d_ptr->m_name = name;
}

Q3DSDataInput::Q3DSDataInput(Q3DSPresentation *presentation, const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataInputPrivate(this))
{
    d_ptr->m_name = name;
    if (presentation)
        presentation->registerDataInput(this);
}

Q3DSDataInput::Q3DSDataInput(Q3DSDataInputPrivate *d, Q3DSPresentation *presentation,
                             const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(d)
{
    d_ptr->m_name = name;
    if (presentation)
        presentation->registerDataInput(this);
}

Q3DSDataInput::~Q3DSDataInput()
{
    delete d_ptr;
}

QString Q3DSDataInput::name() const
{
    return d_ptr->m_name;
}

void Q3DSDataInput::setName(const QString &name)
{
    if (d_ptr->m_name != name) {
        d_ptr->m_name = name;
        if (d_ptr->m_presentation)
            d_ptr->m_presentation->registerDataInput(this);
        Q_EMIT nameChanged();
    }
}

QVariant Q3DSDataInput::value() const
{
    return d_ptr->m_value;
}

void Q3DSDataInput::setValue(const QVariant &value)
{
    if (value != d_ptr->m_value) {
        d_ptr->setValue(value);
        Q_EMIT valueChanged();
    }
}

Q3DSDataInputPrivate::Q3DSDataInputPrivate(Q3DSDataInput *parent)
    : q_ptr(parent)
{
}

Q3DSDataInputPrivate::~Q3DSDataInputPrivate()
{
    if (m_presentation)
        m_presentation->unregisterDataInput(q_ptr);
}

void Q3DSDataInputPrivate::setValue(const QVariant &value)
{
    m_value = value;
    if (m_presentation)
        m_presentation->q_ptr->setDataInputValue(m_name, m_value);
}

void Q3DSDataInputPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;

    if (m_viewerApp && m_value.isValid())
        setValue(m_value);
}

void Q3DSDataInputPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;

    if (m_commandQueue && m_value.isValid())
        setValue(m_value);
}

void Q3DSDataInputPrivate::setPresentation(Q3DSPresentationPrivate *presentation)
{
    m_presentation = presentation;
}

QT_END_NAMESPACE
