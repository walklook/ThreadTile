#include "main.h"
#include "CCEGLView.h"
#include "AppDelegate.h"
#include "cocos2d.h"

USING_NS_CC;

int main()
{
	CCFileUtils::sharedFileUtils()->addSearchPath("../../Resource");

    // create the application instance
    AppDelegate app;
    CCEGLView* eglView = CCEGLView::sharedOpenGLView();
    eglView->setViewName("Fables");
    eglView->setFrameSize(960, 640);
    // The resolution of ipad3 is very large. In general, PC's resolution is smaller than it.
    // So we need to invoke 'setFrameZoomFactor'(only valid on desktop(win32, mac, linux)) to make the window smaller.
    eglView->setFrameZoomFactor(1.0f);
    return CCApplication::sharedApplication()->run();
}
