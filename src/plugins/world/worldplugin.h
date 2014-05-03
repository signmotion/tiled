#ifndef WORLDPLUGIN_H
#define WORLDPLUGIN_H

#include "world_global.h"

#include "gidmapper.h"
#include "mapwriterinterface.h"
#include "Coord3x3.h"

#include <QDir>
#include <QObject>


namespace Tiled {
class MapObject;
class ObjectGroup;
class Properties;
class TileLayer;
class Tileset;
}


namespace World {

class WORLDSHARED_EXPORT WorldPlugin :
    public QObject,
    public Tiled::MapWriterInterface
{
    Q_OBJECT
    Q_INTERFACES( Tiled::MapWriterInterface )
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA( IID "org.mapeditor.MapReaderInterface" FILE "plugin.json" )
#endif


public:
    inline WorldPlugin() {}

    virtual inline ~WorldPlugin() {}

    bool write( const Tiled::Map* map, const QString& fileName );

    inline bool supportsFile( const QString& fileName ) const {
        return QFileInfo( fileName ).suffix() ==
               QLatin1String( "world" );
    }

    inline QString nameFilter() const { return tr( "World files (*.world)"); }

    inline QString errorString() const { return mError; }


private:
    static QString fileName( int mapSize, int tileSize );

    // @return -FLT_MAX when the property 'extent' is not declarated.
    static float extent( const Tiled::Map& );

    bool size(
        int*  mapSize,
        int*  tileSize,
        const Tiled::Map&,
        int   survey = -1
    );

    bool updateSizeAndExtent( Tiled::Map*,  int survey = -1 );

    bool writePartMap(
        const QString&  dir,
        Tiled::Map*,
        const Coord3x3&     = Coord3x3(),
        int     newMapSize  = 0,
        int     newTileSize = 0
    );

    bool writePartBackground(
        const QString&  dir,
        const Tiled::Map&,
        const Coord3x3&,
        int  newMapSize,
        int  newTileSize
    );

    bool harvestLayer(
        Tiled::Map*,
        const QString&  dir,
        const Tiled::Map&,
        const Tiled::ImageLayer&
    );

    bool harvestLayer(
        Tiled::Map*,
        const QString&  dir,
        const Tiled::Map&,
        enum Coord3x3::Direction,
        const Tiled::ObjectGroup&
    );

    static void shrinking(
        Tiled::ObjectGroup*,
        enum Coord3x3::Direction,
        int childMapSize,
        int childTileSize
    );

    static void scaling(
        Tiled::ObjectGroup*,
        enum Coord3x3::Direction,
        int childMapSize,
        int childTileSize,
        int parentMapSize,
        int parentTileSize
    );

    static void scalingAreaQuad(
        Tiled::ObjectGroup*,
        int childMapSize,
        int childTileSize,
        int parentMapSize,
        int parentTileSize
    );

    QImage createSubImage(
        const QImage&,
        const QRect&,
        int size = -1
    );

	
private:
    QString  mError;
};

} // namespace World

#endif // WORLDPLUGIN_H
