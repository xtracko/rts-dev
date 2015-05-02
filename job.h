#include <atomic>
#include <mutex>
#include <condition_variable>

#ifndef _JOB_H
#define _JOB_H

namespace job {

using Guard = std::unique_lock< std::mutex >;

template< typename T >
struct GuardedVar {

    GuardedVar() {
        ready = false;
        canceled = false;
    }

    // try to assign value, if it is curretly being processed, no assignment
    // is done and false is returned
    bool tryAssign( T &val ) {
        auto g = protect( std::try_to_lock );
        if ( g.owns_lock() ) {
            _assign( val );
            return true;
        }
        return false;
    }

    // wait for value to be unguarded and assign it
    void assign( T &val ) {
        auto g = protect();
        _assign( val );
    }

    // waits untill value is assigned and then runs callback which should accept
    // one argument of type T &
    // also invalidates value
    template< typename Callback >
    void waitAndReadOnce( Callback callback ) {
        auto g = protect();
        cond.wait( g, [&] { return ready || canceled; } ); // wait untill ready
        if ( canceled )
            return;

        callback( value );

        ready = false;
    }

    // try to copy value - if it is ready copy it out and invalidate it
    // otherwise return default constructed value
    // first element of tuple signifies if value was present
    std::pair< bool, T > tryCopyOut() {
        if ( ready ) {
            auto g = protect();
            if ( ready ) {
                ready = false;
                return { true, value };
            }
        }
        return { false, T() };
    }

    void cancelWaits() { canceled = true; cond.notify_all(); }

  private:
    std::mutex mutex;
    std::condition_variable cond;
    T value;
    std::atomic< bool > ready;
    std::atomic< bool > canceled;

    template< typename... Args >
    Guard protect( Args... args ) { return Guard( mutex, args... ); }

    void _assign( T &val ) {
        this->value = val;
        ready = true;
        cond.notify_one();
    }
};

} // namespace job

#endif // _JOB_H
