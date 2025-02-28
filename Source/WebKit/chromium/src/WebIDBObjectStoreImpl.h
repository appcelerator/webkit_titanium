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

#ifndef WebIDBObjectStoreImpl_h
#define WebIDBObjectStoreImpl_h

#include "WebCommon.h"
#include "WebIDBObjectStore.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore { class IDBObjectStoreBackendInterface; }

namespace WebKit {

class WebIDBIndex;

// See comment in WebIndexedObjectStore for a high level overview these classes.
class WebIDBObjectStoreImpl : public WebIDBObjectStore {
public:
    WebIDBObjectStoreImpl(WTF::PassRefPtr<WebCore::IDBObjectStoreBackendInterface>);
    ~WebIDBObjectStoreImpl();

    WebString name() const;
    WebString keyPath() const;
    WebDOMStringList indexNames() const;

    void get(const WebIDBKey& key, WebIDBCallbacks*, const WebIDBTransaction&, WebExceptionCode&);
    void put(const WebSerializedScriptValue&, const WebIDBKey&, PutMode, WebIDBCallbacks*, const WebIDBTransaction&, WebExceptionCode&);
    void deleteFunction(const WebIDBKey& key, WebIDBCallbacks*, const WebIDBTransaction&, WebExceptionCode&);
    void clear(WebIDBCallbacks*, const WebIDBTransaction&, WebExceptionCode&);

    WebIDBIndex* createIndex(const WebString& name, const WebString& keyPath, bool unique, const WebIDBTransaction&, WebExceptionCode&);
    WebIDBIndex* index(const WebString& name, WebExceptionCode&);
    void deleteIndex(const WebString& name, const WebIDBTransaction&, WebExceptionCode&);

    void openCursor(const WebIDBKeyRange&, unsigned short direction, WebIDBCallbacks*, const WebIDBTransaction&, WebExceptionCode&);

 private:
    WTF::RefPtr<WebCore::IDBObjectStoreBackendInterface> m_objectStore;
};

} // namespace WebKit

#endif // WebIDBObjectStoreImpl_h
