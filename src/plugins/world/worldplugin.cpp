#include "worldplugin.h"

#include "imagelayer.h"
#include "map.h"
#include "mapobject.h"
#include "mapwriter.h"
#include "objectgroup.h"
#include "properties.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"

#include <QDataStream>
#include <QFile>
#include <QFileInfo>

using namespace World;
using namespace Tiled;


bool
WorldPlugin::write(
    const Map*      map,
    const QString&  fileName
) {
    // # All map saving as current tmx-file and separated
    //   files (bottom and top a survey).
    // # Format saving files are TMX.
    // # Top-tmx is have name 'world.tmx'.
    // # Every map divided on the 9 parts (see Coord3x3::Direction).
    // # Every part is detailing a parent.
    // # All calculating properties declared with the prefix '='.

    const QString  dir = QFileInfo( fileName ).path();

    // @todo Removed the const_cast.
    return writePartMap( dir, const_cast< Map* >( map ) );
}




QString
WorldPlugin::fileName( int mapSize, int tileSize ) {
    return "world " +
        QString::number( mapSize  ) + "x " +
        QString::number( tileSize ) +
        ".tmx";
}




float
WorldPlugin::extent( const Map& map ) {

    foreach ( Layer* layer, map.layers() ) {
        ObjectGroup* objectGroup = layer->asObjectGroup();
        if ( !objectGroup ) { continue; }
        if (objectGroup->name() != "^") { continue; }

        foreach ( MapObject* child, objectGroup->objects() ) {
            const QString&  name = child->name();
            if (name != "0") { continue; }

            const QString  s = child->property( "extent" );
            bool converted = false;
            const float  e = s.toFloat( &converted );

            return ( child->hasProperty( "extent" )
                  && !s.isEmpty()
                  && converted
                  && (e > 0.0f)
            ) ? e : -FLT_MAX;

        } // foreach ( MapObject* child, objectGroup->objects() )

    } // foreach ( Layer* layer, map->layers() )

    return -FLT_MAX;
}



bool
WorldPlugin::size(
    int*        mapSize,
    int*        tileSize,
    const Map&  map,
    int         survey
) {
    const QString  su = ((survey >= 0) && (survey <= 8)) ?
        QString::number( survey ) :
        "0";

    foreach ( Layer* layer, map.layers() ) {
        ObjectGroup* objectGroup = layer->asObjectGroup();
        if ( !objectGroup ) { continue; }
        if (objectGroup->name() != "^") { continue; }

        foreach ( MapObject* child, objectGroup->objects() ) {
            if (child->name() != su) { continue; }

            const QString  fp = (su == "0") ? "=size" : "size";
            if ( !child->hasProperty( fp ) ) {
                mError = tr( ("The property '" + fp + "'"
                    " don't found in the object group"
                    " '^." + su + "'.").toStdString().c_str() );
                return false;
            }
            const QString size = child->property( fp );
            auto v = size.split( ' ' );
            if (v.size() != 2) {
                mError = tr( ("The property 'size' must be greater 2 values"
                    " separated by space.\n" + size).toStdString().c_str() );
                return false;
            }
            if ( !v[ 0 ].endsWith( "x" ) ) {
                mError = tr( ("The first value of the property 'size'"
                    " must be ends with the symbol 'x'.\n" + v[ 0 ]).toStdString().c_str() );
                return false;
            }
            bool convertedMapSize  = false;
            bool convertedTileSize = false;
            *mapSize  = v[ 0 ].remove( "x" ).toInt( &convertedMapSize );
            *tileSize = v[ 1 ].toInt( &convertedTileSize );
            if ( !convertedMapSize || !convertedTileSize ) {
                *mapSize = *tileSize = INT_MIN;
                return false;
            }
            if (*mapSize <= 0) {
                *mapSize = INT_MIN;
                mError = tr( "The first value of the property 'size' must be greater 0." );
                return false;
            }
            if (*tileSize <= 0) {
                *tileSize = INT_MIN;
                mError = tr( "The second value of the property 'size' must be greater 0." );
                return false;
            }

            break;
        }

        break;

    } // foreach ( Layer* layer, map->layers() )

    return true;
}




