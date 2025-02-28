/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebIDBKey.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBBindingUtilities.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "SerializedScriptValue.h"
#include "WebIDBKeyPath.h"
#include "WebSerializedScriptValue.h"

using namespace WebCore;

namespace WebKit {

WebIDBKey WebIDBKey::createNull()
{
    WebIDBKey key;
    key.assignNull();
    return key;
}

WebIDBKey WebIDBKey::createString(const WebString& string)
{
    WebIDBKey key;
    key.assignString(string);
    return key;
}

WebIDBKey WebIDBKey::createDate(double date)
{
    WebIDBKey key;
    key.assignDate(date);
    return key;
}

WebIDBKey WebIDBKey::createNumber(double number)
{
    WebIDBKey key;
    key.assignNumber(number);
    return key;
}

WebIDBKey WebIDBKey::createInvalid()
{
    WebIDBKey key;
    key.assignInvalid();
    return key;
}

WebIDBKey WebIDBKey::createFromValueAndKeyPath(const WebSerializedScriptValue& serializedScriptValue, const WebIDBKeyPath& idbKeyPath)
{
    if (serializedScriptValue.isNull())
        return WebIDBKey::createInvalid();
    return WebCore::createIDBKeyFromSerializedValueAndKeyPath(serializedScriptValue, idbKeyPath);
}

WebSerializedScriptValue WebIDBKey::injectIDBKeyIntoSerializedValue(const WebIDBKey& key, const WebSerializedScriptValue& value, const WebIDBKeyPath& path)
{
    return WebCore::injectIDBKeyIntoSerializedValue(key, value, path);
}

void WebIDBKey::assign(const WebIDBKey& value)
{
    m_private = value.m_private;
}

void WebIDBKey::assignNull()
{
    m_private = IDBKey::createNull();
}

void WebIDBKey::assignString(const WebString& string)
{
    m_private = IDBKey::createString(string);
}

void WebIDBKey::assignDate(double date)
{
    m_private = IDBKey::createDate(date);
}

void WebIDBKey::assignNumber(double number)
{
    m_private = IDBKey::createNumber(number);
}

void WebIDBKey::assignInvalid()
{
    m_private = 0;
}

void WebIDBKey::reset()
{
    m_private.reset();
}

WebIDBKey::Type WebIDBKey::type() const
{
    if (!m_private.get())
        return InvalidType;
    return Type(m_private->type());
}

WebString WebIDBKey::string() const
{
    return m_private->string();
}

double WebIDBKey::date() const
{
    return m_private->date();
}

double WebIDBKey::number() const
{
    return m_private->number();
}

WebIDBKey::WebIDBKey(const PassRefPtr<IDBKey>& value)
    : m_private(value)
{
}

WebIDBKey& WebIDBKey::operator=(const PassRefPtr<IDBKey>& value)
{
    m_private = value;
    return *this;
}

WebIDBKey::operator PassRefPtr<IDBKey>() const
{
    return m_private.get();
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
