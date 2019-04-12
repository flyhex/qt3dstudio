/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef QT3DS_SCENE_GRAPH_DEBUGGER_PROTOCOL_H
#define QT3DS_SCENE_GRAPH_DEBUGGER_PROTOCOL_H
#include "Qt3DSSceneGraphDebugger.h"
#include "Qt3DSSceneGraphDebuggerValue.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "foundation/SerializationTypes.h"

namespace qt3ds {
namespace state {
    namespace debugger {

        // These are the datatypes we will communicate information with
        static QT3DSU32 GetSceneGraphProtocolVersion() { return 1; }

        struct SValueUpdate
        {
            QT3DSI32 m_Hash;
            SSGValue m_Value;
            SValueUpdate(QT3DSI32 h, const SSGValue &v)
                : m_Hash(h)
                , m_Value(v)
            {
            }
            SValueUpdate()
                : m_Hash(0)
            {
            }
            template <typename Listener>
            void IterateProperties(Listener &inListener)
            {
                inListener.Handle(m_Hash);
                inListener.Handle(m_Value);
            };
        };

        struct SElemUpdate
        {
            QT3DSU64 m_Elem;
            NVDataRef<SValueUpdate> m_Updates;
            template <typename Listener>
            void IterateProperties(Listener &inListener)
            {
                inListener.Handle(m_Elem);
                inListener.HandleRef(m_Updates);
            };
        };

        struct SElemMap
        {
            QT3DSU64 m_Elem;
            CRegisteredString m_Id;
            SElemMap()
                : m_Elem(0)
            {
            }
            SElemMap(QT3DSU64 ptr, CRegisteredString name)
                : m_Elem(ptr)
                , m_Id(name)
            {
            }

            template <typename Listener>
            void IterateProperties(Listener &inListener)
            {
                inListener.Handle(m_Elem);
                inListener.Handle(m_Id);
            }
        };

        struct SIdUpdate
        {
            QT3DSU64 m_Presentation;
            CRegisteredString m_PresentationId;
            NVDataRef<SElemMap> m_IdUpdates;

            template <typename Listener>
            void IterateProperties(Listener &inListener)
            {
                inListener.Handle(m_Presentation);
                inListener.Handle(m_PresentationId);
                inListener.HandleRef(m_IdUpdates);
            }
        };

        struct SSGProtocolMessageTypes
        {
            enum Enum {
                UnknownMessage = 0,
                Initialization,
                IdUpdate,
                ElemUpdate,
                Frame,
            };
        };

        // Implemented on runtime side.
        struct SSGProtocolWriter
        {
            IOutStream &m_Stream;
            MemoryBuffer<> m_WriteBuffer;
            QT3DSU32 m_HighWaterMark;

            SSGProtocolWriter(IOutStream &s, NVAllocatorCallback &alloc, QT3DSU32 highWaterMark = 4096)
                : m_Stream(s)
                , m_WriteBuffer(ForwardingAllocator(alloc, "WriteBuffer"))
                , m_HighWaterMark(highWaterMark)
            {
            }

            void Handle(QT3DSU64 data) { m_WriteBuffer.write(data); }

            void Handle(QT3DSI32 data) { m_WriteBuffer.write(data); }

            void Handle(CRegisteredString str)
            {
                QT3DSU32 len = static_cast<QT3DSU32>(strlen(str.c_str()) + 1);
                m_WriteBuffer.write(len);
                m_WriteBuffer.write(str.c_str(), len);
            }
            void Handle(SSGValue &value)
            {
                QT3DSU32 valType = static_cast<QT3DSU32>(value.getType());
                m_WriteBuffer.write(valType);
                switch (value.getType()) {
                case SGPropertyValueTypes::Float:
                    m_WriteBuffer.write(value.getData<float>());
                    break;
                case SGPropertyValueTypes::I32:
                    m_WriteBuffer.write(value.getData<QT3DSI32>());
                    break;
                case SGPropertyValueTypes::String:
                    Handle(value.getData<CRegisteredString>());
                    break;
                case SGPropertyValueTypes::Elem:
                    Handle(value.getData<QT3DSU64>());
                    break;
                case SGPropertyValueTypes::NoSGValue:
                    break;
                default:
                    QT3DS_ASSERT(false);
                }
            }

            template <typename TDataType>
            void HandleRef(NVDataRef<TDataType> &ref)
            {
                m_WriteBuffer.write(ref.size());
                for (QT3DSU32 idx = 0, end = ref.size(); idx < end; ++idx)
                    Handle(ref[idx]);
            }

            template <typename TDataType>
            void Handle(TDataType &dtype)
            {
                dtype.IterateProperties(*this);
            }

            void Flush()
            {
                if (m_WriteBuffer.size()) {
                    NVConstDataRef<QT3DSU8> writeData(m_WriteBuffer);
                    m_Stream.Write(writeData);
                    m_WriteBuffer.clear();
                }
            }

            void CheckBuffer()
            {
                if (m_WriteBuffer.size() > m_HighWaterMark)
                    Flush();
            }

            void Write(SIdUpdate &inIdUpdate)
            {
                m_WriteBuffer.write((QT3DSU32)SSGProtocolMessageTypes::IdUpdate);
                inIdUpdate.IterateProperties(*this);
                Flush();
            }

            void Write(SElemUpdate &inIdUpdate)
            {
                m_WriteBuffer.write((QT3DSU32)SSGProtocolMessageTypes::ElemUpdate);
                inIdUpdate.IterateProperties(*this);
                CheckBuffer();
            }