bool
WorldPlugin::updateSizeAndExtent( Map* map,  int survey ) {

    if (survey == -1) {
        for (survey = 0; survey <= 8; ++survey) {
            if ( !updateSizeAndExtent( map, survey ) ) {
                return false;
            }
        }
        return true;
    }


    // # Global size (equal size of map) and extent (setup manually)
    //   property saving into the object group '^.0'.
    // # Every level above '0' are consist of size (setup manually) and
    //   extent (calculating by the global size of the map).
    int  mapSize  = INT_MIN;
    int  tileSize = INT_MIN;
    if ( !size( &mapSize, &tileSize, *map, survey ) ) {
        // # If a level above 0 and it is not consist of 'size', when
        //   a level is ignoring.
        return true;
    }

    const float  extent = WorldPlugin::extent( *map );
    if (extent == -FLT_MAX) {
        mError = tr( "Don't found extent by run the updateSizeAndExtent()" );
        return false;
    }

    bool  presentTop = false;
    foreach ( Layer* layer, map->layers() ) {
        ObjectGroup* objectGroup = layer->asObjectGroup();
        if ( !objectGroup ) { continue; }
        if (objectGroup->name() != "^") { continue; }

        presentTop = true;
        foreach ( MapObject* child, objectGroup->objects() ) {
            const QString&  name = child->name();
            bool  converted = false;
            const int  n = name.toInt( &converted );
            if ( converted && (n == 0) ) {
                const QString s =
                    QString::number( map->width() ) + "x " +
                    QString::number( map->tileWidth() );
                objectGroup->setProperty( "=size", s );

            } else if ( converted && (n == survey) ) {
                const float  power = std::powf( 3, survey );
                const float  childExtent = extent / power;
                child->setProperty(
                    "=extent",
                    QString::number( childExtent )
                );

            } else {
                mError = tr( "Undefined name into the layer '^' by call updateSizeAndExtent()." );
                return false;
            }
        }

        // # Only one layer named by '^'.
        break;

    } // foreach ( Layer* layer, map->layers() )


    if ( !presentTop ) {
        mError = tr( "Don't found layer '^' in the updateSizeAndExtent()." );
        return false;
    }

    return true;
}



