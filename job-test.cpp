// divine-cflags: -std=c++11

#include "job.h"
#include <vector>
#include <cassert>
#include <thread>
#include <numeric>

#ifdef __divine__
constexpr int lim = 3;

// override new to non-failing version
#include <new>

void* operator new  ( std::size_t count ) { return __divine_malloc( count ); }

enum APs { halt, pre_assign_in, post_get_out, post_get_in, pre_assign_out };
LTL( halt, F( halt ) );
LTL( pass_in, G( pre_assign_in -> X( !pre_assign_in U post_get_in) ) );
LTL( pass_out, G( pre_assign_out -> X( !pre_assign_out U post_get_out ) ) );
#else
constexpr int lim = 1000;
#define AP( x ) ((void)(0))
#endif

int main() {
    using namespace job;

    GuardedVar< std::vector< int > > in;
    GuardedVar< int > out;
    std::atomic< bool > done;
    done = false;

    std::thread t1( [&] {
            for ( int i = 0; i < lim; ++i ) {
                std::vector< int > vec( i );
                int j = 0;
                for ( auto &x : vec )
                    x = j++;
                AP( pre_assign_in );
                auto r = in.tryAssign( vec );
                if ( !r )
                    in.assign( vec );

                std::pair< bool, int > val{ false, 0 };
                while ( !val.first )
                    val = out.tryCopyOut();
                AP( post_get_out );
                assert( val.first );
                assert( std::accumulate( vec.begin(), vec.end(), 0 ) == val.second );
            }
            done = true;
            in.cancelWaits();
        } );

    std::thread t2( [&] {
            while ( !done ) {
                int sum = 0;
                in.waitAndReadOnce( [&]( std::vector< int > &vec ) {
                        AP( post_get_in );
                        sum = std::accumulate( vec.begin(), vec.end(), 0 );
                    } );
                AP( pre_assign_out );
                out.assign( sum );
            }
        } );
    t1.join();
    t2.join();
    assert( done );
    AP( halt );
}
