/*
   Components used:
   https://www.adafruit.com/products/931
   https://www.sparkfun.com/products/9028
   https://www.sparkfun.com/products/8374
   https://www.sparkfun.com/products/13760
   https://www.sparkfun.com/products/97
   http://www.infineon.com/cms/en/product/power/power-mosfet/20v-300v-n-channel-power-mosfet/20v-30v-n-channel-power-mosfet/IRLS3813/productType.html?productType=5546d462533600a401533d4c1e1f77fa
   https://www.radioshack.com/products/radioshack-220-ohm-1-4w-5-carbon-film-resistor-pk-5
   place 220 ohm resistor between MCU pin 7 and gate on mosfet
*/

#include <Button.h>
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

#define uppin 12
#define downpin 11
#define firepin 7
#define battpin A2
#define mosfetpin 3

int32_t frequency = 10000;
int switchstate;
bool switchstateup;
bool switchstatedown;
bool powerlock = 0;

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
enum {twoS, threeS, fourS};

byte batt_type;

long millis_held;
long prev_secs_held;
long secs_held;
byte previous = LOW;
byte previousup = LOW;
byte previousdown = LOW;
unsigned long firsttime;

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
  pinMode(uppin, INPUT_PULLUP);
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

  if (switchstate == HIGH && previous == LOW && (millis() - firsttime) > 200) {
    firsttime = millis();
  }

  if (switchstate == HIGH) {
    millis_held = (millis() - firsttime);
    secs_held = millis_held / 100;
    if (secs_held >= 2) {
      //do fire stuff hehe
      pulsecheck();
      if (pulsestate == 1)
      {
        pwmWrite(mosfetpin, output);
      }
    }
  }
  delay(10);
  previous = switchstate;

  if (switchstate == LOW) {
    //do not firing stuff
    if (pulsestate)
      pwmWrite(mosfetpin, 0);
    pulseran = 0;
    millis_held = 0;
    secs_held = 0;
  }

  updowncheck();
  project();


  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.drawRect(0, 0, 30, 9, WHITE);
  drawbattery();
  display.setCursor(32, 0);
  display.print("%");
  display.setCursor(40, 0);
  display.print(battery);
  display.setCursor(0, 12);
  display.print("Amp= ");
  display.setCursor(23, 12);
  display.print(IProj, 1);
  display.setCursor(0, 23);
  display.print("Volt=");
  display.setCursor(32, 23);
  display.print(vRMS);
  display.setCursor(65, 0);
  display.setTextSize(2);
  display.print("W=");
  display.setCursor(88, 0);
  display.print(WUser, 0);
  display.setTextSize(1);
  display.setCursor(65, 15);
  display.print("R=");
  display.setCursor(77, 15);
  display.print(RFinal, 2);
  display.setCursor(65, 23);
  display.print("Duty=");
  display.setCursor(95, 23);
  display.print(secs_held);
  display.display();


}

