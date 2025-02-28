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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "IDBFactoryBackendProxy.h"

#include "DOMStringList.h"
#include "IDBDatabaseError.h"
#include "IDBDatabaseProxy.h"
#include "WebFrameImpl.h"
#include "WebIDBCallbacksImpl.h"
#include "WebIDBDatabase.h"
#include "WebIDBDatabaseError.h"
#include "WebIDBFactory.h"
#include "WebKit.h"
#include "WebKitClient.h"
#include "WebVector.h"

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

PassRefPtr<IDBFactoryBackendInterface> IDBFactoryBackendProxy::create()
{
    return adoptRef(new IDBFactoryBackendProxy());
}

IDBFactoryBackendProxy::IDBFactoryBackendProxy()
    : m_webIDBFactory(WebKit::webKitClient()->idbFactory())
{
}

IDBFactoryBackendProxy::~IDBFactoryBackendProxy()
{
}

void IDBFactoryBackendProxy::open(const String& name, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<SecurityOrigin> origin, Frame* frame, const String& dataDir, int64_t maximumSize, BackingStoreType backingStoreType)
{
    WebKit::WebFrame* webFrame = WebKit::WebFrameImpl::fromFrame(frame);
    m_webIDBFactory->open(name, new WebIDBCallbacksImpl(callbacks), origin, webFrame, dataDir, maximumSize, static_cast<WebKit::WebIDBFactory::BackingStoreType>(backingStoreType));
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
