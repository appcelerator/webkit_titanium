/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"

#if ENABLE(VIDEO) && USE(AVFOUNDATION)

#import "MediaPlayerPrivateAVFoundationObjC.h"

#import "ApplicationCacheResource.h"
#import "BlockExceptions.h"
#import "FloatConversion.h"
#import "FrameView.h"
#import "FloatConversion.h"
#import "GraphicsContext.h"
#import "KURL.h"
#import "Logging.h"
#import "SoftLinking.h"
#import "TimeRanges.h"
#import "WebCoreSystemInterface.h"
#import <objc/objc-runtime.h>
#import <wtf/UnusedParam.h>

#import <CoreMedia/CoreMedia.h>
#import <AVFoundation/AVFoundation.h>

SOFT_LINK_FRAMEWORK(AVFoundation)
SOFT_LINK_FRAMEWORK(CoreMedia)

SOFT_LINK(CoreMedia, CMTimeCompare, int32_t, (CMTime time1, CMTime time2), (time1, time2))
SOFT_LINK(CoreMedia, CMTimeMakeWithSeconds, CMTime, (Float64 seconds, int32_t preferredTimeScale), (seconds, preferredTimeScale))
SOFT_LINK(CoreMedia, CMTimeGetSeconds, Float64, (CMTime time), (time))
SOFT_LINK(CoreMedia, CMTimeRangeGetEnd, CMTime, (CMTimeRange range), (range))

SOFT_LINK_CLASS(AVFoundation, AVPlayer)
SOFT_LINK_CLASS(AVFoundation, AVPlayerItem)
SOFT_LINK_CLASS(AVFoundation, AVPlayerLayer)
SOFT_LINK_CLASS(AVFoundation, AVURLAsset)
SOFT_LINK_CLASS(AVFoundation, AVAssetImageGenerator)

SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicVisual, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicAudible, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeClosedCaption, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVPlayerItemDidPlayToEndTimeNotification, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAssetImageGeneratorApertureModeCleanAperture, NSString *)

SOFT_LINK_CONSTANT(CoreMedia, kCMTimeZero, CMTime)

#define AVPlayer getAVPlayerClass()
#define AVPlayerItem getAVPlayerItemClass()
#define AVPlayerLayer getAVPlayerLayerClass()
#define AVURLAsset getAVURLAssetClass()
#define AVAssetImageGenerator getAVAssetImageGeneratorClass()

#define AVMediaCharacteristicVisual getAVMediaCharacteristicVisual()
#define AVMediaCharacteristicAudible getAVMediaCharacteristicAudible()
#define AVMediaTypeClosedCaption getAVMediaTypeClosedCaption()
#define AVPlayerItemDidPlayToEndTimeNotification getAVPlayerItemDidPlayToEndTimeNotification()
#define AVAssetImageGeneratorApertureModeCleanAperture getAVAssetImageGeneratorApertureModeCleanAperture()

#define kCMTimeZero getkCMTimeZero()

using namespace WebCore;
using namespace std;

enum MediaPlayerAVFoundationObservationContext {
    MediaPlayerAVFoundationObservationContextPlayerItem,
    MediaPlayerAVFoundationObservationContextPlayer
};

