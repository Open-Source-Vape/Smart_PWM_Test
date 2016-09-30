/*
 * Components used:
 * https://www.adafruit.com/products/931
 * https://www.sparkfun.com/products/9028
 * https://www.sparkfun.com/products/8374
 * https://www.sparkfun.com/products/13760
 * https://www.sparkfun.com/products/97
 * http://www.infineon.com/cms/en/product/power/power-mosfet/20v-300v-n-channel-power-mosfet/20v-30v-n-channel-power-mosfet/IRLS3813/productType.html?productType=5546d462533600a401533d4c1e1f77fa
 * https://www.radioshack.com/products/radioshack-220-ohm-1-4w-5-carbon-film-resistor-pk-5
 * place 220 ohm resistor between MCU pin 7 and gate on mosfet 
 */

#include <PWM.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

int battpin = A2;
int firepin = 7;
int mosfetpin = 3;
int uppin = 12;
int downpin = 11;


int32_t frequency = 20000;
bool switchstate;
bool switchstateup;
bool switchstatedown;
int pulsestate;
int pulseran;
int outputpwm = 0;
int outputvalue = 0;
int output = 0;

float VFinal;
float IFinal;
float RFinal;
float WUser = 0;
float IProj;
int VRaw;
int IRaw;
int voltageValue = 0;
float vout = 0.0;
float vin = 0.0;
float vRMS;
//these values are for the resistors used in voltage divider
float R1 = 100000.0;
float R2 = 10000.0;
int battery;



void setup () {
  InitTimersSafe();
  bool success = SetPinFrequencySafe(mosfetpin, frequency);
  pinMode(mosfetpin, OUTPUT);
  if (success) {
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
  }
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  pinMode(uppin, INPUT);
  pinMode(downpin, INPUT);
  pinMode(firepin, INPUT);
  pinMode(battpin, INPUT);
}

void loop () {
  vRMS = sqrt(WUser * RFinal);
  output = (vRMS / VFinal * vRMS / VFinal ) * 255;
  output = constrain(output, 0, 255);
  //readbattery raw with voltage divider to get unloaded status
  readbattery();
  drawbattery();
  switchstate = digitalRead(firepin);
  switchstateup = digitalRead(uppin);
  switchstatedown = digitalRead(downpin);

  if (switchstate == HIGH) {
    //do fire stuff hehe
    pulsecheck();
    if (pulsestate == 1)
    {
      pwmWrite(mosfetpin, output);
    }
  }

  delay(10);


  if (switchstate == LOW) {
    //do not firing stuff
    if (pulsestate)
      pwmWrite(mosfetpin, 0);
    pulseran = 0;
  }
  updowncheck();
  project();
  drawbattery();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.drawRect(0, 0, 30, 9, WHITE);
  if (battery >= 80) {
    display.fillRect(0, 0, 30, 9, WHITE);
  }
  else if (battery <= 45) {
    display.fillRect(0, 0, 20, 9, WHITE);
  }
  else if (battery <= 30) {
    display.fillRect(0, 0, 10, 9, WHITE);
  }
  else if (battery <= 10) {
    display.fillRect(0, 0, 0, 9, WHITE);
  }
  display.setCursor(32,0);
  display.print("%");
  display.setCursor(40,0);
  display.print(battery);
  display.setCursor(0, 12);
  display.print("Amp= ");
  display.setCursor(23, 12);
  display.print(IProj, 1);
  display.setCursor(0,23);
  display.print("Volt=");
  display.setCursor(32, 23);
  display.print(vRMS);
  display.setCursor(65, 0);
  display.setTextSize(2);
  display.print("W=");
  display.setCursor(88,0);
  display.print(WUser, 0);
  display.setTextSize(1);
  display.setCursor(65,18);
  display.print("R=");
  display.setCursor(77,18);
  display.print(RFinal, 2);
  display.display();


}

void pulsecheck() {
  if (pulseran == 0) {
    digitalWrite(mosfetpin, HIGH);
    delay(45);
    VRaw = analogRead(A0);
    IRaw = analogRead(A1);
    VFinal = VRaw / 12.99;
    IFinal = IRaw / 7.4;
    RFinal = VFinal / IFinal;

    if (RFinal > 0.1) {
      pulsestate = 1;
      pulseran = 1;
    }
    else if (RFinal <= 0) {
      pulsestate = 0;
      pulseran = 0;
    }
    else if (RFinal == NAN) {
      pulsestate = 0;
      pulseran = 0;
      RFinal = 0;
    }
    digitalWrite(mosfetpin, LOW);
  }
  if (pulsestate == 0) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("No Coil");
    display.setCursor(0, 10);
    display.print(pulsestate);
    display.display();
    delay(100);
    pulseran = 0;
  }
}
void updowncheck() {
  if (switchstateup == LOW)
  {
    WUser++;
    delay(25);
  }
  if (switchstatedown == LOW)
  {
    WUser--;
    delay(25);
  }
  if (WUser >= 300) WUser = 300;
  { //display max wattage eror

  }
  if (WUser <= 1) WUser = 1;
  {
    // DIsplay min wattage error
  }
}

void project() {
  IProj = sqrt(WUser / RFinal);
}

void drawbattery() {
  battery = map (vin, 0, 8.4, 0, 100);

}
void readbattery() {
  voltageValue = analogRead(battpin);
  delay(10);
  vout = (voltageValue * 5.26) / 1024.0;
  vin = vout / (R2 / (R1 + R2));
  if (vin < 0.09) {
    vin = 0.0;
  }

}

