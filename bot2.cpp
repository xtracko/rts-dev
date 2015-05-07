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


using namespace ev3dev;
using namespace std::literals::chrono_literals;

std::atomic< bool > killFlag;



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


class SwipeAnalyzer {
    static constexpr int blur_radius = 2;
public:
    int process(SwipeData& swipe) {
        // discard unusable data
        if (swipe.size() < 2)
            return 0;

        const int size = swipe.size();

        /*
        std::cout << "Raw Value - Position: " << std::endl;
        for (const auto& i : swipe) {
            std::cout << i.val << " " << i.pos << std::endl;
        }
        std::cout << std::endl << std::endl;
        */

        median_blur(swipe);
        gradient(swipe);

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
            std::cout << "lost" << std::endl;
            return 0;
        }

        int minpos = minix == -1 ? swipe.front().pos : swipe[ minix ].pos;
        int maxpos = maxix == -1 ? swipe.back().pos  : swipe[ maxix ].pos;

        if ( is_wider( std::abs( maxpos - minpos ) ) ) {
            // dispatch a new job for crosroad analysis
            std::cout << "widening" << std::endl;
        }

        int blackCenter = (minpos + maxpos) / 2;

        std::cout << "bc = diff = " << blackCenter << " (" << minpos << ", " << maxpos << ")" << std::endl;

        int c = _linePid.update( blackCenter );
        if ( c )
            std::cout << "c = " << c << std::endl;
        return c;
    }

protected:
    bool is_wider(const int width) {
        bool wider = false;

        std::cout << float( _last_width ) / float( width ) << std::endl;
        if (_last_width * 1.1 < width)
            wider = true;
        _last_width = width;
        return wider;
    }

    void gradient(SwipeData& swipe) {
        for (auto it = swipe.begin() + 1; it != swipe.end(); ++it)
            it->val -= (it - 1)->val;
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
    std::vector<int> _temp;
    PID              _linePid = PID( 0.5, 10, 15, 100, 0 );
    int              _last_width = 0;
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
    bool check() { return _sensors.check() && _drives.check(); }

    void run() {
        _sensors.init();
        _drives.init();

        _drives.forward();

        while ( !killFlag ) {
            update();
        }

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
    SwipeAnalyzer _analyzer;

    SensorControl _sensors;
    DriveControl  _drives;
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
