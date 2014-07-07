//
//  TileLayer.cpp
//  Fables
//
//  Created by Xi Yu on 7/25/13.
//
//

#include "TileLayer.h"

using namespace cocos2d;

#define TILE_BUFF   1

static std::vector<TileLayer::TileModule*> s_tileQueue;
static pthread_mutex_t s_tileQueueMutex;
static pthread_cond_t s_tileQueueCondition;
static bool s_quitTileThreads;
static CCImage::EImageFormat s_tileImageType;

TileLayer::TileLayer()
: m_dirty( false )
, m_tiles( NULL )
, m_tileSize( 0, 0 )
, m_tileRowNum( 0 )
, m_tileColNum( 0 )
, m_tileRowNumOnScreen( 0 )
, m_tileColNumOnScreen( 0 )
, m_beginRenderedRow( 0 )
, m_endRenderedRow( 0 )
, m_beginRenderedCol( 0 )
, m_endRenderedCol( 0 )
, m_imageSize( 0, 0 )
, m_tileInfos( NULL )
, m_bottomLeftCornerInGameWorld( 0, 0 )
, m_mapOnScreenSize( 0, 0 )
, m_scaleFactor( 1.0f )
{
    //initialize the static variables
    s_quitTileThreads = false;
    s_tileQueueMutex = PTHREAD_MUTEX_INITIALIZER;
    s_tileQueueCondition = PTHREAD_COND_INITIALIZER;
    s_tileImageType = CCImage::kFmtJpg;
}

TileLayer::~TileLayer()
{
    m_dirty = false;
    m_tileSize = CCSizeMake( 0, 0 );
    m_tileRowNum = 0;
    m_tileColNum = 0;
    m_tileRowNumOnScreen = 0;
    m_tileColNumOnScreen = 0;
    m_beginRenderedRow = 0;
    m_endRenderedRow = 0;
    m_beginRenderedCol = 0;
    m_endRenderedCol = 0;
    m_imageSize = CCSizeMake( 0, 0 );
    m_bottomLeftCornerInGameWorld = CCPointMake( 0, 0 );
    m_mapOnScreenSize = CCSizeMake( 0, 0 );
    m_scaleFactor = 1.0f;
    CC_SAFE_RELEASE_NULL( m_placeHolder );
    
    for ( int i = 0; i < m_tileRowNum; i++ )
    {
        for ( int j = 0; j < m_tileColNum; j++ )
        {
            CC_SAFE_DELETE( m_tileInfos[i][j] );
        }
        CC_SAFE_DELETE_ARRAY( m_tileInfos[i] );
    }
    
    CC_SAFE_DELETE_ARRAY( m_tileInfos );
    
    for ( int i = 0; i < m_tileRowNum; i++ )
    {
        for ( int j = 0; j < m_tileColNum; j++ )
        {
            this->removeChild( m_tiles[i][j]->sprite );
            m_tiles[i][j]->sprite = NULL;
            CC_SAFE_DELETE( m_tiles[i][j] );
        }
        CC_SAFE_DELETE_ARRAY( m_tiles[i] );
    }
    
    CC_SAFE_DELETE_ARRAY( m_tiles );
    
    // release the thread resources
    s_quitTileThreads = true;
    for ( int i = 0; i < LOADINGTILE_THREAD_NUM; i++ )
    {
        if ( pthread_mutex_trylock( &m_tileThreads[i]->mutex ) == 0 )
        {
            pthread_cond_signal( &m_tileThreads[i]->cond );
            pthread_mutex_unlock( &m_tileThreads[i]->mutex );
        }
    }
    pthread_mutex_destroy( &s_tileQueueMutex );
    pthread_cond_destroy( &s_tileQueueCondition );
    
    // the elements of m_tileThreads are deleted in the thread function.
    CC_SAFE_DELETE_ARRAY( m_tileThreads );
}

TileLayer* TileLayer::createWithConfigFile( const char* plistName, const char* pplaceHolderName )
{
    TileLayer* layer = new TileLayer();
    if ( layer && layer->initWithConfigFile( plistName, pplaceHolderName ) )
    {
        layer->autorelease();
        return layer;
    }
    else
    {
        delete layer;
        layer = NULL;
        return NULL;
    }
}