void pulsecheck() {
  if (pulseran == 0) {
    digitalWrite(mosfetpin, HIGH);
    delay(10);
    VRaw = analogRead(A0);
    IRaw = analogRead(A1);
    delay(20);
    VFinal = VRaw / 12.99;
    IFinal = IRaw / 7.4;
    RFinal = VFinal / IFinal - .26; //the .26 may not be needed i think it is the resistance of the board itself though or the overall circuit

    if (RFinal > 0.25) {
      pulsestate = 1;
      pulseran = 1;
    }
    if (RFinal >= 0.1 | RFinal <= .23 | VFinal > 10) {
      //low resistance message
      pulsestate = 0;
      pulseran = 0 ;
      RFinal = 0;
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
    if (IProj == NAN) {
      IProj = 0;
    }
    if (IProj == INFINITY) {
      IProj = 0;
    }
    if (vRMS == NAN) {
      vRMS = 0;
    }
    if (RFinal == NAN) {
      RFinal == 0;
    }
  }
}
void updowncheck() {
  if (powerlock == 0) {
    if (switchstateup == HIGH && previousup == LOW && (millis() - firsttime) > 200) {
      firsttime = millis();
    }

    if (switchstateup == HIGH) {
      millis_held = (millis() - firsttime);
      secs_held = millis_held / 100;
      if (secs_held >= 4) {
        WUser = WUser+5;
      }
      if (secs_held <= 3) {
        WUser ++;
        delay(100);
      }
    }
    switch (batt_type) {
      case twoS:
        constrain_2s();
        break;
      case threeS:
        constrain_3s();
        break;
      case fourS:
        constrain_4s();
        break;
      default:
        constrain_2s();
        break;
    }

    previousup = switchstateup;

    if (switchstatedown == HIGH && previousdown == LOW && (millis() - firsttime) > 200) {
      firsttime = millis();
    }

    if (switchstatedown == HIGH) {
      millis_held = (millis() - firsttime);
      secs_held = millis_held / 100;
      if (secs_held >= 4) {
        WUser = WUser -5;
      }
      if (secs_held <= 3) {
        WUser --;
        delay(100);
      }
    }
    previousdown = switchstatedown;

    if (switchstatedown == HIGH)
    {
      WUser--;
      delay(25);
    }
    if (WUser <= 1) WUser = 1;
    {
      // DIsplay min wattage error
    }
  }
  switch (switchstateup == LOW && switchstatedown == LOW) {
      if (powerlock == 1) {
        powerlock = 0;
      }
      if (powerlock == 0) {
        powerlock = 1;
      }
  }
}

void project() {
  IProj = sqrt(WUser / RFinal);
}

void drawbattery() {
  battery = map (vin, 0, 8.4, 0, 100);
  switch (battery) {
    case 100:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 99:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 98:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 97:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 96:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 95:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 94:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 93:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 92:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 91:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 90:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 89:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 88:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 87:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 86:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 85:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 84:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 83:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 82:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 81:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 80:
      display.fillRect(0, 0, 30, 9, WHITE);
      break;
    case 79:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 77:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 76:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 75:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 74:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 73:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 72:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 71:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 70:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 69:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 68:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 67:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 66:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 65:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 64:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 63:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 62:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 61:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 60:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 59:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 58:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 57:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 56:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 55:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 54:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 53:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 52:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 51:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 50:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 49:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 48:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 47:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 46:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 45:
      display.fillRect(0, 0, 20, 9, WHITE);
      break;
    case 44:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 43:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 42:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 41:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 40:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 39:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 38:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 37:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 36:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 35:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 34:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 33:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 32:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 31:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 30:
      display.fillRect(0, 0, 10, 9, WHITE);
      break;
    case 29:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 28:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 27:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 26:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 25:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 24:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 23:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 22:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 21:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 20:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 19:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 18:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 17:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 16:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 15:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 14:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 13:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 12:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 11:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
    case 10:
      display.fillRect(0, 0, 0, 9, WHITE);
      break;
  }

}
void readbattery() {
  voltageValue = analogRead(battpin);
  vout = (voltageValue * 5.26) / 1024.0;
  vin = vout / (R2 / (R1 + R2));
  if (vin < 0.09) {
    vin = 0.0;
  }
  if (vin >= 6.8 & vin <= 8.8) {
    batt_type = twoS;
  }
  if (vin >= 10.5 & vin <= 13.2) {
    batt_type = threeS;
  }
  if (vin >= 14 & vin <= 17.6) {
    batt_type = fourS;
  }

}

void constrain_2s() {
  if (WUser >= 250) WUser = 250;
  { //display max wattage eror

  }
}

void constrain_3s() {
  if (WUser >= 375) WUser = 375;
  { //display max wattage eror

  }
}

void constrain_4s() {
  if (WUser >= 500) WUser = 500;
  { //display max wattage eror

  }
}

