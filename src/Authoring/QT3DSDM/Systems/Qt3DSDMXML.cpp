/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMXML.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSRefCounted.h"
#include "Qt3DSDMStringTable.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include <memory>
#include "foundation/StrConvertUTF.h"
#include "foundation/StringTable.h"
#include "utf8.h"

#include <QtCore/qbytearray.h>

#ifdef QT3DS_VC
#include <winsock2.h>
#include <windows.h> //output debug string
#endif

#include <QtCore/qxmlstream.h>

typedef char XML_Char;
typedef char XML_LChar;

using namespace qt3dsdm;
using std::shared_ptr;
using namespace qt3ds::foundation;
using qt3ds::foundation::Pool;

#define QT3DSXML_FOREACH(idxnm, val)                                                                  \
    for (QT3DSU32 idxnm = 0, __numItems = (QT3DSU32)val; idxnm < __numItems; ++idxnm)

namespace qt3dsdm {

SDOMAttribute::SDOMAttribute(TXMLCharPtr nm, TXMLCharPtr val)
    : m_Name(nm)
    , m_Value(val)
    , m_NextAttribute(NULL)
{
}

SDOMElement::SDOMElement(TXMLCharPtr nm)
    : m_Name(nm)
    , m_FirstAttribute(nullptr)
    , m_LastAttribute(nullptr)
    , m_Parent(nullptr)
    , m_FirstChild(nullptr)
    , m_LastChild(nullptr)
    , m_NextSibling(nullptr)
    , m_Value("")
{
}

void SDOMElement::AddAttribute(SDOMAttribute &att)
{
    if (m_LastAttribute) {
        m_LastAttribute->m_NextAttribute = &att;
        m_LastAttribute = &att;
    } else {
        QT3DS_ASSERT(m_FirstAttribute == NULL);
        m_FirstAttribute = m_LastAttribute = &att;
    }
}
// Used to ensure duplicate attributes can't happen
void SDOMElement::SetAttributeValue(TXMLCharPtr inName, TXMLCharPtr inValue, IDOMFactory &inFactory,
                                    const SDOMFlags &inFlags)
{
    inName = inFactory.GetStringTable()->RegisterStr(inName);
    SDOMAttribute *att = FindAttribute(inName, inFlags);
    if (att) {
        att->m_Value = inFactory.RegisterValue(inValue);
    } else {
        AddAttribute(*inFactory.NextAttribute(inName, inValue));
    }
}
void SDOMElement::SetAttributeValue(TWideXMLCharPtr inName, TWideXMLCharPtr inValue,
                                    IDOMFactory &inFactory, const SDOMFlags &inFlags)
{
    TXMLCharPtr theName = inFactory.GetStringTable()->GetNarrowStr(inName);
    SDOMAttribute *att = FindAttribute(theName, inFlags);
    if (att) {
        att->m_Value = inFactory.RegisterValue(inValue);
    } else {
        AddAttribute(
            *inFactory.NextAttribute(inFactory.GetStringTable()->GetWideStr(inName), inValue));
    }
}
const SDOMAttribute *SDOMElement::FindAttribute(TXMLCharPtr nm, const SDOMFlags &inFlags) const
{
    return const_cast<SDOMElement *>(this)->FindAttribute(nm, inFlags);
}
SDOMAttribute *SDOMElement::FindAttribute(TXMLCharPtr nm, const SDOMFlags &inFlags)
{
    for (SDOMAttribute *att = m_FirstAttribute; att != NULL; att = att->m_NextAttribute) {
        if (att->m_Name == nm)
            return att;
        else if (inFlags.CaselessAttributes() && AreEqualCaseless(nm, att->m_Name))
            return att;
    }
    return NULL;
}
void SDOMElement::RemoveAttribute(TXMLCharPtr nm, const SDOMFlags &inFlags)
{
    SDOMAttribute *preatt = m_FirstAttribute;
    for (SDOMAttribute *att = m_FirstAttribute; att != NULL;
         preatt = att, att = att->m_NextAttribute) {
        if (att->m_Name == nm
            || (inFlags.CaselessAttributes() && AreEqualCaseless(nm, att->m_Name))) {
            if (att == m_FirstAttribute) {
                m_FirstAttribute = att->m_NextAttribute;
            } else {
                preatt->m_NextAttribute = att->m_NextAttribute;
                if (att == m_LastAttribute)
                    m_LastAttribute = preatt;
            }

            att->m_NextAttribute = NULL;
        }
    }
}
TXMLCharPtr SDOMElement::GetAttributeValue(TXMLCharPtr nm, SDOMFlags &inFlags) const
{
    const SDOMAttribute *att = FindAttribute(nm, inFlags);
    if (att)
        return att->m_Value;
    return NULL;
}
void SDOMElement::AddChild(SDOMElement &elem)
{
    elem.m_Parent = this;
    if (m_LastChild) {
        m_LastChild->m_NextSibling = &elem;
        m_LastChild = &elem;
    } else {
        QT3DS_ASSERT(m_FirstChild == NULL);
        m_FirstChild = m_LastChild = &elem;
    }
}
SDOMElement *SDOMElement::FindPreviousChild(SDOMElement &elem)
{
    if (&elem == m_FirstChild)
        return NULL;
    // Find the previous sibling.
    SDOMElement *theChild = m_FirstChild;
    // Empty loop intentional
    for (; theChild && theChild->m_NextSibling != &elem; theChild = theChild->m_NextSibling) {
    }

    return theChild;
}
void SDOMElement::RemoveChild(SDOMElement &elem)
{
    if (elem.m_Parent != this) {
        QT3DS_ASSERT(false);
        return;
    }
    elem.m_Parent = NULL;
    if (&elem == m_FirstChild) {
        m_FirstChild = elem.m_NextSibling;
    } else {
        SDOMElement *theChild(FindPreviousChild(elem));
        QT3DS_ASSERT(theChild);
        if (theChild) {
            theChild->m_NextSibling = elem.m_NextSibling;
            if (&elem == m_LastChild)
                m_LastChild = theChild;
        }
    }
    elem.m_NextSibling = NULL;
}

void SDOMElement::ReplaceChild(SDOMElement &inChild, SDOMElement &inReplacement)
{
    inChild.m_Parent = NULL;
    if (&inChild == m_FirstChild)
        m_FirstChild = &inReplacement;
    else {
        SDOMElement *theChild(FindPreviousChild(inChild));
        QT3DS_ASSERT(theChild);
        if (theChild) {
            theChild->m_NextSibling = &inReplacement;
            if (&inChild == m_LastChild)
                m_LastChild = &inReplacement;
        }
    }
    inReplacement.m_NextSibling = inChild.m_NextSibling;
    inReplacement.m_Parent = this;
    inChild.m_NextSibling = NULL;
}

void SDOMElement::InsertChildBefore(SDOMElement &elem, SDOMElement &theSibling)
{
    // Ensure elem isn't in the graph.
    QT3DS_ASSERT(elem.m_Parent == NULL);
    QT3DS_ASSERT(elem.m_NextSibling == NULL);
    elem.m_Parent = this;
    if (&theSibling == m_FirstChild)
        m_FirstChild = &elem;
    else {
        SDOMElement *thePrevious = FindPreviousChild(theSibling);
        QT3DS_ASSERT(thePrevious);
        if (thePrevious)
            thePrevious->m_NextSibling = &elem;
    }

    elem.m_NextSibling = &theSibling;
}
QT3DSU32 SDOMElement::GetNumChildren(TXMLCharPtr inChildName, const SDOMFlags &inFlags) const
{
    QT3DSU32 idx = 0;
    for (SDOMElement *elem = m_FirstChild; elem != NULL; elem = elem->m_NextSibling) {
        if (elem->m_Name == inChildName)
            ++idx;
        else if (inFlags.CaselessElements() && AreEqualCaseless(inChildName, elem->m_Name))
            ++idx;
    }
    return idx;
}
QT3DSU32 SDOMElement::GetNumChildren() const
{
    QT3DSU32 idx = 0;
    for (SDOMElement *elem = m_FirstChild; elem != NULL; elem = elem->m_NextSibling)
        ++idx;
    return idx;
}
SDOMElement *SDOMElement::FindChildByName(TXMLCharPtr nm, const SDOMFlags &inFlags) const
{
    for (SDOMElement *elem = m_FirstChild; elem != NULL; elem = elem->m_NextSibling) {
        if (elem->m_Name == nm)
            return elem;
        else if (inFlags.CaselessElements() && AreEqualCaseless(nm, elem->m_Name))
            return elem;
    }
    return NULL;
}
SDOMElement *SDOMElement::FindNextSiblingByName(TXMLCharPtr nm, const SDOMFlags &inFlags) const
{
    for (SDOMElement *elem = m_NextSibling; elem != NULL; elem = elem->m_NextSibling) {
        if (elem->m_Name == nm)
            return elem;
        else if (inFlags.CaselessElements() && AreEqualCaseless(nm, elem->m_Name))
            return elem;
    }
    return NULL;
}

}


