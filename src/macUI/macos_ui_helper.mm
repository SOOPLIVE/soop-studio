#import <Cocoa/Cocoa.h>
#include <QObject>
#include "macos_ui_helper.h"
#include "MainFrame/CMainFrame.h"

@interface MacOSTitleBarButtonHandler : NSObject<NSWindowDelegate>
@property (nonatomic, weak) NSWindow *window;
@property (nonatomic, assign) int windowType;
@property (nonatomic, assign) QObject *dialogObject;
@property (nonatomic, strong) NSButton *customButton;
@end

@implementation MacOSTitleBarButtonHandler
@synthesize customButton = _customButton;

// 초기화 시 해당 핸들러를 특정 NSWindow에 연결
- (instancetype)initWithWindow:(NSWindow*)window 
                windowType:(int)windowType
                dialogObject:(QObject*)dialogObject {
    self = [super init];
    if(self){
        _window = window;
        _windowType = windowType;
        _dialogObject = dialogObject;
        [_window setDelegate:self]; // 개별 창마다 delegate 설정
    }
    return self;
}

- (void)windowWillClose:(NSNotification *)notification {
    if(self.dialogObject){
        QMetaObject::invokeMethod(self.dialogObject, "close", Qt::QueuedConnection);
    }
}

// 버튼 이벤트 handler
- (void)customButtonClicked:(id)sender {
    
    NSRect buttonFrame = [self.customButton frame];
    NSPoint buttonOrigin = [self.customButton.superview convertPoint:buttonFrame.origin fromView:nil];
    NSRect windowFrame = [self.window frame];
    
    NSScreen* screen = [self.window screen];
    CGFloat screenHeight = [screen frame].size.height;
    
    int posX = (int)(windowFrame.origin.x + buttonOrigin.x);
    int posY = (int)(screenHeight - (windowFrame.origin.y + windowFrame.size.height)+ buttonFrame.size.height);
    
    if(APP->GetMainView()){
        QMetaObject::invokeMethod(APP->GetMainView(), "qslotMacSwitchToDock", 
                                  Qt::QueuedConnection, 
                                  Q_ARG(int, self.windowType),
                                  Q_ARG(int, posX),
                                  Q_ARG(int, posY));
    }
}

// 윈도우 Resize handler
- (void)windowDidResize:(NSNotification *)notification {
    NSWindow* window = (NSWindow*)notification.object;
    if(!window||!self.customButton)
        return;
    
    NSView *titleBarView = [[window standardWindowButton:NSWindowCloseButton] superview];
    if(!titleBarView)
        return;
    
    NSRect titleBarFrame = [titleBarView frame];
    NSRect frame = [self.customButton frame];
    
    frame.origin.x = titleBarFrame.size.width - 30;
    frame.origin.y = (titleBarFrame.size.height - frame.size.height) / 2;
    [self.customButton setFrame:frame];
}

@end

//// Custom Button
@interface CustomButton : NSButton
@property (nonatomic, strong) NSImage *normalImage;
@property (nonatomic, strong) NSImage *hoverImage;
@end

@implementation CustomButton
- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    
    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds 
                                                           options:(NSTrackingMouseEnteredAndExited|NSTrackingActiveInKeyWindow)
                                                           owner:self
                                                           userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void)mouseEntered:(NSEvent *)event {
    [self setImage:self.hoverImage];
}

- (void)mouseExited:(NSEvent *)event {
    [self setImage:self.normalImage];
}

@end


static NSMutableDictionary<NSValue*, MacOSTitleBarButtonHandler*> *windowHandlers = nil;

void addMacOSTitleBarButton(void* windowPtr, int windowType, void* qObject)
{
    dispatch_async(dispatch_get_main_queue(),^{
        NSView *nsView = (__bridge NSView*)windowPtr;
        if(!nsView)
            return;
        
        NSWindow* nsWindow = [nsView window];
        if(!nsWindow)
            return;
        
        
        NSView *titleBarView = [[nsWindow standardWindowButton:NSWindowCloseButton] superview];
        if(!titleBarView)
            return;
        
        QObject *dialogObject = reinterpret_cast<QObject*>(qObject);
        if(!dialogObject)
            return;
        
        NSValue *windowKey = [NSValue valueWithNonretainedObject:nsWindow];
        
        if(!windowHandlers)
            windowHandlers = [[NSMutableDictionary alloc] init];
        
        MacOSTitleBarButtonHandler* buttonHandler = windowHandlers[windowKey];
        if(!buttonHandler){
            buttonHandler = [[MacOSTitleBarButtonHandler alloc] initWithWindow:nsWindow windowType:windowType dialogObject:dialogObject];
            windowHandlers[windowKey] = buttonHandler;
        }
        
        [nsWindow setDelegate:buttonHandler];
        
        [[NSNotificationCenter defaultCenter] addObserver:buttonHandler
                                              selector:@selector(windowDidResize:)
                                              name:NSWindowDidResizeNotification
                                              object:nsWindow];
        
        NSImage *normalImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"three-dots-mac"
                                                                                              ofType:@"png"
                                                                                              inDirectory:@"assets/mainview/default"]];
        
        NSImage *hoverImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle]  pathForResource:@"three-dots-mac"
                                                                                              ofType:@"png"
                                                                                              inDirectory:@"assets/mainview/mousehover"]];
        
        if(!normalImage){
            return;
        }
        
        if(!hoverImage){
            return;
        }
        
        CustomButton* customButton = [[CustomButton alloc] initWithFrame:NSMakeRect(0,0,20,20)];
        customButton.normalImage = normalImage;
        customButton.hoverImage = hoverImage;
        [customButton setImage:normalImage];
        [customButton setBordered:NO];
        [customButton setButtonType:NSButtonTypeMomentaryChange];
        [customButton setBezelStyle:NSBezelStyleRegularSquare];
        [customButton setImageScaling:NSImageScaleProportionallyUpOrDown];
        [customButton setTarget:buttonHandler];
        [customButton setAction:@selector(customButtonClicked:)];
        
        [titleBarView addSubview:customButton];
        NSRect titleBarFrame = [titleBarView frame];
        NSRect frame = [customButton frame];
        frame.origin.x = titleBarFrame.size.width - 30;
        frame.origin.y = (titleBarFrame.size.height - frame.size.height) / 2;
        [customButton setFrame:frame];
        
        buttonHandler.customButton = customButton;
    });
}

void removeMacOSTitleBarButton(void* windowPtr, int windowType)
{
    dispatch_async(dispatch_get_main_queue(),^{
        if(!windowHandlers){
            return;
        }
        
        NSValue* targetKey = nil;
        for(NSValue *key in windowHandlers){
            MacOSTitleBarButtonHandler *handler = windowHandlers[key];
            if(handler.windowType == windowType){
                targetKey = key;
                break;
            }
        }
        
        if(targetKey){
            [windowHandlers removeObjectForKey:targetKey];
        }
    });
}
