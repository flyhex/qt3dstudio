/****************************************************************************
**
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
#ifndef STUDIOTUTORIALWIDGET_H
#define STUDIOTUTORIALWIDGET_H

#include <QtCore/qdiriterator.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qsettings.h>
#include <QtCore/qdatetime.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qpalette.h>
#include <QtWidgets/qdialog.h>

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class StudioTutorialWidget;
}
QT_END_NAMESPACE

class StudioTutorialWidget : public QDialog
{
    Q_OBJECT
public:
    explicit StudioTutorialWidget(QWidget *parent);

    ~StudioTutorialWidget();

    enum result {
        acceptResult = QDialog::Accepted,
        rejectResult = QDialog::Rejected,
        openSampleResult,
        createNewResult
    };

protected:
    void OnInitDialog();

public:
    void handleDoNotShowAgainChange(int state);
    void handleOpenSample();
    void handleCreateNew();
    void handleQuickStartGuide();

private:
    Ui::StudioTutorialWidget *m_ui;
    QPalette *m_backgroundPalette = nullptr;
};

#endif // STUDIOTUTORIALWIDGET_H
