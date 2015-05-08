#include <memory>
#include <utility>
#include <cassert>

#ifndef _BUFFER_H
#define _BUFFER_H

// see buffer-test.cpp for usage

template< typename Col >
auto reverseRange( Col &col ) {
    // C++ has Voldemort types (http://wiki.dlang.org/Voldemort_types)
    struct RR {
        RR( Col *self ) : self( self ) { }
        Col *self;
        auto begin() { return self->rbegin(); }
        auto end() { return self->rend(); }
    };
    return RR( &col );
}

template< typename T >
struct Buffer {
    using value_type = T;

    template< typename SelfPtr, typename ValRef >
    struct Iterator : std::iterator< std::bidirectional_iterator_tag, T > {
        Iterator( SelfPtr self, int ix ) : _self( self ), _ix( ix ) { }

        ValRef operator *() { return _self->_data.get()[ _ix ]; }

        Iterator &operator++() {
            _ix = _self->_nxt( _ix );
            return *this;
        }

        Iterator operator++( int ) {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        Iterator &operator--() {
            _ix = _self->_nxt( _ix, -1 );
            return *this;
        }

        Iterator operator--( int ) {
            auto copy = *this;
            --(*this);
            return copy;
        }

        bool operator==( Iterator o ) const { return _self == o._self && _ix == o._ix; }
        bool operator!=( Iterator o ) const { return !(*this == o); }

      private:
        SelfPtr _self;
        int _ix;
    };

    using iterator = Iterator< Buffer< T > *, T & >;
    using const_iterator = Iterator< const Buffer< T > *, const T & >;
    using reverse_iterator = std::reverse_iterator< iterator >;
    using const_reverse_iterator = std::reverse_iterator< const_iterator >;

    Buffer( int size ) :
        _size( size + 1 ), _read( 0 ), _write( 0 ),
        _data( new T[ _size ] )
    { }

    Buffer( const Buffer &o ) : Buffer( o._size - 1 ) {
        std::copy( o.begin(), o.end(), std::back_inserter( *this ) );
    }

    Buffer( Buffer &&o ) :
        _size( o._size ), _read( o._read ), _write( o._write ), _data( o._data.release() )
    { } // only operation alloved on o after this ctor is called is dtor

    Buffer &operator=( const Buffer &o ) {
        if ( &o == this )
            return *this;

        assert( _size == o._size );
        assert( _data );

        // insert new
        std::copy( o.begin(), o.end(), std::back_inserter( *this ) );
        return *this;
    }

    Buffer &operator=( Buffer &&o ) {
        if ( &o == this )
            return *this;

        swap( *this, o );
        return *this;
    }

    void swap( Buffer &o ) {
        assert( _size == o._size );
        std::swap( _read, o._read );
        std::swap( _write, o._write );
        std::swap( _data, o._data );
    }

    void push_back( const T &val ) { _push_back( [&]( T *to ) { *to = val; } ); }
    void push_back( T &&val ) { _push_back( [&]( T *to ) { *to = std::move( val ); } ); }

    void pop_back() {
        assert( _read != _write );
        _write = _nxt( _write, -1 );
    }

    void pop_front() {
        assert( _read != _write );
        _read = _nxt( _read );
    }

    bool empty() const { return _read == _write; }

    void clear() {
        _read = _write = 0;
    }

    // oldest element
    T &front() {
        assert( !empty() );
        return _data.get()[ _read ];
    }
    const T &front() const {
        assert( !empty() );
        return _data.get()[ _read ];
    }

    // newest element
    T &back() {
        assert( !empty() );
        return _data.get()[ _nxt( _write, -1 ) ];
    }

    const T &back() const {
        assert( !empty() );
        return _data.get()[ _nxt( _write, -1 ) ];
    }

    int size() const { return (_write + _size - _read) % _size; }

    iterator begin() { return iterator( this, _read ); }
    const_iterator begin() const { return const_iterator( this, _read ); }
    const_iterator cbegin() const { return begin(); }

    iterator end() { return iterator( this, _write ); }
    const_iterator end() const { return const_iterator( this, _write ); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return reverse_iterator( end() ); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator( end() ); }
    const_reverse_iterator crbegin() const { return rbegin(); }

    reverse_iterator rend() { return reverse_iterator( begin() ); }
    const_reverse_iterator rend() const { return const_reverse_iterator( begin() ); }
    const_reverse_iterator crend() const { return rend(); }

    T &operator[]( int ix ) { return _data.get()[ _nxt( _read, ix ) ]; }
    const T &operator[]( int ix ) const { return _data.get()[ _nxt( _read, ix ) ]; }

  private:
    const int _size;
    int _read;
    int _write;
    std::unique_ptr< T[] > _data;

    int _nxt( int x, int ix = 1 ) const {
        // this is weird, but we need to make sure result is positive even if
        // x + ix < 0
        return (((x + ix) % _size) + _size) % _size;
    }

    template< typename Push >
    void _push_back( Push push ) {
        auto nwrite = _nxt( _write );
        if ( nwrite == _read )
            pop_front();
        push( _data.get() + _write );
        _write = nwrite;
    }
};

namespace std {

template< typename T >
void swap( Buffer< T > &a, Buffer< T > &b ) {
    a.swap( b );
}

}

#endif // _BUFFER_H
