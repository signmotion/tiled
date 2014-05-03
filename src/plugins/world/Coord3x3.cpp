#include "Coord3x3.h"
#include <assert.h>


using namespace World;


Coord3x3::Coord3x3( enum Direction d ) {
    clear();
    mRaw[ 0 ] = d;
}




Coord3x3::Coord3x3( const Coord3x3& b ) {
    mRaw = b.mRaw;
}




Coord3x3::~Coord3x3() {
}




void
Coord3x3::clear() {
    mRaw.fill( UNDEFINED );
}




size_t
Coord3x3::survey() const {

    int  s = -1;
    for (auto itr = std::begin( mRaw );
         (itr != std::end( mRaw )) && (*itr != UNDEFINED);
         ++itr
    ) {
        s++;
    }

    return static_cast< size_t >( s );
}




enum Coord3x3::Direction
Coord3x3::reduce() {

    const auto thr = mRaw[ 0 ];
    const auto iLast = MAX_SURVEY - 1;
    for (size_t i = 0; i < iLast; ++i) {
        mRaw[ i ] = mRaw[ i + 1 ];
    }
    mRaw[ iLast ] = UNDEFINED;

    return thr;
}




bool
Coord3x3::detail( enum Direction d ) {

    const auto iLast = MAX_SURVEY - 1;
    if (mRaw[ iLast ] != UNDEFINED) { return false; }

    for (size_t i = iLast - 1; i >=0; --i) {
        mRaw[ i + 1 ] = mRaw[ i ];
    }
    mRaw[ 0 ] = d;

    return true;
}




std::pair< Cartesian2R, float >
Coord3x3::area(
    const Cartesian2R&  startCenter,
    const float         startSize
) const {

    assert( (startCenter.x() >= -1.0) && (startCenter.x() <= 1.0) );
    assert( (startCenter.y() >= -1.0) && (startCenter.y() <= 1.0) );
    assert( (startSize > 0.0) && (startSize <= 1.0) );

    if ( undefined() ) {
        return std::make_pair( startCenter, startSize );
    }

    // в какую ячейку территории 3 x 3 попадает область
    // работаем по обзору согласно координатам
    Coord3x3  c( *this );
    const enum Direction  d = c.reduce();
    assert( d != UNDEFINED );

    const auto   sh = shift< Cartesian2R >( d );
    const float  size = startSize / 3.0f;
    const Cartesian2R  center = startCenter + sh * size;

    return c.area( center, size );
}




Coord3x3::operator LineDirection() const {

    LineDirection  r = 0;
    int  pow = 1;
    for (auto itr = std::begin( mRaw ); itr != std::end( mRaw ); ++itr) {
        const enum Direction  d = *itr;
        if (d == UNDEFINED) { break; }
        r += d * pow;
        pow *= 10;
    }

    return (pow == 1) ? -1 : r;
}
