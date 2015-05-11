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
#include "job.h"
#include "buffer.h"
#include "navigator.h"


using namespace ev3dev;
using namespace std::literals::chrono_literals;

std::atomic< bool > killFlag;

constexpr int HISTORY_SIZE = 9;


struct DataPoint {
    int pos;
    int val;
};

using SwipeData = std::vector<DataPoint>;


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



struct CrossroadAnalyzer {

    CrossroadAnalyzer() :
        data( std::pair< Buffer< SwipeData >, int >( { HISTORY_SIZE }, 0 ) )
    {
        _navigator.initialize();
    }

    void run() {
        while ( !killFlag ) {
            data.waitAndReadOnce( [&]( std::pair< Buffer< SwipeData >, int > &sensorData ) { process( sensorData ); } );
        }
    }

    job::GuardedVar< std::pair< Buffer< SwipeData >, int > > data;
    job::GuardedVar< int > result; // or watever data type is needed here

protected:
    // this function will be called every time data are avalibale, it should
    // produce result into result variable, it shoud not access data variable
    void process( std::pair< Buffer< SwipeData >, int > &sensorData ) {
        process( sensorData.first, sensorData.second );

        sensorData.first.clear();
    }

    void process( Buffer< SwipeData > &sensorData, int distance ) {
        std::cout << "crossroad analyser" << std::endl;

        int low_last = -1000;
        int high_last = 1000;

        bool left = false;
        bool right = false;

        int index=0;
        for ( const SwipeData &swipe : sensorData ) { // iterate from oldest to data()
            int low, high = 0;

            for (auto i = swipe.cbegin(); i != swipe.cend(); ++i) {
                if ((index % 2) ? (i->val > 0) : (i->val < 0)) {
                    high = i->pos;
                } else if ((index % 2) ? (i->val < 0) : (i->val > 0)) {
                    low = i->pos;
                }
            }

            int diff_low = std::abs(low_last - low);
            int diff_high = std::abs(high_last - high);

            int width = std::abs(low - high);
            int last_width = std::abs(low_last - high_last);

            if (diff_high > 10 && width > last_width * 1.1)
                left = true;
            if (diff_low > 10 && width > last_width * 1.1)
                right = true;

            //std::cout << "path(" << low << " " << high << ") diff(" << diff_low << " " << diff_high << ") wider(" << left << " " << right << ")" << std::endl;

            low_last = low;
            high_last = high;

            index++;
        }

        int direction = 0;

        std::cout << "turn(" << left << " " << right << ") distance(" << distance << ")" << std::endl;

        if (left && !right) {
            _navigator.turnMet(-1, distance);

            direction = -1;
        } else if (!left && right) {
            _navigator.turnMet(1, distance);

            direction = 1;
        } else if (left && right) {
            direction = _navigator.crossroadsMet(distance);
        } else {
            direction = 0;
        }

        result.assign( direction );
    }
private:
    Navigator _navigator;

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

    void turn(const int direction) {
        std::cout << "turn: " << direction << std::endl;

        stop();

        _motor_L.set_run_mode( motor::run_mode_position );
        _motor_R.set_run_mode( motor::run_mode_position );

        _motor_L.set_pulses_per_second_sp( 400 );
        _motor_R.set_pulses_per_second_sp( 400 );

        _motor_L.set_position_mode( motor::position_mode_relative );
        _motor_R.set_position_mode( motor::position_mode_relative );

        // reset position
        _motor_L.set_position( 0 );
        _motor_R.set_position( 0 );

        // go forward by 350
        _motor_L.set_position_sp( 340 );
        _motor_R.set_position_sp( 340 );

        _motor_L.start();
        _motor_R.start();

        // wait till performed
        while (_motor_L.running() || _motor_R.running());

        // turn
        if (direction != 0) {
            const int position_sp = 335;

            std::cout << "turning" << std::endl;

            _motor_L.set_position_sp( direction == -1 ? position_sp : -position_sp );
            _motor_R.set_position_sp( direction == -1 ? -position_sp : position_sp );

            _motor_L.start();
            _motor_R.start();

            // wait till performed
            while (_motor_L.running() || _motor_R.running());
        }


        // reset position
        _motor_L.set_position( 0 );
        _motor_R.set_position( 0 );

        // set the default speed
        _motor_L.set_pulses_per_second_sp( speed );
        _motor_R.set_pulses_per_second_sp( speed );

        forward();
    }

