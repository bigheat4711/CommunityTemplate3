//
// config.cpp
//
// (C) MobiFlight Project 2022
//

#include "config.h"
#include "commandmessenger.h"
#include "allocateMem.h"
#include "MFEEPROM.h"
#include "Button.h"
#include "Encoder.h"
#include "Output.h"
#if !defined(ARDUINO_ARCH_AVR)
#include "ArduinoUniqueID.h"
#endif

#ifdef MF_CUSTOMDEVICE_SUPPORT
#include "CustomDevice.h"
#endif

// --- DEIN STATISCHER CONFIG-STRING ---
// Format: Typ.Pin(s).Name:  (Getrennt durch Punkte, Ende durch Doppelpunkt)
// 4 = Encoder, 1 = Button, 10 = CustomDevice
const char CustomDeviceConfig[] PROGMEM = "1.2.AP:1.3.FD:1.4.HDG:1.5.ALT:1.6.NAV:1.7.VNV:1.8.APR:1.9.BC:1.10.VS:1.11.UP:1.12.FLC:1.13.DN:17.board1.5|6|7..cgrau's board1:8.14.15.0.Enc1:8.16.17.0.Enc2:";

// The build version comes from an environment variable
#define STRINGIZER(arg) #arg
#define STR_VALUE(arg)  STRINGIZER(arg)
#define VERSION         STR_VALUE(BUILD_VERSION)
#define CORE_VERSION    STR_VALUE(CORE_BUILD_VERSION)

// Leeres Dummy-Objekt, damit der restliche Code beim Kompilieren nicht meckert
MFEEPROM MFeeprom;

const uint8_t MEM_OFFSET_NAME   = 0;
const uint8_t MEM_LEN_NAME      = 48;
const uint8_t MEM_OFFSET_SERIAL = MEM_OFFSET_NAME + MEM_LEN_NAME;
const uint8_t MEM_LEN_SERIAL    = 11;
const uint8_t MEM_OFFSET_CONFIG = MEM_OFFSET_NAME + MEM_LEN_NAME + MEM_LEN_SERIAL;

#ifdef ARDUINO_ARCH_AVR
char serial[11]; 
#endif
char           name[MEM_LEN_NAME]              = MOBIFLIGHT_NAME;
const int      MEM_LEN_CONFIG                  = MEMLEN_CONFIG;
char           nameBuffer[MEMLEN_NAMES_BUFFER] = "";
boolean        configActivated                 = false;
uint16_t       pNameBuffer                     = 0;
const uint16_t configLengthFlash               = sizeof(CustomDeviceConfig);
bool boardReady                                = false;

void resetConfig();
void readConfig();
void _activateConfig();
void readConfigFromMemory();

bool getBoardReady() { return boardReady; }

void loadConfig()
{
#ifdef DEBUG2CMDMESSENGER
    cmdMessenger.sendCmd(kDebug, F("Load config"));
#endif
    readConfig();
    _activateConfig();
}

void OnSetConfig()
{
#ifdef DEBUG2CMDMESSENGER
    cmdMessenger.sendCmd(kDebug, F("Setting config start"));
    cmdMessenger.sendCmd(kDebug, F("Setting config end"));
#endif
    // connector does not check for status = -1
    cmdMessenger.sendCmd(kStatus, -1);
}

void resetConfig()
{
    Button::Clear();
    Encoder::Clear();
    Output::Clear();
#ifdef MF_CUSTOMDEVICE_SUPPORT
    CustomDevice::Clear();
#endif
    configActivated    = false;
    pNameBuffer        = 0;
    ClearMemory();
}

void OnResetConfig()
{
    resetConfig();
    cmdMessenger.sendCmd(kStatus, F("OK"));
}

void OnSaveConfig()
{
    cmdMessenger.sendCmd(kConfigSaved, F("OK"));
}

void OnActivateConfig()
{
    readConfig();
    _activateConfig();
}

void _activateConfig()
{
    configActivated = true;
    cmdMessenger.sendCmd(kConfigActivated, F("OK"));
}

uint8_t readUint(volatile uint16_t *addrMem)
{
    char    params[4] = {0}; 
    uint8_t counter   = 0;
    do {
            params[counter++] = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++);
    } while (params[counter - 1] != '.' && counter < sizeof(params)); 
    params[counter - 1] = 0x00; 
    return atoi(params);
}

bool readName(uint16_t *addrMem, char *buffer, uint16_t *pBuffer)
{
    char     temp   = 0;
    do {
            temp = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++); 
            if (*addrMem > configLengthFlash)                             
                return false;
        buffer[(*pBuffer)++] = temp;         
        if (*pBuffer >= MEMLEN_NAMES_BUFFER) 
            return false; 
    } while (temp != ':'); 
    buffer[(*pBuffer) - 1] = 0x00; 
    return true;
}

