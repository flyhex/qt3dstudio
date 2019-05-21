/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "q3dsdataoutput_p.h"
#include "q3dspresentation_p.h"
#include "q3dscommandqueue_p.h"

Q3DSDataOutput::Q3DSDataOutput(QObject *parent)
  : QObject(parent)
  , d_ptr(new Q3DSDataOutputPrivate(this))
{

}

Q3DSDataOutput::Q3DSDataOutput(const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataOutputPrivate(this))
{
    d_ptr->m_name = name;
}

Q3DSDataOutput::~Q3DSDataOutput()
{
    delete d_ptr;
}

QString Q3DSDataOutput::name() const
{
    return d_ptr->m_name;
}

void Q3DSDataOutput::setName(const QString &name)
{
    if (d_ptr->m_name != name) {
        d_ptr->m_name = name;
        if (d_ptr->m_presentation)
            d_ptr->m_presentation->registerDataOutput(this);
        Q_EMIT nameChanged(name);
    }
}

QVariant Q3DSDataOutput::value() const
{
    return d_ptr->m_value;
}

void Q3DSDataOutput::setValue(const QVariant &value)
{
    if (d_ptr->m_value == value)
        return;

    d_ptr->m_value = value;
    Q_EMIT valueChanged(value);
}

void Q3DSDataOutputPrivate::setPresentation(Q3DSPresentation *presentation)
{
    m_presentation = presentation;
}

Q3DSDataOutputPrivate::Q3DSDataOutputPrivate(Q3DSDataOutput *parent)
    : q_ptr(parent)
{
}

Q3DSDataOutputPrivate::~Q3DSDataOutputPrivate()
{
    if (m_presentation)
        m_presentation->unregisterDataOutput(q_ptr);
}

void Q3DSDataOutputPrivate::setValue(const QVariant &value)
{
    m_value = value;
    Q_EMIT q_ptr->valueChanged(value);
}

void Q3DSDataOutputPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;

    if (m_viewerApp && m_value.isValid())
        setValue(m_value);
}

void Q3DSDataOutputPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;

    if (m_commandQueue && m_value.isValid())
        setValue(m_value);
}