bool TileLayer::initWithConfigFile( const char* plistName, const char* pplaceHolderName )
{ 
    CCAssert( plistName != NULL, "TileLayer : Invalid plistname!" );
    
    if ( !CCLayer::init() )
    {
        return false;
    }
    
    std::string fullPath = CCFileUtils::sharedFileUtils()->fullPathForFilename( pplaceHolderName );
    
    if ( !CCFileUtils::sharedFileUtils()->isFileExist( fullPath ) )
    {
        CCLOG( "TileLayer : can't find %s!", pplaceHolderName );
        return false;
    }
    
    CCImage* placeHolderImage = new CCImage();
    placeHolderImage->initWithImageFile( pplaceHolderName, CCImage::kFmtJpg ); //use jpg image placeholder now. if not, you should check the image type here.
    m_placeHolder = new CCTexture2D();
    m_placeHolder->initWithImage( placeHolderImage );
    m_placeHolder->retain();
    CCAssert( m_placeHolder != NULL, "TileLayer : Invalid place holder file!" );
    
    fullPath = CCFileUtils::sharedFileUtils()->fullPathForFilename( plistName );
    
    if ( !CCFileUtils::sharedFileUtils()->isFileExist( fullPath ) )
    {
        CCLOG( "TileLayer : can't find %s!", plistName );
        return false;
    }
    
    CCDictionary* plist = CCDictionary::createWithContentsOfFile( plistName );
    CCAssert( plist != NULL, "TileLayer : can't init .plist." );
    CCDictionary* source = (CCDictionary*)plist->objectForKey( "Source" );
    m_imageSize = CCSizeFromString( ((CCString*)source->objectForKey( "Size" ))->getCString() );
    m_tileSize = CCSizeFromString( ((CCString*)source->objectForKey( "TileSize" ))->getCString() );
    m_tileRowNum = ((CCString*)source->objectForKey( "Row" ))->intValue();
    m_tileColNum = ((CCString*)source->objectForKey( "Column" ))->intValue();
    
    //init tile information
    CCArray* tileInfosFromPlist = (CCArray*)plist->objectForKey( "Tiles" );
    int tileInfosLength = tileInfosFromPlist->count();
    CCAssert( tileInfosLength != 0, "Invalid filename for TileLayer!" );
    
    m_tileInfos = new TileInfo**[m_tileRowNum];
    memset( m_tileInfos, 0, sizeof( TileInfo** ) * m_tileRowNum );
    
    for ( int i = 0; i < m_tileRowNum; i++ )
    {
        TileInfo** tmpTileInfos = new TileInfo*[m_tileColNum];
        memset( tmpTileInfos, 0, sizeof( TileInfo* ) * m_tileColNum );
        m_tileInfos[i] = tmpTileInfos;
        for ( int j = 0; j < m_tileColNum; j++ )
        {
            CCAssert( i < tileInfosFromPlist->capacity(), "it's wired that m_tileInfosLength exceeds tileInfos' capacity!" );
            TileInfo* info = new TileInfo();
            CCDictionary* dic = (CCDictionary*)tileInfosFromPlist->objectAtIndex( i * m_tileColNum + j );
            info->tileName = (CCString*)dic->objectForKey( "Name" );
            //CCLog( "tile name = %s", info->tileName->getCString() );
            info->rect = CCRectFromString( ((CCString*)dic->objectForKey( "Rect" ))->getCString() );
            m_tileInfos[i][j] = info;
        }
    }
    
    return true;
}

