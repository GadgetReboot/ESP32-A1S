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

//Michael-Nolan - I took this and extended the functionality
//Hack v00 -      I worked on the play button, stop and start the id3/mp3 playing including capture pos
//                seek back to the pos after re-opening the file (check) and start playing from there works.
/*     v01 -      Add the volume keys on 18/19 (key5 and 3) - when I used the 13 (key2) it seemed to
                  mess up SD card communications 13 is defined as SS - 3 parameters used 
                  for SDSPI.begin(sck, miso, mosi, -1);
                  the SS used SD.begin(ss, SDSPI); - i don't see where SD is allocated as an object ...
                  So no, don't use 13 because its connected to SD - which is a stupid overlap with a key ... sheesh
                  Timing for long and shorter press functions like volume change or next prev track
                  Files array of file from directory
*/
#include <Arduino.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorFLAC.h"
#include "AudioOutputI2S.h"

#include "AC101.h"

// ESP32 pins for AC101 chip
#define IIS_SCLK                    27
#define IIS_LCLK                    26
#define IIS_DSIN                    25

#define IIC_CLK                     32
#define IIC_DATA                    33

#define UNKNOWNFILE                 0
#define FLACFILE                    1
#define MP3FILE                     2

//Sat Jun 19 2021
//Michael hack - take some code from ESP32AudioKit to add key input so I can start
// coding up features and tests
#define PIN_PLAY                    (23)      // KEY 4
#define PIN_VOL_UP                  (18)      // KEY 5
#define PIN_VOL_DOWN                (19)       // KEY 3
#define MAX_FILES 20

// You may need a fast SD card. Set this as high as it will work (40MHz max).
#define SPI_SPEED SD_SCK_MHZ(40)

static AC101 ac;

File file, currentFile, dir;
String filenames[MAX_FILES]; // so we can facilitate fwd/back tracks lets make an array of files
uint8_t fileIndex = 0;  // where we are as we write the files or play the files
uint8_t lastFileNumber = 0;
AudioFileSourceID3 *id3;
AudioGeneratorMP3 *mp3;
AudioGeneratorFLAC *flac;
AudioOutputI2S *out;
AudioFileSourceSD *source = NULL;

// SD card connected to these pins
// using HSPI channel on ESP32
SPIClass SDSPI(HSPI);

#define sck  14
#define miso  2
#define mosi 15
#define ss   13

static uint8_t volume = 40;
const uint8_t volume_step = 2;
unsigned long debounce = 0;
unsigned long upPressedTime, downPressedTime;
bool  playState = LOW;
String currentFilename = "";
uint32_t currentSongPos;
static uint8_t fileTypePlaying = UNKNOWNFILE;



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

  ac.SetVolumeSpeaker(0);
  ac.SetVolumeHeadphone(volume);

  audioLogger = &Serial;
  source = new AudioFileSourceSD();
  id3 = NULL;
  mp3 = new AudioGeneratorMP3();
  flac = new AudioGeneratorFLAC();
  out = new AudioOutputI2S();
  String fileName = "";
  
   
  // define I2S pins for AC101
  out->SetPinout(IIS_SCLK /*bclkPin*/, IIS_LCLK /*wclkPin*/, IIS_DSIN /*doutPin*/);

  // init SD card
  SDSPI.begin(sck, miso, mosi, -1);
  SD.begin(ss, SDSPI);

  dir  = SD.open("/");

  // Configure keys on ESP32 Audio Kit board
  pinMode(PIN_PLAY, INPUT_PULLUP);
  pinMode(PIN_VOL_UP, INPUT_PULLUP);
  pinMode(PIN_VOL_DOWN, INPUT_PULLUP);


  Serial.println("Directory file list:\n");
  readDirectory(dir, 4);
  
  Serial.println("\nEnd of directory.");
  
  fileIndex = 0;
  dir.rewindDirectory();
}
/* key pressed function stolen from ESP32Audio Kit
 *  
 */
bool pressed( const int pin )
{
  if (millis() > (debounce + 500))
  {
      if (digitalRead(pin) == LOW)
      {
        debounce = millis();
        return true;
      }
  }
  
  return false;
}

bool stillPressed(const int pin)
{
  return (digitalRead(pin) == LOW);
  
}

