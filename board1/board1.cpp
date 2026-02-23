#include "board1.h"
#include "Arduino.h"
#include "allocateMem.h"
#include "commandmessenger.h"
#include <U8x8lib.h>

/* **********************************************************************************
    This is just the basic code to set up your custom device.
    Change/add your code as needed.
********************************************************************************** */

U8X8_SSD1309_128X64_NONAME0_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

board1::board1()
{
}

void board1::begin()
{
  u8x8.begin();
  u8x8.setFlipMode(3); // Falls es wieder auf dem Kopf steht
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clearLine(0);
    u8x8.setCursor(0, 0);
    u8x8.print( F("begin()"));
 _initialised=true;
}

void board1::attach()
{
  u8x8.clearLine(0);
    u8x8.setCursor(0, 0);
    u8x8.print( F("attach()"));
}

void board1::detach()
{
  u8x8.clearLine(0);
    u8x8.setCursor(0, 0);
    u8x8.print( F("detach()"));
    if (!_initialised)
        return;
    _initialised = false;
}

void board1::set(int16_t messageID, char *setPoint)
{
    // 1. Zeile explizit löschen, damit der alte, lange Text verschwindet
    u8x8.clearLine(0); 
    

    u8x8.setCursor(0, 0);
    /* **********************************************************************************
        Each messageID has it's own value
        check for the messageID and define what to do.
        Important Remark!
        MessageID == -2 will be send from the board when PowerSavingMode is set
            Message will be "0" for leaving and "1" for entering PowerSavingMode
        MessageID == -1 will be send from the connector when Connector stops running
        Put in your code to enter this mode (e.g. clear a display)

    ********************************************************************************** */
    int32_t  data = atoi(setPoint);
    uint16_t output;

    // do something according your messageID
    switch (messageID) {
    case -1:
      u8x8.print(F("MF shutdown"));
      break;
        // tbd., get's called when Mobiflight shuts down
    case -2:
      u8x8.print(F("MF power save"));
      break;
        // tbd., get's called when PowerSavingMode is entered
    case 0:
        output = (uint16_t)data;
        data   = output;
        u8x8.setCursor(0, 0);
        u8x8.print(data);
        u8x8.setCursor(0, 1);
        u8x8.print(setPoint);
        break;
    case 1:
        /* code */
        u8x8.print(F("1 - undefined"));
        break;
    case 2:
        /* code */
        u8x8.print(F("2 - undefined"));
        break;
    default:
        u8x8.print(F("default - undefined"));
        break;
    }
}

void board1::update()
{ 
    // Do something which is required regulary
    //u8x8.setCursor(0, 0);
    //u8x8.print( F("update()"));
}