bool TileLayer::initTiles( const CCPoint& bottomLeftCornerInGameWorld, const CCSize& screenSize, const float scaleFactor )
{
    CCAssert( screenSize.width != 0 && screenSize.height != 0, "Invalid screen size for TileLayer!" );
    m_bottomLeftCornerInGameWorld = bottomLeftCornerInGameWorld;
    m_screenSize = screenSize;
    m_scaleFactor = scaleFactor;
    m_mapOnScreenSize = screenSize / scaleFactor;
    CCPoint topLeftCornerInGameWorld = CCPointMake( m_bottomLeftCornerInGameWorld.x, m_imageSize.height - m_mapOnScreenSize.height - m_bottomLeftCornerInGameWorld.y );
    
    m_beginRenderedRow = tileFloor( topLeftCornerInGameWorld.y / m_tileSize.height - TILE_BUFF );
    m_beginRenderedCol = tileFloor( topLeftCornerInGameWorld.x / m_tileSize.width - TILE_BUFF );
    m_endRenderedRow = tileCeilForRow( m_beginRenderedRow + m_mapOnScreenSize.height / m_tileSize.height + TILE_BUFF + 1 );
    m_endRenderedCol = tileCeilForCol( m_beginRenderedCol + m_mapOnScreenSize.width / m_tileSize.width + TILE_BUFF + 1 );
    m_tileRowNumOnScreen = m_endRenderedRow - m_beginRenderedRow + 1;
    m_tileColNumOnScreen = m_endRenderedCol - m_beginRenderedCol + 1;
    CCLOG( "TileLayer init : beginRow = %d, beginCol = %d, endRow = %d, endCol = %d.", m_beginRenderedRow, m_beginRenderedCol, m_endRenderedRow, m_endRenderedCol );
    CCAssert( m_tileRowNumOnScreen * m_tileSize.height > m_mapOnScreenSize.height, "Not enough tile rows to display on screen!" );
    CCAssert( m_tileColNumOnScreen * m_tileSize.width > m_mapOnScreenSize.width, "Not enough tile cols to display on screen!" );
    
    //init tile sprites
    m_tiles = new TileModule**[m_tileRowNum];
    memset( m_tiles, 0, sizeof( TileModule** ) * m_tileRowNum );
    for ( int i = 0; i < m_tileRowNum; i++ )
    {
        TileModule** tmpModules = new TileModule*[m_tileColNum];
        memset( tmpModules, 0, sizeof( TileModule* ) * m_tileColNum );
        m_tiles[i] = tmpModules;
    }
    
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    
    for ( int i = 0; i < m_tileRowNum; i++ )
    {
        for ( int j = 0; j < m_tileColNum; j++ )
        {
            if ( i >= m_beginRenderedRow && i <= m_endRenderedRow && j >= m_beginRenderedCol && j <= m_endRenderedCol )
            {
                CCSprite* sprite = CCSprite::create( m_tileInfos[i][j]->tileName->getCString() );
                CCAssert( sprite != NULL, "Tilelayer init : Invalid tile image!" );
                sprite->setAnchorPoint( CCPointMake( 0, 1 ) );
                sprite->setPositionX( m_tileInfos[i][j]->rect.origin.x );
                sprite->setPositionY( m_imageSize.height - m_tileInfos[i][j]->rect.origin.y );
                //CCLOG( "TileLayer init : sprite[%d][%d]'s x = %f, y = %f", i, j, sprite->getPositionX(), sprite->getPositionY() );
                m_tiles[i][j] = new TileModule( m_tileInfos[i][j]->tileName->m_sString, NULL, sprite, LOADED );
                this->addChild( sprite );
            }
            else
            {
                //you may initialize all sprites with placeholder, then update them with real tiles and placeholder image.
                m_tiles[i][j] = new TileModule( m_tileInfos[i][j]->tileName->m_sString, NULL, NULL, UNLOADED );
            }
        }
    }
    
    m_tileThreads = new TileThread*[LOADINGTILE_THREAD_NUM];
    for ( int i = 0; i < LOADINGTILE_THREAD_NUM; i++ )
    {
        m_tileThreads[i] = new TileThread();
        int rc = pthread_create( &m_tileThreads[i]->threadId, &attr, loadTile, (void*)m_tileThreads[i] );
        if ( rc )
        {
            CCLOG( "TileLayer init : return code from pthread_create is %d.", rc );
            assert( false );
        }
    }
    
    pthread_attr_destroy( &attr );
    
    scheduleUpdate();
    
    return true;
}

