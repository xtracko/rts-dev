/*
 * C++ API to the sensors, motors, buttons, LEDs and battery of the ev3dev
 * Linux kernel for the LEGO Mindstorms EV3 hardware
 *
 * Copyright (c) 2014 - Franz Detro
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Modification:
 *  Add new button management for ev3dev Release 02.00.00 (ev3dev-jessie-2014-07-12) - Christophe Chaudelet
 *
 */

#pragma once

//-----------------------------------------------------------------------------
//~autogen autogen-header
    // Sections of the following code were auto-generated based on spec v0.9.2-pre, rev 1. 
//~autogen
//-----------------------------------------------------------------------------

#include <map>
#include <set>
#include <string>
#include <functional>

//-----------------------------------------------------------------------------

namespace ev3dev {

//-----------------------------------------------------------------------------

typedef std::string         device_type;
typedef std::string         port_type;
typedef std::string         mode_type;
typedef std::set<mode_type> mode_set;
typedef std::string         address_type;

//-----------------------------------------------------------------------------

const port_type INPUT_AUTO;          //!< Automatic input selection
const port_type INPUT_1  { "in1" };  //!< Sensor port 1
const port_type INPUT_2  { "in2" };  //!< Sensor port 2
const port_type INPUT_3  { "in3" };  //!< Sensor port 3
const port_type INPUT_4  { "in4" };  //!< Sensor port 4

const port_type OUTPUT_AUTO;         //!< Automatic output selection
const port_type OUTPUT_A { "outA" }; //!< Motor port A
const port_type OUTPUT_B { "outB" }; //!< Motor port B
const port_type OUTPUT_C { "outC" }; //!< Motor port C
const port_type OUTPUT_D { "outD" }; //!< Motor port D

//-----------------------------------------------------------------------------

class device
{
public:
  bool connect(const std::string &dir,
               const std::string &pattern,
               const std::map<std::string,
                              std::set<std::string>> &match) noexcept;
  inline bool connected() const { return !_path.empty(); }

  int         device_index() const;

  int         get_attr_int   (const std::string &name) const;
  void        set_attr_int   (const std::string &name,
                              int value);
  std::string get_attr_string(const std::string &name) const;
  void        set_attr_string(const std::string &name,
                              const std::string &value);

  std::string get_attr_line  (const std::string &name) const;
  mode_set    get_attr_set   (const std::string &name,
                              std::string *pCur = nullptr) const;

  std::string get_attr_from_set(const std::string &name) const;

protected:
  std::string _path;
  mutable int _device_index = -1;
};

//-----------------------------------------------------------------------------

class sensor : protected device
{
public:
  typedef device_type sensor_type;

  static const sensor_type ev3_touch;
  static const sensor_type ev3_color;
  static const sensor_type ev3_ultrasonic;
  static const sensor_type ev3_gyro;
  static const sensor_type ev3_infrared;

  static const sensor_type nxt_touch;
  static const sensor_type nxt_light;
  static const sensor_type nxt_sound;
  static const sensor_type nxt_ultrasonic;
  static const sensor_type nxt_i2c_sensor;

  sensor(port_type);
  sensor(port_type, const std::set<sensor_type>&);

  using device::connected;
  using device::device_index;

  int   value(unsigned index=0) const;
  float float_value(unsigned index=0) const;
  std::string type_name() const;

  //~autogen cpp_generic-get-set classes.sensor>currentClass

    int decimals() const { return get_attr_int("decimals"); }
    std::string mode() const { return get_attr_string("mode"); }
    void set_mode(std::string v) { set_attr_string("mode", v); }
    mode_set modes() const { return get_attr_set("modes"); }
    void set_command(std::string v) { set_attr_string("command", v); }
    mode_set commands() const { return get_attr_set("commands"); }
    int num_values() const { return get_attr_int("num_values"); }
    std::string port_name() const { return get_attr_string("port_name"); }
    std::string units() const { return get_attr_string("units"); }
    std::string driver_name() const { return get_attr_string("driver_name"); }

//~autogen

protected:
  sensor() {}

  bool connect(const std::map<std::string, std::set<std::string>>&) noexcept;
};

//-----------------------------------------------------------------------------

class i2c_sensor : public sensor
{
public:
  i2c_sensor(port_type port = INPUT_AUTO);
  i2c_sensor(port_type port, address_type address);

  //~autogen cpp_generic-get-set classes.i2cSensor>currentClass

