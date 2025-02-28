/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#ifndef WebURLLoaderClient_h
#define WebURLLoaderClient_h

namespace WebKit {

class WebURLLoader;
class WebURLRequest;
class WebURLResponse;
struct WebURLError;

class WebURLLoaderClient {
public:
    // Called when following a redirect.  |newRequest| contains the request
    // generated by the redirect.  The client may modify |newRequest|.
    virtual void willSendRequest(
        WebURLLoader*, WebURLRequest& newRequest, const WebURLResponse& redirectResponse) { }

    // Called to report upload progress.  The bytes reported correspond to
    // the HTTP message body.
    virtual void didSendData(
        WebURLLoader*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent) { }

    // Called when response headers are received.
    virtual void didReceiveResponse(WebURLLoader*, const WebURLResponse&) { }

    // Called when a chunk of response data is downloaded.  This is only called
    // if WebURLRequest's downloadToFile flag was set to true.
    virtual void didDownloadData(WebURLLoader*, int dataLength) { }

    // Called when a chunk of response data is received.
    // FIXME(vsevik): rename once original didReceiveData() is removed.
    virtual void didReceiveData2(WebURLLoader*, const char* data, int dataLength, int lengthReceived) { }

    // FIXME(vsevik): remove once not used downstream
    virtual void didReceiveData(WebURLLoader*, const char* data, int dataLength) { }

    // Called when a chunk of renderer-generated metadata is received from the cache.
    virtual void didReceiveCachedMetadata(WebURLLoader*, const char* data, int dataLength) { }

    // Called when the load completes successfully.
    virtual void didFinishLoading(WebURLLoader*, double finishTime) { }

    // Called when the load completes with an error.
    virtual void didFail(WebURLLoader*, const WebURLError&) { }

protected:
    ~WebURLLoaderClient() { }
};

} // namespace WebKit

#endif
