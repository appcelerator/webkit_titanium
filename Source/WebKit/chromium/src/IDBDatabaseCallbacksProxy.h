/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef IDBDatabaseCallbacksProxy_h
#define IDBDatabaseCallbacksProxy_h

#include "IDBDatabaseCallbacks.h"

#if ENABLE(INDEXED_DATABASE)

#include <wtf/PassOwnPtr.h>

namespace WebKit { class WebIDBDatabaseCallbacks; }

namespace WebCore {

class IDBDatabaseCallbacksProxy : public IDBDatabaseCallbacks {
public:
    static PassRefPtr<IDBDatabaseCallbacksProxy> create(PassOwnPtr<WebKit::WebIDBDatabaseCallbacks>);
    virtual ~IDBDatabaseCallbacksProxy();

    virtual void onVersionChange(const String& requestedVersion);

private:
    IDBDatabaseCallbacksProxy(PassOwnPtr<WebKit::WebIDBDatabaseCallbacks>);

    OwnPtr<WebKit::WebIDBDatabaseCallbacks> m_callbacks;
};

} // namespace WebCore

#endif

#endif // IDBDatabaseCallbacksProxy_h