    std::string fw_version() const { return get_attr_string("fw_version"); }
    std::string address() const { return get_attr_string("address"); }
    int poll_ms() const { return get_attr_int("poll_ms"); }
    void set_poll_ms(int v) { set_attr_int("poll_ms", v); }

//~autogen
};

//-----------------------------------------------------------------------------

class touch_sensor : public sensor
{
public:
  touch_sensor(port_type port_ = INPUT_AUTO);
};

//-----------------------------------------------------------------------------

class color_sensor : public sensor
{
public:
  color_sensor(port_type port_ = INPUT_AUTO);

  static const mode_type mode_reflect;
  static const mode_type mode_ambient;
  static const mode_type mode_color;
};

//-----------------------------------------------------------------------------

class ultrasonic_sensor : public sensor
{
public:
  ultrasonic_sensor(port_type port_ = INPUT_AUTO);

  static const mode_type mode_dist_cm;
  static const mode_type mode_dist_in;
  static const mode_type mode_listen;
  static const mode_type mode_single_cm;
  static const mode_type mode_single_in;
};

//-----------------------------------------------------------------------------

class gyro_sensor : public sensor
{
public:
  gyro_sensor(port_type port_ = INPUT_AUTO);

  static const mode_type mode_angle;
  static const mode_type mode_speed;
  static const mode_type mode_angle_and_speed;
};

//-----------------------------------------------------------------------------

class infrared_sensor : public sensor
{
public:
  infrared_sensor(port_type port_ = INPUT_AUTO);

  static const mode_type mode_proximity;
  static const mode_type mode_ir_seeker;
  static const mode_type mode_ir_remote;
};

//-----------------------------------------------------------------------------

class motor : protected device
{
public:
  typedef device_type motor_type;

  motor(port_type);
  motor(port_type, const motor_type&);

  static const motor_type motor_large;
  static const motor_type motor_medium;

  static const mode_type mode_off;
  static const mode_type mode_on;

  static const mode_type run_mode_forever;
  static const mode_type run_mode_time;
  static const mode_type run_mode_position;

  static const mode_type stop_mode_coast;
  static const mode_type stop_mode_brake;
  static const mode_type stop_mode_hold;

  static const mode_type position_mode_absolute;
  static const mode_type position_mode_relative;

  using device::connected;
  using device::device_index;

  //~autogen cpp_generic-get-set classes.motor>currentClass

    int duty_cycle() const { return get_attr_int("duty_cycle"); }
    int duty_cycle_sp() const { return get_attr_int("duty_cycle_sp"); }
    void set_duty_cycle_sp(int v) { set_attr_int("duty_cycle_sp", v); }
    std::string encoder_mode() const { return get_attr_string("encoder_mode"); }
    void set_encoder_mode(std::string v) { set_attr_string("encoder_mode", v); }
    mode_set encoder_modes() const { return get_attr_set("encoder_modes"); }
    std::string emergency_stop() const { return get_attr_string("estop"); }
    void set_emergency_stop(std::string v) { set_attr_string("estop", v); }
    std::string debug_log() const { return get_attr_string("log"); }
    std::string polarity_mode() const { return get_attr_string("polarity_mode"); }
    void set_polarity_mode(std::string v) { set_attr_string("polarity_mode", v); }
    mode_set polarity_modes() const { return get_attr_set("polarity_modes"); }
    std::string port_name() const { return get_attr_string("port_name"); }
    int position() const { return get_attr_int("position"); }
    void set_position(int v) { set_attr_int("position", v); }
    std::string position_mode() const { return get_attr_string("position_mode"); }
    void set_position_mode(std::string v) { set_attr_string("position_mode", v); }
    mode_set position_modes() const { return get_attr_set("position_modes"); }
    int position_sp() const { return get_attr_int("position_sp"); }
    void set_position_sp(int v) { set_attr_int("position_sp", v); }
    int pulses_per_second() const { return get_attr_int("pulses_per_second"); }
    int pulses_per_second_sp() const { return get_attr_int("pulses_per_second_sp"); }
    void set_pulses_per_second_sp(int v) { set_attr_int("pulses_per_second_sp", v); }
    int ramp_down_sp() const { return get_attr_int("ramp_down_sp"); }
    void set_ramp_down_sp(int v) { set_attr_int("ramp_down_sp", v); }
    int ramp_up_sp() const { return get_attr_int("ramp_up_sp"); }
    void set_ramp_up_sp(int v) { set_attr_int("ramp_up_sp", v); }
    std::string regulation_mode() const { return get_attr_string("regulation_mode"); }
    void set_regulation_mode(std::string v) { set_attr_string("regulation_mode", v); }
    mode_set regulation_modes() const { return get_attr_set("regulation_modes"); }
    int run() const { return get_attr_int("run"); }
    void set_run(int v) { set_attr_int("run", v); }
    std::string run_mode() const { return get_attr_string("run_mode"); }
    void set_run_mode(std::string v) { set_attr_string("run_mode", v); }
    mode_set run_modes() const { return get_attr_set("run_modes"); }
    int speed_regulation_p() const { return get_attr_int("speed_regulation_P"); }
    void set_speed_regulation_p(int v) { set_attr_int("speed_regulation_P", v); }
    int speed_regulation_i() const { return get_attr_int("speed_regulation_I"); }
    void set_speed_regulation_i(int v) { set_attr_int("speed_regulation_I", v); }
    int speed_regulation_d() const { return get_attr_int("speed_regulation_D"); }
    void set_speed_regulation_d(int v) { set_attr_int("speed_regulation_D", v); }
    int speed_regulation_k() const { return get_attr_int("speed_regulation_K"); }
    void set_speed_regulation_k(int v) { set_attr_int("speed_regulation_K", v); }
    std::string state() const { return get_attr_string("state"); }
    std::string stop_mode() const { return get_attr_string("stop_mode"); }
    void set_stop_mode(std::string v) { set_attr_string("stop_mode", v); }
    mode_set stop_modes() const { return get_attr_set("stop_modes"); }
    int time_sp() const { return get_attr_int("time_sp"); }
    void set_time_sp(int v) { set_attr_int("time_sp", v); }
    std::string type() const { return get_attr_string("type"); }

//~autogen

