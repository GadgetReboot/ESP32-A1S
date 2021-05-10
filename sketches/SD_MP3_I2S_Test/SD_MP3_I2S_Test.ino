// ESP32-A1S module demo
// display directory contents
// playback mp3 files from root directory of an SD card
// to I2S AC101 audio device
//
// requires audio library:  https://github.com/earlephilhower/ESP8266Audio
// requires AC101 library:  https://github.com/marcel-licence/AC101
//
// note: this doesn't work properly - it was hacked together from other esp8266 audio player examples
//       it will skip loading the first mp3 and start from the second, play it, then give an error buflen 0
//       but it does read from SD and play over I2S, which is what the goal was.
// Gadget Reboot

#include <Arduino.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AC101.h"

// ESP32 pins for AC101 chip
#define IIS_SCLK                    27
#define IIS_LCLK                    26
#define IIS_DSIN                    25

#define IIC_CLK                     32
#define IIC_DATA                    33

// You may need a fast SD card. Set this as high as it will work (40MHz max).
#define SPI_SPEED SD_SCK_MHZ(40)

static AC101 ac;

File file, dir;
AudioFileSourceID3 *id3;
AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
AudioFileSourceSD *source = NULL;

// SD card connected to these pins
// using HSPI channel on ESP32
SPIClass SDSPI(HSPI);

#define sck  14
#define miso  2
#define mosi 15
#define ss   13

void setup()
{
  Serial.begin(115200);

  Serial.printf("Init AC101 codec ");
  while (not ac.begin(IIC_DATA, IIC_CLK))
  {
    Serial.printf(".");
    delay(500);
  }
  Serial.printf("OK\n");

  ac.SetVolumeSpeaker(50);
  ac.SetVolumeHeadphone(50);

  audioLogger = &Serial;
  source = new AudioFileSourceSD();
  id3 = NULL;
  mp3 = new AudioGeneratorMP3();
  out = new AudioOutputI2S();
   
  // define I2S pins for AC101
  out->SetPinout(IIS_SCLK /*bclkPin*/, IIS_LCLK /*wclkPin*/, IIS_DSIN /*doutPin*/);

  // init SD card
  SDSPI.begin(sck, miso, mosi, -1);
  SD.begin(ss, SDSPI);

  dir  = SD.open("/");

   Serial.println("Directory file list:\n");
  printDirectory(dir, 0);
  Serial.println("\nEnd of directory.");
  dir.rewindDirectory();
}

void loop()
{
  String fileName = "";

  if (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  }
  else {
    // Find first MP3 file and play it
    while ((file = dir.openNextFile())) {
      if (String(file.name()).endsWith(".mp3")) {
        if (source->open(file.name())) {
          fileName = String(file.name());
          break;
        }
      }
    }

    if (fileName.length() > 0) {
      id3 = new AudioFileSourceID3(source);
      id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
      mp3->begin(id3, out);
      Serial.printf("Starting playback of '%s'...\n", fileName.c_str());
    } else {
      Serial.println("Can't find .mp3 file on SD card.");
      delay (1000);
    }
  }
}

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 info: %s = '", type);

  if (isUnicode) {
    string += 2;
  }

  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
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
