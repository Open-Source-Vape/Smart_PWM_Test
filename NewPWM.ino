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


int32_t frequency = 10000;
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
float WFinal;
float WUser = 0;
int VRaw;
int IRaw;
int voltageValue = 0;
float vout = 0.0;
float vin = 0.0;
float R1 = 100000.0;
float R2 = 10000.0;
float vRMS;


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
  output = (vRMS/VFinal * vRMS/VFinal ) * 255;
    if (output >= 255){
      output = 255;
    }
  //readbattery raw with voltage divider to get unloaded status
  voltageValue = analogRead(battpin);
  vout = (voltageValue * 5.26) / 1024.0;
  vin = vout / (R2 / (R1 + R2));

  switchstate = digitalRead(firepin);
  switchstateup = digitalRead(uppin);
  switchstatedown = digitalRead(downpin);
  
  if (switchstate == HIGH) {
    //do fire stuff
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
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Vout=");
  display.setCursor(30, 0);
  display.print(VFinal);
  display.setCursor(0, 9);
  display.print("Amps=");
  display.setCursor(30, 9);
  display.print(IFinal);
  display.setCursor(0, 18);
  display.print("Watt=");
  display.setCursor(30, 18);
  display.print(WUser, 1);
  display.setCursor(0, 27);
  display.print(vin);
  display.setCursor(70, 0);
  display.print("Fire =");
  display.setCursor(105, 0);
  display.print(switchstate);
  display.setCursor(70, 9);
  display.print(RFinal, 2);
  display.setCursor(70, 18);
  display.print(output);
  display.setCursor(70, 27);
  display.print(VFinal);
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
    WFinal = VFinal * IFinal;

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

