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

#ifndef IDBTransactionCallbacksProxy_h
#define IDBTransactionCallbacksProxy_h

#include "IDBTransactionCallbacks.h"

#if ENABLE(INDEXED_DATABASE)

#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebKit { class WebIDBTransactionCallbacks; }

namespace WebCore {

class IDBTransactionCallbacksProxy : public IDBTransactionCallbacks {
public:
    static PassRefPtr<IDBTransactionCallbacksProxy> create(PassOwnPtr<WebKit::WebIDBTransactionCallbacks>);
    virtual ~IDBTransactionCallbacksProxy();

    virtual void onAbort();
    virtual void onComplete();

private:
    IDBTransactionCallbacksProxy(PassOwnPtr<WebKit::WebIDBTransactionCallbacks>);

    OwnPtr<WebKit::WebIDBTransactionCallbacks> m_callbacks;
};


} // namespace WebCore

#endif

#endif // IDBTransactionCallbacksProxy_h
