/*
  Based on Arduino SD card example sketch: Listfiles
  Modified to use ESP32 HSPI pins
  and to ignore files that begin with "/."  (hidden files)

  This example shows how print out the files in a
  directory on an SD card

  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 15
 ** MISO - pin 2
 ** CLK - pin 14
 ** CS - pin 13

  created   Nov 2010
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe
  modified 2 Feb 2014
  by Scott Fitzgerald
  modified 2021
  by Gadget Reboot

  This example code is in the public domain.

*/
#include <SPI.h>
#include <SD.h>

File root;

SPIClass SDSPI(HSPI);

#define sck 14
#define miso 2
#define mosi 15
#define ss 13

void setup() {
  Serial.begin(115200);

  Serial.print("\n\nInitializing SD card...");
  SDSPI.begin(sck, miso, mosi, -1);
  SD.begin(ss, SDSPI);

  if (!SD.begin(ss, SDSPI)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("done.\n");
  Serial.println("Directory file list:\n");
  root = SD.open("/");
  printDirectory(root, 0);
  Serial.println("\nEnd of directory.");
}

void loop() {
  // nothing to do
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    /*
      for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
      }
    */
    String s1 = String(entry.name());
    if (s1[1] != '.') {                // skip file if it begins with . 
      s1 = s1.substring(1);            // remove first character '/' from file name
      Serial.print(s1);
      if (entry.isDirectory()) {
        Serial.println("/");
        printDirectory(entry, numTabs + 1);
      } else {
        // files have sizes, directories do not
        Serial.print("\t\t");
        Serial.println(entry.size(), DEC);
      }
      entry.close();
    }
  }
}
