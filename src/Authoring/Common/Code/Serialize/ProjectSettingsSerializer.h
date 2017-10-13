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
#ifndef INCLUDED_PROJECT_SETTINGS_SERIALIZER_H
#define INCLUDED_PROJECT_SETTINGS_SERIALIZER_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "StudioProjectSettings.h"
#include "StudioProjectVariables.h"
#include "UICDMXML.h"
#include "UICDMWStrOpsImpl.h"

#include <QtWidgets/qcolordialog.h>

namespace qt3dsdm {
}

class CProjectSettingsSerializer
{
    class CustomColorSerializer
    {
    public:
        void Serialize(qt3dsdm::IDOMWriter &ar)
        {
            using namespace qt3dsdm;
            ar.Att(L"count", static_cast<qt3ds::QT3DSI32>(QColorDialog::customCount()));
            QString strColor;
            for (int i = 0; i < QColorDialog::customCount(); ++i) {
                if (i > 0)
                    strColor += QLatin1Char(' ');
                QColor color = QColorDialog::customColor(i);
                strColor += QVariant(color).toString();
            }
            QByteArray data = strColor.toLatin1();
            ar.Value(data.data());
        }
        void Serialize(qt3dsdm::IDOMReader &ar)
        {
            using namespace qt3dsdm;
            TCharStr countStr;
            ar.Att(L"count", countStr);
            int count = QString::fromWCharArray(countStr.wide_str()).toInt();
            const wchar_t *value;
            if (ar.Value(value)) {
                QString strColor = QString::fromWCharArray(value);
                QStringList strColors = strColor.split(" ");
                if (count != strColors.size())
                    qWarning() << "Color count is invalid.";
                for (int i = 0; i < strColors.size(); ++i) {
                    QColor color = QColor(strColors.at(i));
                    QColorDialog::setCustomColor(i, color);
                }
            }
        }
    };

public:
    CProjectSettingsSerializer(CStudioProjectSettings *inSettings)
        : m_ProjectSettings(inSettings)
    {
    }
    virtual ~CProjectSettingsSerializer() {}

    void Serialize(qt3dsdm::IDOMWriter &ar)
    {
        using namespace qt3dsdm;
        using namespace std;
        CStudioProjectSettings *theProjectSettings = GetProjectSettings();
        Q3DStudio::CString author =
                Q3DStudio::CString::fromQString(theProjectSettings->GetAuthor());
        TCharPtr theAuthor = author;
        ar.Att(L"author", theAuthor);
        Q3DStudio::CString company =
                Q3DStudio::CString::fromQString(theProjectSettings->GetCompany());
        TCharPtr theCompany = company;
        ar.Att(L"company", theCompany);

        CPt theSize = theProjectSettings->GetPresentationSize();
        ar.Att(L"presentationWidth", static_cast<qt3ds::QT3DSI32>(theSize.x));
        ar.Att(L"presentationHeight", static_cast<qt3ds::QT3DSI32>(theSize.y));

        if (theProjectSettings->GetRotatePresentation()) {
            ar.Att("presentationRotation", "90");
        }

        bool theMaintainAspect = theProjectSettings->GetMaintainAspect();
        ar.Att(L"maintainAspect", theMaintainAspect);

        if (QColorDialog::customCount() > 0) {
            CustomColorSerializer ccs;
            ar.Serialize(L"CustomColors", ccs);
        }
    }
    void Serialize(qt3dsdm::IDOMReader &ar)
    {
        using namespace qt3dsdm;
        using namespace std;
        CStudioProjectSettings *theProjectSettings = GetProjectSettings();
        theProjectSettings->Reset();

        TCharStr theAuthor;
        ar.Att(L"author", theAuthor);
        Q3DStudio::CString theAuthorStr(theAuthor.wide_str());
        theProjectSettings->SetAuthor(theAuthorStr.toQString());

        TCharStr theCompany;
        ar.Att(L"company", theCompany);
        Q3DStudio::CString theCompanyStr(theCompany.wide_str());
        theProjectSettings->SetCompany(theCompanyStr.toQString());

        qt3ds::QT3DSI32 thePresentationWidth;
        qt3ds::QT3DSI32 thePresentationHeight;
        ar.Att("presentationWidth", thePresentationWidth);
        ar.Att("presentationHeight", thePresentationHeight);
        theProjectSettings->SetPresentationSize(CPt(thePresentationWidth, thePresentationHeight));

        qt3ds::QT3DSI32 thePresentationRotate;
        if (ar.Att("presentationRotation", thePresentationRotate) && thePresentationRotate == 90)
            theProjectSettings->SetRotatePresentation(true);

        bool theMaintainAspect;
        ar.Att(L"maintainAspect", theMaintainAspect);
        theProjectSettings->SetMaintainAspect(theMaintainAspect);

        {
            CustomColorSerializer ccs;
            ar.Serialize(L"CustomColors", ccs);
        }
    }

    CStudioProjectSettings *GetProjectSettings() { return m_ProjectSettings; }

private:
    CStudioProjectSettings *m_ProjectSettings;
};

template <typename Archive>
void save(Archive &ar, const CProjectSettingsSerializer &inData, unsigned int)
{
    CStudioProjectSettings *theProjectSettings =
        const_cast<CProjectSettingsSerializer &>(inData).GetProjectSettings();
}

template <typename Archive>
void load(Archive &ar, CProjectSettingsSerializer &inData, unsigned int)
{
}

#endif // INCLUDED_PROJECT_SETTINGS_SERIALIZER_H