@interface WebCoreAVFMovieObserver : NSObject
{
    MediaPlayerPrivateAVFoundationObjC* m_callback;
    int m_delayCallbacks;
}
-(id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
-(void)disconnect;
-(void)playableKnown;
-(void)metadataLoaded;
-(void)timeChanged:(double)time;
-(void)didEnd:(NSNotification *)notification;
-(void)observeValueForKeyPath:keyPath ofObject:(id)object change:(NSDictionary *)change context:(MediaPlayerAVFoundationObservationContext)context;
@end

namespace WebCore {

static NSArray *assetMetadataKeyNames();
static NSArray *itemKVOProperties();

#if !LOG_DISABLED
static const char *boolString(bool val)
{
    return val ? "true" : "false";
}
#endif

static const float invalidTime = -1.0f;

MediaPlayerPrivateInterface* MediaPlayerPrivateAVFoundationObjC::create(MediaPlayer* player) 
{ 
    return new MediaPlayerPrivateAVFoundationObjC(player);
}

void MediaPlayerPrivateAVFoundationObjC::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

MediaPlayerPrivateAVFoundationObjC::MediaPlayerPrivateAVFoundationObjC(MediaPlayer* player)
    : MediaPlayerPrivateAVFoundation(player)
    , m_objcObserver(AdoptNS, [[WebCoreAVFMovieObserver alloc] initWithCallback:this])
    , m_timeObserver(0)
{
}

MediaPlayerPrivateAVFoundationObjC::~MediaPlayerPrivateAVFoundationObjC()
{
    cancelLoad();
    [m_objcObserver.get() disconnect];
}

void MediaPlayerPrivateAVFoundationObjC::cancelLoad()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::cancelLoad(%p)", this);
    tearDownVideoRendering();

    [[NSNotificationCenter defaultCenter] removeObserver:m_objcObserver.get()];

    // Tell our observer to do nothing when our cancellation of pending loading calls its completion handler.
    setIgnoreLoadStateChanges(true);
    if (m_avAsset) {
        [m_avAsset.get() cancelLoading];
        m_avAsset = nil;
    }
    if (m_avPlayerItem) {
        for (NSString *keyName in itemKVOProperties())
            [m_avPlayerItem.get() removeObserver:m_objcObserver.get() forKeyPath:keyName];
        
        m_avPlayerItem = nil;
    }
    if (m_avPlayer) {
        if (m_timeObserver)
            [m_avPlayer.get() removeTimeObserver:m_timeObserver];
        [m_avPlayer.get() removeObserver:m_objcObserver.get() forKeyPath:@"rate"];
        m_avPlayer = nil;
    }
    setIgnoreLoadStateChanges(false);
}

bool MediaPlayerPrivateAVFoundationObjC::hasLayerRenderer() const
{
    return m_videoLayer;
}

bool MediaPlayerPrivateAVFoundationObjC::hasContextRenderer() const
{
    return m_imageGenerator;
}

void MediaPlayerPrivateAVFoundationObjC::createContextVideoRenderer()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createContextVideoRenderer(%p)", this);

    if (!m_avAsset || m_imageGenerator)
        return;

    m_imageGenerator = [AVAssetImageGenerator assetImageGeneratorWithAsset:m_avAsset.get()];

    [m_imageGenerator.get() setApertureMode:AVAssetImageGeneratorApertureModeCleanAperture];
    [m_imageGenerator.get() setAppliesPreferredTrackTransform:YES];

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageGenerator(%p) - returning %p", this, m_imageGenerator.get());
}

void MediaPlayerPrivateAVFoundationObjC::destroyContextVideoRenderer()
{
    if (!m_imageGenerator)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyContextVideoRenderer(%p) - destroying", this, m_imageGenerator.get());

    m_imageGenerator = 0;
}

void MediaPlayerPrivateAVFoundationObjC::createVideoLayer()
{
    if (!m_avPlayer)
        return;

    if (!m_videoLayer) {
        m_videoLayer.adoptNS([[AVPlayerLayer alloc] init]);
        [m_videoLayer.get() setPlayer:m_avPlayer.get()];
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createVideoLayer(%p) - returning", this, m_videoLayer.get());
    }
}

void MediaPlayerPrivateAVFoundationObjC::destroyVideoLayer()
{
    if (!m_videoLayer)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyVideoLayer(%p) - destroying", this, m_videoLayer.get());

    [m_videoLayer.get() setPlayer:nil];

    m_videoLayer = 0;
}

bool MediaPlayerPrivateAVFoundationObjC::videoLayerIsReadyToDisplay() const
{
    return (m_videoLayer && [m_videoLayer.get() isReadyForDisplay]);
}

