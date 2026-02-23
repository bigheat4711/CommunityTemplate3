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
#ifdef HAS_CONFIG_IN_FLASH
#include "MFCustomDevicesConfig.h"
#else
const char CustomDeviceConfig[] PROGMEM = {};
#endif

// The build version comes from an environment variable
#define STRINGIZER(arg) #arg
#define STR_VALUE(arg)  STRINGIZER(arg)
#define VERSION         STR_VALUE(BUILD_VERSION)
#define CORE_VERSION    STR_VALUE(CORE_BUILD_VERSION)
MFEEPROM MFeeprom;

const uint8_t MEM_OFFSET_NAME   = 0;
const uint8_t MEM_LEN_NAME      = 48;
const uint8_t MEM_OFFSET_SERIAL = MEM_OFFSET_NAME + MEM_LEN_NAME;
const uint8_t MEM_LEN_SERIAL    = 11;
const uint8_t MEM_OFFSET_CONFIG = MEM_OFFSET_NAME + MEM_LEN_NAME + MEM_LEN_SERIAL;

#ifdef ARDUINO_ARCH_AVR
char serial[11]; // 3 characters for "SN-",7 characters for "xyz-zyx" plus terminating NULL
#endif
char           name[MEM_LEN_NAME]              = MOBIFLIGHT_NAME;
const int      MEM_LEN_CONFIG                  = MEMLEN_CONFIG;
char           nameBuffer[MEMLEN_NAMES_BUFFER] = "";
boolean        configActivated                 = false;
uint16_t       pNameBuffer                     = 0; // pointer for nameBuffer during reading of config
const uint16_t configLengthFlash               = sizeof(CustomDeviceConfig);
bool boardReady                                = false;

void resetConfig();
void readConfig();
void _activateConfig();
void readConfigFromMemory();

bool getBoardReady()
{
    return boardReady;
}

// ************************************************************
// configBuffer handling
// ************************************************************

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

// reads an ascii value which is '.' terminated from EEPROM or Flash and returns it's value
uint8_t readUint(volatile uint16_t *addrMem)
{
    char    params[4] = {0}; // max 3 (255) digits NULL terminated
    uint8_t counter   = 0;
    do {
            params[counter++] = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++);
    } while (params[counter - 1] != '.' && counter < sizeof(params)); // reads until limiter '.' and for safety reason not more then size of params[]
    params[counter - 1] = 0x00; // replace '.' by NULL to terminate the string
    return atoi(params);
}

// reads a string from EEPROM or Flash at given address which is ':' terminated and saves it in the nameBuffer
// once the nameBuffer is not needed anymore, just read until the ":" termination -> see function below
bool readName(uint16_t *addrMem, char *buffer, uint16_t *pBuffer)
{
    char     temp   = 0;
    do {
            temp = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++); // read the first character
            if (*addrMem > configLengthFlash)                             // abort if config array size will be exceeded
                return false;
        buffer[(*pBuffer)++] = temp;         // save character and locate next buffer position
        if (*pBuffer >= MEMLEN_NAMES_BUFFER) // nameBuffer will be exceeded
        {
            return false; // abort copying from EEPROM to nameBuffer
        }
    } while (temp != ':'); // reads until limiter ':' and locates the next free buffer position
    buffer[(*pBuffer) - 1] = 0x00; // replace ':' by NULL, terminates the string
    return true;
}

// steps thru the EEPRROM or Flash until the delimiter is detected
// it could be ":" for end of one device config
// or "." for end of type/pin/config entry for custom device
bool readEndCommand(uint16_t *addrMem, uint8_t delimiter)
{
    char     temp   = 0;
    do {
            temp = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++);
            if (*addrMem > configLengthFlash) // abort if config array size will be exceeded
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
    uint16_t addrMem;
    uint8_t  device;
        addrMem = 0;

    device = readUint(&addrMem);

    // step through the Memory and calculate the number of devices for each type
    do {
        numberDevices[device]++;
        copy_success = readEndCommand(&addrMem, ':'); // check EEPROM until end of name
        device       = readUint(&addrMem);
    } while (device && copy_success);

    if (!copy_success) { // too much/long names for input devices -> tbd how to handle this!!
        cmdMessenger.sendCmd(kStatus, F("Failure, EEPROM size exceeded "));
        return false;
    }
    return true;
}

void InitArrays(uint8_t *numberDevices)
{
    // Call the function to allocate required memory for the arrays of each type
    if (!Button::setupArray(numberDevices[kTypeButton]))
        sendFailureMessage("Button");
    if (!Output::setupArray(numberDevices[kTypeOutput]))
        sendFailureMessage("Output");

    if (!Encoder::setupArray(numberDevices[kTypeEncoder] + numberDevices[kTypeEncoderSingleDetent]))
        sendFailureMessage("Encoders");

#ifdef MF_CUSTOMDEVICE_SUPPORT
    if (!CustomDevice::setupArray(numberDevices[kTypeCustomDevice]))
        sendFailureMessage("CustomDevice");
#endif
    return;
}

