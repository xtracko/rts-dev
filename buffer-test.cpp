#include "buffer.h"

int main() {
    Buffer< int > buf( 16 );
    for ( int i = 0; i < 16; ++i ) {
        buf.emplace_back( i );
        assert( buf.size() == i + 1 );
    }
    for ( int i = 0; i < 16; ++i )
        assert( buf[ i ] == i );

    for ( int i = 16; i < 64; ++i ) {
        buf.emplace_back( i );
        assert( buf.size() == 16 );
    }

    for ( int i = 0; i < 16; ++i )
        assert( buf[ i ] == 48 + i );

    {
        int i = 48;
        for ( auto x : buf )
            assert( x == i++ );
        assert( i == 64 );
    }

    for ( int i = 0; i < 8; ++i )
        buf.emplace_back( i );

    for ( int i = 0; i < 8; ++i )
        assert( buf[ i ] == 56 + i );
    for ( int i = 8; i < 16; ++i )
        assert( buf[ i ] == i - 8 );

    for ( int i = 0; i < 8; ++i ) {
        assert( buf.front() == 56 + i );
        buf.pop_front();
        assert( buf.size() == 16 - i - 1 );
    }
    assert( buf.size() == 8 );

    {
        int i = 0;
        for ( auto x : buf )
            assert( x == i++ );
        assert( i == 8 );
    }

    for ( int i = 8; i < 24; ++i ) {
        buf.emplace_back( i );
        assert( buf.size() == std::min( i + 1, 16 ) );
    }

    {
        int i = 8;
        for ( auto x : buf )
            assert( x == i++ );
        assert( i == 24 );
    }

    buf.pop_back();
    {
        int i = 8;
        for ( auto x : buf )
            assert( x == i++ );
        assert( i == 23 );
    }
}