const QT3DSU16 g_BOMMarker = (QT3DSU16)0xFEFF;

SElemPointer::SElemPointer(SDOMElement *elem)
    : std::pair<SDOMElement *, SDOMAttribute *>(elem, NULL)
{
}
SElemPointer &SElemPointer::operator=(SDOMElement *elem)
{
    first = elem;
    second = NULL;
    return *this;
}
SElemPointer &SElemPointer::operator=(SDOMAttribute *att)
{
    second = att;
    return *this;
}
SElemPointer &SElemPointer::operator=(const std::pair<SDOMElement *, SDOMAttribute *> &other)
{
    std::pair<SDOMElement *, SDOMAttribute *>::operator=(other);
    return *this;
}
SElemPointer::operator SDOMElement *() const { return first; }
SDOMElement *SElemPointer::operator->() const { return first; }


// Some DOM parsing operations are destructive.  If you need
// them to not be destructive, then we need to modify
// the reader.  Specifically parsing lists of floats, due
// to a bug in strtod, is destructive.

SDOMReader::SDOMReader(SDOMElement &te, std::shared_ptr<qt3dsdm::IStringTable> s,
           std::shared_ptr<IDOMFactory> inFactory)
    : IDOMReader(s)
    , m_TopElement(&te)
    , m_Factory(inFactory)
{
}

