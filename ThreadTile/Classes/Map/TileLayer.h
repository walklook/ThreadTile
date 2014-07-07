//
//  TileLayer.h
//  Fables
//
//  Created by Xi Yu on 7/25/13.
//
//  The map supported in this class consists of the tiles which are from tile images. You only need to set bottom left corner
//  cordination and scale factor to navigate it.
//

#ifndef Fables_TileLayer_h
#define Fables_TileLayer_h

#include "cocos2d.h"
#include "pthread.h"

class TileLayer : public cocos2d::CCLayer
{
 
#define LOADINGTILE_THREAD_NUM      2

public:
    struct TileInfo
    {
        cocos2d::CCString* tileName; // the name of the tile image
        cocos2d::CCRect rect;        // the position and the size of the tile in the map
    };
    
    enum TileModuleStatus
    {
        UNLOADED,
        LOADING,
        LOADED,
        BE_DELETED
    };
    
    // the tile in the game
    struct TileModule
    {
        std::string imageName;
        cocos2d::CCImage* image;
        cocos2d::CCSprite* sprite;
        volatile TileModuleStatus status;
        pthread_mutex_t mutex; // the mutex to keep this tile thread-safe.
        
        TileModule( std::string imageName, cocos2d::CCImage* image, cocos2d::CCSprite* sprite, TileModuleStatus status )
        : imageName( imageName ), image( image ), sprite( sprite ), status( status )
        {
            mutex = PTHREAD_MUTEX_INITIALIZER;
        }
        
        ~TileModule()
        {
            pthread_mutex_lock( &mutex );
            CC_SAFE_RELEASE_NULL( image );
            CC_SAFE_RELEASE_NULL( sprite );
            pthread_mutex_unlock( &mutex );
            pthread_mutex_destroy( &mutex );
        }
    };
    
    // the wrapper of the mutex and condition used in loading tile thread
    struct TileThread
    {
        pthread_t threadId;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        
        TileThread()
        {
            mutex = PTHREAD_MUTEX_INITIALIZER;
            cond = PTHREAD_COND_INITIALIZER;
        }
        
        ~TileThread()
        {
            pthread_mutex_destroy( &mutex );
            pthread_cond_destroy( &cond );
        }
    };
    
    virtual ~TileLayer();
    
    static TileLayer* createWithConfigFile( const char* plistName, const char* pplaceHolderName );
    
    // this init() is inherited from CCLayer.
    virtual bool init() { CCAssert( false, "this function is deprecated." ); return false; }
    
    //you should call createWithConfigFile function to new a TileLayer.
    bool initWithConfigFile( const char* plistName, const char* pplaceHolderName );
    bool initTiles( const cocos2d::CCPoint& topLeftCornerInGameWorld, const cocos2d::CCSize& screenSize, const float scaleFactor = 1.0f );
    
    void setTileImageType( cocos2d::CCImage::EImageFormat imageType );
    
    inline void setScaleFactor( const float scaleFactor )
    {
        if ( m_scaleFactor != scaleFactor )
        {
            m_scaleFactor = scaleFactor;
            m_dirty = true;
        }
    }
    
    inline float getScaleFactor() const
    {
        return m_scaleFactor;
    }
    
    //argument bottomLeftCornerInGameWorld is the screen's left bottom left point's logic cordinate in the game world
    inline void setBottomLeftCornerInGameWorld( const cocos2d::CCPoint& bottomLeftCornerInGameWorld )
    {
        if ( !m_bottomLeftCornerInGameWorld.equals( bottomLeftCornerInGameWorld ) )
        {
            if ( bottomLeftCornerInGameWorld.x < 0 || bottomLeftCornerInGameWorld.y < 0 || bottomLeftCornerInGameWorld.x > m_imageSize.width - m_screenSize.width || bottomLeftCornerInGameWorld.y > m_imageSize.height - m_screenSize.height )
            {
                CCLOG( "TileLayer : invalid value for bottomLeftCornerInGameWorld, x = %f, y = %f", bottomLeftCornerInGameWorld.x, bottomLeftCornerInGameWorld.y );
                assert( false );
            }
            m_bottomLeftCornerInGameWorld = bottomLeftCornerInGameWorld;
            m_dirty = true;
        }
    }
    
    inline cocos2d::CCPoint getBottomLeftCornerInGameWorld() const
    {
        return m_bottomLeftCornerInGameWorld;
    }
    
    inline cocos2d::CCSize getMapSize() const
    {
        return m_imageSize;
    }
    
private:
    bool m_dirty; // the sign whether the screen moves or the map is scaled
    cocos2d::CCPoint m_bottomLeftCornerInGameWorld;
    cocos2d::CCSize m_screenSize;
    cocos2d::CCSize m_mapOnScreenSize;
    TileModule*** m_tiles;
    cocos2d::CCSize m_tileSize;
    unsigned int m_tileRowNum;
    unsigned int m_tileColNum;
    unsigned int m_tileRowNumOnScreen;
    unsigned int m_tileColNumOnScreen;
    unsigned int m_beginRenderedRow;
    unsigned int m_endRenderedRow;
    unsigned int m_beginRenderedCol;
    unsigned int m_endRenderedCol;
    //std::string m_imageName;
    cocos2d::CCSize m_imageSize;
    TileInfo*** m_tileInfos;
    float m_scaleFactor; // this variable affects m_mapOnScreenSize, m_tileRowNumOnScreen and m_tileColNumOnScreen
    cocos2d::CCTexture2D* m_placeHolder;
    
    TileThread** m_tileThreads;
    
    TileLayer();

    virtual void update( float deltaTime );
    
    void updateTiles();
    
    static void* loadTile( void* p );
    
    inline void deleteTile( TileModule* tm );
    
    void checkTileStatus();
    
    inline int tileFloor( const int tile ) const
    {
        if ( tile < 0 )
        {
            return 0;
        }
        
        return tile;
    }

    inline int tileCeilForRow( const int tileRow ) const
    {
        if ( tileRow >= m_tileRowNum )
        {
            return m_tileRowNum - 1;
        }
        
        return tileRow;
    }

    inline int tileCeilForCol( const int tileCol ) const
    {
        if ( tileCol >= m_tileColNum )
        {
            return m_tileColNum - 1;
        }
        
        return tileCol;
    }
    
    CREATE_FUNC( TileLayer );
};

#endif
