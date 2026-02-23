#include "board1.h"
#include <U8g2lib.h>
#include <Wire.h>

// Das u8g2 Objekt für SSD1309 via I2C erstellen (Page-Buffer '_1_' für minimalen RAM, 'R2' für 180° Drehung)
U8G2_SSD1309_128X64_NONAME0_1_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

// Variablen für unsere Anzeige-Werte im RAM
char currentHdg[10] = "---";
char currentAlt[15] = "-----";

board1::board1()
{
}

void board1::begin()
{
    u8g2.begin();

    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_helvB08_tr);
        u8g2.setCursor(0, 10);
        u8g2.print(F("begin()"));
    } while ( u8g2.nextPage() );

    _initialised = true;
}

void board1::attach()
{
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_helvB08_tr);
        u8g2.setCursor(0, 10);
        u8g2.print(F("attach()"));
    } while ( u8g2.nextPage() );
}

void board1::detach()
{
    if (!_initialised)
        return;
    _initialised = false;

    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_helvB08_tr);
        u8g2.setCursor(0, 10);
        u8g2.print(F("detach()"));
    } while ( u8g2.nextPage() );
}

void board1::set(int16_t messageID, char *setPoint)
{
    // Eingehende Werte speichern
    if (messageID == 0) strncpy(currentHdg, setPoint, sizeof(currentHdg));
    if (messageID == 1) strncpy(currentAlt, setPoint, sizeof(currentAlt));

    u8g2.firstPage();
    do {
        // 1. "hdg" zentriert
        u8g2.setFont(u8g2_font_helvB08_tr); 
        int w = u8g2.getStrWidth("hdg");
        u8g2.setCursor((128 - w) / 2, 10);
        u8g2.print(F("hdg")); 

        // 2. HDG Wert zentriert
        u8g2.setFont(u8g2_font_helvB14_tr); 
        w = u8g2.getStrWidth(currentHdg);
        u8g2.setCursor((128 - w) / 2, 28);
        u8g2.print(currentHdg); 

        // 3. "alt" zentriert
        u8g2.setFont(u8g2_font_helvB08_tr);
        w = u8g2.getStrWidth("alt");
        u8g2.setCursor((128 - w) / 2, 44);
        u8g2.print(F("alt")); 

        // 4. ALT Wert zentriert
        u8g2.setFont(u8g2_font_helvB14_tr);
        w = u8g2.getStrWidth(currentAlt);
        u8g2.setCursor((128 - w) / 2, 62);
        u8g2.print(currentAlt); 

    } while ( u8g2.nextPage() );
}