SDOMElement *SDOMReader::Current() const { return m_TopElement.first; }
void SDOMReader::SetDOMFlags(SDOMFlags inFlags) { m_Flags = inFlags; }
SDOMFlags SDOMReader::GetDOMFlags() const { return m_Flags; }

void SDOMReader::PushScope() { m_ScopeStack.push_back(m_TopElement); }
void SDOMReader::PopScope() {
    if (m_ScopeStack.size()) {
        m_TopElement = m_ScopeStack.back();
        m_ScopeStack.pop_back();
    } else {
        m_TopElement = std::pair<SDOMElement *, SDOMAttribute *>(NULL, NULL);
    }
}

void *SDOMReader::GetScope() { return m_TopElement.first; }

void SDOMReader::SetScope(void *inScope) {
    m_TopElement =
        std::make_pair(reinterpret_cast<SDOMElement *>(inScope), (SDOMAttribute *)NULL);
}

QString SDOMReader::GetElementName() const {
    if (!Current()) {
        QT3DS_ASSERT(false);
        return NULL;
    }
    return QString::fromLatin1(Current()->m_Name);
}

bool SDOMReader::UnregisteredAtt(TWideXMLCharPtr name, TWideXMLCharPtr &outValue) {
    outValue = L"";
    SDOMElement *current(Current());
    if (current) {
        TXMLCharPtr theValue =
            current->GetAttributeValue(m_StringTable->GetNarrowStr(name), m_Flags);
        if (theValue && *theValue) {
            ConvertUTF(theValue, 0, m_TempBuffer);
            outValue = reinterpret_cast<const wchar_t *>(m_TempBuffer.c_str());
            return true;
        }
    } else {
        QT3DS_ASSERT(false);
    }
    return false;
}

bool SDOMReader::UnregisteredAtt(TXMLCharPtr name, TXMLCharPtr &outValue) {
    outValue = "";
    SDOMElement *current(Current());
    if (current) {
        outValue = current->GetAttributeValue(m_StringTable->GetNarrowStr(name), m_Flags);
        if (outValue)
            return true;
    } else {
        QT3DS_ASSERT(false);
    }
    return false;
}

bool SDOMReader::Att(TWideXMLCharPtr name, TWideXMLCharPtr &outValue) {
    if (UnregisteredAtt(name, outValue)) {
        outValue = m_StringTable->RegisterStr(outValue);
        return true;
    }
    return false;
}
bool SDOMReader::Att(TXMLCharPtr name, TXMLCharPtr &outValue) {
    if (UnregisteredAtt(name, outValue)) {
        outValue = m_StringTable->RegisterStr(outValue);
        return true;
    }
    return false;
}

QT3DSU32 SDOMReader::CountChildren()
{
    SDOMElement *elem = Current();
    if (elem == NULL) {
        QT3DS_ASSERT(false);
        return 0;
    }
    return elem->GetNumChildren();
}

QT3DSU32 SDOMReader::CountChildren(TWideXMLCharPtr childName)
{
    return CountChildren(m_StringTable->GetNarrowStr(childName));
}

QT3DSU32 SDOMReader::CountChildren(TXMLCharPtr childName)
{
    SDOMElement *elem = Current();
    if (elem == NULL) {
        QT3DS_ASSERT(false);
        return 0;
    }
    return elem->GetNumChildren(m_StringTable->GetNarrowStr(childName), m_Flags);
}

std::pair<QString, QString> SDOMReader::CurrentAttNarrow()
{
    if (m_TopElement.second) {
        return std::make_pair(QString::fromLatin1(m_TopElement.second->m_Name),
                              QString::fromLatin1(m_TopElement.second->m_Value));
    }
    return std::make_pair(QString(), QString());
}

std::pair<QString, QString> SDOMReader::GetNextAttribute()
{
    if (m_TopElement.second)
        m_TopElement.second = m_TopElement.second->m_NextAttribute;
    return CurrentAttNarrow();
}

std::pair<QString, QString> SDOMReader::GetFirstAttribute()
{
    if (m_TopElement.first == NULL) {
        QT3DS_ASSERT(false);
        std::make_pair(QString(), QString());
    }
    m_TopElement.second = m_TopElement.first->m_FirstAttribute;
    return CurrentAttNarrow();
}

bool SDOMReader::MoveToFirstChild()
{
    SDOMElement *elem = Current();
    if (elem == NULL) {
        QT3DS_ASSERT(false);
        return false;
    }
    if (elem->m_FirstChild) {
        m_TopElement = elem->m_FirstChild;
        return true;
    }
    return false;
}

bool SDOMReader::MoveToFirstChild(TXMLCharPtr childName)
{
    SDOMElement *elem = Current();
    if (elem == NULL) {
        QT3DS_ASSERT(false);
        return false;
    }
    SDOMElement *child = elem->FindChildByName(m_StringTable->RegisterStr(childName), m_Flags);
    if (child != NULL) {
        m_TopElement = child;
        return true;
    }
    return false;
}

bool SDOMReader::MoveToFirstChild(TWideXMLCharPtr childName)
{
    return MoveToFirstChild(m_StringTable->GetNarrowStr(childName));
}