void TileLayer::updateTiles()
{
    m_mapOnScreenSize = m_screenSize / m_scaleFactor;
    CCPoint topLeftCornerInGameWorld = CCPointMake( m_bottomLeftCornerInGameWorld.x, m_imageSize.height - m_mapOnScreenSize.height - m_bottomLeftCornerInGameWorld.y );
    
    int beginRow = tileFloor( topLeftCornerInGameWorld.y / m_tileSize.height - TILE_BUFF );
    int beginCol = tileFloor( topLeftCornerInGameWorld.x / m_tileSize.width - TILE_BUFF );
    int endRow = tileCeilForRow( beginRow + m_mapOnScreenSize.height / m_tileSize.height + TILE_BUFF + 1 );
    int endCol = tileCeilForCol( beginCol + m_mapOnScreenSize.width / m_tileSize.width + TILE_BUFF + 1 );
    m_tileRowNumOnScreen = endRow - beginRow + 1;
    m_tileColNumOnScreen = endCol - beginCol + 1;
    //CCLOG( "-------------------------------------" );
    //CCLOG( "TileLayer updateTiles : scaleFactor = %f, beginRow = %d, beginCol = %d, endRow = %d, endCol = %d, topLeft.x = %f, topLeft.y = %f, mapOnScreenSize.width = %f, mapOnScreenSize.height = %f", m_scaleFactor, beginRow, beginCol, endRow, endCol, topLeftCornerInGameWorld.x, topLeftCornerInGameWorld.y, m_mapOnScreenSize.width, m_mapOnScreenSize.height );
    CCAssert( m_tileRowNumOnScreen * m_tileSize.height > m_mapOnScreenSize.height, "Not enough tile rows to display on screen!" );
    CCAssert( m_tileColNumOnScreen * m_tileSize.width > m_mapOnScreenSize.width, "Not enough tile cols to display on screen!" );
    
    if ( beginRow > m_beginRenderedRow )
    {
        for ( int i = m_beginRenderedRow; i < beginRow; i++ )
            for ( int j = m_beginRenderedCol; j <= m_endRenderedCol; j++ )
        {
            deleteTile( m_tiles[i][j] );
        }
    }
    
    if ( endRow < m_endRenderedRow )
    {
        for ( int i = endRow + 1; i <= m_endRenderedRow; i++ )
            for ( int j = m_beginRenderedCol; j <= m_endRenderedCol; j++ )
        {
            deleteTile( m_tiles[i][j] );
        }
    }
    
    if ( beginCol > m_beginRenderedCol )
    {
        for ( int i = m_beginRenderedCol; i < beginCol; i++ )
            for ( int j = m_beginRenderedRow; j <= m_endRenderedRow; j++ )
        {            
            deleteTile( m_tiles[j][i] );
        }
    }
    
    if ( endCol < m_endRenderedCol )
    {
        for ( int i = endCol + 1; i <= m_endRenderedCol; i++ )
            for ( int j = m_beginRenderedRow; j <= m_endRenderedRow; j++ )
        {
            deleteTile( m_tiles[j][i] );
        }
    }
    
    m_beginRenderedRow = beginRow;
    m_endRenderedRow = endRow;
    m_beginRenderedCol = beginCol;
    m_endRenderedCol = endCol;
    
    for ( int i = beginRow; i <= endRow; i++ )
    {
        for ( int j = beginCol; j <= endCol; j++ )
        {
            TileModule* tm = m_tiles[i][j];
            
            if ( pthread_mutex_trylock( &tm->mutex ) == 0 )
            {
                if ( tm->status == UNLOADED || tm->status == BE_DELETED )
                {
                    CCAssert( tm->sprite == NULL && tm->image == NULL, "TileLayer updateTiles : status should be consistent with sprite and image." );
                
                    CCTexture2D* texture = CCTextureCache::sharedTextureCache()->addUIImage( NULL, tm->imageName.c_str() );
                    CCSprite* sprite = NULL;
                    if ( texture != NULL )
                    {
                        sprite = CCSprite::createWithTexture( texture );
                        CCAssert( sprite != NULL, "Tilelayer updateTiles : Invalid tile image from cache!" );
                        //CCLOG( "TileLayer updateTiles from cache : sprite[%d][%d]'s x = %f, y = %f", i, j, sprite->getPositionX(), sprite->getPositionY() );
                        tm->status = LOADED;
                    }
                    else
                    {
                        sprite = CCSprite::createWithTexture( m_placeHolder );
                        CCAssert( sprite != NULL, "Tilelayer updateTiles : Invalid sprite from placeholder!" );
                        //CCLOG( "TileLayer updateTiles using placeholder : sprite[%d][%d]'s x = %f, y = %f", i, j, sprite->getPositionX(), sprite->getPositionY() );
                        //pthread_cond_signal( &tm->cond );
                        if ( tm->status != BE_DELETED )
                        {
                            pthread_mutex_lock( &s_tileQueueMutex );
                            s_tileQueue.push_back( tm );
                            pthread_mutex_unlock( &s_tileQueueMutex );
                        }
                        tm->status = LOADING;
                    }

                    sprite->setAnchorPoint( CCPointMake( 0, 1 ) );
                    sprite->setPositionX( m_tileInfos[i][j]->rect.origin.x );
                    sprite->setPositionY( m_imageSize.height - m_tileInfos[i][j]->rect.origin.y );
                    tm->sprite = sprite;
                    this->addChild( sprite );
                }
            
                pthread_mutex_unlock( &tm->mutex );
            }
        }
    }
}

void TileLayer::update( float deltaTime )
{
    if ( m_dirty )
    {
        updateTiles();
        m_dirty = false;
    }
    
    checkTileStatus();
}


