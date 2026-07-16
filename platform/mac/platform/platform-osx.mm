/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <sstream>
#include <dlfcn.h>
#include <util/base.h>
#include <util/threading.h>
#include <obs-config.h>
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include <unistd.h>

#import <AppKit/AppKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>
#import <sys/sysctl.h>
#import <Metal/Metal.h>
#import <ApplicationServices/ApplicationServices.h>

#import "CrashReporter/CrashReporter.h"


#include "CoreModel/Locale/CLocaleTextManager.h"


using namespace std;

bool isInBundle()
{
    NSRunningApplication *app = [NSRunningApplication currentApplication];
    return [app bundleIdentifier] != nil;
}

bool GetDataFilePath(const char *data, string &output)
{
    NSURL *bundleUrl = [[NSBundle mainBundle] bundleURL];
    NSString *path = [[bundleUrl path] stringByAppendingFormat:@"/%@/%s", @"Contents/Resources", data];
    output = path.UTF8String;

    return !access(output.c_str(), R_OK);
}

void CheckIfAlreadyRunning(bool &already_running)
{
    NSString *bundleId = [[NSBundle mainBundle] bundleIdentifier];

    NSUInteger appCount = [[NSRunningApplication runningApplicationsWithBundleIdentifier:bundleId] count];

    already_running = appCount > 1;
}

string GetDefaultVideoSavePath()
{
    NSFileManager *fm = [NSFileManager defaultManager];
    NSURL *url = [fm URLForDirectory:NSMoviesDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:true
                               error:nil];

    if (!url)
        return getenv("HOME");

	//return url.path.fileSystemRepresentation;
    return url.path.fileSystemRepresentation + string("/SOOP");
}

vector<string> GetPreferredLocales()
{
    NSArray *preferred = [NSLocale preferredLanguages];

    auto locales = AFLocaleTextManager::GetLocaleNames();
    auto lang_to_locale = [&locales](string lang) -> string {
        string lang_match = "";

        for (const auto &locale : locales) {
            if (locale.first == lang.substr(0, locale.first.size()))
                return locale.first;

            if (!lang_match.size() && locale.first.substr(0, 2) == lang.substr(0, 2))
                lang_match = locale.first;
        }

        return lang_match;
    };

    vector<string> result;
    result.reserve(preferred.count);

    for (NSString *lang in preferred) {
        string locale = lang_to_locale(lang.UTF8String);
        if (!locale.size())
            continue;

        if (find(begin(result), end(result), locale) != end(result))
            continue;

        result.emplace_back(locale);
    }

    return result;
}

