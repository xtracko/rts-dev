#include <thread>
#include <chrono>
#include <iostream>
#include <ios>

#include "ev3dev.h"


using namespace ev3dev;
using namespace std;
using namespace std::literals::chrono_literals;



enum class Color : int {
    none = 0,
    black,
    blue,
    green,
    yellow,
    red,
    white,
    brown
};



ostream& operator<< (ostream& os, const ev3dev::sensor& dev) {
    os << "[conn: " << dev.connected();
    if ( dev.connected() )
        os  << " port: " << dev.port_name()
            << " mode: " << dev.mode();
    return os << "]";
}

ostream& operator<< (ostream& os, const ev3dev::motor& dev) {
    os << "[conn: " << dev.connected();
    if ( dev.connected() )
        os << " port: " << dev.port_name()
           << " type: " << dev.type();
    return os << "]";
}



class DriveControl {
    static constexpr const int _speed = 100;
public:
    bool check() const {
        return _motor_L.connected() && _motor_R.connected();
    }

    void dump() const {
        cout << "left  motor  " << _motor_L << endl;
        cout << "right motor  " << _motor_R << endl;
    }

    void  init() {
        _motor_L.reset();
        _motor_R.reset();

        _motor_L.set_stop_mode( motor::stop_mode_hold );
        _motor_R.set_stop_mode( motor::stop_mode_hold );

        _motor_L.set_regulation_mode( motor::mode_on );
        _motor_R.set_regulation_mode( motor::mode_on );

        _motor_L.set_pulses_per_second_sp( _speed );
        _motor_R.set_pulses_per_second_sp( _speed );
    }

    void stop() {
        _motor_L.stop();
        _motor_R.stop();
    }

    void start() {
        _motor_L.start();
        _motor_R.start();
    }
public:
    void forward() {
        stop();
        _motor_L.set_polarity_mode( dc_motor::polarity_normal );
        _motor_R.set_polarity_mode( dc_motor::polarity_normal );

        _motor_L.set_run_mode( motor::run_mode_forever );
        _motor_R.set_run_mode( motor::run_mode_forever );
        start();
    }

    void turn( int position ) {
        stop();
        _motor_L.set_polarity_mode( dc_motor::polarity_inverted );
        _motor_R.set_polarity_mode( dc_motor::polarity_normal );

        _motor_L.set_run_mode( motor::run_mode_position );
        _motor_R.set_run_mode( motor::run_mode_position );

        _motor_L.set_position_mode( motor::position_mode_relative );
        _motor_R.set_position_mode( motor::position_mode_relative );

        _motor_L.set_position_sp( position );
        _motor_R.set_position_sp( position );
        start();
    }
private:
    large_motor  _motor_L = large_motor( OUTPUT_A );
    large_motor  _motor_R = large_motor( OUTPUT_D );

    int error_L = 0;
    int error_R = 0;
};




class Controler {
public:
    bool check() const {
        return _sensor_color.connected() && _drive_control.check();
    }

    void dump() const {
        cout << "color sensor " << _sensor_color << endl;
        _drive_control.dump();
    }

    void run() {
        cout << "initializing..." << endl;
        _sensor_color.set_mode( color_sensor::mode_color );
        _drive_control.init();

        cout << "running..." << endl;
        while ( update() );

        cout << "exiting..." << endl;
        _drive_control.stop();
    }
protected:
    bool update() {
        if ( button::enter.pressed() )
            return false;

        Color color = static_cast< Color >( _sensor_color.value() );
        if (color != _state) {
            _state = color;
            cout << "\rstate: " << static_cast<int>(color);
            cout.flush();
            state_changed();
        }
        std::this_thread::sleep_for( 100ms );

        return true;
    }

    void state_changed() {
        switch ( _state ) {
        case Color::black:
            //_drive_control.forward();
            break;
        case Color::white:
            _drive_control.turn( 15 );
            break;
        default:
            break;
        }
    }
private:
    color_sensor _sensor_color = color_sensor( INPUT_AUTO );   
    DriveControl _drive_control;

    Color _state = Color::none;
};



/*
class CorrectDir {
    using std::chrono::high_resolution_clock;
public:
    CorrectDir( Controler* bot ) :
        _bot( bot ),
        _position_L( _motor_L.position() )
        _position_R( _motor_R.position() )
    {}
public:
    void run() {
        //time demanding task first
        int pos_L = _motor_L.position();
        int pos_R = _motor_R.position();

        int rot_L = pos_L - _position_L;
        int rot_R = pos_R - _position_R;

        _position_L = pos_L;
        _position_R = pos_R;


        //now we can measure time
        high_resolution_clock::now time_now;
        high_resolution_clock::duration time_diff = time_now - _last_run;
        _last_run = time_now;


        //after that we adjust speed
        time_diff

        _bot->motor_L().set_pulses_per_second_sp( _speed_L );
        _bot->motor_R().set_pulses_per_second_sp( _speed_R );
    }
private:
    Controler* const _bot;

    constexpr int speed = 400;
    int _position_L;
    int _position_R;

    high_resolution_clock::time_point _last_run;
};
*/



int main()
{
    cout << boolalpha;

    Controler bot;

    if ( !bot.check() ) {
        cout << "miscount detected!" << endl;
        bot.dump();

        return 1;
    }
    bot.run();

    return 0;
}
