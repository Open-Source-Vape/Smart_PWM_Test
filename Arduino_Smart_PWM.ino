//pieced together and coded by the Zanderist(AWA)
// display and smart pot added by zarboz
//Display is Adafruit SSD1306 128x32 i2c display 
// voltage/amp detector is a TI-INA-169. 
/*Todo: 
 * add boolean state to  tell program wether screen is "active or not"
 * add intelligent or user defined battery states IE 1s/2s/3s/4s etc etc
 * Add boolean that monitors battery charge while screen is active
 * once checking battery charge against user setting/auto setting evaluate if battery charge is sufficient to continue
 * POSSIBLY: monitor under load and show average % of sag on source voltage
 */
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <avr/eeprom.h>


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

int potpin = A3;
int battpin = A2;
int firepin = 7;
int Vraw = A0;
int IRaw = A1;
int mosfetpin = 3;


int switchstate = 0;
int mode = 0;
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


float Rratio=0.4;
float vout=0;
float vin=0;
float VFinal;
float IFinal;

struct resistance_t
{
  long alarm;
  int mode;
} resistance;

void setup() {
  Serial.begin(9600);
  pinMode(firepin, INPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
   display.display();
   display.clearDisplay();
   eeprom_read_block((void*)&resistance, (void*)0, sizeof(resistance));
}

void loop () {

  potvalue = analogRead(potpin);
  outputvalue = map(potvalue, 0, 1023, 0, 255);
  voltagevalue = analogRead(battpin);
  vout = (voltagevalue*4.68)/1024.0;
  vin = vout/Rratio;

  switchstate = digitalRead(firepin);
  if(readenable==1){
  outputpwm=outputvalue;
  if(vin>7.51){mode=2;}
  if(6.99<=vin&&vin<=7.50){mode=1;}
  if(vin<6.98){mode=0;}
  }
  
  
  if(switchstate==1){
   readenable=0;
    if(mode==2){
      
    bathighvoltage();}
    
   if(mode==1){
    
    batmediumvoltage();}
  
  
   if(mode==0) {
    
    batlowvoltage(); }
  }

  
  if(switchstate==0){
    readenable=1;
    samplepwm=1;
    analogWrite(mosfetpin, 0);
    analogRead(Vraw);
    analogRead(IRaw);
    VFinal = Vraw/12.99; 
    IFinal = IRaw/7.4;
    //insert calculation of resistance of current build
    //match it against last known resistance
    //if different ask if new atomizer
    //if same or within 10% cold ohm reading continue without prompt
    // either way write new resistance to eeprom
    //eeprom_read_block((void*)&resistance, (void*)0, sizeof(resistance));
    //eeprom_write_block((const void*)&resistance, (void*)0, sizeof(resistance));
    
    //insert prompt for new coil TODO LATER
    done = 0;
  }

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(readenable);
  display.setCursor(1,0);
  display.print(potvalue);
  display.setCursor(0,5);
  display.print(outputvalue);
  display.setCursor(1,5);
  display.print(vin);
  display.setCursor(0,10);
  display.println(VFinal);
  
}



void bathighvoltage(){
  analogWrite(mosfetpin, outputpwm);
  
}
/////////////////////////////Todo add a screen reaction for overvoltage\
void batmediumvoltage(){
  if(samplepwm==1){heatpwm=outputvalue;samplepwm=0;}
  if(done==0){
            if(heatpwm>0){
             heatpwm=heatpwm+heatpwminc;
                        }
    if(heatpwm<=0){analogWrite(mosfetpin,0);}
                }
}

/////////////////////////////Todo add screen reaction for medium voltage\
void batlowvoltage(){
}
///////////////////////////// Todo add screeen reaction for low battery and throttle output after a cutoff level
