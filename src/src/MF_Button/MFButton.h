//
// MFButton.h
//
// (C) MobiFlight Project 2022
//

#pragma once

#include <Arduino.h>
#include "commandmessenger.h"

extern "C" {
// callback functions always follow the signature: void cmd(void);
typedef void (*buttonEvent)(uint8_t, const char *);
};

enum {
    btnOnPress,
    btnOnRelease,
};

/////////////////////////////////////////////////////////////////////
/// \class MFButton MFButton.h <MFButton.h>
class MFButton
{
public:
    MFButton();
    static void attachHandler(buttonEvent newHandler);
    void        attach(const char *name);
    void        detach();
    void        update();
    void        setCommandMessenger(CmdMessenger *cmdMessengerNew);

private:
    CmdMessenger *cmdMess;
    const char *_name;
    bool        _initialized;
    bool        _state;

    static buttonEvent _inputHandler;
};

// MFButton.h