void MediaPlayerPrivateAVFoundationObjC::createAVPlayerForURL(const String& url)
{
    setDelayCallbacks(true);

    if (!m_avAsset) {
        NSURL *cocoaURL = KURL(ParsedURLString, url);
        m_avAsset.adoptNS([[AVURLAsset alloc] initWithURL:cocoaURL options:nil]);
    }
    
    createAVPlayer();
}

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
void MediaPlayerPrivateAVFoundationObjC::createAVPlayerForCacheResource(ApplicationCacheResource* resource)
{
    // AVFoundation can't open arbitrary data pointers, so if this ApplicationCacheResource doesn't 
    // have a valid local path, just open the resource's original URL.
    if (resource->path().isEmpty()) {
        createAVPlayerForURL(resource->url());
        return;
    }
    
    setDelayCallbacks(true);

    if (!m_avAsset) {
        NSURL* localURL = [NSURL fileURLWithPath:resource->path()];
        m_avAsset.adoptNS([[AVURLAsset alloc] initWithURL:localURL options:nil]);
    }

    createAVPlayer();
}
#endif

void MediaPlayerPrivateAVFoundationObjC::createAVPlayer()
{
    if (!m_avPlayer) {
        m_avPlayer.adoptNS([[AVPlayer alloc] init]);
        
        [m_avPlayer.get() addObserver:m_objcObserver.get() forKeyPath:@"rate" options:nil context:(void *)MediaPlayerAVFoundationObservationContextPlayer];
        
        // Add a time observer, ask to be called infrequently because we don't really want periodic callbacks but
        // our observer will also be called whenever a seek happens.
        const double veryLongInterval = 60*60*60*24*30;
        WebCoreAVFMovieObserver *observer = m_objcObserver.get();
        m_timeObserver = [m_avPlayer.get() addPeriodicTimeObserverForInterval:CMTimeMakeWithSeconds(veryLongInterval, 10) queue:nil usingBlock:^(CMTime time){
            [observer timeChanged:CMTimeGetSeconds(time)];
        }];
    }

    if (!m_avPlayerItem) {
        // Create the player item so we can media data. 
        m_avPlayerItem.adoptNS([[AVPlayerItem alloc] initWithAsset:m_avAsset.get()]);
        
        [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()selector:@selector(didEnd:) name:AVPlayerItemDidPlayToEndTimeNotification object:m_avPlayerItem.get()];

        for (NSString *keyName in itemKVOProperties())
            [m_avPlayerItem.get() addObserver:m_objcObserver.get() forKeyPath:keyName options:nil context:(void *)MediaPlayerAVFoundationObservationContextPlayerItem];

        [m_avPlayer.get() replaceCurrentItemWithPlayerItem:m_avPlayerItem.get()];
    }

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::checkPlayability()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::checkPlayability(%p)", this);

    [m_avAsset.get() loadValuesAsynchronouslyForKeys:[NSArray arrayWithObject:@"playable"] completionHandler:^{
        [m_objcObserver.get() playableKnown];
    }];
}

void MediaPlayerPrivateAVFoundationObjC::beginLoadingMetadata()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::playabilityKnown(%p) - requesting metadata loading", this);
    [m_avAsset.get() loadValuesAsynchronouslyForKeys:[assetMetadataKeyNames() retain] completionHandler:^{
        [m_objcObserver.get() metadataLoaded];
    }];
}

MediaPlayerPrivateAVFoundation::ItemStatus MediaPlayerPrivateAVFoundationObjC::playerItemStatus() const
{
    if (!m_avPlayerItem)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusUnknown;

    AVPlayerItemStatus status = [m_avPlayerItem.get() status];
    if (status == AVPlayerItemStatusUnknown)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusUnknown;
    if (status == AVPlayerItemStatusFailed)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusFailed;
    if ([m_avPlayerItem.get() isPlaybackLikelyToKeepUp])
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackLikelyToKeepUp;
    if ([m_avPlayerItem.get() isPlaybackBufferFull])
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackBufferFull;
    if ([m_avPlayerItem.get() isPlaybackBufferEmpty])
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackBufferEmpty;

    return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusReadyToPlay;
}

