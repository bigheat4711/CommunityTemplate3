#pragma once

#include "Arduino.h"

class board1
{
public:
    board1();
    void begin();
    void attach();
    void detach();
    void set(int16_t messageID, char *setPoint);
    void update();

private:
    bool    _initialised;
};