bool SDOMReader::MoveToNextSibling()
{
    SDOMElement *elem = Current();
    if (elem == NULL) {
        QT3DS_ASSERT(false);
        return false;
    }
    if (elem->m_NextSibling) {
        m_TopElement = elem->m_NextSibling;
        return true;
    }
    return false;
}

bool SDOMReader::MoveToNextSibling(TXMLCharPtr childName)
{
    SDOMElement *elem = Current();
    if (elem == NULL) {
        QT3DS_ASSERT(false);
        return false;
    }
    SDOMElement *nextSibling
            = elem->FindNextSiblingByName(m_StringTable->RegisterStr(childName), m_Flags);
    if (nextSibling) {
        m_TopElement = nextSibling;
        return true;
    }
    return false;
}

bool SDOMReader::MoveToNextSibling(TWideXMLCharPtr childName)
{
    return MoveToNextSibling(m_StringTable->GetNarrowStr(childName));
}

bool SDOMReader::MoveToNextSibling(const QString &childName)
{
    return MoveToNextSibling(qPrintable(childName));
}

bool SDOMReader::MoveToFirstChild(const QString &childName)
{
    return MoveToFirstChild(qPrintable(childName));
}

// Leave element means go to its parent.
void SDOMReader::Leave()
{
    if (m_TopElement)
        m_TopElement = m_TopElement->m_Parent;

    QT3DS_ASSERT(m_TopElement);
}
bool SDOMReader::Value(TXMLCharPtr &outValue)
{
    SDOMElement *current(Current());
    if (!current) {
        QT3DS_ASSERT(false);
        return false;
    }
    outValue = current->m_Value;
    return true;
}

bool SDOMReader::Value(TWideXMLCharPtr &outValue)
{
    outValue = L"";
    TXMLCharPtr theValue;
    if (Value(theValue)) {
        ConvertUTF(theValue, 0, m_TempBuffer);
        outValue = reinterpret_cast<const wchar_t *>(m_TempBuffer.c_str());
        return true;
    }
    return false;
}

bool SDOMReader::Value(QString &outValue)
{
    outValue = "";
    TXMLCharPtr theValue;
    if (Value(theValue)) {
        outValue = QString::fromLatin1(theValue);
        return true;
    }
    return false;
}

SDOMElement *SDOMReader::GetTopElement() const
{
    SDOMElement *current(Current());
    while (current && current->m_Parent)
        current = current->m_Parent;
    return current;
}

std::shared_ptr<IDOMFactory> SDOMReader::GetFactory() const
{
    return m_Factory;
}

SDOMWriter::SDOMWriter(std::shared_ptr<IDOMFactory> inDOMFactory,
           std::shared_ptr<qt3dsdm::IStringTable> inStringTable, SDOMElement &inTopElem)
    : m_FactoryPtr(inDOMFactory)
    , m_Factory(*inDOMFactory)
    , SDOMReader(inTopElem, inStringTable)
{
}
void SDOMWriter::SetDOMFlags(SDOMFlags inFlags)  { m_Flags = inFlags; }
SDOMFlags SDOMWriter::GetDOMFlags() const  { return m_Flags; }

void SDOMWriter::Begin(TXMLCharPtr inElemName)
{
    if (!m_TopElement) {
        QT3DS_ASSERT(false);
        return;
    }
    SDOMElement *current(Current());
    SDOMElement *newElement(m_Factory.NextElement(inElemName));
    current->AddChild(*newElement);
    m_TopElement = newElement;
}

void SDOMWriter::Begin(TWideXMLCharPtr inElemName)
{
    Begin(m_FactoryPtr->GetStringTable()->GetNarrowStr(inElemName));
}

void SDOMWriter::Begin(const QString &inElemName)
{
    Begin(qPrintable(inElemName));
}

void SDOMWriter::Att(TXMLCharPtr name, TXMLCharPtr value)
{
    if (!m_TopElement) {
        QT3DS_ASSERT(false);
        return;
    }
    m_TopElement->SetAttributeValue(name, value, m_Factory, m_Flags);
}
// Attributes.  They may be sorted just before write
void SDOMWriter::Att(TWideXMLCharPtr name, TWideXMLCharPtr value)
{
    if (!m_TopElement) {
        QT3DS_ASSERT(false);
        return;
    }
    m_TopElement->SetAttributeValue(name, value, m_Factory, m_Flags);
}

void SDOMWriter::Value(TWideXMLCharPtr value)
{
    if (!m_TopElement) {
        QT3DS_ASSERT(false);
        return;
    }
    if (value == NULL)
        value = L"";
    size_t len = wcslen(value);
    m_Factory.AppendStrBuf(value, (QT3DSU32)len);
    m_TopElement->m_Value = m_Factory.FinalizeStrBuf();
}
void SDOMWriter::Value(TXMLCharPtr value)
{
    if (!m_TopElement) {
        QT3DS_ASSERT(false);
        return;
    }
    if (value == NULL)
        value = "";
    size_t len = strlen(value);
    m_Factory.AppendStrBuf(value, (QT3DSU32)len);
    m_TopElement->m_Value = m_Factory.FinalizeStrBuf();
}

void SDOMWriter::Value(const QString &value)
{
    Value(qUtf16Printable(value));
}