bool getPlayableType()
{
  if(String(file.name()).endsWith(".mp3")){
     fileTypePlaying = MP3FILE;
     return true;
  } else if(String(file.name()).endsWith(".flac")){
     fileTypePlaying = FLACFILE;
     return true;
  } else
     fileTypePlaying = UNKNOWNFILE;
     return true;
  
}

boolean flacCheck(){
  boolean check = flac->isRunning();
  if (check){
      if (!flac->loop()) 
      {
        flac->stop();
        fileIndex++;
        Serial.printf("Finished playback of '%s'...\n", currentFilename.c_str());
      }
  }
  return check;
}

void loop()
{
   
  //monitoring and ending file media player
   if (mp3->isRunning()) {
     if (!mp3->loop()) 
     {
      mp3->stop();
      fileIndex++;
      Serial.printf("Finished playback of '%s'...\n", currentFilename.c_str());
     }
   } else if (flacCheck()) flacCheck();
   // file is not playing - if playState = high get another file and try to continue playing
  else if(playState){
    // Find MP3 file and play it
    constrain(fileIndex,0,lastFileNumber);
    while ((fileIndex < lastFileNumber) && SD.exists(filenames[fileIndex])) {     
      file = SD.open(filenames[fileIndex]);
      
      if (getPlayableType()) {
        printDirectory();
        if (source->open(file.name())) {
          currentFilename = String(file.name());
          currentFile = file;
          
          break;
        } 
      } else {
          fileIndex++;
          if (fileIndex >= lastFileNumber){
            currentFilename = "";
            playState = LOW;
          }
        }
      
      
    } //close while
    if(source->isOpen()){// this check stops trying to play unopened unloadable + prevent not ready buzz sound
      switch(fileTypePlaying){
        case MP3FILE:
          playSourceAsID3(); 
          break;
        case FLACFILE:
          playSourceAsFLAC();
          break;
      }
    }
  }//close if

  //Michael's key commands Play and Pause/resume functionality capturing file reading position 
  flacCheck();
  if (pressed(PIN_PLAY))
    {
      // Start playing
      if(playState) //if playing then pause
      {
        stopPlayPause(HIGH); //multifiletype aware command
        
      } else {
        
      playState=HIGH; //meaning on
      Serial.println("Play State to On");
      if(source->open(currentFile.name())) { // figure out how to get this file again from dir.whatever ...
       // this check stops trying to play unopened unloadable + prevent not ready buzz sound
        switch(fileTypePlaying){
          case MP3FILE:
            //resumeSourceAsID3();
            id3->seek(currentSongPos, dir);  //source?? why dir?
            mp3->begin(id3, out); 
            break;
            
          case FLACFILE:
            //resumeSourceAsFLAC();
            source->seek(currentSongPos, dir);
            flac->begin(source, out); 
            break;
        }
        
        
      } else Serial.println("Can't reopen "+ String(currentFile.name()));
    }
  }
  flacCheck();
   //----------------UP VOL button processing
  if (pressed(PIN_VOL_UP)){
     if(!upPressedTime) 
       if(upPressedTime == 0)  upPressedTime = millis();  //when first detecting press start timing
  } else {
    if (upPressedTime !=0){
      long int timeSinceUp = millis() - upPressedTime;
      if(timeSinceUp > 1500)    // after 2 sec is it still pressed ... trigger action alternate
      {
        if (stillPressed(PIN_VOL_UP)){
          playNext(timeSinceUp);  // longpress action
          if(volume > volume_step) //kludge because the button press gets triggered in the time after
            volume -= volume_step;                    
        } else {
          if(volume < 90)
            volume += volume_step;   // short press action
          ac.SetVolumeHeadphone(volume);
          Serial.println("volume up a step to " + String(volume));
        }
            
        upPressedTime = 0;
      }  
    }
  }
  //----------------DOWN VOL button processing
   if (pressed(PIN_VOL_DOWN)){
     if(!upPressedTime) 
       if(downPressedTime == 0)  downPressedTime = millis();  //when first detecting press start timing
  } else {
    if (downPressedTime !=0){
      long int timeSinceDown = millis() - downPressedTime;
      if(timeSinceDown > 1500)    // after 2 sec is it still pressed ... trigger action alternate
      {
        if (stillPressed(PIN_VOL_DOWN)){
          playPrev(timeSinceDown);  // longpress action
          if(volume < 90) //kludge because the button press gets triggered in the time after
            volume += volume_step;                    
        } else {
          if(volume > volume_step)
            volume -= volume_step;   // short press action
          ac.SetVolumeHeadphone(volume);
          Serial.println("volume down a step to " + String(volume));
        }
            
        downPressedTime = 0;
      }  
    }
  }
 /* 
  if (pressed(PIN_VOL_DOWN)){
     if(volume > 2) volume -= volume_step;
     ac.SetVolumeHeadphone(volume);
     if(downPressedTime == 0)  downPressedTime = millis();
  } */
}
//---------------------------------- End Loop Code
void stopPlayPause(boolean pause)
{
  switch(fileTypePlaying){
          case MP3FILE:
            //resumeSourceAsID3();
            if (mp3->isRunning()){
              if(pause) {
                playState=LOW; //meaning off
                currentSongPos = id3->getPos();  // capture where we are in song so we can restart from there
                Serial.println("Play State to Off. Pausing "+ String(currentFile.name()) + "@" + String(currentSongPos));
              } 
              mp3->stop();
            }
            break;
          case FLACFILE:
            //resumeSourceAsFLAC();
            if (flac->isRunning()){
              
              if(pause){ 
                playState=LOW; //meaning off
                currentSongPos = source->getPos();
                Serial.println("Play State to Off. Pausing "+ String(currentFile.name()) + "@" + String(currentSongPos));
              }
              flac->stop(); 
            }
            break;
        }
}
void playNext(long int timeHeld) // need to adapt to when it is paused
{
  
  Serial.println("play next triggered " + String(timeHeld));
  if(playState){
    stopPlayPause(LOW);//mp3->stop();
    fileIndex++;
  } else {
    playState = HIGH;
    fileIndex++;
  }
}

