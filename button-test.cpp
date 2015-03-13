#include <thread>
#include <chrono>
#include <iostream>
#include <ios>

#include "ev3dev.h"

using namespace ev3dev;
using namespace std::literals::chrono_literals;

int main() {
  bool up = false, down = false, left = false, right = false, enter = false,
      escape = false;
  std::cout << std::boolalpha;

  while ( !escape ) {

      up = button::up.pressed();
      down = button::down.pressed();
      left = button::left.pressed();
      right = button::right.pressed();
      enter = button::enter.pressed();
      escape = button::back.pressed();

      std::cout << "up: " << up
                << ", down: " << down
                << ", left: " << left
                << ", right: " << right
                << ", enter: " << enter
                << ", esc: " << escape << std::endl;
      std::this_thread::sleep_for( 100ms );
    }
}