bool IsAlwaysOnTop(QWidget *window)
{
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

void disableColorSpaceConversion(QWidget *window)
{
    NSView *view = (__bridge NSView *) reinterpret_cast<void *>(window->winId());
    view.window.colorSpace = NSColorSpace.sRGBColorSpace;
}

void SetAlwaysOnTop(QWidget *window, bool enable)
{
    Qt::WindowFlags flags = window->windowFlags();

    if (enable) {
        NSView *view = (__bridge NSView *) reinterpret_cast<void *>(window->winId());

        [[view window] setLevel:NSScreenSaverWindowLevel];

        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }

    window->setWindowFlags(flags);
    window->show();
}

bool SetDisplayAffinitySupported(void)
{
    // Not implemented yet
    return false;
}

typedef void (*set_int_t)(int);

void EnableOSXVSync(bool enable)
{
    static bool initialized = false;
    static bool valid = false;
    static set_int_t set_debug_options = nullptr;
    static set_int_t deferred_updates = nullptr;

    if (!initialized) {
        void *quartzCore = dlopen("/System/Library/Frameworks/"
                                  "QuartzCore.framework/QuartzCore",
                                  RTLD_LAZY);
        if (quartzCore) {
            set_debug_options = (set_int_t) dlsym(quartzCore, "CGSSetDebugOptions");
            deferred_updates = (set_int_t) dlsym(quartzCore, "CGSDeferredUpdates");

            valid = set_debug_options && deferred_updates;
        }

        initialized = true;
    }

    if (valid) {
        set_debug_options(enable ? 0 : 0x08000000);
        deferred_updates(enable ? 1 : 0);
    }
}

void EnableOSXDockIcon(bool enable)
{
    if (enable)
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    else
        [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
}

@interface DockView : NSView {
      @private
    QIcon _icon;
}
@end

@implementation DockView

- (id)initWithIcon:(QIcon)icon
{
    self = [super init];
    _icon = icon;
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    CGSize size = dirtyRect.size;

    /* Draw regular app icon */
    NSImage *appIcon = [[NSWorkspace sharedWorkspace] iconForFile:[[NSBundle mainBundle] bundlePath]];
    [appIcon drawInRect:CGRectMake(0, 0, size.width, size.height)];

    /* Draw small icon on top */
    float iconSize = 0.45;
    CGImageRef image = _icon.pixmap(size.width, size.height).toImage().toCGImage();
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    CGContextDrawImage(
        context, CGRectMake(size.width * (1 - iconSize), 0, size.width * iconSize, size.height * iconSize), image);
    CGImageRelease(image);
}

@end

MacPermissionStatus CheckPermissionWithPrompt(MacPermissionType type, bool prompt_for_permission)
{
    __block MacPermissionStatus permissionResponse = kPermissionNotDetermined;

    switch (type) {
        case kAudioDeviceAccess: {
            AVAuthorizationStatus audioStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];

            if (audioStatus == AVAuthorizationStatusNotDetermined && prompt_for_permission) {
                os_event_t *block_finished;
                os_event_init(&block_finished, OS_EVENT_TYPE_MANUAL);
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
                                         completionHandler:^(BOOL granted __attribute((unused))) {
                                             os_event_signal(block_finished);
                                         }];
                os_event_wait(block_finished);
                os_event_destroy(block_finished);
                audioStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
            }

            permissionResponse = (MacPermissionStatus) audioStatus;

            blog(LOG_INFO, "[macOS] Permission for audio device access %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");

            break;
        }
        case kVideoDeviceAccess: {
            AVAuthorizationStatus videoStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

            if (videoStatus == AVAuthorizationStatusNotDetermined && prompt_for_permission) {
                os_event_t *block_finished;
                os_event_init(&block_finished, OS_EVENT_TYPE_MANUAL);
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                                         completionHandler:^(BOOL granted __attribute((unused))) {
                                             os_event_signal(block_finished);
                                         }];

                os_event_wait(block_finished);
                os_event_destroy(block_finished);
                videoStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
            }

            permissionResponse = (MacPermissionStatus) videoStatus;

            blog(LOG_INFO, "[macOS] Permission for video device access %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");

            break;
        }
        case kScreenCapture: {
            permissionResponse = (CGPreflightScreenCaptureAccess() ? kPermissionAuthorized : kPermissionDenied);

            if (permissionResponse != kPermissionAuthorized && prompt_for_permission) {
                permissionResponse = (CGRequestScreenCaptureAccess() ? kPermissionAuthorized : kPermissionDenied);
            }

            blog(LOG_INFO, "[macOS] Permission for screen capture %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");

            break;
        }
        case kAccessibility: {
            permissionResponse = (AXIsProcessTrusted() ? kPermissionAuthorized : kPermissionDenied);

            if (permissionResponse != kPermissionAuthorized && prompt_for_permission) {
                NSDictionary *options = @{(__bridge id) kAXTrustedCheckOptionPrompt: @YES};
                permissionResponse = (AXIsProcessTrustedWithOptions((CFDictionaryRef) options) ? kPermissionAuthorized
                                                                                               : kPermissionDenied);
            }

            blog(LOG_INFO, "[macOS] Permission for accessibility %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");
            break;
        }
    }

    return permissionResponse;
}

