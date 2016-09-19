//pieced together and coded by the Zanderist(AWA)
// display and smart pot added by zarboz
//Display is Adafruit SSD1306 128x32 i2c display
// voltage/amp detector is a TI-INA-169. http://cdn.sparkfun.com/datasheets/Sensors/Current/DC%20Voltage%20and%20Current%20Sense%20PCB%20with%20Analog%20Output.pdf
// used 50v/90a model
/*Todo:
 * add boolean state to  tell program wether screen is "active or not"
 * add intelligent or user defined battery states IE 1s/2s/3s/4s etc etc
 * Add boolean that monitors battery charge while screen is active
 * once checking battery charge against user setting/auto setting evaluate if battery charge is sufficient to continue
 * POSSIBLY: monitor under load and show average % of sag on source voltage
 * Add two loops running for wattage Loop 1 will check live battery source vs percent of pot open using this it will then calculate a projected wattage off last known resistance stored in eeprom
 * second loop will run only while firing and will show live wattage calc next to projected or overlaying projected wattage calculations
 */
#include <PWM.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <avr/eeprom.h>



#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

int potpin = A3;
int battpin = A2;
int firepin = 7;
int mosfetpin = 3;

int VRaw;
int IRaw;
int switchstate = 0;
int mode = 2;
int levelshutdown = 0;
int readenable = 1;
int potvalue = 0;
int outputvalue = 0;
int heatpwm = 0;
int heatpwminc = -5;
int samplepwm = 0;
int outputpwm = 0;
int voltagevalue = 0;
int voutputvoltage = 0;
int done = 0;
int32_t frequency = 20000;

float Rratio = 0.4;
float vout = 0;
float vin = 0;
float VFinal;
float IFinal;
float RFinal;
float WFinal;

boolean pulse = 0;

void setup() {
  InitTimersSafe();
  bool success = SetPinFrequencySafe(mosfetpin, frequency);
  Serial.begin(9600);
  pinMode(firepin, INPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("C9 Vape");
  display.display();
  display.clearDisplay();
  
}

void loop () {
  //pulsecheck();

  potvalue = analogRead(potpin);
  outputvalue = map(potvalue, 0, 1023, 0, 255);
  voltagevalue = analogRead(battpin);
  //vout will become a NULL when the VFinal is impimented as it will be a real life output
  vout = (voltagevalue * 1.56) / 1024.0;
  vin = vout / Rratio;

  switchstate = digitalRead(firepin);

  if (readenable == 1) {
    outputpwm = outputvalue;
    //add code to determine if battery is 1s/2s/3s/4s etc
    /*if(vin>7.51){mode=2;}
    if(6.99<=vin&&vin<=7.50){mode=1;}
    if(vin<6.98){mode=0;}*/
  }

  getswitchstate();
 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Vout=");
  display.setCursor(30, 0);
  display.print(VFinal);
  display.setCursor(0, 9);
  display.print("Amp=");
  display.setCursor(30, 9);
  display.print(IFinal, 2);
  display.setCursor(0, 18);
  display.print("Watt=");
  display.setCursor(30, 18);
  display.print(WFinal, 1);
  display.setCursor(70, 0);
  display.print("Fire =");
  display.setCursor(105, 0);
  display.print(switchstate);
  display.setCursor(70, 9);
  display.print(RFinal, 2);
  display.setCursor(70, 18);
  display.print(pulse);
  display.display();

  Serial.print(VFinal);
  Serial.print("   Volts");

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void getswitchstate() {
   if (switchstate == HIGH) {
    readenable = 0;
    if (mode == 2) {

      batmedvoltage();
    }

    if (mode == 1) {

      powersaver();
    }


    if (mode == 0) {

      batlowvoltage();
    }
  }


  if (switchstate == LOW) {
    readenable = 1;
    samplepwm = 1;
    pwmWrite(mosfetpin, 0);
    //VFinal = 0;
    IFinal = 0;
    WFinal = 0;
    pulse = 0;
    done = 0;
  }
  
}
void batmedvoltage() {
  pulsecheck();
  if (pulse == 1) {
    pwmWrite(mosfetpin, outputpwm);

  }
  else if (pulse == 0)
  {
    pwmWrite(mosfetpin, 0);
  }

}
/////////////////////////////Todo add a screen reaction for overvoltage\


void powersaver() {
  pulsecheck();
  if (pulse == 1) {
    if (samplepwm == 1) {
      heatpwm = outputvalue;
      samplepwm = 0;
    }
    if (done == 0) {
      if (heatpwm > 0) {
        heatpwm = heatpwm + heatpwminc;
      }
      if (heatpwm <= 0) {

        pwmWrite(mosfetpin, 0);

      }
    }
  }
  else if (pulse == 0) {

  }
}

/////////////////////////////Todo add screen reaction for medium voltage\
void batlowvoltage() {
}
///////////////////////////// Todo add screeen reaction for low battery and throttle output after a cutoff level

void pulsecheck() {
  pwmWrite(mosfetpin, 255);
  VRaw = analogRead(A0);
  IRaw = analogRead(A1);

  //these are for 1s 3.3v supplied boards use normal values for 5v or 2s/3s/4s configuration

  VFinal = VRaw / 57.75606;
  IFinal = IRaw / 22.22006;
  /*please uncomment these values and comment the ones above if you use a 2s->greater input
   * 
   * VFinal = VRaw/12.99;
   * IFinal = IRaw/7.4;
   * 
   */
  RFinal = VFinal / IFinal;
  WFinal = VFinal * IFinal;
  delay(10);
  if (RFinal > .01) {
    pulse = 1;
    pwmWrite(mosfetpin, 0);
  }
  else if (RFinal <= 0) {
    pulse = 0;
    noresistance();
    pwmWrite(mosfetpin, 0);
  }
  else if (RFinal == NAN) {
    pulse = 0;
    noresistance();
    pwmWrite(mosfetpin, 0);
  }
   
   
}

void noresistance() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("No Coil");
  display.display();
  delay(200);
}