// the thread function
void* TileLayer::loadTile( void* p )
{
    // create autorelease pool for iOS
    CCThread thread;
    thread.createAutoreleasePool();
    
    TileThread* tt = (TileThread*)p;
    TileModule* tm = NULL;
    
    while ( true )
    {
        if ( s_quitTileThreads )
        {
            break;
        }
        
        pthread_mutex_lock( &tt->mutex );
        pthread_cond_wait( &tt->cond, &tt->mutex );
            
        if ( s_quitTileThreads )
        {
            pthread_mutex_unlock( &tt->mutex );
            break;
        }
        
        pthread_mutex_lock( &s_tileQueueMutex );
        if ( s_tileQueue.empty() )
        {
            pthread_mutex_unlock( &s_tileQueueMutex );
            continue;
        }
        else
        {
            tm = s_tileQueue.front();
            s_tileQueue.erase( s_tileQueue.begin() );
        }
            
        pthread_mutex_unlock( &s_tileQueueMutex );
        
        CCAssert( tm != NULL, "TileLayer loadTile : don't get tile module to continue!" );
        CCAssert( tm->image == NULL, "TileLayer loadTile : image isn't null when you want to load image." );
        
        // generate image
        CCImage *pImage = new CCImage();
        if ( pImage && !pImage->initWithImageFileThreadSafe( tm->imageName.c_str(), s_tileImageType ) )
        {
            CC_SAFE_RELEASE( pImage );
            CCLOG( "TileLayer : can not load %s", tm->imageName.c_str() );
            continue;
        }
        
        if ( s_quitTileThreads )
        {
            CC_SAFE_RELEASE( pImage );
            pthread_mutex_unlock( &tt->mutex );
            break;
        }
        
        pthread_mutex_lock( &tm->mutex );
        
        if ( tm->status == LOADING )
        {
            tm->image = pImage;
        }
        else if ( tm->status == BE_DELETED )
        {
            CC_SAFE_RELEASE( pImage );
            tm->status = UNLOADED;
        }
        else
        {
            CCAssert( false, "TileLayer loadTile : it's a logic error to get here." );
        }
        
        pthread_mutex_unlock( &tm->mutex );
        pthread_mutex_unlock( &tt->mutex );
    }
    
    delete tt;
    pthread_exit( NULL );
    return NULL;
}

void TileLayer::deleteTile( TileModule* tm )
{
    if ( pthread_mutex_trylock( &tm->mutex ) == 0 )
    {
        if ( tm->status == LOADED )
        {
            CCAssert( tm->image == NULL, "TileLayer : image should be NULL when status is LOADED." );
            this->removeChild( tm->sprite );
            tm->sprite = NULL;
            tm->status = UNLOADED;
        }
        else if ( tm->status == LOADING )
        {
            this->removeChild( tm->sprite );
            tm->sprite = NULL;
            if ( tm->image != NULL )
            {
                CC_SAFE_RELEASE_NULL( tm->image );
                tm->status = UNLOADED;
            }
            else
            {
                tm->status = BE_DELETED;
            }
        }
        else if ( tm->status != UNLOADED && tm->status != BE_DELETED )
        {
            CCAssert( false, "TileLayer : how are you in this case?" );
        }
    
        pthread_mutex_unlock( &tm->mutex );
    }
}

void TileLayer::checkTileStatus()
{
    if ( !s_tileQueue.empty() )
    {
        for ( int i = 0; i < LOADINGTILE_THREAD_NUM; i++ )
        {
            if ( pthread_mutex_trylock( &m_tileThreads[i]->mutex ) == 0 )
            {
                pthread_cond_signal( &m_tileThreads[i]->cond );
                pthread_mutex_unlock( &m_tileThreads[i]->mutex );
            }
        }
    }
    
    for ( int i = 0; i < m_tileRowNum; i++ )
        for ( int j = 0; j < m_tileColNum; j++ )
    {
        TileModule* tm = m_tiles[i][j];
        
        if ( pthread_mutex_trylock( &tm->mutex ) == 0 )
        {
            if ( tm->status == LOADING && tm->image != NULL )
            {
                //CCLOG( "TileLayer checkTileStatus : sprite[%d][%d]'s x = %f, y = %f", i, j, tm->sprite->getPositionX(), tm->sprite->getPositionY() );
                CCTexture2D* texture = CCTextureCache::sharedTextureCache()->addUIImage( tm->image, tm->imageName.c_str() );
                CC_SAFE_RELEASE_NULL( tm->image );
                CCAssert( tm->sprite != NULL, "Tilelayer checkTileStatus : Invalid tile image!" );
                tm->sprite->setTexture( texture );
                tm->status = LOADED;
            }
            pthread_mutex_unlock( &tm->mutex );
        }
    }
}

void TileLayer::setTileImageType( CCImage::EImageFormat imageType )
{
    s_tileImageType = imageType;
}