bool
WorldPlugin::writePartMap(
    const QString&   dir,
    Map*             map,
    const Coord3x3&  coord,
    int              newMapSize,
    int              newTileSize
) {
    // get info about this map
    const auto  orientation = map->orientation();
    if (orientation != Map::Orthogonal) {
        mError = tr( "Only an orthogonal map maybe exporting." );
        return false;
    }
    const auto  width  = map->width();
    const auto  height = map->height();
    if (width != height) {
        mError = tr( "Width and height for the map are must be equals." );
        return false;
    }
    const auto  tileWidth  = map->tileWidth();
    const auto  tileHeight = map->tileHeight();
    if (tileWidth != tileHeight) {
        mError = tr( "Width and height of the tile are must be equals." );
        return false;
    }


    // get a global extent for the map
    const float  extent = WorldPlugin::extent( *map );
    if (extent == -FLT_MAX) {
        mError = tr( "The property 'extent' must be declared." );
        return false;
    }
    if (extent < 0) {
        mError = tr( "The property 'extent' must be greater 0." );
        return false;
    }


    if ( coord.undefined() ) {
        // calc and set a size of properties for the top map
        updateSizeAndExtent( map );

        // saving original background images to the top 'dir'
        foreach ( Layer* layer, map->layers() ) {
            ImageLayer* imageLayer = layer->asImageLayer();
            if ( !imageLayer ) { continue; }
            const QString src = imageLayer->imageSource();
            const QString srcFileName = QFileInfo( src ).fileName();
            const QString dest = dir + "/" + srcFileName;
            if ( QFile::exists( dest ) && !QFile::remove( dest ) ) {
                mError = tr( "Could not open image for copy." );
                return false;
            }
            if ( !QFile::copy( src, dest ) ) {
                mError = tr( "Could not copy image file." );
                return false;
            }
            imageLayer->setSource( srcFileName );

        } // foreach ( Layer* layer, map->layers() )

        // saving the top map
        const QString  fn =
            dir + "/" + WorldPlugin::fileName( width, tileWidth );
        QFile  file( fn );
        if ( !file.open( QFile::WriteOnly | QFile::Text ) ) {
            mError = tr( "Could not open file for writing the top map." );
            return false;
        }
        MapWriter  mapWriter;
        if ( !mapWriter.writeMap( map, fn ) ) {
            mError = mapWriter.errorString();
            return false;
        }

        // dividing map on the 9 parts and saving them
        // @see Notes above.
        for (size_t k = 0; k <= 8; ++k) {
            // @test
            if (k != 0) { continue; }

            const auto  d = static_cast< Coord3x3::Direction >( k );
            for (int survey = 0; survey <= 4; ++survey) {
                coord.detail( k );

                // # Child size declared into the object group '^.*'.
                int childSize     = INT_MIN;
                int childTileSize = INT_MIN;
                const int  nextSurvey = survey + 1;
                if ( !WorldPlugin::size( &childSize, &childTileSize, *map, nextSurvey  ) ) {
                    // # If a level above 0 and it is not consist of 'size', when
                    //   a level is ignoring.
                    continue;
                }

                const QString  pathD = dir + "/" + QString::number( d );
                if ( !QDir( pathD ).exists() && !QDir().mkdir( pathD ) ) {
                    mError = tr( "Could not create dir for writing." );
                    return false;
                }
                if ( !writePartMap(
                    pathD, map, d, nextSurvey,
                    childSize, childTileSize
                ) ) {
                    //mError = tr( "Don't create a part of map." );
                    return false;
                }
            }

        } // for (size_t k = 0; ...

        return true;

    } // if (survey == -1)


    if (d == Coord3x3::UNDEFINED) {
        mError = tr( "Undefined number of part (aka direction)." );
        return false;
    }

    if (survey < 1) {
        mError = tr( "Incorrect survey." );
        return false;
    }


    // get a child size for the child-maps
    // # Child size declared into the object group '^.*'.
    int childSize     = INT_MIN;
    int childTileSize = INT_MIN;
    const int  nextSurvey = survey + 1;
    if ( !WorldPlugin::size( &childSize, &childTileSize, *map, nextSurvey  ) ) {
        mError = tr( "Don't harvested sizes on the next survey." );
        return true;
    }


    // copiyng all inner images to the file folder
    // # From all images grab a part by the coords.
    const QString  parentDir = dir + "/..";
    if ( !writePartBackground(
        parentDir, *map, d, survey,
        newMapSize, newTileSize
    ) ) {
        //mError = tr( "Don't wrote a part of images." );
        return false;
    }


    Map r(
        map->orientation(),
        newMapSize,  newMapSize,
        newTileSize, newTileSize
    );

    foreach ( Layer* layer, map->layers() ) {
        if ( layer->isImageLayer() ) {
            if ( !harvestLayer( &r, dir, *map, *layer->asImageLayer() ) ) {
                return false;
            }
        } else if ( layer->isObjectGroup() ) {
            if ( !harvestLayer( &r, dir, *map, d, *layer->asObjectGroup() ) ) {
                return false;
            }
        }
    } // foreach ( Layer* layer, map.layers() )


    // # Size and extent are equal to all parts.


    // saving the prepared map
    const QString  fn =
        dir + "/" + fileName( newMapSize, newTileSize );
    QFile  file( fn );
    if ( !file.open( QFile::WriteOnly | QFile::Text ) ) {
        mError = tr( "Could not open file for writing the part of map." );
        return false;
    }
    MapWriter  mapWriter;
    if ( !mapWriter.writeMap( &r, fn ) ) {
        mError = mapWriter.errorString();
        return false;
    }

    return true;
}




