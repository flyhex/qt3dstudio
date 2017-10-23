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

#ifndef ACTIONVIEW_H
#define ACTIONVIEW_H

#include <QQuickWidget>
#include <QColor>
#include <QPointer>
#include <QTimer>

#include "stdafx.h"
#include "DispatchListeners.h"
#include "EventsBrowserView.h"
#include "EventsModel.h"
#include "ObjectBrowserView.h"
#include "ObjectListModel.h"
#include "PropertyModel.h"
#include "SelectedValueImpl.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSignals.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMMetaDataTypes.h"
#include "TabOrderHandler.h"

class ActionModel;
class CClientDataModelBridge;
class CCore;
class CDoc;
class IObjectReferenceHelper;

class QAbstractItemModel;

struct HandlerArgument {
    Q_PROPERTY(qt3dsdm::HandlerArgumentType::Value type MEMBER m_type FINAL)
    Q_PROPERTY(QString name MEMBER m_name FINAL)
    Q_PROPERTY(int handle MEMBER m_handle FINAL)
    Q_PROPERTY(QVariant value MEMBER m_value FINAL)
    Q_PROPERTY(qt3dsdm::CompleteMetaDataType::Enum completeType MEMBER m_completeType FINAL)

    qt3dsdm::Qt3DSDMHandlerArgHandle m_handle;
    qt3dsdm::HandlerArgumentType::Value m_type;
    qt3dsdm::CompleteMetaDataType::Enum m_completeType;
    QString m_name;
    QVariant m_value;

    Q_GADGET
};

Q_DECLARE_METATYPE(HandlerArgument)

class ActionView : public QQuickWidget,
                   public CPresentationChangeListener,
                   public TabNavigable
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *actionsModel READ actionsModel NOTIFY itemChanged FINAL)
    Q_PROPERTY(QAbstractItemModel *propertyModel READ propertyModel NOTIFY propertyModelChanged FINAL)
    Q_PROPERTY(QString itemIcon READ itemIcon NOTIFY itemChanged FINAL)
    Q_PROPERTY(QString itemText READ itemText NOTIFY itemChanged FINAL)
    Q_PROPERTY(QColor itemColor READ itemColor NOTIFY itemChanged FINAL)
    Q_PROPERTY(QString triggerObjectName READ triggerObjectName NOTIFY actionChanged FINAL)
    Q_PROPERTY(QString targetObjectName READ targetObjectName NOTIFY actionChanged FINAL)
    Q_PROPERTY(QString eventName READ eventName NOTIFY actionChanged FINAL)
    Q_PROPERTY(QString handlerName READ handlerName NOTIFY actionChanged FINAL)
    Q_PROPERTY(QVariantList handlerArguments READ handlerArguments NOTIFY actionChanged FINAL)
    Q_PROPERTY(PropertyInfo property READ property NOTIFY propertyChanged FINAL)
    Q_PROPERTY(QString firedEvent MEMBER m_firedEvent NOTIFY firedEventChanged FINAL)

