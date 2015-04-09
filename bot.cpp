#include "ev3dev.h"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>


using namespace ev3dev;






class SensorData {
public:
    void clear() {
        _positions.clear();
        _colors.clear();
    }

    size_t size() const {
        return _colors.size();
    }

    void add( int position, int color ) {
        _positions.push_back( position );
        _colors.push_back( color );
    }

    int col(const size_t i) const { return _colors[i]; }
    int pos(const size_t i) const { return _positions[i]; }

    int& col(const size_t i) { return _colors[i]; }

    void swap( SensorData& other ) {
        _colors.swap( other._colors );
        _positions.swap( other._positions );
    }

    void swap_cols( std::vector< int >& colors ) {
        _colors.swap( colors );
    }
private:
    std::vector< int > _positions;
    std::vector< int > _colors;
};





class SensorAnalyzer {
public:
    void swap ( SensorData& data ) {
        _data.swap( data );
    }

    void analyze() {
        // discard unusable data
        if (_data.size() < 2)
            return;

        median_blur();
        gradient();

        // TODO
    }
protected:
    void median_blur() {
        const int radius = 1;
        std::array< int, 2*radius + 1 > neighbors;

        _temp.clear();
        const int size = static_cast< int >(_data.size());

        for ( int i = 0; i < size; i++ ) {
            for ( int j = -radius; j <= radius; j++ ) {
                neighbors[ radius-j ] = ( i+j < 0 || i+j > size ) ? 0 : _data.col( i );
            }
            std::sort( neighbors.begin(), neighbors.end() );

            _temp.push_back( neighbors[radius] );
        }
        _data.swap_cols( _temp );
    }

    void gradient() {
        for ( size_t i = 0; i < _data.size() - 1; i++ )
            _data.col( i ) = _data.col( i ) - _data.col( i+1 );
        _data.col( _data.size() - 1 ) = 0;
    }
private:
    SensorData _data;
    std::vector< int > _temp;
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
            analyzer.swap( _data );
            _data.clear();

            written = true;
        }
        _data.add( _motor.position(), _eye.value() );

        return written;
    }
protected:
    void init_modes() {
        _eye.set_mode( color_sensor::mode_color );

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
    bool check() {
        return _sensors.check() && _drives.check();
    }

    void run() {
        _sensors.init();
        _drives.init();

        while ( update() );

        _drives.stop();
    }
protected:
    bool update() {
        if ( button::enter.pressed() )
            return false;

        if ( _sensors.update( _analyzer ) ) {
            _analyzer.analyze();
            // TODO
        }

        return true;
    }
private:
    SensorAnalyzer _analyzer;

    SensorControl _sensors;
    DriveControl  _drives;
};





int main() {
    MainControl bot;

    if ( !bot.check() )
        goto error;

    bot.run();

    return 0;
error:
    std::cout << "miscount detected!" << std::endl;

    return 1;
}