void SDOMWriter::End()
{
    if (!m_TopElement) {
        QT3DS_ASSERT(false);
        return;
    }
    Leave();
}
void SDOMWriter::RemoveCurrent()
{
    SDOMElement *current(Current());
    if (!current) {
        QT3DS_ASSERT(false);
        return;
    }
    if (current->m_Parent) {
        m_TopElement = current->m_Parent;
        m_TopElement->RemoveChild(*current);
    }
}
void SDOMWriter::ReplaceCurrent(SDOMElement &inElement)
{
    SDOMElement *current(Current());
    if (!current) {
        QT3DS_ASSERT(false);
        return;
    }
    if (current->m_Parent) {
        current->m_Parent->ReplaceChild(*current, inElement);
        m_TopElement = &inElement;
    } else {
        m_TopElement = &inElement;
        inElement.m_Parent = NULL;
        inElement.m_NextSibling = NULL;
    }
}
void SDOMWriter::AppendChildren(SDOMElement &inElement)
{
    SDOMElement *current(Current());
    if (!current) {
        QT3DS_ASSERT(false);
        return;
    }
    SDOMElement *theChild = inElement.m_FirstChild;
    inElement.m_FirstChild = inElement.m_LastChild = NULL;
    while (theChild) {
        SDOMElement *theCurrentChild = theChild;
        theChild = theChild->m_NextSibling;

        theCurrentChild->m_Parent = NULL;
        theCurrentChild->m_NextSibling = NULL;
        current->AddChild(*theCurrentChild);
    }
}
void SDOMWriter::RemoveAttribute(TXMLCharPtr inItem)
{
    SDOMElement *current(Current());
    if (!current) {
        QT3DS_ASSERT(false);
        return;
    }
    current->RemoveAttribute(m_StringTable->RegisterStr(inItem), m_Flags);
}
void SDOMWriter::RemoveAttribute(TWideXMLCharPtr inItem)
{
    RemoveAttribute(m_StringTable->GetNarrowStr(inItem));
}

void SDOMWriter::MoveBefore(TXMLCharPtr inItem, TXMLCharPtr inSibling)
{
    SDOMElement *current(Current());
    if (!current) {
        QT3DS_ASSERT(false);
        return;
    }

    SDOMElement *theItem = current->FindChildByName(m_StringTable->RegisterStr(inItem), m_Flags);
    SDOMElement *theSibling
            = current->FindChildByName(m_StringTable->RegisterStr(inSibling), m_Flags);
    QT3DS_ASSERT(theItem && theSibling);
    if (theItem && theSibling) {
        current->RemoveChild(*theItem);
        current->InsertChildBefore(*theItem, *theSibling);
    }
}

void SDOMWriter::MoveBefore(TWideXMLCharPtr inItem, TWideXMLCharPtr inSibling)
{
    MoveBefore(m_StringTable->GetNarrowStr(inItem), m_StringTable->GetNarrowStr(inSibling));
}

// If current has no parent, then we are at the top
// of the tree and we should return 0.  Or if there is no
// current.
// If there is one parent, we should return 1.
QT3DSU32 SDOMWriter::GetTabs()
{
    QT3DSU32 retval = 0;
    SDOMElement *current(Current());
    do {
        if (current)
            current = current->m_Parent;
        if (current)
            ++retval;
    } while (current);
    return retval;
}

SDOMElement *SDOMWriter::GetTopElement() const
{
    return SDOMReader::GetTopElement();
}

std::shared_ptr<IDOMFactory> SDOMWriter::GetFactory() const
{
    return m_FactoryPtr;
}


struct SimpleXmlWriter
{
    QIODevice &m_file;
    std::vector<std::pair<TXMLCharPtr, bool>> m_OpenElements;
    bool m_ElementOpen;
    wchar_t m_PrintBuf[256];
    QT3DSU32 m_Tabs;
    QTextStream m_stream;

