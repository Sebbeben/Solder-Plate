#ifndef BUTTONS_H
#define BUTTONS_H

// Include any necessary libraries here
#include <Arduino.h>
#include "Button.h"
#include "leds/leds.h"
#include "../statechangeevent.h"
// Button pins
#define upButton 21
#define downButton 22
#define backButton 23
#define selectButton 24

class Buttons
{
public:
  // Constructor
  Buttons(){};

  StateChangeEvent<ButtonKind, ButtonState> *handleButtons();
  void setup();
};

#endif // BUTTONS_H
