#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "TileLayer.h"

using namespace cocos2d;
using namespace CocosDenshion;


CCScene* HelloWorld::scene()
{
    // 'scene' is an autorelease object
    CCScene *scene = CCScene::create();
    
    // 'layer' is an autorelease object
    HelloWorld *layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !CCLayer::init() )
    {
        return false;
    }
    
    setTouchEnabled(true);
    
    m_contentScale = 1.0f;
    m_minScale = 0.5f;
    
    setAnchorPoint(CCPointZero);
	setScale(m_contentScale);

    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    CCMenuItemImage *pCloseItem = CCMenuItemImage::create(
                                        "CloseNormal.png",
                                        "CloseSelected.png",
                                        this,
                                        menu_selector(HelloWorld::menuCloseCallback) );
    pCloseItem->setPosition( ccp(CCDirector::sharedDirector()->getWinSize().width - 20, 20) );

    // create menu, it's an autorelease object
    CCMenu* pMenu = CCMenu::create(pCloseItem, NULL);
    pMenu->setPosition( CCPointZero );
    this->addChild(pMenu, 1);

    /////////////////////////////
    // 3. add your codes below...

    // add a label shows "Hello World"
    // create and initialize a label
    CCLabelTTF* pLabel = CCLabelTTF::create("Hello World", "Thonburi", 34);

    // ask director the window size
    CCSize size = CCDirector::sharedDirector()->getWinSize();

    // position the label on the center of the screen
    pLabel->setPosition( ccp(size.width / 2, size.height - 20) );

    // add the label as a child to this layer
    this->addChild(pLabel, 1);

    // add TileLayer
    m_tileLayer = TileLayer::createWithConfigFile( "level2_HD.plist", "cloud.jpg" );
    m_tileLayer->setTileImageType( CCImage::kFmtJpg );
    m_mapSize = m_tileLayer->getMapSize();
    m_minScale = MAX((float)size.width / (float)m_mapSize.width, (float)size.height / (float)m_mapSize.height);
    m_tileLayer->initTiles( CCPointMake( 0, 0 ), size );
    if ( m_tileLayer != NULL )
    {
        this->addChild( m_tileLayer );
    }
    
    return true;
}

void HelloWorld::menuCloseCallback(CCObject* pSender)
{
    CCDirector::sharedDirector()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

void HelloWorld::update(float dt)
{

}

/////////////////////touch handler///////////////////////

void HelloWorld::ccTouchesBegan(cocos2d::CCSet *pTouches, cocos2d::CCEvent *pEvent)
{
	if(pTouches->count() == 1)
	{
		CCSetIterator itr = pTouches->begin();
		CCTouch *touch1 = (CCTouch*)(*itr);
		ccTouchBegan(touch1, pEvent);
	}
	
}

void HelloWorld::ccTouchesMoved(cocos2d::CCSet *pTouches, cocos2d::CCEvent *pEvent)
{
	if(pTouches->count() == 2)
	{
		CCSetIterator itr = pTouches->begin();
		CCTouch *touch1 = (CCTouch*)(*itr);
		itr++;
		CCTouch *touch2 = (CCTouch*)(*itr);
		
		CCPoint curPosTouch1 = CCDirector::sharedDirector()->convertToGL(touch1->getLocationInView());
		CCPoint curPosTouch2 = CCDirector::sharedDirector()->convertToGL(touch2->getLocationInView());
		
		CCPoint prevPosTouch1 = CCDirector::sharedDirector()->convertToGL(touch1->getPreviousLocationInView());
		CCPoint prevPosTouch2 = CCDirector::sharedDirector()->convertToGL(touch2->getPreviousLocationInView());
		
		float prevScale = this->getScale();
		float curScale = this->getScale() * ccpDistance(curPosTouch1, curPosTouch2) / ccpDistance(prevPosTouch1, prevPosTouch2);
		
		if(curScale > 3.0 || curScale < m_minScale)
			return;
		
		CCPoint preCenter = convertToNodeSpace(ccpMult(ccpAdd(curPosTouch1, curPosTouch2), 0.5));
		this->setScale( curScale );
        CCPoint p = ccpAdd(getPosition(), ccpMult(preCenter, prevScale - curScale));
        CCSize winsize = CCDirector::sharedDirector()->getWinSize();
        p.x = MIN( 0, MAX( p.x, winsize.width - m_mapSize.width * curScale ) );
        p.y = MIN( 0, MAX( p.y, winsize.height - m_mapSize.height * curScale ) );
		setPosition( p );
		
        // for TileLayer
		m_contentScale = curScale;
        m_tileLayer->setBottomLeftCornerInGameWorld( ccp( -p.x, -p.y ) / m_contentScale );
        m_tileLayer->setScaleFactor( curScale );
	}
	else
	{
		CCSetIterator itr = pTouches->begin();
		CCTouch *touch1 = (CCTouch*)(*itr);
		ccTouchMoved(touch1, pEvent);
	}
}

void HelloWorld::ccTouchesEnded(cocos2d::CCSet *pTouches, cocos2d::CCEvent *pEvent)
{
	if(pTouches->count() == 1)
	{
		CCSetIterator itr = pTouches->begin();
		CCTouch *touch1 = (CCTouch*)(*itr);
		ccTouchEnded(touch1, pEvent);
	}
}


bool HelloWorld::ccTouchBegan(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent)
{
	return true;
}

void HelloWorld::ccTouchMoved(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent)
{
	CCPoint touchLocation = CCDirector::sharedDirector()->convertToGL(pTouch->getLocationInView());
	CCPoint prevPosTouch = CCDirector::sharedDirector()->convertToGL(pTouch->getPreviousLocationInView());
    
	CCSize winsize = CCDirector::sharedDirector()->getWinSize();
	CCPoint newPos = ccpSub(getPosition(), ccpSub(prevPosTouch, touchLocation));
	int x = MIN(0, MAX(winsize.width - m_mapSize.width * m_contentScale, newPos.x));
	int y = MIN(0, MAX(winsize.height - m_mapSize.height * m_contentScale, newPos.y));
    
	setPosition(ccp(x, y));
    
    // for TileLayer
    m_tileLayer->setBottomLeftCornerInGameWorld( ccp( -x, -y ) / m_contentScale );
}

void HelloWorld::ccTouchCancelled(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent)
{
	return ccTouchEnded(pTouch, pEvent);
}

void HelloWorld::ccTouchEnded(CCTouch *pTouch, CCEvent *pEvent)
{
    
}

