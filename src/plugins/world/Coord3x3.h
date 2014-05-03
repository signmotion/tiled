#ifndef COORD3X3PLUGIN_H
#define COORD3X3PLUGIN_H

#include <array>
#include <type_traits>


namespace World {


template< typename T, typename P >
class Cartesian2 {
public:
    inline Cartesian2() :
        mX( UNDEFINED_VALUE() ),  mY( UNDEFINED_VALUE() )
    {
    }

    inline Cartesian2( T x, T y ) :
        mX( x ),  mY( y )
    {
        static_assert(
            std::is_arithmetic< T >::value,
            "Type must be arithmetic (integral or floating-point)."
        );
    }

    inline T x() const { return mX; }
    inline T y() const { return mY; }

    inline bool operator==( const Cartesian2& b ) const {
        return (b.x() == UNDEFINED_VALUE) && (b.y() == UNDEFINED_VALUE);
    }

    inline bool undefined() const {
        return (*this == UNDEFINED);
    }

    inline P operator+( const Cartesian2& b ) const {
        return P( mX + b.x(),  mY + b.y() );
    }

    inline P operator-( const Cartesian2& b ) const {
        return P( mX - b.x(),  mY - b.y() );
    }

    inline P operator*( T k ) const {
        return P( mX * k,  mY * k );
    }

    inline P operator/( T k ) const {
        return P( mX / k,  mY / k );
    }


public:
    static T  UNDEFINED_VALUE() {
        static const T  u = std::numeric_limits< T >::max();
        return u;
    }

    static P const&  UNDEFINED() {
        static const P  u(
            UNDEFINED_VALUE(),  UNDEFINED_VALUE()
        );
        return u;
    }

    static P const&  ZERO() {
        static const P  zero(
            static_cast< T >( 0 ),  static_cast< T >( 0 )
        );
        return zero;
    }

    static P const&  ONE() {
        static const P  one(
            static_cast< T >( 1 ),  static_cast< T >( 1 )
        );
        return one;
    }


private:
    T  mX;
    T  mY;
};




class Cartesian2I;
class Cartesian2R;




class Cartesian2I : public Cartesian2< int, Cartesian2I > {
public:
    inline Cartesian2I( int x, int y ) :
        Cartesian2< int, Cartesian2I >( x, y ) {}

    operator Cartesian2R() const;
};




class Cartesian2R : public Cartesian2< float, Cartesian2R > {
public:
    inline Cartesian2R( float x, float y ) :
        Cartesian2< float, Cartesian2R >( x, y ) {}

    operator Cartesian2I() const;
};




inline Cartesian2I::operator Cartesian2R() const {
    return Cartesian2R(
        static_cast< float >( x() ),
        static_cast< float >( y() )
    );
}




inline Cartesian2R::operator Cartesian2I() const {
    return Cartesian2I(
        static_cast< int >( x() ),
        static_cast< int >( y() )
    );
}




class Coord3x3 {
public:
    // Макс. высота обзора, с которой может работать класс.
    static const size_t  MAX_SURVEY = 2;

    // NW  N  NE
    //   8 1 5
    // W 4 0 2 E
    //   7 3 6
    // SW  S  SE
    enum Direction {
        UNDEFINED = -1,
        CENTER = 0,
        N  = 1,  E, S,   W,
        NE = 5, SE, SW, NW
    };

    typedef std::array< Direction, MAX_SURVEY + 1 >  raw_t;

    typedef long long  LineDirection;


public:
    explicit Coord3x3( enum Direction d = UNDEFINED );

    explicit Coord3x3( const Coord3x3& );

    virtual ~Coord3x3();

    void clear();

    // @return Высота обзора по имеющимся координатам.
    size_t survey() const;

    // Координата, в которой значение max() удалено
    // (увеличение размера области).
    // @return Отброшенная координата или UNDEFINED, если
    //         отбрасывать нечего.
    enum Direction reduce();

    // Координата, в которой крайнее значение заменено
    // (уменьшение размера области).
    // @return true если уточнение успешно.
    bool detail( enum Direction );

    inline bool undefined() const {
        return (mRaw[ 0 ] == UNDEFINED);
    }

    // @return Смещение относительно заданной ячейки.
    //         Диапазон ( -1, 1 ).
    template< class Coord >
    static Coord const& shift( enum Direction );

    // @return Координаты центра (first) и *относительный*
    //         размер (second) области.
    // # Размер рассматриваемой территории принимается == 1.
    // # Центр территории имеет координаты (0, 0). => Возвращаемое
    //   значение (first) лежит в диапазоне [-0.5, 0.5), (second) -
    //   в диапазоне (0.0, 1.0].
    std::pair< Cartesian2R, float >  area(
        const Cartesian2R&  startCenter = Cartesian2R::ZERO(),
        const float         startSize   = 1.0f
    ) const;

    operator LineDirection() const;

	
private:
    raw_t  mRaw;
};




template< class Coord >
Coord const&
Coord3x3::shift( enum Direction d ) {

    static const Coord  SHIFT[ 9 + 1 ] = {
        Coord(  0,  0 ),
        Coord(  0,  1 ),
        Coord(  1,  0 ),
        Coord(  0, -1 ),
        Coord( -1,  0 ),
        Coord(  1,  1 ),
        Coord(  1, -1 ),
        Coord( -1, -1 ),
        Coord( -1,  1 ),
        Coord(
            Coord::UNDEFINED_VALUE(),
            Coord::UNDEFINED_VALUE()
        )
    };

    return ((d < 0) || (d > 8)) ? SHIFT[ 9 ] : SHIFT[ d ];
}


} // namespace World

#endif // COORD3X3PLUGIN_H