void OpenMacOSPrivacyPreferences(const char *tab)
{
    NSURL *url = [NSURL
        URLWithString:[NSString
                          stringWithFormat:@"x-apple.systempreferences:com.apple.preference.security?Privacy_%s", tab]];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

void SetMacOSDarkMode(bool dark)
{
    if (dark) {
        NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    } else {
        NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    }
}

int GetHeightDock(QWidget* window)
{
    NSView* view = (__bridge NSView *) reinterpret_cast<void *>(window->winId());
    NSScreen* screen = [view.window screen];
    NSRect visibleFrame = [screen visibleFrame];
    NSRect screenFrame = screen.frame;
    
    return visibleFrame.origin.y - screenFrame.origin.y;
}

void InitPLCrashReporter(CrashSignalCallback crashCallback)
{
    PLCrashReporterSignalHandlerType signalHandlerType = PLCrashReporterSignalHandlerTypeMach;
    PLCrashReporterConfig* config = [[PLCrashReporterConfig alloc]
                                     initWithSignalHandlerType: signalHandlerType
                                     symbolicationStrategy: PLCrashReporterSymbolicationStrategyAll];
    PLCrashReporter* reporter = [[PLCrashReporter alloc] initWithConfiguration: config];

    PLCrashReporterCallbacks cb = {
        .version = 0,
        .context = (__bridge void *) reporter,
        .handleSignal = crashCallback
    };
    
    [reporter setCrashCallbacks: &cb];
    
    NSError* error = nil;
    if (![reporter enableCrashReporterAndReturnError: &error])
        blog(LOG_INFO, "[macOS] Fail to enable crash reporter");
}

std::string PrintLogCrash(void* pobjPLCrashReporter)
{
    std::string res;
    res.clear();
    
    NSError* error = nil;
    
    PLCrashReporter* reporter = (__bridge PLCrashReporter*)pobjPLCrashReporter;
    
    NSData* data = [reporter loadPendingCrashReportDataAndReturnError: &error];
    if (data == nil)
    {
        blog(LOG_INFO, "[macOS] Fail to load pending crash report data");
        return res;
    }
    
    [reporter purgePendingCrashReport];
    
    PLCrashReport* report = [[PLCrashReport alloc] initWithData: data error: &error];
    if (report == nil)
    {
        blog(LOG_INFO, "[macOS] Fail to make crash report data");
        return res;
    }
    
    
    NSString* text = [PLCrashReportTextFormatter 
                      stringValueForCrashReport: report
                      withTextFormat: PLCrashReportTextFormatiOS];
    
    if (text != nil)
        return [text UTF8String];
    
    
    return res;
}

std::string GetCPUModel()
{
    size_t size;
    if (sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0) == 0)
    {
        char* cpuModel = (char*)malloc(size);
        sysctlbyname("machdep.cpu.brand_string", cpuModel, &size, NULL, 0);
        
        NSString *cpuModelStr = [NSString stringWithUTF8String:cpuModel];
        free(cpuModel);
        
        return [cpuModelStr UTF8String];
    }
}

std::string GetHWModel()
{
    size_t size;
    sysctlbyname("hw.model", NULL, &size, NULL, 0);
    
    char* model = (char*)malloc(size);
    sysctlbyname("hw.model", model, &size, NULL, 0);
    
    NSString *machineModel = [NSString stringWithUTF8String:model];
    free(model);
    
    return [machineModel UTF8String];
}

std::string GetOSVersion()
{
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    NSOperatingSystemVersion version = [processInfo operatingSystemVersion];
    return [[NSString stringWithFormat:@"macOS %ld.%ld.%ld",
            version.majorVersion, version.minorVersion, version.patchVersion] UTF8String];
}

std::string GetMemSize()
{
    int64_t memSize;
    size_t size = sizeof(memSize);
    sysctlbyname("hw.memsize", &memSize, &size, NULL, 0);
    return [[NSString stringWithFormat:@"%lld GB", memSize / (1024 * 1024 * 1024)] UTF8String];
}