    SimpleXmlWriter(QIODevice &stream, QT3DSU32 inTabs = 0)
        : m_file(stream)
        , m_ElementOpen(false)
        , m_Tabs(inTabs)
        , m_stream(&m_file)
    {
    }
    void Write(TWideXMLCharPtr data)
    {
        if (!IsTrivial(data))
            m_stream << QString::fromWCharArray(data);
    }
    void Write(const char8_t *data)
    {
        if (!IsTrivial(data))
            m_stream << data;
    }
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
    void BeginWideWrite() { m_WideBuffer.clear(); }
    void WriteTemp(wchar_t data) { m_WideBuffer.append(1, data); }
    void WriteTemp(const wchar_t *data)
    {
        m_WideBuffer.append(reinterpret_cast<const TWCharEASTLConverter::TCharType *>(data));
    }
    void EndWideWrite() { Write(reinterpret_cast<const wchar_t *>(m_WideBuffer.c_str())); }
#endif
    void Write(char8_t data) { m_stream << data; }
    void Tabs()
    {
        QT3DSXML_FOREACH(idx, (m_OpenElements.size() + m_Tabs))
        Write('\t');
    }
    void Close(bool newline)
    {
        if (m_ElementOpen) {
            Write(L" >");
            if (newline)
                Write('\n');
        }
        m_ElementOpen = false;
    }
    void Begin(TXMLCharPtr name)
    {
        Close(true);
        Tabs();
        Write('<');
        Write(name);
        m_OpenElements.push_back(std::pair<TXMLCharPtr, bool>(name, false));
        m_ElementOpen = true;
    }
    TWideXMLCharPtr ToStr(char8_t val)
    {
        m_PrintBuf[0] = val;
        m_PrintBuf[1] = 0;
        return m_PrintBuf;
    }
    template <typename TDataType>
    TWideXMLCharPtr ToStr(TDataType val)
    {
        WStrOps<TDataType>().ToStr(val, NVDataRef<wchar_t>(m_PrintBuf, 256));
        return m_PrintBuf;
    }
    void Att(TXMLCharPtr name, TXMLCharPtr value)
    {
        QT3DS_ASSERT(m_ElementOpen);
        Write(' ');
        Write(name);
        Write("=\"");
        size_t valueLen = strlen(NonNull(value));
        TXMLCharPtr start = value;
        TXMLCharPtr end = value + valueLen;
        TXMLCharPtr last = start;
        uint32_t item = 0;
        // Write out the data escaping unicode values where necessary
        // I am using utf8::internal because it returns an error code and does not through
        // exceptions; we don't always know the system we are running on will support or handle
        // exceptions gracefully.
        for (utf8::internal::utf_error err_code = utf8::internal::validate_next(start, end, item);
             last != end && err_code == utf8::internal::UTF8_OK;
             err_code = utf8::internal::validate_next(start, end, item)) {
            switch (item) {
            case '\r':
                break;
            case '\n':
                Write("&#10;");
                break;
            case '\t':
                Write("&#09;");
                break;
            case '<':
                Write("&lt;");
                break;
            case '>':
                Write("&gt;");
                break;
            case '"':
                Write("&quot;");
                break;
            case '&':
                Write("&amp;");
                break;
            default:
                m_stream << QString::fromLatin1(last, start - last);
                break;
            }
            last = start;
        }
        Write("\"");
    }
    template <typename TData>
    void Att(TXMLCharPtr name, TData value)
    {
        Att(name, ToStr(value));
    }

    void Value(TXMLCharPtr value)
    {
        if (!IsTrivial(value)) {
            Close(false);
            size_t valueLen = strlen(NonNull(value));
            TXMLCharPtr start = value;
            TXMLCharPtr end = value + valueLen;
            TXMLCharPtr last = start;
            uint32_t item = 0;
            // Write out the data escaping unicode values where necessary
            for (utf8::internal::utf_error err_code =
                     utf8::internal::validate_next(start, end, item);
                 last != end && err_code == utf8::internal::UTF8_OK;
                 err_code = utf8::internal::validate_next(start, end, item)) {
                switch (item) {
                case '<':
                    Write("&lt;");
                    break;
                case '>':
                    Write("&gt;");
                    break;
                case '&':
                    Write("&amp;");
                    break;
                default:
                    m_stream << QString::fromLatin1(last, start - last);
                    break;
                }
                last = start;
            }
            m_OpenElements.back().second = true;
        }
    }
    void ChildValue(TXMLCharPtr name, TXMLCharPtr value)
    {
        Begin(name);
        Value(value);
        End();
    }
    void End(bool newlineAfterClose = true)
    {
        QT3DS_ASSERT(m_OpenElements.size());
        std::pair<TXMLCharPtr, bool> topElem = m_OpenElements.back();
        m_OpenElements.pop_back();
        if (m_ElementOpen)
            Write(" />");
        else {
            if (topElem.second == false)
                Tabs();
            Write("</");
            Write(topElem.first);
            Write(">");
        }
        m_ElementOpen = false;
        if (newlineAfterClose == true)
            Write('\n');
    }
};

struct DOMParser
{
    IDOMFactory &m_Factory;
    SDOMElement *m_TopElement;
    SDOMElement *m_FirstElement;

    DOMParser(IDOMFactory &factory)
        : m_Factory(factory)
        , m_FirstElement(NULL)
    {
    }

