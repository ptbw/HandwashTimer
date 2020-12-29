/*

*/

#include "Adafruit_VL53L0X.h"   // TOF sensor
#include <TFT_eSPI.h>           // Hardware-specific library
#include <SPI.h>
#include "Button2.h"

#define TFT_GREY 0x5AEB

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35
#define BUTTON_2            0

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
Button2 btn1(BUTTON_1);

int btnCick = false;
int vref = 1100;

uint32_t targetTime = 0;                    // for next 1 second timeout
int8_t ss = 46;
byte oss = 99;
byte xsecs = 0;

bool checking = true;

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

// Use a long press on upper button to enable sleep (press again to wake up)
void button_init()
{
    btn1.setPressedHandler([](Button2 & b) {        
        btnCick = false;
        tft.writecommand(TFT_DISPON);
        digitalWrite(TFT_BL, true);
        //Serial.print("Button 1 pressed ");
        //int r = digitalRead(TFT_BL);
        showVoltage();        
        tft.setTextSize(3);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);        
        tft.drawString("Good Night",  tft.width() / 2, tft.height() / 2 );
        espDelay(2000);
       
        digitalWrite(TFT_BL, false);
        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
        //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);        
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        delay(200);
        esp_deep_sleep_start();
    });    
}

void showVoltage()
{
    static uint64_t timeStamp = 0;    
    uint16_t v = analogRead(ADC_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    String voltage = "Batt: " + String(battery_voltage) + "V";
    Serial.println(voltage);
    tft.fillScreen(TFT_BLACK);        
    tft.drawString(voltage,  tft.width() / 6, tft.height() / 2 );   
    espDelay(3000);
}

// Update the display returning false until timer expires
bool mayBeUpdate()
{
  
  if (targetTime < millis()) {
      // Set next update for 1 second later
      targetTime = millis() + 1000;
  
      ss--;               // Subtract a second
      if (ss < 0) {      // Check for roll-over
        ss = 45;          // Reset seconds to zero      
        tft.fillScreen(TFT_BLACK);
        return true;
      }
    
      // Update digital time
      int xpos = 40;
      int ypos = 0; // Top left corner of clock text, about half way down
      int ysecs = ypos + 20;
      int xsecs = xpos;
  
      if (oss != ss) { // Redraw seconds time every second
        oss = ss;
        xpos = xsecs;
  
        if ( ss <= 10 ) {
          tft.setTextColor(TFT_RED, TFT_BLACK);
        }
        else {
          tft.setTextColor(TFT_YELLOW, TFT_BLACK);    // Set colour back to yellow
        }
      
        if (ss < 10) xpos += tft.drawChar('0', xpos, ysecs, 6); // Add leading zero
        tft.drawNumber(ss, xpos, ysecs, 6);                     // Draw seconds      
      }
    }
    return false;
}

void setup(void) {
  Serial.begin(115200);

  // wait until serial port opens for native USB devices
  // while (! Serial) {
  //   delay(1);
  // }

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(3);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  //tft.setTextDatum(MC_DATUM);
  tft.drawString("Hello",  tft.width() / 4, tft.height() / 2 );
  espDelay(2000);
  showVoltage();  
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(3);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  targetTime = millis() + 1000;

  button_init();

  checking = true;
  
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;
  
  if( checking )
  {
    digitalWrite(TFT_BL, false);
    tft.writecommand(TFT_DISPOFF);    
    // Serial.print("Reading a measurement... ");
    lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

    if (measure.RangeStatus != 4) {  // phase failures have incorrect data
      Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
      if( measure.RangeMilliMeter < 100 )
        checking = false;  
    }
  }
  else {
    tft.writecommand(TFT_DISPON);
    digitalWrite(TFT_BL, true);
    checking = mayBeUpdate();
  }

  btn1.loop();
}