void playPrev(long int timeHeld)
{
  Serial.println("play prev triggered " + String(timeHeld));
  if(playState){ //if playing the stop and index back 1 track
    stopPlayPause(LOW);//mp3->stop();
    fileIndex--;
  } else {  //if paused and track back then backup to the start of this track
    playState = HIGH;
    if(currentSongPos > 100)  //this value can become a value where if its very close to the start it will
      currentSongPos = 0;      //backup to previous track instead of start of this track reset SongPos
    else
      fileIndex--;
  }
}

void playSourceAsID3() // relies on global variabls like fileName
{
  if (currentFilename.length() > 0) {
      id3 = new AudioFileSourceID3(source);
      id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
      mp3->begin(id3, out);
      Serial.printf("Starting playback of '%s'...\n", currentFilename.c_str());
    } else {
      Serial.println("Can't find .mp3 file on SD card.");
      playState=LOW;
      delay (700);
    }
}

void playSourceAsFLAC()
{
    if (currentFilename.length() > 0) {
      
      flac->begin(source, out);
      Serial.printf("Starting playback of '%s'...\n", currentFilename.c_str());
    } else {
      Serial.println("Can't find .flac file on SD card.");
      playState=LOW;
      delay (700);
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

void readDirectory(File dir, int numTabs) {
  //fileIndex = 0;
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      lastFileNumber = fileIndex;
      Serial.println("Number of Files Indexed " + String(lastFileNumber));
      // reset to start of list
      break;
    } else {
    if(fileIndex < MAX_FILES){
      filenames[fileIndex] = String(entry.name());
      fileIndex++;
      }
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
        readDirectory(entry, numTabs + 1);
      } else {
        // files have sizes, directories do not
        Serial.print("\t\t");
        Serial.println(entry.size(), DEC);
      }
      entry.close();
    }
  }
  
}

void printDirectory() // also show a marker for the fileIndex where we are playing song
{
  int tmpIndex = fileIndex;
  for(fileIndex = 0; fileIndex <= lastFileNumber; fileIndex++)
  {
    if (fileIndex == tmpIndex) Serial.print("* "+ String(fileIndex+1)+"/"+ String(lastFileNumber)+" "); // in case you want to use this to follow where we are
    Serial.println(filenames[fileIndex]);
    
  }
  fileIndex = tmpIndex;
}