PlatformMedia MediaPlayerPrivateAVFoundationObjC::platformMedia() const
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformMedia(%p)", this);
    PlatformMedia pm;
    pm.type = PlatformMedia::AVFoundationMediaPlayerType;
    pm.media.avfMediaPlayer = m_avPlayer.get();
    return pm;
}

PlatformLayer* MediaPlayerPrivateAVFoundationObjC::platformLayer() const
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformLayer(%p)", this);
    return m_videoLayer.get();
}

void MediaPlayerPrivateAVFoundationObjC::play()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::play(%p)", this);
    if (!metaDataAvailable())
        return;

    setDelayCallbacks(true);
    [m_avPlayer.get() setRate:requestedRate()];
    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::pause()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::pause(%p)", this);
    if (!metaDataAvailable())
        return;

    setDelayCallbacks(true);
    [m_avPlayer.get() setRate:nil];
    setDelayCallbacks(false);
}

float MediaPlayerPrivateAVFoundationObjC::platformDuration() const
{
    if (!metaDataAvailable() || !m_avPlayerItem)
        return 0;
    
    float duration;
    CMTime cmDuration = [m_avPlayerItem.get() duration];
    if (CMTIME_IS_NUMERIC(cmDuration))
        duration = narrowPrecisionToFloat(CMTimeGetSeconds(cmDuration));
    else if (CMTIME_IS_INDEFINITE(cmDuration))
        duration = numeric_limits<float>::infinity();
    else {
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::duration(%p) - invalid duration, returning 0", this);
        return 0;
    }

    return duration;
}

float MediaPlayerPrivateAVFoundationObjC::currentTime() const
{
    if (!metaDataAvailable() || !m_avPlayerItem)
        return 0;

    CMTime itemTime = [m_avPlayerItem.get() currentTime];
    if (CMTIME_IS_NUMERIC(itemTime))
        return narrowPrecisionToFloat(CMTimeGetSeconds(itemTime));

    return 0;
}

void MediaPlayerPrivateAVFoundationObjC::seekToTime(float time)
{
    // setCurrentTime generates several event callbacks, update afterwards.
    setDelayCallbacks(true);

    float now = currentTime();
    if (time != now)
        [m_avPlayerItem.get() seekToTime:CMTimeMakeWithSeconds(time, 600) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
    else {
        // Force a call to the "time changed" notifier manually because a seek to the current time is a noop 
        // so the seek will never seem to complete.
        [m_objcObserver.get() timeChanged:now];
    }

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::setVolume(float volume)
{
    if (!metaDataAvailable())
        return;

    [m_avPlayer.get() setVolume:volume];
}

void MediaPlayerPrivateAVFoundationObjC::setClosedCaptionsVisible(bool closedCaptionsVisible)
{
    if (!metaDataAvailable())
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setClosedCaptionsVisible(%p) - setting to %s", this, boolString(closedCaptionsVisible));
    [m_avPlayer.get() setClosedCaptionDisplayEnabled:closedCaptionsVisible];
}

float MediaPlayerPrivateAVFoundationObjC::rate() const
{
    if (!metaDataAvailable())
        return 0;

    return [m_avPlayer.get() rate];
}

PassRefPtr<TimeRanges> MediaPlayerPrivateAVFoundationObjC::platformBufferedTimeRanges() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();

    if (!m_avPlayerItem)
        return timeRanges.release();

    NSArray *loadedRanges = [m_avPlayerItem.get() loadedTimeRanges];
    for (NSValue *thisRangeValue in loadedRanges) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (CMTIMERANGE_IS_VALID(timeRange) && !CMTIMERANGE_IS_EMPTY(timeRange)) {
            float rangeStart = narrowPrecisionToFloat(CMTimeGetSeconds(timeRange.start));
            float rangeEnd = narrowPrecisionToFloat(CMTimeGetSeconds(CMTimeRangeGetEnd(timeRange)));
            timeRanges->add(rangeStart, rangeEnd);
        }
    }
    return timeRanges.release();
}