            void WriteInitialization()
            {
                m_WriteBuffer.write((QT3DSU32)SSGProtocolMessageTypes::Initialization);
                m_WriteBuffer.write(GetSceneGraphProtocolVersion());
                Flush();
            }
            void WriteFrame()
            {
                m_WriteBuffer.write((QT3DSU32)SSGProtocolMessageTypes::Frame);
                Flush();
            }
        };

        struct SSGProtocolReader
        {
            NVConstDataRef<QT3DSU8> m_Message;
            SDataReader m_Reader;
            IStringTable &m_StringTable;
            eastl::vector<QT3DSU8> m_DataBuffer;
            eastl::string m_TempString;
            QT3DSU32 m_Allocated;
            bool m_RestartRead;
            SSGProtocolReader(NVConstDataRef<QT3DSU8> msg, IStringTable &strTable)
                : m_Message(msg)
                , m_Reader(const_cast<QT3DSU8 *>(msg.begin()), const_cast<QT3DSU8 *>(msg.end()))
                , m_StringTable(strTable)
                , m_Allocated(0)
                , m_RestartRead(false)
            {
            }

            SSGProtocolMessageTypes::Enum MessageType()
            {
                QT3DSU32 data = m_Reader.LoadRef<QT3DSU32>();
                return static_cast<SSGProtocolMessageTypes::Enum>(data);
            }

            template <typename TDataType>
            Option<NVDataRef<TDataType>> AllocateData(size_t size)
            {
                if (m_RestartRead)
                    return Empty();
                if (size == 0)
                    return NVDataRef<TDataType>();

                QT3DSU32 current = m_Allocated;
                QT3DSU32 newAlloc = (QT3DSU32)(size * sizeof(TDataType));
                // 8 byte align
                if (newAlloc % 8)
                    newAlloc += 8 - (newAlloc % 8);

                QT3DSU32 required = current + newAlloc;

                if (required > m_DataBuffer.size()) {
                    m_RestartRead = true;
                    m_DataBuffer.resize(required * 2);
                    return Empty();
                }
                TDataType *offset = reinterpret_cast<TDataType *>(&m_DataBuffer[current]);
                m_Allocated += newAlloc;
                return toDataRef(offset, (QT3DSU32)size);
            }

            void Handle(QT3DSU64 &data) { data = m_Reader.LoadRef<QT3DSU64>(); }

            void Handle(QT3DSI32 &data) { data = m_Reader.LoadRef<QT3DSI32>(); }

            void Handle(CRegisteredString &str)
            {
                QT3DSU32 len = m_Reader.LoadRef<QT3DSU32>();
                m_TempString.clear();
                if (len)
                    m_TempString.assign((const char *)m_Reader.m_CurrentPtr, (size_t)(len - 1));
                m_Reader.m_CurrentPtr += len;
                if (m_Reader.m_CurrentPtr > m_Reader.m_EndPtr)
                    m_Reader.m_CurrentPtr = m_Reader.m_EndPtr;

                str = m_StringTable.RegisterStr(m_TempString.c_str());
            }

            void Handle(SSGValue &value)
            {
                QT3DSU32 valType = m_Reader.LoadRef<QT3DSU32>();
                switch (valType) {
                case SGPropertyValueTypes::Float:
                    value = SSGValue(m_Reader.LoadRef<float>());
                    break;
                case SGPropertyValueTypes::I32:
                    value = SSGValue(m_Reader.LoadRef<QT3DSI32>());
                    break;
                case SGPropertyValueTypes::String: {
                    CRegisteredString temp;
                    Handle(temp);
                    value = SSGValue(temp);
                } break;
                case SGPropertyValueTypes::Elem:
                    value = m_Reader.LoadRef<QT3DSU64>();
                    break;
                case SGPropertyValueTypes::NoSGValue:
                    break;
                default:
                    QT3DS_ASSERT(false);
                }
            }

            template <typename TDataType>
            void HandleRef(NVDataRef<TDataType> &ref)
            {
                QT3DSU32 numItems = m_Reader.LoadRef<QT3DSU32>();
                Option<NVDataRef<TDataType>> refOpt = AllocateData<TDataType>(numItems);
                if (refOpt.hasValue()) {
                    ref = *refOpt;
                    for (QT3DSU32 idx = 0, end = ref.size(); idx < end && m_RestartRead == false;
                         ++idx)
                        Handle(ref[idx]);
                }
            }

            template <typename TDataType>
            void Handle(TDataType &dtype)
            {
                dtype.IterateProperties(*this);
            }

            template <typename TDataType>
            void DoRead(TDataType &ioValue)
            {
                QT3DSU8 *startPtr = m_Reader.m_CurrentPtr;
                QT3DSU32 restartCount = 0;
                do {
                    m_RestartRead = false;
                    m_Allocated = 0;
                    m_Reader.m_CurrentPtr = startPtr;
                    ioValue.IterateProperties(*this);
                    ++restartCount;
                } while (m_RestartRead);
            }

            SIdUpdate ReadIdUpdate()
            {
                SIdUpdate retval;
                DoRead(retval);
                return retval;
            };

            SElemUpdate ReadElemUpdate()
            {
                SElemUpdate retval;
                DoRead(retval);
                return retval;
            };

            QT3DSU32 ReadInitialization() { return m_Reader.LoadRef<QT3DSU32>(); }

            bool Finished() { return m_Reader.m_CurrentPtr >= m_Reader.m_EndPtr; }
        };
    }
}
}
#endif