  void start()         { set_attr_int("run", 1); }
  void stop()          { set_attr_int("run", 0); }
  bool running() const { return run(); }
  void reset()         { set_attr_int("reset", 1);    }

protected:
  motor() {}

  bool connect(const std::map<std::string, std::set<std::string>>&) noexcept;
};

//-----------------------------------------------------------------------------

class medium_motor : public motor
{
public:
  medium_motor(port_type port_ = OUTPUT_AUTO);
};

//-----------------------------------------------------------------------------

class large_motor : public motor
{
public:
  large_motor(port_type port_ = OUTPUT_AUTO);
};

//-----------------------------------------------------------------------------

class dc_motor : protected device
{
public:
  dc_motor(port_type port_ = OUTPUT_AUTO);

  static const std::string command_run;
  static const std::string command_brake;
  static const std::string command_coast;
  static const std::string polarity_normal;
  static const std::string polarity_inverted;

  using device::connected;
  using device::device_index;

  //~autogen cpp_generic-get-set classes.dcMotor>currentClass

    void set_command(std::string v) { set_attr_string("command", v); }
    mode_set commands() const { return get_attr_set("commands"); }
    int duty_cycle() const { return get_attr_int("duty_cycle"); }
    void set_duty_cycle(int v) { set_attr_int("duty_cycle", v); }
    std::string driver_name() const { return get_attr_string("driver_name"); }
    std::string port_name() const { return get_attr_string("port_name"); }
    int ramp_down_ms() const { return get_attr_int("ramp_down_ms"); }
    void set_ramp_down_ms(int v) { set_attr_int("ramp_down_ms", v); }
    int ramp_up_ms() const { return get_attr_int("ramp_up_ms"); }
    void set_ramp_up_ms(int v) { set_attr_int("ramp_up_ms", v); }
    std::string polarity() const { return get_attr_string("polarity"); }
    void set_polarity(std::string v) { set_attr_string("polarity", v); }

//~autogen

protected:
  std::string _port_name;
};

//-----------------------------------------------------------------------------

class servo_motor : protected device
{
public:
  servo_motor(port_type port_ = OUTPUT_AUTO);

  static const std::string command_run;
  static const std::string command_float;
  static const std::string polarity_normal;
  static const std::string polarity_inverted;

  using device::connected;
  using device::device_index;

  //~autogen cpp_generic-get-set classes.servoMotor>currentClass

    std::string command() const { return get_attr_string("command"); }
    void set_command(std::string v) { set_attr_string("command", v); }
    std::string driver_name() const { return get_attr_string("driver_name"); }
    std::string port_name() const { return get_attr_string("port_name"); }
    int max_pulse_ms() const { return get_attr_int("max_pulse_ms"); }
    void set_max_pulse_ms(int v) { set_attr_int("max_pulse_ms", v); }
    int mid_pulse_ms() const { return get_attr_int("mid_pulse_ms"); }
    void set_mid_pulse_ms(int v) { set_attr_int("mid_pulse_ms", v); }
    int min_pulse_ms() const { return get_attr_int("min_pulse_ms"); }
    void set_min_pulse_ms(int v) { set_attr_int("min_pulse_ms", v); }
    std::string polarity() const { return get_attr_string("polarity"); }
    void set_polarity(std::string v) { set_attr_string("polarity", v); }
    int position() const { return get_attr_int("position"); }
    void set_position(int v) { set_attr_int("position", v); }
    int rate() const { return get_attr_int("rate"); }
    void set_rate(int v) { set_attr_int("rate", v); }

//~autogen
};

//-----------------------------------------------------------------------------

class led : protected device
{
public:
  led(std::string name);