const char* myConfig = "4,6,7,Encoder;1,5,Button;10,8,9,cgrau's board1;";

void readConfig()
{
    strncpy(serial, "SN-D3E-D82", sizeof(serial));
    uint8_t numberDevices[kTypeMax] = {0};

    // Determine which configuration to use and proceed
    GetArraySizes(numberDevices);
    InitArrays(numberDevices);
    readConfigFromMemory();
}

void readConfigFromMemory()
{
    uint16_t addrMem      = 0;    // define first memory location where config is saved in EEPROM or Flash
    char     params[8]    = "";   // buffer for reading parameters from EEPROM or Flash and sending to ::Add() function of device
    uint8_t  command      = 0;    // read the first value from EEPROM or Flash, it's a device definition
    bool     copy_success = true; // will be set to false if copying input names to nameBuffer exceeds array dimensions
                                  // not required anymore when pins instead of names are transferred to the UI

    // read the first value from EEPROM, it's a device definition
    command = readUint(&addrMem);

    // go through the EEPROM or Flash until it is NULL terminated
    do {
        switch (command) {
        case kTypeButton:
            params[0] = readUint(&addrMem);                              // Pin number
            Button::Add(params[0], &nameBuffer[pNameBuffer]);                             // MUST be before readName because readName returns the pointer for the NEXT Name
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer); // copy the NULL terminated name to nameBuffer and set to next free memory location
            break;

        case kTypeOutput:
            params[0] = readUint(&addrMem); // Pin number
            Output::Add(params[0]);
            copy_success = readEndCommand(&addrMem, ':'); // check EEPROM until end of name
            break;

        case kTypeEncoderSingleDetent:
        case kTypeEncoder:
            params[0] = readUint(&addrMem); // Pin1 number
            params[1] = readUint(&addrMem); // Pin2 number
            params[2] = 0;                                   // type

            if (command == kTypeEncoder)
                params[2] = readUint(&addrMem); // type

            Encoder::Add(params[0], params[1], params[2], &nameBuffer[pNameBuffer]);      // MUST be before readName because readName returns the pointer for the NEXT Name
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer); // copy the NULL terminated name to nameBuffer and set to next free memory location
            break;

#ifdef MF_CUSTOMDEVICE_SUPPORT
        case kTypeCustomDevice: {
            uint16_t adrType = addrMem; // first location of custom Type in EEPROM
            copy_success     = readEndCommand(&addrMem, '.');
            if (!copy_success)
                break;

            uint16_t adrPin = addrMem; // first location of custom pins in EEPROM
            copy_success    = readEndCommand(&addrMem, '.');
            if (!copy_success)
                break;

            uint16_t adrConfig = addrMem; // first location of custom config in EEPROM
            copy_success       = readEndCommand(&addrMem, '.');
            if (copy_success) {
                CustomDevice::Add(adrPin, adrType, adrConfig, true);
                copy_success = readEndCommand(&addrMem, ':'); // check EEPROM until end of command
            }
            break;
        }
#endif

        default:
            copy_success = readEndCommand(&addrMem, ':'); // check EEPROM until end of name
        }
        command = readUint(&addrMem);
    } while (command && copy_success);
    if (!copy_success) {                            // too much/long names for input devices
        nameBuffer[MEMLEN_NAMES_BUFFER - 1] = 0x00; // terminate the last copied (part of) string with 0x00
        cmdMessenger.sendCmd(kStatus, F("Failure on reading config"));
    }
}

void OnGetConfig()
{
    cmdMessenger.sendCmdStart(kInfo);
    
        cmdMessenger.sendCmdArg((char)pgm_read_byte_near(CustomDeviceConfig));
        for (uint16_t i = 1; i < (configLengthFlash - 1); i++) {
            cmdMessenger.sendArg((char)pgm_read_byte_near(CustomDeviceConfig + i));
        }
    cmdMessenger.sendCmdEnd();
    boardReady = true;
}

void OnGetInfo()
{
    // read the serial number and generate if 1st start up, was before in ResetBoard()
    // moved to this position as the time to generate a serial number in ResetBoard() is always the same
    // OnGetInfo() is called from the connector and the time is very likely always different
    // Therefore millis() can be used for randomSeed
    cmdMessenger.sendCmdStart(kInfo);
    cmdMessenger.sendCmdArg(F(MOBIFLIGHT_TYPE));
    cmdMessenger.sendCmdArg(name);
    cmdMessenger.sendCmdArg(serial);
    cmdMessenger.sendCmdArg(VERSION);
    cmdMessenger.sendCmdArg(CORE_VERSION);
    cmdMessenger.sendCmdEnd();
}

bool getStatusConfig()
{
    return configActivated;
}

// ************************************************************
// serial number handling
// ************************************************************

void OnGenNewSerial()
{
    cmdMessenger.sendCmd(kInfo, serial);
}

// ************************************************************
// Naming handling
// ************************************************************
void storeName()
{
}

void restoreName()
{
}

void OnSetName()
{
    cmdMessenger.sendCmd(kStatus, name);
}

// config.cpp
