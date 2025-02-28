/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IDBObjectStoreProxy.h"

#include "DOMStringList.h"
#include "IDBCallbacks.h"
#include "IDBIndexBackendProxy.h"
#include "IDBKeyRange.h"
#include "IDBTransactionBackendProxy.h"
#include "WebIDBCallbacksImpl.h"
#include "WebIDBKeyRange.h"
#include "WebIDBIndex.h"
#include "WebIDBKey.h"
#include "WebIDBObjectStore.h"
#include "WebIDBTransactionImpl.h"
#include "WebSerializedScriptValue.h"

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

PassRefPtr<IDBObjectStoreBackendInterface> IDBObjectStoreProxy::create(PassOwnPtr<WebKit::WebIDBObjectStore> objectStore)
{
    return adoptRef(new IDBObjectStoreProxy(objectStore));
}

IDBObjectStoreProxy::IDBObjectStoreProxy(PassOwnPtr<WebKit::WebIDBObjectStore> objectStore)
    : m_webIDBObjectStore(objectStore)
{
}

IDBObjectStoreProxy::~IDBObjectStoreProxy()
{
}

String IDBObjectStoreProxy::name() const
{
    return m_webIDBObjectStore->name();
}

String IDBObjectStoreProxy::keyPath() const
{
    return m_webIDBObjectStore->keyPath();
}

PassRefPtr<DOMStringList> IDBObjectStoreProxy::indexNames() const
{
    return m_webIDBObjectStore->indexNames();
}

void IDBObjectStoreProxy::get(PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks> callbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    // The transaction pointer is guaranteed to be a pointer to a proxy object as, in the renderer,
    // all implementations of IDB interfaces are proxy objects.
    IDBTransactionBackendProxy* transactionProxy = static_cast<IDBTransactionBackendProxy*>(transaction);
    m_webIDBObjectStore->get(key, new WebIDBCallbacksImpl(callbacks), *transactionProxy->getWebIDBTransaction(), ec);
}

void IDBObjectStoreProxy::put(PassRefPtr<SerializedScriptValue> value, PassRefPtr<IDBKey> key, PutMode putMode, PassRefPtr<IDBCallbacks> callbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    // The transaction pointer is guaranteed to be a pointer to a proxy object as, in the renderer,
    // all implementations of IDB interfaces are proxy objects.
    IDBTransactionBackendProxy* transactionProxy = static_cast<IDBTransactionBackendProxy*>(transaction);
    m_webIDBObjectStore->put(value, key, static_cast<WebKit::WebIDBObjectStore::PutMode>(putMode), new WebIDBCallbacksImpl(callbacks), *transactionProxy->getWebIDBTransaction(), ec);
}

void IDBObjectStoreProxy::deleteFunction(PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks> callbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    // The transaction pointer is guaranteed to be a pointer to a proxy object as, in the renderer,
    // all implementations of IDB interfaces are proxy objects.
    IDBTransactionBackendProxy* transactionProxy = static_cast<IDBTransactionBackendProxy*>(transaction);
    m_webIDBObjectStore->deleteFunction(key, new WebIDBCallbacksImpl(callbacks), *transactionProxy->getWebIDBTransaction(), ec);
}

void IDBObjectStoreProxy::clear(PassRefPtr<IDBCallbacks> callbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    // The transaction pointer is guaranteed to be a pointer to a proxy object as, in the renderer,
    // all implementations of IDB interfaces are proxy objects.
    IDBTransactionBackendProxy* transactionProxy = static_cast<IDBTransactionBackendProxy*>(transaction);
    m_webIDBObjectStore->clear(new WebIDBCallbacksImpl(callbacks), *transactionProxy->getWebIDBTransaction(), ec);
}

PassRefPtr<IDBIndexBackendInterface> IDBObjectStoreProxy::createIndex(const String& name, const String& keyPath, bool unique, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    // The transaction pointer is guaranteed to be a pointer to a proxy object as, in the renderer,
    // all implementations of IDB interfaces are proxy objects.
    IDBTransactionBackendProxy* transactionProxy = static_cast<IDBTransactionBackendProxy*>(transaction);
    WebKit::WebIDBIndex* index = m_webIDBObjectStore->createIndex(name, keyPath, unique, *transactionProxy->getWebIDBTransaction(), ec);
    if (!index)
        return 0;
    return IDBIndexBackendProxy::create(index);
}

PassRefPtr<IDBIndexBackendInterface> IDBObjectStoreProxy::index(const String& name, ExceptionCode& ec)
{
    WebKit::WebIDBIndex* index = m_webIDBObjectStore->index(name, ec);
    if (!index)
        return 0;
    return IDBIndexBackendProxy::create(index);
}

void IDBObjectStoreProxy::deleteIndex(const String& name, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    // The transaction pointer is guaranteed to be a pointer to a proxy object as, in the renderer,
    // all implementations of IDB interfaces are proxy objects.
    IDBTransactionBackendProxy* transactionProxy = static_cast<IDBTransactionBackendProxy*>(transaction);
    m_webIDBObjectStore->deleteIndex(name, *transactionProxy->getWebIDBTransaction(), ec);
}

void IDBObjectStoreProxy::openCursor(PassRefPtr<IDBKeyRange> range, unsigned short direction, PassRefPtr<IDBCallbacks> callbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    // The transaction pointer is guaranteed to be a pointer to a proxy object as, in the renderer,
    // all implementations of IDB interfaces are proxy objects.
    IDBTransactionBackendProxy* transactionProxy = static_cast<IDBTransactionBackendProxy*>(transaction);
    m_webIDBObjectStore->openCursor(range, direction, new WebIDBCallbacksImpl(callbacks), *transactionProxy->getWebIDBTransaction(), ec);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
