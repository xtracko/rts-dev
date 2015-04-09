#include <iostream>
//#include <vector>
//#include <chrono>
//#include <thread>


#include "ev3dev.h"


using namespace std;
using namespace ev3dev;


class Color
{
public:
    Color(const color_sensor sensor) :
        _r( sensor.value( 0 ) ),
        _g( sensor.value( 1 ) ),
        _b( sensor.value( 2 ) )
    {}

    Color(int r, int g, int b) :
        _r(r), _g(g), _b(b)
    {}
    
    Color(const Color&) = default;

    Color& operator= (const Color& rhs) = default;
public:
    int r() const { return _r; }
    int g() const { return _g; }
    int b() const { return _b; }

    int dot(const Color& rhs) const {
        return (_r * rhs._r) + (_g * rhs._g) + (_b * rhs._b);
    }
private:
    int _r = 0;
    int _g = 0;
    int _b = 0;
};


ostream& operator<< (ostream& os, const Color& col) {
    return os << "[" << col.r() << " " << col.g() << " " << col.b() << "]";
}


/*
class Histogram {
public:
    void add(const Color& col) { _data.push_back(col); }
    size_t size() const { return _data.size(); }
    void clear() { _data.clear(); }
private:
    vector< Color > _data;
};



class DriveControler
{
public:
    bool check() const {
        return _motor_L.connected() && _motor_R.connected();
    }

    void  init() {
        _motor_L.reset();
        _motor_R.reset();

        _motor_L.set_stop_mode( motor::stop_mode_hold );
        _motor_R.set_stop_mode( motor::stop_mode_hold );

        _motor_L.set_regulation_mode( motor::mode_on );
        _motor_R.set_regulation_mode( motor::mode_on );


        _motor_L.set_position_mode( motor::position_mode_relative );
        _motor_R.set_position_mode( motor::position_mode_relative );
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
        _motor_L.set_polarity_mode( dc_motor::polarity_inverted );
        _motor_R.set_polarity_mode( dc_motor::polarity_inverted );

        _motor_L.set_run_mode( motor::run_mode_forever );
        _motor_R.set_run_mode( motor::run_mode_forever );
    }

    void update( int correction ) {
        if (correction < 0) {
            _motor_L.set_pulses_per_second_sp( _speed + abs(correction) );
            _motor_R.set_pulses_per_second_sp( _speed );
        } else {
            _motor_L.set_pulses_per_second_sp( _speed );
            _motor_R.set_pulses_per_second_sp( _speed + abs(correction) );
        }
    }
private:
    static constexpr const int _speed = 80;
    large_motor  _motor_L = large_motor( OUTPUT_A );
    large_motor  _motor_R = large_motor( OUTPUT_D );
};
*/


class SensorControler {
public:
    bool check() const {
        return /*_dev_touch.connected() &&*/ _dev_color.connected() && _dev_motor.connected();
    }
    
    void init() {
        _dev_color.set_mode( "RGB-RAW" );

        _dev_motor.reset();
        _dev_motor.set_run_mode( motor::run_mode_position );
        _dev_motor.set_stop_mode( motor::stop_mode_hold );
        _dev_motor.set_position_mode( motor::position_mode_relative );
        _dev_motor.set_regulation_mode( motor::mode_on );
        _dev_motor.set_pulses_per_second_sp( _speed );
    }
public:
    void update() {
        /*
        if ( _dev_touch.value() )
            _dev_motor.stop();
        if ( !_dev_motor.running() )
            swipe();

        _histogram.add( Color( _dev_color ) );
        */

        Color color( _dev_color );
        static int pos = -170;
        if ( !_dev_motor.running() ) {
            pos = -pos;
            _dev_motor.set_position_sp( pos );
            _dev_motor.start();
            cout << endl;
        }
        cout << color;
        //std::this_thread::sleep_for( 100ms );
    }
protected:
    /*
    void swipe() {
        _position = -_position; //invert direction

        _dev_motor.set_position_sp( 170 );
        _dev_motor.start();

        _histogram.clear();
    }
    */
private:
    static constexpr const int _speed = 900;

    //Histogram _histogram;

    touch_sensor _dev_touch = touch_sensor( INPUT_AUTO );
    color_sensor _dev_color = color_sensor( INPUT_AUTO );
    medium_motor _dev_motor = medium_motor( OUTPUT_AUTO );
};




class MainControler {
public:
    bool check() const {
        return /*_drives.check() &&*/ _sensors.check();
    }
public:
    void run() {
        _sensors.init();
        //_drives.init();

        //_drives.forward();
        //_drives.start();

        while ( update() );
        //_drives.stop();
    }

    bool update() {
        if ( button::enter.pressed() )
            return false;

        _sensors.update();

        //_drives.update( correction/3 );

        return true;
    }
public:
    //DriveControler _drives;
    SensorControler _sensors;
};



int main() {
    MainControler bot;
    if ( !bot.check() ) {
        cout << "miscount detected!" << endl;
        return 1;
    }

    bot.run();

    return 0;
}