  using device::connected;

  //~autogen cpp_generic-get-set classes.led>currentClass

    int max_brightness() const { return get_attr_int("max_brightness"); }
    int brightness() const { return get_attr_int("brightness"); }
    void set_brightness(int v) { set_attr_int("brightness", v); }
    std::string trigger() const { return get_attr_string("trigger"); }
    void set_trigger(std::string v) { set_attr_string("trigger", v); }

//~autogen

  mode_set  triggers() const  { return get_attr_set     ("trigger"); }

  void on()  { set_brightness(max_brightness()); }
  void off() { set_brightness(0); }

  void flash(unsigned interval_ms);
  void set_on_delay (unsigned ms) { set_attr_int("delay_on",  ms); }
  void set_off_delay(unsigned ms) { set_attr_int("delay_off", ms); }

  static led red_right;
  static led red_left;
  static led green_right;
  static led green_left;

  static void red_on   ();
  static void red_off  ();
  static void green_on ();
  static void green_off();
  static void all_on   ();
  static void all_off  ();

protected:
  int _max_brightness = 0;
};

//-----------------------------------------------------------------------------

class power_supply : protected device
{
public:
  power_supply(std::string name);

  using device::connected;

  //~autogen cpp_generic-get-set classes.powerSupply>currentClass

    int current_now() const { return get_attr_int("current_now"); }
    int voltage_now() const { return get_attr_int("voltage_now"); }
    int voltage_max_design() const { return get_attr_int("voltage_max_design"); }
    int voltage_min_design() const { return get_attr_int("voltage_min_design"); }
    std::string technology() const { return get_attr_string("technology"); }
    std::string type() const { return get_attr_string("type"); }

//~autogen

  float current_amps()       const { return current_now() / 1000000.f; }
  float voltage_volts()      const { return voltage_now() / 1000000.f; }

  static power_supply battery;
};

//-----------------------------------------------------------------------------

class button
{
public:
  button(int bit);
  ~button()
  {
    delete _buf;
  }

  bool pressed() const;

  static button back;
  static button left;
  static button right;
  static button up;
  static button down;
  static button enter;

private:
  int _bit;
  int _fd;
  int _bits_per_long;
  unsigned long *_buf;
  unsigned long _buf_size;

};

//-----------------------------------------------------------------------------

class sound
{
public:
  static void beep();
  static void tone(unsigned frequency, unsigned ms);

  static void play (const std::string &soundfile, bool bSynchronous = false);
  static void speak(const std::string &text, bool bSynchronous = false);

  static unsigned volume();
  static void set_volume(unsigned);
};

//-----------------------------------------------------------------------------

class lcd
{
public:
  lcd();
  ~lcd();

  bool available() const { return _fb != nullptr; }

  uint32_t resolution_x()   const { return _xres; }
  uint32_t resolution_y()   const { return _yres; }
  uint32_t bits_per_pixel() const { return _bpp; }

  uint32_t frame_buffer_size() const { return _fbsize; }
  uint32_t line_length()       const { return _llength; }

  unsigned char *frame_buffer() { return _fb; }

  void fill(unsigned char pixel);

protected:
  void init();
  void deinit();

private:
  unsigned char *_fb;
  uint32_t _fbsize;
  uint32_t _llength;
  uint32_t _xres;
  uint32_t _yres;
  uint32_t _bpp;
};

//-----------------------------------------------------------------------------

class remote_control
{
public:
  remote_control(unsigned channel = 1);
  remote_control(infrared_sensor&, unsigned channel = 1);
  virtual ~remote_control();

  inline bool   connected() const { return _sensor->connected(); }
  inline unsigned channel() const { return _channel+1; }

  bool process();

  std::function<void (bool)> on_red_up;
  std::function<void (bool)> on_red_down;
  std::function<void (bool)> on_blue_up;
  std::function<void (bool)> on_blue_down;
  std::function<void (bool)> on_beacon;

protected:
  virtual void on_value_changed(int value);

  enum
  {
    red_up    = (1 << 0),
    red_down  = (1 << 1),
    blue_up   = (1 << 2),
    blue_down = (1 << 3),
    beacon    = (1 << 4),
  };

protected:
  infrared_sensor *_sensor = nullptr;
  bool             _owns_sensor = false;
  unsigned         _channel = 0;
  int              _value = 0;
  int              _state = 0;
};

//-----------------------------------------------------------------------------

} // namespace ev3dev

//-----------------------------------------------------------------------------