std::string GetGPUModel()
{
    io_iterator_t iter;
    io_service_t service;
    NSString* gpuModel = nil;

    if (IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOPCIDevice"), &iter) == KERN_SUCCESS) {
        while ((service = IOIteratorNext(iter))) {
            CFTypeRef gpuName = IORegistryEntryCreateCFProperty(service, CFSTR("model"), kCFAllocatorDefault, 0);
            if (gpuName) {
                if (CFGetTypeID(gpuName) == CFDataGetTypeID()) {
                    // CFData → NSString
                    const UInt8* bytes = CFDataGetBytePtr((CFDataRef)gpuName);
                    long length = CFDataGetLength((CFDataRef)gpuName);
                    gpuModel = [[NSString alloc] initWithBytes:bytes length:length encoding:NSUTF8StringEncoding];
                    CFRelease(gpuName);
                } else if (CFGetTypeID(gpuName) == CFStringGetTypeID()) {
                    // CFString → NSString (ARC가 관리하므로 CFRelease 금지)
                    gpuModel = (__bridge_transfer NSString *)gpuName;
                    // CFRelease(gpuName) -> __bridge_transfer ㅇㅔㅅㅓ ㅊㅓㄹㅣ
                } else {
                    CFRelease(gpuName);
                }

                IOObjectRelease(service);
                break;
            }
            IOObjectRelease(service);
        }
        IOObjectRelease(iter);
    }

    // Metal fallback
    if (gpuModel == nil || [gpuModel length] == 0) {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        gpuModel = [device name];
    }

    return gpuModel ? [gpuModel UTF8String] : "Unknown GPU";
}

std::string GetGPUMemSize()
{
    io_iterator_t iter;
    io_service_t service;
    NSString* gpuMemory = @"Unknown GPU Memory";
    
    if (IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOPCIDevice"), &iter) == KERN_SUCCESS) {
        while ((service = IOIteratorNext(iter))) {
            CFTypeRef vramSize = IORegistryEntryCreateCFProperty(service, CFSTR("VRAM,totalMB"), kCFAllocatorDefault, 0);
            if (vramSize) {
                gpuMemory = [NSString stringWithFormat:@"%@ MB", vramSize];
                CFRelease(vramSize);
                IOObjectRelease(service);
                break;
            }
            IOObjectRelease(service);
        }
        IOObjectRelease(iter);
    }
    
    
    if ([gpuMemory isEqualToString:@"Unknown GPU Memory"])
    {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        uint64_t vramSize = [device recommendedMaxWorkingSetSize] / (1024.0 * 1024.0);
        gpuMemory = [NSString stringWithFormat:@"%d MB", vramSize];
    }
    
    return [gpuMemory UTF8String];
}

void TaskbarOverlayInit() {}

void TaskbarOverlaySetStatus(TaskbarOverlayStatus status)
{
    QIcon icon;
    if (status == TaskbarOverlayStatusActive)
        icon = QIcon::fromTheme("obs-active", QIcon(":/res/images/active_mac.png"));
    else if (status == TaskbarOverlayStatusPaused)
        icon = QIcon::fromTheme("obs-paused", QIcon(":/res/images/paused_mac.png"));

    NSDockTile *tile = [NSApp dockTile];
    [tile setContentView:[[DockView alloc] initWithIcon:icon]];
    [tile display];
}

/*
 * This custom NSApplication subclass makes the app compatible with CEF. Qt
 * also has an NSApplication subclass, but it doesn't conflict thanks to Qt
 * using arcane magic to hook into the NSApplication superclass itself if the
 * program has its own NSApplication subclass.
 */

@protocol CrAppProtocol
- (BOOL)isHandlingSendEvent;
@end

@interface OBSApplication : NSApplication <CrAppProtocol>
@property (nonatomic, getter=isHandlingSendEvent) BOOL handlingSendEvent;
@end

@implementation OBSApplication

- (void)sendEvent:(NSEvent *)event
{
    _handlingSendEvent = YES;
    [super sendEvent:event];
    _handlingSendEvent = NO;
}

@end

void InstallNSThreadLocks()
{
    [[NSThread new] start];

    if ([NSThread isMultiThreaded] != 1) {
        abort();
    }
}

void InstallNSApplicationSubclass()
{
    [OBSApplication sharedApplication];
}

bool HighContrastEnabled()
{
    return [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldIncreaseContrast];
}