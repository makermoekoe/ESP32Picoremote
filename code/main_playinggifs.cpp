#include <Arduino.h>
#include <driver/dac.h>
#include <FastLED.h>
#include <Wire.h>
#include "ClosedCube_HDC1080.h"

#include <AnimatedGIF.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "../test_images/moekoe2.h"
//#include "../test_images/badgers.h"
//#include "../test_images/homer.h"
//#include "../test_images/homer_tiny.h"
//#include "../test_images/pattern.h"
//#include <Adafruit_LIS3DH.h>
//#include <Adafruit_Sensor.h>

ClosedCube_HDC1080 hdc;
TFT_eSPI tft = TFT_eSPI();
//Adafruit_LIS3DH lis = Adafruit_LIS3DH();


#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 80
AnimatedGIF gif;

#define NORMAL_SPEED
#define GIF_IMAGE1 moekoe2

struct box {
  uint8_t left,top,width,height;
  uint32_t color,color_in;
  uint8_t border,radius;

  int left_in(){return left + border;}
  int top_in(){return top + border;}
  int width_in(){return width - 2*border;}
  int height_in(){return height - 2*border;}
};

box bbox_left = {1,2,2,76,TFT_WHITE,TFT_BLUE,0,2};
box bbox_right = {157,2,2,76,TFT_WHITE,TFT_BLUE,0,2};

box center = {bbox_left.left+bbox_left.width,0,bbox_right.left-(bbox_left.left+bbox_left.width),80,0,0,0,0};

const int btn1 = 25;
const int btn2 = 5;
const int latch = 26;
const int led_pin = 27;
const int tft_bl = 12;
const int stat_mcp = 9;

#define NUM_LEDS 2
CRGB leds[NUM_LEDS];

bool ostate_left, state_left = false;
bool ostate_right, state_right = false;

void draw_buttons(bool left_pressed, bool right_pressed){
  tft.fillRect(bbox_left.left, bbox_left.top, bbox_left.width, bbox_left.height, TFT_BLACK);
  tft.fillRect(bbox_right.left, bbox_right.top, bbox_right.width, bbox_right.height, TFT_BLACK);

  tft.fillRoundRect(bbox_left.left, bbox_left.top, bbox_left.width, bbox_left.height, bbox_left.radius, bbox_left.color);
  tft.fillRoundRect(bbox_right.left, bbox_right.top, bbox_right.width, bbox_right.height, bbox_right.radius, bbox_right.color);

  if(left_pressed){
    tft.fillRoundRect(bbox_left.left_in(), bbox_left.top_in(), bbox_left.width_in(), bbox_left.height_in(), bbox_left.radius, bbox_left.color_in);
  }
  if(right_pressed){
    tft.fillRoundRect(bbox_right.left_in(), bbox_right.top_in(), bbox_right.width_in(), bbox_right.height_in(), bbox_right.radius, bbox_right.color_in);
  }
}

int read_buttons(bool &b1, bool &b2, bool upd_tft, bool upd_leds){
  b1 = digitalRead(btn2);
  b2 = digitalRead(btn1);
  if(upd_tft && (ostate_left != state_left || ostate_right != state_right)){
    draw_buttons(state_left, state_right);
    ostate_left = state_left;
    ostate_right = state_right;
  }
  if(upd_leds){
    leds[1] = CRGB(0,b1*150,0);
    leds[0] = CRGB(0,0,b2*150);
    FastLED.show();
  }
}

void update_hdc(){
  float temp_hdc = hdc.readTemperature();
  float hum_hdc = hdc.readHumidity();
  tft.fillRect(center.left, center.top, center.width, center.height, TFT_BLACK);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);

  tft.setTextDatum(MC_DATUM);
  tft.drawString((String(temp_hdc) + "C"),50,40,2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString((String(hum_hdc) + "%"),110,40,2);
}


#define BUFFER_SIZE 256
uint16_t usTemp[1][BUFFER_SIZE];
bool     dmaBuf = 0;
  
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

  // Displ;ay bounds chech and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
    return;

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0][0];
      while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE )
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA // 71.6 fps (ST7796 84.5 fps)
    tft.dmaWait();
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
#endif

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA
      tft.dmaWait();
      tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
      dmaBuf = !dmaBuf;
#else
      tft.pushPixels(&usTemp[0][0], iCount);
#endif
      iWidth -= iCount;
    }
  }
} /* GIFDraw() */

void play_gif1(){
  if (gif.open((uint8_t *)GIF_IMAGE1, sizeof(GIF_IMAGE1), GIFDraw)) {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    tft.startWrite(); // The TFT chip slect is locked low
    while (gif.playFrame(true, NULL)) {
      yield();
    }
    gif.close();
    tft.endWrite(); // Release TFT chip select for other SPI devices
  }
}


void setup() {
  pinMode(latch,OUTPUT);
  digitalWrite(latch,HIGH);

  dac_output_disable(DAC_CHANNEL_1);

  pinMode(btn1,INPUT);
  pinMode(btn2,INPUT);
  Serial.begin(115200);
  Serial.println("start");

  pinMode(tft_bl,OUTPUT);
  digitalWrite(tft_bl,HIGH);

  Wire.begin(13,14);

  FastLED.addLeds<WS2812, led_pin, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(200);
  delay(1);
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(150, 0, 0);
  FastLED.show();

  hdc.begin(0x40);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);

  gif.begin(BIG_ENDIAN_PIXELS);

  digitalWrite(tft_bl,LOW);

  play_gif1();

  //draw_buttons(0,0);
}



unsigned long t = millis();
unsigned long t_both_btn_pressed = 0;



void loop() {
  play_gif1();

  read_buttons(state_left, state_right, true, true);
  if(state_left && state_right){
    if(millis() > t_both_btn_pressed + 1000){
      //shut down device
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(150, 0, 0);
      FastLED.show();
      digitalWrite(latch,LOW);
      delay(200);
      while(digitalRead(btn1)) delay(100);
    }
  }
  else if(state_left){
    
  }
  else if(state_right){

  }
  else{
    t_both_btn_pressed = millis();
  }

  delay(10);

  /*if(millis() > t + 1000){
    update_hdc();
    t = millis();
  }*/
  
}