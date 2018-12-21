// based on Adafruit's readMifare sketch, with extras removed
// removed support for Mifare Ultralight
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)
#define RELAY_PIN (10)

#define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINT(...)     Serial.print (__VA_ARGS__)
 //#define DEBUG_PRINTDEC(x)     Serial.print (x, DEC)
 //#define DEBUG_PRINTHEX(x)     Serial.print (x, HEX)
 #define DEBUG_PRINTLN(...)  Serial.println (__VA_ARGS__)
 //#define DEBUG_PRINTLN(x)  Serial.println (x)
 //#define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 //#define DEBUG_PRINTDEC(x)
 //#define DEBUG_PRINTHEX(x)
 #define DEBUG_PRINTLN(x) 
#endif

// Breakout board with a software SPI connection:
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
   #define Serial SerialUSB
#endif

void setup(void) {
  // PREPARE OUTPUTS
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // BEGIN SERIAL IF DEBUGGING
  #ifdef DEBUG
    // if debugging, wait for a serial connection before proceeding
    #ifndef ESP8266
      while (!Serial); // for Leonardo/Micro/Zero
    #endif
    Serial.begin(115200);
  #endif
  DEBUG_PRINTLN("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    DEBUG_PRINT("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  DEBUG_PRINT("Found chip PN5"); DEBUG_PRINTLN((versiondata>>24) & 0xFF, HEX); 
  DEBUG_PRINT("Firmware ver. "); DEBUG_PRINT((versiondata>>16) & 0xFF, DEC); 
  DEBUG_PRINT('.'); DEBUG_PRINTLN((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();

  DEBUG_PRINTLN("Waiting for an ISO14443A Card ...");
}


void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    // Display some basic information about the card
    DEBUG_PRINTLN("Found an ISO14443A card");
    DEBUG_PRINT("  UID Length: ");DEBUG_PRINT(uidLength, DEC);DEBUG_PRINTLN(" bytes");
    DEBUG_PRINT("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    DEBUG_PRINTLN("");
    
    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ... 
      DEBUG_PRINTLN("Seems to be a Mifare Classic card (4 byte UID)");
    
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      DEBUG_PRINTLN("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    
    // Start with block 4 (the first block of sector 1) since sector 0
    // contains the manufacturer data and it's probably better just
    // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
    
      if (success)
      {
        DEBUG_PRINTLN("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);
    
        if (success)
        {
          // Data seems to have been read ... spit it out
          DEBUG_PRINTLN("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
          DEBUG_PRINTLN("");

          // CHECK THE PASSWORD
          if(strncmp((char *)data, "dave", 4) == 0)
          {
            DEBUG_PRINTLN("Password is correct. Opening");
            // FLASH THE LED AND TOGGLE THE RELAY
            digitalWrite(RELAY_PIN, HIGH);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(300); // MS
            digitalWrite(RELAY_PIN, LOW);
            delay(1200); // keep the LED on for a bit longer
            digitalWrite(LED_BUILTIN, LOW);
                      
          } else {
            DEBUG_PRINT("Password is incorrect: ");DEBUG_PRINTLN(data[0]);
          }
      
          // Wait a bit before reading the card again
          delay(1000);
        }
        else
        {
          DEBUG_PRINTLN("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        DEBUG_PRINTLN("Ooops ... authentication failed: Try another key?");
      }
    }
  }
}