float MediaPlayerPrivateAVFoundationObjC::platformMaxTimeSeekable() const
{
    NSArray *seekableRanges = [m_avPlayerItem.get() seekableTimeRanges];
    if (!seekableRanges)
        return 0;

    float maxTimeSeekable = 0;
    for (NSValue *thisRangeValue in seekableRanges) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;
        
        float endOfRange = narrowPrecisionToFloat(CMTimeGetSeconds(CMTimeRangeGetEnd(timeRange)));
        if (maxTimeSeekable < endOfRange)
            maxTimeSeekable = endOfRange;
    }
    return maxTimeSeekable;   
}

float MediaPlayerPrivateAVFoundationObjC::platformMaxTimeLoaded() const
{
    NSArray *loadedRanges = [m_avPlayerItem.get() loadedTimeRanges];
    if (!loadedRanges)
        return 0;

    float maxTimeLoaded = 0;
    for (NSValue *thisRangeValue in loadedRanges) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;
        
        float endOfRange = narrowPrecisionToFloat(CMTimeGetSeconds(CMTimeRangeGetEnd(timeRange)));
        if (maxTimeLoaded < endOfRange)
            maxTimeLoaded = endOfRange;
    }

    return maxTimeLoaded;   
}

unsigned MediaPlayerPrivateAVFoundationObjC::totalBytes() const
{
    if (!metaDataAvailable())
        return 0;

    long long totalMediaSize = 0;
    NSArray *tracks = [m_avAsset.get() tracks];
    for (AVAssetTrack *thisTrack in tracks)
        totalMediaSize += [thisTrack totalSampleDataLength];

    return static_cast<unsigned>(totalMediaSize);
}

void MediaPlayerPrivateAVFoundationObjC::setAsset(id asset)
{
    m_avAsset = asset;
}

MediaPlayerPrivateAVFoundation::AVAssetStatus MediaPlayerPrivateAVFoundationObjC::assetStatus() const
{
    if (!m_avAsset)
        return MediaPlayerAVAssetStatusUnknown;

    for (NSString *keyName in assetMetadataKeyNames()) {
        AVKeyValueStatus keyStatus = [m_avAsset.get() statusOfValueForKey:keyName error:nil];
        if (keyStatus < AVKeyValueStatusLoaded)
            return MediaPlayerAVAssetStatusLoading;// At least one key is not loaded yet.
        
        if (keyStatus == AVKeyValueStatusFailed)
            return MediaPlayerAVAssetStatusFailed; // At least one key could not be loaded.
        if (keyStatus == AVKeyValueStatusCancelled)
            return MediaPlayerAVAssetStatusCancelled; // Loading of at least one key was cancelled.
    }

    if ([[m_avAsset.get() valueForKey:@"playable"] boolValue])
        return MediaPlayerAVAssetStatusPlayable;

    return MediaPlayerAVAssetStatusLoaded;
}

void MediaPlayerPrivateAVFoundationObjC::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (!metaDataAvailable() || context->paintingDisabled())
        return;

    paint(context, rect);
}

