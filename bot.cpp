#include "ev3dev.h"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include <cassert>
#include "buffer.h"
#include "job.h"

using namespace ev3dev;
using namespace std::literals::chrono_literals;

std::atomic< bool > killFlag;

// http://www.mstarlabs.com/apeng/techniques/pidsoftw.html
struct PID {
    PID( float gain, float ti, float td, float upd, float setpoint = 0 ) :
        gain( gain ), ti( ti ), td( td ), upd( upd ), setpoint( setpoint ),
        integral( 0 ), derivative( setpoint )
    { }

    float operator()( float in ) { return update( in ); }

    float update( float in ) {
        auto err = in - setpoint;
        auto out = err;

        out += integral * ti / upd;
        integral += err;

        out += (err - derivative) * td / upd;
        derivative = err;

        return - gain * out;
    }

  private:
    // params
    const float gain;
    const float ti;
    const float td;
    const float upd;

    const float setpoint;

    // state
    float integral;
    float derivative;
};

struct SensorData {
    struct DataPoint {
        DataPoint() = default;
        DataPoint( int p, int c ) : position( p ), color( c ) { }

        int position;
        int color; // 1 - black, 6 - white
    };

    void clear() { _data.clear(); }

    size_t size() const { return _data.size(); }

    void add( int position, int color ) {
        _data.emplace_back( position, color );
    }

    int &col( size_t i ) { return _data[ i ].color; }
    int col( size_t i ) const { return _data[ i ].color; }
    int pos( size_t i ) const { return _data[ i ].position; }

    auto begin() { return _data.begin(); }
    auto end() { return _data.end(); }

    DataPoint operator[]( size_t i ) { return _data[ i ]; }

    void swap( SensorData& other ) {
        _data.swap( other._data );
    }

    void swap_cols( std::vector< int >& colors ) {
        assert( colors.size() == _data.size() );
        auto dit = _data.begin();
        auto cit = colors.begin();
        const auto end = colors.end();
        for ( ; cit < end; ++cit, ++dit )
            dit->color = *cit;
    }

    void print() const {
        for ( auto x : _data )
            std::cout << x.color << ", ";
        std::cout << std::endl;
    }

private:
    std::vector< DataPoint > _data;
};

int isBlack (int x)
{
    return (x==1);
}

struct CrossroadAnalyzer {

    CrossroadAnalyzer() : data( 8 ) { }

    void run() {
        while ( !killFlag ) {
            data.waitAndReadOnce( [&]( Buffer< SensorData > &sensorData ) {
                    process( sensorData );
                } );
        }
    }

    // this function will be called every time data are avalibale, it should
    // produce result into result variable, it shoud not access data variable
    void process( Buffer< SensorData > &sensorData ) {

        /*TODO: this could mean that we are on crossroad/angle
        we should look at history and analyze it:*/
        int whatsonline[8]={0};
        /*
        0 - straight line
        1 - long line
        2 - 2 lines
        3 - 3 lines
        4 - line on the left
        5 - line on the right
        */
        int index=0;
        for ( SensorData &x : reverseRange( sensorData ) ) { // iterate from oldest to data()
            int leftmost=200, rightmost=200, longest=0, number=0;
            int i;
            int leftnow=200, blacknow=0, nonblacknow=0;
            for (i=0; i < int(x.size()); i++)
            {
                if (leftnow!=200)
                {
                    if (isBlack(x.col(i)))
                    {
                        rightmost = x.pos(i);
                        nonblacknow = 0;
                    }
                    else
                    {
                        nonblacknow++;
                        if (nonblacknow>2)
                        {
                            nonblacknow = 0;
                            leftnow -= rightmost;
                            leftnow = (leftnow>=0) ? leftnow : -leftnow;
                            longest = (longest<leftnow) ? leftnow : longest;
                            leftnow = 200;
                            number++;
                            continue;
                        }
                    }
                }
                else
                {
                    if (isBlack(x.col(i)))
                    {
                        blacknow++;
                        if (blacknow>2)
                        {
                            blacknow = 0;
                            leftnow = x.pos(i-2);
                            if (leftmost==200) leftmost = x.pos(i-2);
                            rightmost = x.pos(i);
                        }
                    }
                    else
                    {
                        blacknow = 0;
                    }
                }
            }
            if (leftnow!=200)
            {
                leftnow -= rightmost;
                leftnow = (leftnow>=0) ? leftnow : -leftnow;
                longest = (longest<leftnow) ? leftnow : longest;
                number++;
            }
            if (x.pos(0)>x.pos(x.size() - 1))
            {
                i = leftmost;
                leftmost = rightmost;
                rightmost = i;
            }
            if (number>2) whatsonline[index] = 3;
            else if (number > 1) whatsonline[index] = 2;
            else if (leftmost<-30 && rightmost<0) whatsonline[index] = 4;
            else if (leftmost>0 && rightmost>30) whatsonline[index] = 5;
            else if (longest>40) whatsonline[index] = 1;
            index++;
        }

        result.assign( 42 /* pass result back to main thread */ );
    }