    static SDOMElement *ParseXMLFile(IDOMFactory &factory, QIODevice &inStream,
                                     CXmlErrorHandler *handler = nullptr)
    {
        QByteArray dataRead = inStream.readAll();
        return ParseXMLFile(factory, dataRead, handler);
    }
    static SDOMElement *ParseXMLFile(IDOMFactory &factory, const QByteArray &inData,
                                     CXmlErrorHandler *handler = nullptr)
    {
        QXmlStreamReader sreader;

        DOMParser domParser(factory);

        sreader.addData(inData);

        while (!sreader.atEnd()) {
            QXmlStreamReader::TokenType token = sreader.readNext();

            if (token == QXmlStreamReader::StartElement) {
                domParser.m_Factory.IgnoreStrBuf();
                SDOMElement *newElem = domParser.m_Factory.NextElement(
                            (TXMLCharPtr)sreader.name().toUtf8().data());
                if (domParser.m_FirstElement == NULL) {
                    domParser.m_FirstElement = newElem;
                    domParser.m_TopElement = newElem;
                } else {
                    domParser.m_TopElement->AddChild(*newElem);
                    domParser.m_TopElement = newElem;
                }
                const QXmlStreamAttributes& attributes = sreader.attributes();
                for (auto attrib : attributes) {
                    SDOMAttribute *att = domParser.m_Factory.NextAttribute(
                                (TXMLCharPtr)attrib.name().toUtf8().data(),
                                (TXMLCharPtr)attrib.value().toUtf8().data());
                    newElem->AddAttribute(*att);
                }
            } else if (token == QXmlStreamReader::Characters) {
                QByteArray text = sreader.text().toUtf8();
                domParser.m_Factory.AppendStrBuf(text.data(),text.length());
            } else if (token == QXmlStreamReader::EndElement) {
                domParser.m_TopElement->m_Value = domParser.m_Factory.FinalizeStrBuf();
                domParser.m_TopElement = domParser.m_TopElement->m_Parent;
            }
            if (sreader.hasError()) {
                if (handler) {
                    handler->OnXmlError(sreader.errorString(), sreader.lineNumber(),
                                        sreader.columnNumber());
                } else  {
                    qWarning() << "XML parse error:" << sreader.errorString()
                               << "line:" << sreader.lineNumber()
                               << "column:" << sreader.columnNumber();
                }
                return nullptr;
            }
        }
        return domParser.m_FirstElement;
    }
};

class SimpleDomFactory : public IDOMFactory
{
    typedef eastl::basic_string<char8_t> TNarrowStr;
    Pool<SDOMElement> m_ElementPool;
    Pool<SDOMAttribute> m_AttributePool;
    eastl::vector<char8_t *> m_BigStrings;
    eastl::vector<char8_t> m_StringBuilder;
    TNarrowStr m_ConvertBuffer;
    std::shared_ptr<qt3dsdm::IStringTable> m_StringTable;

public:
    SimpleDomFactory(std::shared_ptr<qt3dsdm::IStringTable> strt)
        : m_StringTable(strt)
    {
    }
    ~SimpleDomFactory()
    {
        QT3DSXML_FOREACH(idx, m_BigStrings.size())
        free(m_BigStrings[idx]);
    }

    TWideXMLCharPtr RegisterStr(TWideXMLCharPtr str)
    {
        if (str == NULL || *str == 0)
            return L"";
        return m_StringTable->RegisterStr(str);
    }

    TXMLCharPtr RegisterStr(TXMLCharPtr str)
    {
        if (str == NULL || *str == 0)
            return "";
        return m_StringTable->RegisterStr(str);
    }

    void Release() override { delete this; }
    void AppendStrBuf(TXMLCharPtr str, QT3DSU32 len) override
    {
        if (len && *str) {
            QT3DSU32 offset = m_StringBuilder.size();
            m_StringBuilder.resize(offset + len);
            memCopy(&m_StringBuilder[0] + offset, str, len * sizeof(char8_t));
        }
    }
    // Str does not need to be null terminated.
    void AppendStrBuf(TWideXMLCharPtr str, QT3DSU32 len) override
    {
        if (len && *str) {
            const TWCharEASTLConverter::TCharType *bufPtr =
                reinterpret_cast<const TWCharEASTLConverter::TCharType *>(str);
            ConvertUTF(bufPtr, len, m_ConvertBuffer);
            AppendStrBuf(m_ConvertBuffer.data(), m_ConvertBuffer.size());
        }
    }

    // Null terminate what is there and return the buffer.
    // This pointer needs to be persistent.
    TXMLCharPtr FinalizeStrBuf() override
    {
        if (m_StringBuilder.size() == 0)
            return "";
        m_StringBuilder.push_back(0);
        QT3DSU32 len = m_StringBuilder.size();
        QT3DSU32 numBytes = len * sizeof(char8_t);
        char8_t *newMem = (char8_t *)malloc(numBytes);
        memCopy(newMem, &m_StringBuilder[0], numBytes);
        m_BigStrings.push_back(newMem);
        m_StringBuilder.clear();
        return newMem;
    }
    void IgnoreStrBuf() override { m_StringBuilder.clear(); }

    SDOMAttribute *NextAttribute(TXMLCharPtr name, TXMLCharPtr val) override
    {
        TXMLCharPtr n(m_StringTable->GetNarrowStr(name));
        TXMLCharPtr v(RegisterValue(val));
        return m_AttributePool.construct(n, v, __FILE__, __LINE__);
    }

    SDOMAttribute *NextAttribute(TWideXMLCharPtr name, TWideXMLCharPtr val) override
    {
        TXMLCharPtr n(m_StringTable->GetNarrowStr(name));
        TXMLCharPtr v(RegisterValue(val));
        return m_AttributePool.construct(n, v, __FILE__, __LINE__);
    }

    SDOMElement *NextElement(TXMLCharPtr name) override
    {
        IgnoreStrBuf();
        TXMLCharPtr n(m_StringTable->GetNarrowStr(name));
        return m_ElementPool.construct(n, __FILE__, __LINE__);
    }
    SDOMElement *NextElement(TWideXMLCharPtr name) override
    {
        IgnoreStrBuf();
        TXMLCharPtr n(m_StringTable->GetNarrowStr(name));
        return m_ElementPool.construct(n, __FILE__, __LINE__);
    }

