/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ArgumentEncoder.h"

#include <algorithm>
#include <stdio.h>

namespace CoreIPC {

PassOwnPtr<ArgumentEncoder> ArgumentEncoder::create(uint64_t destinationID)
{
    return adoptPtr(new ArgumentEncoder(destinationID));
}

ArgumentEncoder::ArgumentEncoder(uint64_t destinationID)
    : m_buffer(0)
    , m_bufferPointer(0)
    , m_bufferSize(0)
    , m_bufferCapacity(0)
{
    // Encode the destination ID.
    encodeUInt64(destinationID);
}

ArgumentEncoder::~ArgumentEncoder()
{
    if (m_buffer)
        fastFree(m_buffer);
#if !PLATFORM(QT)
    // FIXME: We need to dispose of the attachments in cases of failure.
#else
    for (int i = 0; i < m_attachments.size(); ++i)
        m_attachments[i].dispose();
#endif
}

static inline size_t roundUpToAlignment(size_t value, unsigned alignment)
{
    return ((value + alignment - 1) / alignment) * alignment;
}

uint8_t* ArgumentEncoder::grow(unsigned alignment, size_t size)
{
    size_t alignedSize = roundUpToAlignment(m_bufferSize, alignment);
    
    if (alignedSize + size > m_bufferCapacity) {
        size_t newCapacity = std::max(alignedSize + size, std::max(static_cast<size_t>(32), m_bufferCapacity + m_bufferCapacity / 4 + 1));
        if (!m_buffer)
            m_buffer = static_cast<uint8_t*>(fastMalloc(newCapacity));
        else
            m_buffer = static_cast<uint8_t*>(fastRealloc(m_buffer, newCapacity));
        
        // FIXME: What should we do if allocating memory fails?

        m_bufferCapacity = newCapacity;        
    }

    m_bufferSize = alignedSize + size;
    m_bufferPointer = m_buffer + alignedSize + size;
    
    return m_buffer + alignedSize;
}

void ArgumentEncoder::encodeBytes(const uint8_t* bytes, size_t size)
{
    // Encode the size.
    encodeUInt64(static_cast<uint64_t>(size));
    
    uint8_t* buffer = grow(1, size);
    
    memcpy(buffer, bytes, size);
}

void ArgumentEncoder::encodeBool(bool n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast<bool*>(buffer) = n;
}

void ArgumentEncoder::encodeUInt32(uint32_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast<uint32_t*>(buffer) = n;
}

void ArgumentEncoder::encodeUInt64(uint64_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast<uint64_t*>(buffer) = n;
}

void ArgumentEncoder::encodeInt32(int32_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast<int32_t*>(buffer) = n;
}

void ArgumentEncoder::encodeInt64(int64_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast<int64_t*>(buffer) = n;
}

void ArgumentEncoder::encodeFloat(float n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));

    *reinterpret_cast<float*>(buffer) = n;
}

void ArgumentEncoder::encodeDouble(double n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));

    *reinterpret_cast<double*>(buffer) = n;
}

void ArgumentEncoder::addAttachment(const Attachment& attachment)
{
    m_attachments.append(attachment);
}

Vector<Attachment> ArgumentEncoder::releaseAttachments()
{
    Vector<Attachment> newList;
    newList.swap(m_attachments);
    return newList;
}

#ifndef NDEBUG
void ArgumentEncoder::debug()
{
    printf("ArgumentEncoder::debug()\n");
    printf("Number of Attachments: %d\n", (int)m_attachments.size());
    printf("Size of buffer: %d\n", (int)m_bufferSize);
}
#endif

} // namespace CoreIPC