void MediaPlayerPrivateAVFoundationObjC::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!metaDataAvailable() || context->paintingDisabled())
        return;

    setDelayCallbacks(true);
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    RetainPtr<CGImageRef> image = createImageForTimeInRect(currentTime(), rect);
    if (image) {
        context->save();
        context->translate(rect.x(), rect.y() + rect.height());
        context->scale(FloatSize(1.0f, -1.0f));
        context->setImageInterpolationQuality(InterpolationLow);
        IntRect paintRect(IntPoint(0, 0), IntSize(rect.width(), rect.height()));
        CGContextDrawImage(context->platformContext(), CGRectMake(0, 0, paintRect.width(), paintRect.height()), image.get());
        context->restore();
        image = 0;
    }

    END_BLOCK_OBJC_EXCEPTIONS;
    setDelayCallbacks(false);

    MediaPlayerPrivateAVFoundation::paint(context, rect);
}

static HashSet<String> mimeTypeCache()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;

    if (typeListInitialized)
        return cache;
    typeListInitialized = true;

    NSArray *types = [AVURLAsset audiovisualMIMETypes];
    for (NSString *mimeType in types)
        cache.add(mimeType);

    return cache;
} 

RetainPtr<CGImageRef> MediaPlayerPrivateAVFoundationObjC::createImageForTimeInRect(float time, const IntRect& rect)
{
    if (!m_imageGenerator)
        createContextVideoRenderer();
    ASSERT(m_imageGenerator);

#if !LOG_DISABLED
    double start = WTF::currentTime();
#endif

    [m_imageGenerator.get() setMaximumSize:CGSize(rect.size())];
    CGImageRef image = [m_imageGenerator.get() copyCGImageAtTime:CMTimeMakeWithSeconds(time, 600) actualTime:nil error:nil];

#if !LOG_DISABLED
    double duration = WTF::currentTime() - start;
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageForTimeInRect(%p) - creating image took %.4f", this, narrowPrecisionToFloat(duration));
#endif

    return image;
}

void MediaPlayerPrivateAVFoundationObjC::getSupportedTypes(HashSet<String>& supportedTypes)
{
    supportedTypes = mimeTypeCache();
} 

MediaPlayer::SupportsType MediaPlayerPrivateAVFoundationObjC::supportsType(const String& type, const String& codecs)
{
    // Only return "IsSupported" if there is no codecs parameter for now as there is no way to ask if it supports an
    // extended MIME type until rdar://6220037 is fixed.
    if (mimeTypeCache().contains(type))
        return codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;

    return MediaPlayer::IsNotSupported;
}

bool MediaPlayerPrivateAVFoundationObjC::isAvailable()
{
    return true;
}

float MediaPlayerPrivateAVFoundationObjC::mediaTimeForTimeValue(float timeValue) const
{
    if (!metaDataAvailable())
        return timeValue;

    // FIXME - impossible to implement until rdar://8721510 is fixed.
    return timeValue;
}

void MediaPlayerPrivateAVFoundationObjC::tracksChanged()
{
    // This is called whenever the tracks collection changes so cache hasVideo and hasAudio since we get
    // asked about those fairly fequently.
    setHasVideo([[m_avAsset.get() tracksWithMediaCharacteristic:AVMediaCharacteristicVisual] count]);
    setHasAudio([[m_avAsset.get() tracksWithMediaCharacteristic:AVMediaCharacteristicAudible] count]);
    setHasClosedCaptions([[m_avAsset.get() tracksWithMediaType:AVMediaTypeClosedCaption] count]);

    sizeChanged();
}

void MediaPlayerPrivateAVFoundationObjC::sizeChanged()
{
    NSArray *tracks = [m_avAsset.get() tracks];

    // Some assets don't report track properties until they are completely ready to play, but we
    // want to report a size as early as possible so use presentationSize when an asset has no tracks.
    if (![tracks count]) {
        setNaturalSize(IntSize([m_avPlayerItem.get() presentationSize]));
        return;
    }

    // AVAsset's 'naturalSize' property only considers the movie's first video track, so we need to compute
    // the union of all visual track rects.
    CGRect trackUnionRect = CGRectZero;
    for (AVAssetTrack *track in tracks) {
        CGSize trackSize = [track naturalSize];
        CGRect trackRect = CGRectMake(0, 0, trackSize.width, trackSize.height);
        trackUnionRect = CGRectUnion(trackUnionRect, CGRectApplyAffineTransform(trackRect, [track preferredTransform]));
    }

    // The movie is always displayed at 0,0 so move the track rect to the origin before using width and height.
    trackUnionRect = CGRectOffset(trackUnionRect, trackUnionRect.origin.x, trackUnionRect.origin.y);
    
    // Also look at the asset's preferred transform so we account for a movie matrix.
    CGSize naturalSize = CGSizeApplyAffineTransform(trackUnionRect.size, [m_avAsset.get() preferredTransform]);

    // Cache the natural size (setNaturalSize will notify the player if it has changed).
    setNaturalSize(IntSize(naturalSize));
}

