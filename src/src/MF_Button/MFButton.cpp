//
// MFButton.cpp
//
// (C) MobiFlight Project 2022
//
#include "MFButton.h"
#include "commandmessenger.h"

buttonEvent MFButton::_inputHandler = NULL;

// Mapping der Namen passend zum G1000 Layout
// Index Struktur: [SPALTE][REIHE]
const char* buttonNames[2][7] = {
  // Spalte 0 (Links: COL_L)
  { "AP", "HDG", "NAV", "APR", "VS", "FLC", "ENC_L_PUSH" }, 
  
  // Spalte 1 (Rechts: COL_R)
  { "FD", "ALT", "VNV", "BC", "UP", "DN",  "ENC_R_PUSH" }
};

// Hardware Pins (Laut deinem Schaltplan)
// Spalten (Inputs)
uint8_t colPins[2] = { 6, 7 }; 
// Reihen (Outputs zum Scannen)
// ROW1=D8, ROW2=D9, ROW3=D10, ROW4=D11, ROW5=D12, ROW6=D13, ROW7=A0
uint8_t rowPins[7] = { 8, 9, 10, 11, 12, 13, A0 }; 

// Speicher für den letzten Zustand (false=offen, true=gedrückt)
bool lastState[2][7];

MFButton::MFButton()
{
    _initialized = false;
}

void MFButton::attach(const char *name)
{
    //pinMode(6, INPUT_PULLUP);
    //pinMode(8, OUTPUT);
    // 1. Spalten als INPUT_PULLUP (Standard ist HIGH)
    for (uint8_t c = 0; c < 2; c++) {
        pinMode(colPins[c], INPUT_PULLUP);
    }

    // 2. Reihen als OUTPUT und standardmäßig HIGH (inaktiv)
    for (uint8_t r = 0; r < 7; r++) {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH); 
        
        // State initialisieren
        for(uint8_t c=0; c<2; c++) lastState[c][r] = false;
    }
    _initialized = true;
}

void MFButton::detach()
{
    _initialized = false;
}

void MFButton::setCommandMessenger(CmdMessenger *cmdMessengerNew){
    cmdMess = cmdMessengerNew;
}

void MFButton::update()
{
#ifdef DEBUG2CMDMESSENGER
    //cmdMessenger.sendCmd(kDebug, F("cgrau 004"));
#endif
    if (!_initialized)
        return;
// ÄUSSERE SCHLEIFE: Wir iterieren durch die REIHEN (Scanning)
    for (uint8_t r = 0; r < 7; r++) {
        
        // A. Reihe aktivieren (Auf GND ziehen)
        // Strom kann jetzt fließen, falls eine Taste in dieser Reihe gedrückt ist
        digitalWrite(rowPins[r], LOW);

        // INNERE SCHLEIFE: Wir lesen die SPALTEN
        for (uint8_t c = 0; c < 2; c++) {
            
            // Lesen: Wenn LOW, dann fließt Strom -> Taste gedrückt
            // (Wir drehen die Logik um: !digitalRead bedeutet true bei LOW)
            bool currentState = !digitalRead(colPins[c]);

            // Check auf Änderung (Flankenerkennung)
            if (currentState != lastState[c][r]) {
                lastState[c][r] = currentState; // Speichern

                // Event feuern
                if (currentState == true) {
                    // TASTE GEDRÜCKT (Press Event)
                    // Hier senden wir den Namen aus dem Array!
                    (*_inputHandler)(btnOnPress, buttonNames[c][r]);
                } else {
                    // TASTE LOSGELASSEN (Release Event)
                    // Optional, falls MF Release braucht. EventID 0 meist für Release.
                    (*_inputHandler)(btnOnRelease, buttonNames[c][r]);
                }
            }
        }

        // B. Reihe deaktivieren (Wieder auf HIGH)
        // Wichtig, bevor wir zur nächsten Reihe gehen!
        digitalWrite(rowPins[r], HIGH);
    }
}

void MFButton::attachHandler(buttonEvent newHandler)
{
    _inputHandler = newHandler;
}

// MFButton.cpp