public:
    ActionView(QWidget *parent = nullptr);
    ~ActionView();

    QSize sizeHint() const override;

    void setItem(const qt3dsdm::Qt3DSDMInstanceHandle &handle);
    QString itemIcon() const;
    QString itemText() const;
    QColor itemColor() const;
    QAbstractItemModel *actionsModel() const;
    QAbstractItemModel *propertyModel() const;
    QString targetObjectName() const;
    QString triggerObjectName() const;
    QString eventName() const;
    QString handlerName() const;
    QVariantList handlerArguments() const;
    PropertyInfo property() const;

    Q_INVOKABLE void setCurrentActionIndex(int index);
    Q_INVOKABLE void setCurrentPropertyIndex(int handle, int index);
    Q_INVOKABLE void addAction();
    Q_INVOKABLE void deleteAction(int index);
    Q_INVOKABLE QObject *showTriggerObjectBrowser(const QPoint &point);
    Q_INVOKABLE QObject *showTargetObjectBrowser(const QPoint &point);
    Q_INVOKABLE void showContextMenu(int x, int y);
    Q_INVOKABLE QObject *showEventBrowser(const QPoint &point);
    Q_INVOKABLE QObject *showHandlerBrowser(const QPoint &point);
    Q_INVOKABLE QObject *showEventBrowserForArgument(int handle, const QPoint &point);
    Q_INVOKABLE void setArgumentValue(int handle, const QVariant &value);
    Q_INVOKABLE QStringList slideNames();
    Q_INVOKABLE int slideNameToIndex(const QString &name);


    // CPresentationChangeListener
    void OnNewPresentation() override;

    // ISelectionChangeListener
    void OnSelectionSet(Q3DStudio::SSelectedValue inSelectable);

    // Action callback
    void OnActionAdded(qt3dsdm::Qt3DSDMActionHandle inAction, qt3dsdm::Qt3DSDMSlideHandle inSlide,
                       qt3dsdm::Qt3DSDMInstanceHandle inOwner);
    void OnActionDeleted(qt3dsdm::Qt3DSDMActionHandle inAction, qt3dsdm::Qt3DSDMSlideHandle inSlide,
                         qt3dsdm::Qt3DSDMInstanceHandle inOwner);
    void OnActionModified(qt3dsdm::Qt3DSDMActionHandle inAction);
    void OnHandlerArgumentModified(qt3dsdm::Qt3DSDMHandlerArgHandle inHandlerArgument);
    void OnInstancePropertyValueChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle inProperty);

Q_SIGNALS:
    void itemChanged();
    void actionChanged();
    void propertyModelChanged();
    void propertyChanged();
    void firedEventChanged();

private Q_SLOTS:
    void copyAction();
    void cutAction();
    void pasteAction();

private:
    void setTriggerObject(const qt3dsdm::SObjectRefType &object);
    void setTargetObject(const qt3dsdm::SObjectRefType &object);
    void setEvent(const qt3dsdm::Qt3DSDMEventHandle &event);
    void setHandler(const qt3dsdm::Qt3DSDMHandlerHandle &handler);
    QVariant handlerArgumentValue(int handle) const;
    void updateHandlerArguments();
    void emitActionChanged();
    void updateFiredEvent();
    void resetFiredEvent();
    void updateFiredEventFromHandle(int handle);
    void showBrowser(QQuickWidget *browser, const QPoint &point);

    static CDoc *GetDoc();
    static CClientDataModelBridge *GetBridge();

    void initialize();
    QColor m_baseColor = QColor::fromRgb(75, 75, 75);
    QColor m_selectColor = Qt::transparent;
    qt3dsdm::Qt3DSDMInstanceHandle m_itemHandle;
    IObjectReferenceHelper *m_objRefHelper = nullptr;
    ActionModel *m_actionsModel = nullptr;
    PropertyModel *m_propertyModel = nullptr;
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>>
        m_connections; /// connections to the DataModel
    QPointer<ObjectListModel> m_objectsModel;
    QPointer<ObjectBrowserView> m_triggerObjectBrowser;
    QPointer<ObjectBrowserView> m_targetObjectBrowser;
    QPointer<EventsModel> m_eventsModel;
    QPointer<EventsModel> m_handlersModel;
    QPointer<EventsModel> m_fireEventsModel;
    QPointer<EventsBrowserView> m_eventsBrowser;
    QPointer<EventsBrowserView> m_handlerBrowser;
    QPointer<EventsBrowserView> m_fireEventsBrowser;
    int m_currentActionIndex = -1;
    int m_currentPropertyIndex = -1;
    qt3dsdm::Qt3DSDMHandlerArgHandle m_currentPropertyNameHandle;
    qt3dsdm::Qt3DSDMHandlerArgHandle m_currentPropertyValueHandle;
    QVariantList m_handlerArguments;
    QTimer m_actionChangedCompressionTimer;
    QString m_firedEvent;
};

#endif // ACTIONVIEW_H