    job::GuardedVar< Buffer< SensorData > > data;
    job::GuardedVar< int > result; // or watever data type is needed here
};


class SensorAnalyzer {
public:
    SensorAnalyzer( CrossroadAnalyzer &ca ) :
        _dataBuf( 8 ), _linePid( -1, 10, 15, 100, 0 ), crossroadAnalyzer( ca )
    { }

    void save( SensorData &&data ) {
        _dataBuf.emplace_back( std::move( data ) );
    }

    int analyze() {
        // discard unusable data
        if (rawData().size() < 2)
            return 0;

        const int size = rawData().size();
        int cval = 10, cix = -1;
        for ( int i = 0; i < size; ++i ) {
            int p = std::abs( rawData().pos( i ) );
            if ( cval > p ) {
                cval = p;
                cix = i;
            }
        }

        // check if we got some weird distribution
        if ( cix < size / 4 || cix > (size / 4) * 3 ) {
            std::cout << "center = (" << cval << "," << cix << ")" << std::endl;
            // invalid data, get rid of them
            _dataBuf.pop_back();
            return 0;
        }

        int cpos = rawData()[ cix ].position;
        
        std::cout << "Color Position: " << std::endl;
        for(int i = 0; i < int(rawData().size()); i++)
        {
            std::cout << rawData().col(i) << " " << rawData().pos(i) << " " << std::endl;
        }
        
        std::cout << std::endl << std::endl;

        // if CrossroadAnalyzer is not working already get it running (otherwise
        // do nothing with it)
        crossroadAnalyzer.data.tryAssign( _dataBuf );
        // try to get result from (which was not yet processed) CrossroadAnalyzer
        auto res = crossroadAnalyzer.result.tryCopyOut();
        if ( res.first ) { // we got resuts

            // TODO: put crossroad driving code here (and maybe skip next part in that case)
        }

        median_blur();
        gradient();

        int min = 0, max = 0, minix = -1, maxix = -1;

        for ( int i = 0; i < size; ++i ) {
            int v = data().col( i );
            if ( v < min ) {
                min = v;
                minix = i;
            }
            if ( v > max ) {
                max = v;
                maxix = i;
            }
        }

        if ( minix == -1 || maxix == -1 ) { // lost :-/, just continue staright
            // invalid data, get rid of them
            _dataBuf.pop_back();
            return 0;
        }

        int minpos = data()[ minix ].position;
        int maxpos = data()[ maxix ].position;

        int blackCenter = (minpos + maxpos) / 2;
        int diff = cpos - blackCenter;

        std::cout << "bc = " << blackCenter << " (" << minpos << ", " << maxpos << ") cp = " << cpos << " diff = " << diff << std::endl;

        int c = _linePid.update( diff );
        if ( c )
            std::cout << "c = " << c << std::endl;
        return c;
    }
protected:

    // create data() from rawData()
    void median_blur() {
        const int radius = 2;
        std::array< int, 2*radius + 1 > neighbors;

        data().clear();
        const int size = int(rawData().size());

        for ( int i = 0; i < size; i++ ) {
            for ( int j = -radius; j <= radius; j++ ) {
                neighbors[ radius-j ] = ( i+j < 0 || i+j > size ) ? 0 : rawData().col( i );
            }
            std::sort( neighbors.begin(), neighbors.end() );

            data().add( rawData().pos( i ), neighbors[radius] );
        }
    }

    void gradient() {
        for ( size_t i = data().size()-1; i > 0; i-- )
            data().col( i ) -= data().col( i-1 );
    }

    SensorData &rawData() { return _dataBuf.front(); }
    SensorData &data() { return _data; }

private:
    Buffer< SensorData > _dataBuf;
    SensorData _data;
    PID _linePid;
    CrossroadAnalyzer &crossroadAnalyzer;
};





class SensorControl {
    static constexpr const int speed = 900;
public:
    bool check() const {
        return _eye.connected() && _motor.connected();
    }

    void init() {
        init_modes();
        calibrate_motor();
    }

