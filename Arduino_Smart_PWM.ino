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


int32_t frequency = 120;
bool switchstate;
bool switchstateup;
bool switchstatedown;
int pulsestate;
int pulseran;
int outputpwm = 0;
int outputvalue = 0;
int outputbutton = 0;

float VFinal;
float IFinal;
float RFinal;
float WFinal;
byte WUser = 0;
int VRaw;
int IRaw;
int voltageValue = 0;
float vout = 0.0;
float vin = 0.0;
float R1 = 100000.0; 
float R2 = 10000.0; 

void setup () {
  InitTimersSafe();
  bool success = SetPinFrequencySafe(mosfetpin, frequency);
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
  //only use these for analog pot
  outputbutton = map(WUser, 0, 100, 0, 255);
  //clean up this for VV/VW later
  voltageValue=analogRead(battpin);
  vout=(voltageValue*5.26)/1024.0;
  vin = vout/ (R2/(R1+R2));
  
  switchstate = digitalRead(firepin);
  switchstateup = digitalRead(uppin);
  switchstatedown = digitalRead(downpin);

  if (switchstate == HIGH) {
    //do fire stuff
    if (pulseran == 0) {
      digitalWrite(mosfetpin, HIGH);
      delay(20);
      VRaw = analogRead(A0);
      IRaw = analogRead(A1);
      // these numbers are not valid ... yet...
      //1s input
      //VFinal = VRaw / 61.06;
      //IFinal = IRaw / 23.37165;
      //2s or greater ==
      VFinal = VRaw / 42.03;
      IFinal = IRaw / 16.99;
      
      RFinal = VFinal / IFinal;
      WFinal = VFinal * IFinal;
      
      digitalWrite(mosfetpin, LOW);

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
    if (pulsestate == 1)
    {
      pwmWrite(mosfetpin, outputbutton);
    }
  }

  
  
  delay(10);

  
  if (switchstate == LOW) {
    //do not firing stuff
    if (pulsestate)
    pwmWrite(mosfetpin,0);
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
  display.print("Duty=");
  display.setCursor(30, 18);
  display.print(WUser, 1);
  display.setCursor(0,27);
  display.print(vin);
  display.setCursor(70, 0);
  display.print("Fire =");
  display.setCursor(105, 0);
  display.print(switchstate);
  display.setCursor(70, 9);
  display.print(RFinal, 2);
  display.setCursor(70, 18);
  display.print(outputbutton);
  display.setCursor(70, 27);
  display.print(VRaw);
  display.setCursor(100, 18);
  display.print(pulsestate);
  display.setCursor(100, 27);
  display.print(pulseran);
  display.display();


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
  if (WUser >= 100) WUser = 100;
  { //display max wattage eror

  }
  if (WUser <= 1) WUser = 1;
  {
    // DIsplay min wattage error
  }
}