    std::shared_ptr<qt3dsdm::IStringTable> GetStringTable() override { return m_StringTable; }
};

bool IDOMReader::Value(DataModelDataType::Value type, SValue &outValue)
{
    TXMLCharPtr value;
    if (Value(value)) {
        WCharTReader reader(const_cast<char8_t *>(value), m_TempBuf, *GetStringTable());
        outValue = WStrOps<SValue>().BufTo(type, reader);
        return true;
    }
    return false;
}

std::shared_ptr<IDOMReader>
IDOMReader::CreateDOMReader(SDOMElement &inRootElement,
                            std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                            std::shared_ptr<IDOMFactory> inFactory)
{
    return std::make_shared<SDOMReader>(std::ref(inRootElement), std::ref(inStringTable),
                                          inFactory);
}

std::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>>
IDOMWriter::CreateDOMWriter(std::shared_ptr<IDOMFactory> inFactory, SDOMElement &inRootElement,
                            std::shared_ptr<qt3dsdm::IStringTable> inStringTable)
{
    std::shared_ptr<SDOMWriter> writer(std::make_shared<SDOMWriter>(
        inFactory, std::ref(inStringTable), std::ref(inRootElement)));
    return std::make_pair(writer, writer);
}

TXMLCharPtr IDOMFactory::RegisterValue(TWideXMLCharPtr inValue)
{
    if (IsTrivial(inValue))
        return "";
    IgnoreStrBuf();
    AppendStrBuf(inValue, (QT3DSU32)wcslen(inValue));
    return FinalizeStrBuf();
}

TXMLCharPtr IDOMFactory::RegisterValue(TXMLCharPtr inValue)
{
    if (IsTrivial(inValue))
        return "";
    IgnoreStrBuf();
    AppendStrBuf(inValue, (QT3DSU32)strlen(inValue));
    return FinalizeStrBuf();
}

std::shared_ptr<IDOMFactory>
IDOMFactory::CreateDOMFactory(std::shared_ptr<qt3dsdm::IStringTable> inStrTable)
{
    return std::make_shared<SimpleDomFactory>(std::ref(inStrTable));
}

void CDOMSerializer::WriteXMLHeader(QIODevice &inStream)
{
    SimpleXmlWriter writer(inStream);
    writer.Write("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
}

// Lexigraphically sort the attributes.
struct SAttributeComparator
{
    bool operator()(const SDOMAttribute *lhs, const SDOMAttribute *rhs)
    {
        return strcmp(lhs->m_Name, rhs->m_Name) < 0;
    }
};

// Write an element with attributes sorted by name so a file diff is effective.
void WriteElement(SDOMElement &inElement, SimpleXmlWriter &inWriter,
                  std::vector<SDOMAttribute *> &inAttSorter)
{
    inAttSorter.clear();
    for (SDOMAttribute *att = inElement.m_FirstAttribute; att; att = att->m_NextAttribute)
        inAttSorter.push_back(att);
    // We decided that we don't want attribute sorting; the code that adds attributes needs
    // to be consistent.
    // std::sort( inAttSorter.begin(), inAttSorter.end(), SAttributeComparator() );
    // This element doesn't add anything to the system in this case.
    if (inElement.m_FirstAttribute == NULL && inElement.m_FirstChild == NULL
        && IsTrivial(inElement.m_Value))
        return;

    inWriter.Begin(inElement.m_Name);

    const char8_t *theLastAttName = 0;
    for (size_t idx = 0, end = inAttSorter.size(); idx < end; ++idx) {
        SDOMAttribute *theAtt(inAttSorter[idx]);
        if (theAtt->m_Name != theLastAttName)
            inWriter.Att(theAtt->m_Name, theAtt->m_Value);
        else {
            QT3DS_ASSERT(false);
        }
        theLastAttName = theAtt->m_Name;
    }
    // Elements can either have children or values but not both at this point.
    if (inElement.m_FirstChild) {
        for (SDOMElement *theChild = inElement.m_FirstChild; theChild;
             theChild = theChild->m_NextSibling)
            WriteElement(*theChild, inWriter, inAttSorter);
        inWriter.End();
    } else {
        if (!IsTrivial(inElement.m_Value))
            inWriter.Value(inElement.m_Value);

        inWriter.End();
    }
}

void CDOMSerializer::Write(SDOMElement &inElement, QIODevice &inStream, QT3DSU32 inTabs)
{
    // TODO: QXmlStreamWriter here?
    std::vector<SDOMAttribute *> theAttributes;
    SimpleXmlWriter writer(inStream, inTabs);
    std::vector<SDOMAttribute *> theAttSorter;
    WriteElement(inElement, writer, theAttSorter);
}

SDOMElement *CDOMSerializer::Read(IDOMFactory &inFactory, QIODevice &inStream,
                                  CXmlErrorHandler *inErrorHandler)
{
    return DOMParser::ParseXMLFile(inFactory, inStream, inErrorHandler);
}

SDOMElement *CDOMSerializer::Read(IDOMFactory &inFactory, const QByteArray &inData,
                                  CXmlErrorHandler *inErrorHandler)
{
    return DOMParser::ParseXMLFile(inFactory, inData, inErrorHandler);
}