    bool update( SensorAnalyzer& analyzer ) {
        bool written = false;

        if ( update_motor() ) {
            analyzer.save( std::move( _data ) );
            _data = SensorData(); // slightly safer then clean after move

            written = true;
        }

        const int intensity = _eye.value(0) + _eye.value(1) + _eye.value(2);
        const int is_black = (intensity < 382) ? 1 : 0;

        _data.add( _motor.position(), is_black );

        return written;
    }
protected:
    void init_modes() {
        _eye.set_mode( "RGB-RAW" );

        _motor.reset();
        _motor.set_run_mode( motor::run_mode_position );
        _motor.set_stop_mode( motor::stop_mode_brake );
        _motor.set_regulation_mode( motor::mode_on );
        _motor.set_pulses_per_second_sp( speed );
        _motor.set_position_mode( motor::position_mode_absolute );
    }

    void calibrate_motor() {
        _motor.set_position( 0 );

        _limit_ccw = 80;
        _limit_cw = -80;
    }

    bool update_motor() {
        if ( _motor.running() )
            return false;

        int pos_sp = ( _motor.position_sp() < 0 ) ? _limit_ccw : _limit_cw;

        _motor.set_position_sp( pos_sp );
        _motor.start();

        return true;
    }
private:
    int _limit_ccw;
    int _limit_cw;

    SensorData _data;

    color_sensor _eye = color_sensor( INPUT_AUTO );
    medium_motor _motor = medium_motor( OUTPUT_AUTO );
};





class DriveControl {
    static constexpr const int speed = 80;
public:
    bool check() {
        return _motor_L.connected() && _motor_R.connected();
    }

    void init() {
        init_modes();
    }

    void stop() {
        _motor_L.stop();
        _motor_R.stop();
    }

    void forward() {
        _motor_L.set_run_mode( motor::run_mode_forever );
        _motor_R.set_run_mode( motor::run_mode_forever );

        _motor_L.start();
        _motor_R.start();
    }

    void adjust( int i ) {
        _motor_L.set_pulses_per_second_sp( speed - i );
        _motor_R.set_pulses_per_second_sp( speed + i );
    }
protected:
    void init_modes() {
        _motor_L.reset();
        _motor_R.reset();

        _motor_L.set_stop_mode( motor::stop_mode_hold );
        _motor_R.set_stop_mode( motor::stop_mode_hold );

        _motor_L.set_regulation_mode( motor::mode_on );
        _motor_R.set_regulation_mode( motor::mode_on );

        _motor_L.set_polarity_mode( dc_motor::polarity_inverted );
        _motor_R.set_polarity_mode( dc_motor::polarity_inverted );

        _motor_L.set_pulses_per_second_sp( speed );
        _motor_R.set_pulses_per_second_sp( speed );
    }
private:
    large_motor  _motor_L = large_motor( OUTPUT_A );
    large_motor  _motor_R = large_motor( OUTPUT_D );
};





class MainControl {
public:

    MainControl() : _analyzer( _crossroadAnalyzer ) { }

    bool check() {
        return _sensors.check() && _drives.check();
    }

    void run() {
        _sensors.init();
        _drives.init();

        _corssroadThread = std::thread( [&] { _crossroadAnalyzer.run(); } );

        _drives.forward();
        while ( !killFlag && update() );

        _crossroadAnalyzer.data.cancelWaits();
        _crossroadAnalyzer.result.cancelWaits();
        _corssroadThread.join();
        _drives.stop();
    }
protected:
    bool update() {
        if ( button::enter.pressed() )
            return false;

        if ( _sensors.update( _analyzer ) ) {
            _drives.adjust( _analyzer.analyze() );
            // TODO
        }

        return true;
    }
private:
    SensorAnalyzer _analyzer;
    CrossroadAnalyzer _crossroadAnalyzer;
    std::thread _corssroadThread;

    SensorControl _sensors;
    DriveControl  _drives;
};

struct KillSwitch {

    KillSwitch() : _button( INPUT_4 ) { }
    ~KillSwitch() {
        if ( _thr.joinable() ) {
            _thr.join();
            _thr = std::thread();
        }
    }

    void run() {
        while ( !killFlag ) {
            if ( _button.value() > 0 ) {
                killFlag = true;
                std::cout << "killed" << std::endl;
            } else
                std::this_thread::sleep_for( 100ms );
        }
    }

    void spawn() {
        assert( !_thr.joinable() );
        _thr = std::thread( [&] { this->run(); } );
    }

  private:
    ev3dev::touch_sensor _button;
    std::thread _thr;
};

int main() {

    // stop control loop on signal
    killFlag = false;
    std::signal( SIGINT, []( int ) { killFlag = true; } );

    MainControl bot;
    KillSwitch killSwith;

    if ( !bot.check() )
        goto error;

    killSwith.spawn();
    bot.run();

    return 0;
error:
    std::cout << "miscount detected!" << std::endl;

    return 1;
}