NSArray* assetMetadataKeyNames()
{
    static NSArray* keys;
    if (!keys) {
        keys = [[NSArray alloc] initWithObjects:@"duration",
                    @"naturalSize",
                    @"preferredTransform",
                    @"preferredVolume",
                    @"preferredRate",
                    @"playable",
                    @"tracks",
                   nil];
    }
    return keys;
}

NSArray* itemKVOProperties()
{
    static NSArray* keys;
    if (!keys) {
        keys = [[NSArray alloc] initWithObjects:@"presentationSize",
                @"status",
                @"asset",
                @"tracks",
                @"seekableTimeRanges",
                @"loadedTimeRanges",
                @"playbackLikelyToKeepUp",
                @"playbackBufferFull",
                @"playbackBufferEmpty",
                nil];
    }
    return keys;
}

} // namespace WebCore

@implementation WebCoreAVFMovieObserver

- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback
{
    m_callback = callback;
    return [super init];
}

- (void)disconnect
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    m_callback = 0;
}

- (void)metadataLoaded
{
    if (!m_callback)
        return;

    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::AssetMetadataLoaded);
}

- (void)playableKnown
{
    if (!m_callback)
        return;

    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::AssetPlayabilityKnown);
}

- (void)timeChanged:(double)time
{
    if (!m_callback)
        return;

    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::PlayerTimeChanged, time);
}

- (void)didEnd:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (!m_callback)
        return;
    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemDidPlayToEndTime);
}

- (void)observeValueForKeyPath:keyPath ofObject:(id)object change:(NSDictionary *)change context:(MediaPlayerAVFoundationObservationContext)context
{
    UNUSED_PARAM(change);

    LOG(Media, "WebCoreAVFMovieObserver:observeValueForKeyPath(%p) - keyPath = %s", self, [keyPath UTF8String]);

    if (!m_callback)
        return;

    if (context == MediaPlayerAVFoundationObservationContextPlayerItem) {
        // A value changed for an AVPlayerItem
        if ([keyPath isEqualToString:@"status"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemStatusChanged);
        else if ([keyPath isEqualToString:@"playbackLikelyToKeepUp"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemIsPlaybackLikelyToKeepUpChanged);
        else if ([keyPath isEqualToString:@"playbackBufferEmpty"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemIsPlaybackBufferEmptyChanged);
        else if ([keyPath isEqualToString:@"playbackBufferFull"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemIsPlaybackBufferFullChanged);
        else if ([keyPath isEqualToString:@"asset"])
            m_callback->setAsset([object asset]);
        else if ([keyPath isEqualToString:@"loadedTimeRanges"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemLoadedTimeRangesChanged);
        else if ([keyPath isEqualToString:@"seekableTimeRanges"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemSeekableTimeRangesChanged);
        else if ([keyPath isEqualToString:@"tracks"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemTracksChanged);
        else if ([keyPath isEqualToString:@"presentationSize"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemPresentationSizeChanged);

        return;
    }

    if (context == MediaPlayerAVFoundationObservationContextPlayer) {
        // A value changed for an AVPlayer.
        if ([keyPath isEqualToString:@"rate"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::PlayerRateChanged);
}
}

@end

#endif