    int position() {
        return (_motor_R.position() + _motor_L.position()) / 2;
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


class SwipeAnalyzer {
    static constexpr int blur_radius = 2;
public:

    SwipeAnalyzer( CrossroadAnalyzer &crossroad, DriveControl &drives  ) :
        _crossroad( &crossroad ), _drives( &drives )
    { }

    int process( SwipeData& swipe ) {

        auto cross = _crossroad->result.tryCopyOut();
        if ( cross.first ) { // results are valid
            // do crossroad

            // exit if in target
            if (cross.second == -2)
                exit(0);

            _drives->turn(cross.second);

            _oldpos = _drives->position();
            return 0;
        }

        // discard unusable data
        if (swipe.size() < 2)
            return 0;

        const int size = swipe.size();
/*
        std::cout << "Raw Value - Position: " << std::endl;
        for (const auto& i : swipe) {
            std::cout << (i.val ? '#' : '.');
        }
        std::cout << std::endl;
*/
        median_blur(swipe);
        gradient(swipe);
        _history.first.push_back( swipe );

        /*
        std::cout << "Processed Value - Position: " << std::endl;
        for (const auto& i : swipe) {
            std::cout << "(" << i.val << ")";
        }
        std::cout << std::endl << std::endl;
        */

        int min = 0, max = 0, minix = -1, maxix = -1;

        for ( int i = 0; i < size; ++i ) {
            int v = swipe[i].val;
            if ( v < min ) {
                min = v;
                minix = i;
            }
            if ( v > max ) {
                max = v;
                maxix = i;
            }
        }

        if ( minix == -1 && maxix == -1 ) { // lost :-/, just continue staright
//            std::cout << "lost" << std::endl;
            return 0;
        }

        int minpos = minix == -1 ? swipe.front().pos : swipe[ minix ].pos;
        int maxpos = maxix == -1 ? swipe.back().pos  : swipe[ maxix ].pos;

        int width = std::abs( maxpos - minpos );

        if ( !_was_wider ) {
            _was_wider = is_wider( width );
        }

        if ( _was_wider && is_narrower( width ) ) {
            _was_wider = false;
            // dispatch a new job for crosroad analysis
            int position = _drives->position();
//            std::cout << "widening, distance = " << _oldpos - position << std::endl;
            int dist = (_oldpos - position);
            _history.second = std::ceil(float(dist) / float(320));
            _crossroad->data.assign( _history );
            _last_width.clear();
        }

        if (_was_wider)
            return 0;

        int blackCenter = (minpos + maxpos) / 2;

//        std::cout << "bc = diff = " << blackCenter << " (" << minpos << ", " << maxpos << ")" << std::endl;

        int c = _linePid.update( blackCenter );
//        if ( c )
//            std::cout << "c = " << c << std::endl;
        return c;
    }

protected:
    bool is_wider(const int width) {
        _last_width.push_back( width );

//        for ( auto v : _last_width )
//            std::cout << float( v ) / float( width ) << " < ";
//        std::cout << std::endl;
        if (_last_width.size() != 3)
            return false;

        auto it2 = _last_width.begin(),
             it1 = it2++;
        for ( ; it2 != _last_width.end(); ++it1, ++it2 )
            if ( *it1 * 1.1 >= *it2 )
                return false;
        return true;
    }

    bool is_narrower(const int width) {
        _last_width.push_back( width );

        if (_last_width.size() != 3)
            return false;

        auto it2 = _last_width.begin(),
             it1 = it2++;
        for ( ; it2 != _last_width.end(); ++it1, ++it2 )
            if ( *it1 * 1.1 <= *it2 )
                return false;
        return true;
    }

    void gradient(SwipeData& swipe) {
        for (auto it = swipe.begin(); it != swipe.end() - 1; ++it)
            it->val -= (it + 1)->val;
    }

    void median_blur(SwipeData& swipe) {
        std::array< int, 2*blur_radius + 1 > neighbors;

        _temp.clear();
        for ( const auto& point : swipe )
            _temp.push_back(point.val);

        int size = int(_temp.size());
        for ( int i = 0; i < size; ++i ) {
            for ( int j = -blur_radius; j <= blur_radius; j++ )
                neighbors[ blur_radius-j ] = ( i+j < 0 || i+j > size ) ? 0 : _temp[i];
            std::sort( neighbors.begin(), neighbors.end() );

            swipe[i].val = neighbors[blur_radius];
        }
    }

private:
    std::vector<int>    _temp;
    PID                 _linePid = PID( 0.5, 10, 15, 100, 0 );
    Buffer< int >       _last_width = { 3 };
    CrossroadAnalyzer  *_crossroad = nullptr;
    DriveControl       *_drives = nullptr;
    std::pair< Buffer< SwipeData >, int > _history = { { HISTORY_SIZE }, 0 };
    int _oldpos = 0;
    bool _was_wider = false;
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



class SensorControl {
    static constexpr int speed = 900;
public:
    bool check() const { return _eye.connected() && _motor.connected(); }

    void init() {
        _eye.set_mode( "RGB-RAW" );

        _motor.reset();
        _motor.set_run_mode( motor::run_mode_position );
        _motor.set_stop_mode( motor::stop_mode_brake );
        _motor.set_regulation_mode( motor::mode_on );
        _motor.set_pulses_per_second_sp( speed );
        _motor.set_position_mode( motor::position_mode_absolute );

        _motor.set_position( 0 );
    }


    void update(SwipeData& data) {
        const int intensity = _eye.value(0) + _eye.value(1) + _eye.value(2);

        DataPoint point;
        point.pos = _motor.position();
        point.val = (intensity < 382) ? 1 : 0;

        _data.push_back(point);

        if (update_motor()) {
            _data.swap(data);
            _data.clear();
        }
    }

protected:
    bool update_motor() {
        if ( _motor.running() )
            return false;

        int pos_sp = ( _motor.position_sp() < 0 ) ? _limit_ccw : _limit_cw;

        _motor.set_position_sp( pos_sp );
        _motor.start();

        return true;
    }

private:
    SwipeData _data;

    int _limit_ccw = 80;
    int _limit_cw = -80;

    color_sensor _eye = color_sensor( INPUT_AUTO );
    medium_motor _motor = medium_motor( OUTPUT_AUTO );
};

class MainControl {
public:
    bool check() { return _sensors.check() && _drives.check(); }

    void run() {
        _sensors.init();
        _drives.init();

        _drives.forward();

        _crossroadThr = std::thread( [&] { _crossroad.run(); } );

        while ( !killFlag ) {
            update();
        }

        _crossroad.data.cancelWaits();
        _crossroad.result.cancelWaits();
        _crossroadThr.join();
        _drives.stop();
    }

protected:
    void update() {
        _swipe.clear();
        _sensors.update(_swipe);
        if (!_swipe.empty()) {
            int correction = _analyzer.process(_swipe);
            _drives.adjust( correction );
        }
    }

private:
    SwipeData     _swipe;
    CrossroadAnalyzer _crossroad;
    std::thread _crossroadThr;

    SensorControl _sensors;
    DriveControl  _drives;

    SwipeAnalyzer _analyzer = { _crossroad, _drives };
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
