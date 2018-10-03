/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef EDITPRESENTATIONIDDLG_H
#define EDITPRESENTATIONIDDLG_H

#include <QtWidgets/qdialog.h>

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class EditPresentationIdDlg;
}
QT_END_NAMESPACE

class EditPresentationIdDlg : public QDialog
{
    Q_OBJECT

public:
    enum DialogType {
        EditPresentationId,
        EditQmlStreamId,
        EditPresentationName,
        EditQmlStreamName
    };

    explicit EditPresentationIdDlg(const QString &src, DialogType type = EditPresentationId,
                                   QWidget *parent = nullptr);
    ~EditPresentationIdDlg();

public Q_SLOTS:
    void accept() override;

private:
    enum WarningType {
        EmptyWarning,
        UniqueWarning
    };
    void displayWarning(WarningType warningType);

    Ui::EditPresentationIdDlg *m_ui;
    QString m_src; // src attribute value for the current presentation in the project file
    QString m_presentationId;
    DialogType m_dialogType;
};

#endif // EDITPRESENTATIONIDDLG_H