bool
WorldPlugin::writePartBackground(
    const QString&            dir,
    const Map&                map,
    const Coord3x3&           coord,
    int                       newMapSize,
    int                       newTileSize
) {
    // # All images on the map already saved in the full quad.

    const int sizeImage = newMapSize * newTileSize;

    // extracting and saving background images
    const auto  relative = coord.area();
    const Cartesian2R&  shift = relative.first;
    const float  si = relative.second;

    foreach ( Layer* layer, map.layers() ) {
        ImageLayer* imageLayer = layer->asImageLayer();
        if ( !imageLayer ) { continue; }

        const QString&  bfn = imageLayer->imageSource();
        const QString  src = dir + "/" + bfn;
        if ( !QFile::exists( src ) ) {
            mError = tr( ("Could not found an image for the background"
                " by the path '" + src + "'").toStdString().c_str() );
            return false;
        }

        const QImage  image( src );
        if ( image.isNull() ) {
            mError = tr( ("Could not load an image for the background"
                " by the path '" + src + "'").toStdString().c_str() );
            return false;
        }
        if (image.width() != image.height()) {
            mError = tr( "Image for the background must be quad." );
            return false;
        }

        // @see Divide in the note of enum Direction.
        const int  size = image.width();
        const Cartesian2R  rcf(
                   (shift.x() + 0.5f - si / 2.0f) * size,
            size - (shift.y() + 0.5f + si / 2.0f) * size
        );
        const auto rc = static_cast< Cartesian2I >( rcf );
        const float  rs = si * size;
        const QRect  rect( rc.x(), rc.y(), rs, rs );

        // @test
        if ( (rc.x() < 0) || (rc.y() < 0)
          || (rs < 0)
          || ((rc.x() + rs) > size)
          || ((rc.y() + rs) > size)
        ) {
            const QString  msg =
                "Not correct rectangle for the background image.\n" +
                QString::number( shift.x() ) + " " +
                QString::number( shift.y() ) + "\n" +
                QString::number( rc.x() ) + " " +
                QString::number( rc.y() ) + "\n" +
                QString::number( rs ) + "\n";
            mError = tr( msg.toStdString().c_str() );
            return false;
        }

        const QImage r = createSubImage( image, rect, sizeImage );
        if ( r.isNull() ) {
            mError = tr( "Could not create a subimage for the background." );
            return false;
        }

        /* @test
        const QString  t =
            QString::number( rc.x() ) + " " +
            QString::number( rc.y() ) + " " +
            QString::number( rs.x() ) + " " +
            QString::number( rs.y() );
        mError = tr( t.toStdString().c_str() );
        return false;
        */

        const QString pathD =
            dir + "/" + QString::number( d ) + "/" + bfn;
        // # The 'pathD' is already created.
        QFile  fileD( pathD );
        if ( fileD.exists() && !fileD.remove() ) {
            mError = tr( "Can't remove an old file of background subimage." );
            return false;
        }
        if ( !r.save( pathD ) ) {
            mError = tr( "Could not save a background subimage." );
            return false;
        }

    } // foreach ( Layer* layer, map.layers() )


    return true;
}




bool
WorldPlugin::harvestLayer(
    Tiled::Map*               r,
    const QString&            dir,
    const Tiled::Map&         map,
    const Tiled::ImageLayer&  layer
) {
    // # All images already prepared. See writePartBackground().
    ImageLayer*  l = static_cast< ImageLayer* >( layer.clone() );

    /* @test
    const QString  file = dir + "/" + layer.imageSource();
    const QImage  image( file );
    if ( image.isNull() || !l->loadFromImage( image, file ) ) {
        mError = tr( ("Could not load from image in the"
            " harvestLayer( ImageLayer ).\n" + file).toStdString().c_str() );
        return false;
    }
    */

    l->setSource( layer.imageSource() );
    l->setImage( QPixmap() );
    r->addLayer( l );

    return true;
}




bool
WorldPlugin::harvestLayer(
    Tiled::Map*                r,
    const QString&             dir,
    const Tiled::Map&          map,
    enum Coord3x3::Direction   d,
    const Tiled::ObjectGroup&  group
) {
    const int childMapSize  = r->width();
    const int childTileSize = r->tileWidth();
    const int parentMapSize  = map.width();
    const int parentTileSize = map.tileWidth();

    ObjectGroup*  g = static_cast< ObjectGroup* >( group.clone() );
    const QString&  name = g->name();
    if (name == "^") {
        scalingAreaQuad(
            g,
            childMapSize,  childTileSize,
            parentMapSize, parentTileSize
        );

    } else {
        /* - @todo Требует отладки.
        shrinking(
            g, d,
            childMapSize,  childTileSize
        );
        scaling(
            g, d,
            childMapSize,  childTileSize,
            parentMapSize, parentTileSize
        );
        */
    }

    if ( !g->isEmpty() ) {
        r->addLayer( g );
    }

    return true;
}