bool readEndCommand(uint16_t *addrMem, uint8_t delimiter)
{
    char     temp   = 0;
    do {
            temp = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++);
            if (*addrMem > configLengthFlash) 
                return false;
    } while (temp != delimiter);
    return true;
}

void sendFailureMessage(const char *deviceName)
{
    cmdMessenger.sendCmdStart(kStatus);
    cmdMessenger.sendCmdArg(deviceName);
    cmdMessenger.sendCmdArg(F("does not fit in Memory"));
    cmdMessenger.sendCmdEnd();
}

bool GetArraySizes(uint8_t *numberDevices)
{
    bool     copy_success = true;
    uint16_t addrMem = 0;
    uint8_t  device = readUint(&addrMem);

    do {
        numberDevices[device]++;
        copy_success = readEndCommand(&addrMem, ':'); 
        device       = readUint(&addrMem);
    } while (device && copy_success);

    return copy_success;
}

void InitArrays(uint8_t *numberDevices)
{
    if (!Button::setupArray(numberDevices[kTypeButton])) sendFailureMessage("Button");
    if (!Output::setupArray(numberDevices[kTypeOutput])) sendFailureMessage("Output");
    if (!Encoder::setupArray(numberDevices[kTypeEncoder] + numberDevices[kTypeEncoderSingleDetent])) sendFailureMessage("Encoders");

#ifdef MF_CUSTOMDEVICE_SUPPORT
    if (!CustomDevice::setupArray(numberDevices[kTypeCustomDevice])) sendFailureMessage("CustomDevice");
#endif
}

void readConfig()
{
    // Hier wird deine Seriennummer sicher gesetzt
    strncpy(serial, "SN-D3E-D82", sizeof(serial));
    strncpy(name, "cgrau board1 Nano", sizeof(name)); // Board-Name ebenfalls fixiert
    
    uint8_t numberDevices[kTypeMax] = {0};
    GetArraySizes(numberDevices);
    InitArrays(numberDevices);
    readConfigFromMemory();
}

void readConfigFromMemory()
{
    uint16_t addrMem      = 0;    
    char     params[8]    = "";   
    uint8_t  command      = 0;    
    bool     copy_success = true; 

    command = readUint(&addrMem);

    do {
        switch (command) {
        case kTypeButton:
            params[0] = readUint(&addrMem);                              
            Button::Add(params[0], &nameBuffer[pNameBuffer]);                                          
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer); 
            break;

        case kTypeOutput:
            params[0] = readUint(&addrMem); 
            Output::Add(params[0]);
            copy_success = readEndCommand(&addrMem, ':'); 
            break;

        case kTypeEncoderSingleDetent:
        case kTypeEncoder:
            params[0] = readUint(&addrMem); 
            params[1] = readUint(&addrMem); 
            params[2] = 0;                                   

            if (command == kTypeEncoder) params[2] = readUint(&addrMem); 

            Encoder::Add(params[0], params[1], params[2], &nameBuffer[pNameBuffer]);      
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer); 
            break;

#ifdef MF_CUSTOMDEVICE_SUPPORT
        case kTypeCustomDevice: {
            uint16_t adrType = addrMem; 
            copy_success     = readEndCommand(&addrMem, '.');
            if (!copy_success) break;

            uint16_t adrPin = addrMem; 
            copy_success    = readEndCommand(&addrMem, '.');
            if (!copy_success) break;

            uint16_t adrConfig = addrMem; 
            copy_success       = readEndCommand(&addrMem, '.');
            if (copy_success) {
                CustomDevice::Add(adrPin, adrType, adrConfig);
                copy_success = readEndCommand(&addrMem, ':'); 
            }
            break;
        }
#endif
        default:
            copy_success = readEndCommand(&addrMem, ':'); 
        }
        command = readUint(&addrMem);
    } while (command && copy_success);
}

void OnGetConfig()
{
    // BUG GEFIXT: Sendet jetzt den gesamten Flash-String am Stück, ohne falsche Kommas!
    cmdMessenger.sendCmdStart(kInfo);
    cmdMessenger.sendCmdArg((const __FlashStringHelper*)CustomDeviceConfig);
    cmdMessenger.sendCmdEnd();
    boardReady = true;
}

void OnGetInfo()
{
    cmdMessenger.sendCmdStart(kInfo);
    cmdMessenger.sendCmdArg(F(MOBIFLIGHT_TYPE));
    cmdMessenger.sendCmdArg(name);
    cmdMessenger.sendCmdArg(serial);
    cmdMessenger.sendCmdArg(VERSION);
    cmdMessenger.sendCmdArg(CORE_VERSION);
    cmdMessenger.sendCmdEnd();
}

bool getStatusConfig() { return configActivated; }

void OnGenNewSerial() { cmdMessenger.sendCmd(kInfo, serial); }

void storeName() {}
void restoreName() {}

void OnSetName() { cmdMessenger.sendCmd(kStatus, name); }