#include <thread>
#include <chrono>
#include <iostream>
#include <ios>

#include "ev3dev.h"


using namespace ev3dev;
using namespace std;
using namespace std::literals::chrono_literals;




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




class Controler
{
public:
    Controler() :
        m_sensor_color(INPUT_AUTO),
        m_motor_L(OUTPUT_A),
        m_motor_R(OUTPUT_D)
    {
        m_sensor_color.set_mode(color_sensor::mode_color);
    }
public:
    void dump_devices() const {
        cout << "color sensor " << m_sensor_color << endl;
        cout << "left motor   " << m_motor_L << endl;
        cout << "right motor  " << m_motor_R << endl;
    }


    void run_loop() {
        cout << "running..." << endl;
        while(update());
        cout << "exiting..." << endl;
    }

    bool update();
protected:
    void turn(int angle);
private:
    ev3dev::color_sensor m_sensor_color;
    ev3dev::large_motor  m_motor_L;
    ev3dev::large_motor  m_motor_R;
};



bool Controler::update() {
    if (button::enter.pressed())
        return false;

    Color color = static_cast<Color>(m_sensor_color.value());

    cout << "\rcolor: " << static_cast<int>(color) << flush;

    m_motor_L.set_run_mode(motor::run_mode_forever);
    m_motor_R.set_run_mode(motor::run_mode_forever);

    m_motor_R.set_stop_mode(motor::stop_mode_hold);
    m_motor_L.set_stop_mode(motor::stop_mode_hold);

    m_motor_R.set_regulation_mode(motor::mode_on);
    m_motor_L.set_regulation_mode(motor::mode_on);

    m_motor_L.set_pulses_per_second_sp(80);
    m_motor_R.set_pulses_per_second_sp(80);

    switch (color) {
    case Color::black:
        m_motor_L.start();
        m_motor_R.start();
        break;
    case Color::white:
        m_motor_L.stop();
        m_motor_R.stop();
        break;
    default:
        break;
    }

    std::this_thread::sleep_for(100ms);
    return true;
}


void Controler::turn(int angle) {
    m_motor_R.set_run_mode(motor::run_mode_position);
    m_motor_L.set_run_mode(motor::run_mode_position);

    m_motor_R.set_stop_mode(motor::stop_mode_hold);
    m_motor_L.set_stop_mode(motor::stop_mode_hold);

    m_motor_R.set_regulation_mode(motor::mode_on);
    m_motor_R.set_regulation_mode(motor::mode_on);
}



int main()
{
    cout << boolalpha;

    Controler bot;
    bot.dump_devices();
    bot.run_loop();

    return 0;
}