void
WorldPlugin::shrinking(
    Tiled::ObjectGroup*       group,
    enum Coord3x3::Direction  d,
    int  childMapSize,
    int  childTileSize
) {
    const Coord3x3  coord( d );
    const auto  relative = coord.area();
    const Cartesian2R&  shift = relative.first;
    const float  si = relative.second;
    const int  size = childMapSize * childTileSize;
    const Cartesian2R  rcf(
               (shift.x() + 0.5f - si / 2.0f) * size,
        size - (shift.y() + 0.5f + si / 2.0f) * size
    );
    const auto  rc = static_cast< Cartesian2I >( rcf / childTileSize );
    const float  rs = si * size / childTileSize;
    const QRectF  rect( rc.x(), rc.y(), rs, rs );

    QList< MapObject* >  forRemoved;
    foreach ( MapObject* child, group->objects() ) {
        const enum MapObject::Shape  shape = child->shape();
        // # Other - without changes.
        if ( (shape == MapObject::Polygon)
          || (shape == MapObject::Polyline)
        ) {
            QPolygonF  poly = child->polygon();
            const QPointF  pos = child->position();
            poly.translate( pos );
            bool  inner = false;
            foreach (QPointF point, poly) {
                if ( rect.contains( point ) ) {
                    inner = true;
                    break;
                }
            }
            if ( !inner ) {
                forRemoved.push_back( child );
            }
        }
    } // foreach ( MapObject* child, group->objects() )

    foreach ( MapObject* child, forRemoved ) {
        group->removeObject( child );
    }
}




void
WorldPlugin::scaling(
    Tiled::ObjectGroup*  group,
    enum Coord3x3::Direction  d,
    int  childMapSize,
    int  childTileSize,
    int  parentMapSize,
    int  parentTileSize
) {
    const Coord3x3  coord( d );
    const auto  relative = coord.area();
    const Cartesian2R&  shift = relative.first;
    const float  si = relative.second;
    const int  size = childMapSize * childTileSize;
    const Cartesian2R  rcf(
               (shift.x() + 0.5f - si / 2.0f) * size,
        size - (shift.y() + 0.5f + si / 2.0f) * size
    );
    const auto  rc = static_cast< Cartesian2I >( rcf / childTileSize );
    const QPointF  sh( -rc.x(), -rc.y() );

    const float  k =
        static_cast< float >( childMapSize  * childTileSize ) /
        static_cast< float >( parentMapSize * parentTileSize );
    const float  t =
        static_cast< float >( parentTileSize ) /
        static_cast< float >( childTileSize );
    const float  kt = k * t;

    foreach ( MapObject* child, group->objects() ) {
        const enum MapObject::Shape  shape = child->shape();
        if ( (shape == MapObject::Polygon)
          || (shape == MapObject::Polyline)
        ) {
            child->setPosition( child->position() * kt );
            QPolygonF  poly = child->polygon();
            for (auto ptr = poly.begin(); ptr != poly.end(); ++ptr) {
                *ptr = *ptr * kt;
            }
            child->setPolygon( poly );
        }
    } // foreach ( MapObject* child, group->objects() )
}




void
WorldPlugin::scalingAreaQuad(
    Tiled::ObjectGroup*  group,
    int  childMapSize,
    int  childTileSize,
    int  parentMapSize,
    int  parentTileSize
) {
    const float  k =
        static_cast< float >( childMapSize  * childTileSize ) /
        static_cast< float >( parentMapSize * parentTileSize );
    const float  t =
        static_cast< float >( parentTileSize ) /
        static_cast< float >( childTileSize );
    const float  kt = k * t;
    foreach ( MapObject* child, group->objects() ) {
        child->setPosition( child->position() * kt );
        child->setWidth(    child->width()    * kt );
        child->setHeight(   child->height()   * kt );
    }
}



QImage
WorldPlugin::createSubImage(
    const QImage&  source,
    const QRect&   rect,
    const int      size
) {
    const size_t  offset =
        rect.x() * source.depth() / 8 +
        rect.y() * source.bytesPerLine();
    const QImage  r(
        source.bits() + offset,
        rect.width(),
        rect.height(),
        source.bytesPerLine(),
        source.format()
    );
    return (size == -1) ?
        r :
        r.scaled( size, size, Qt::IgnoreAspectRatio, Qt::FastTransformation );
}




#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2( World, WorldPlugin )
#endif
