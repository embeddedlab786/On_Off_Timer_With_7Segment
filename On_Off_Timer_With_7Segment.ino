#include <EEPROM.h>  

#define data_pin  6
#define clock_pin 7

#define bt_set  A0
#define bt_up   A1
#define bt_down A2
#define bt_stop A3

#define Relay  8
#define buzzer 13

const int digitToSegment[] = {
 // XGFEDCBA
  0b00111111,    // 0
  0b00000110,    // 1
  0b01011011,    // 2
  0b01001111,    // 3
  0b01100110,    // 4
  0b01101101,    // 5
  0b01111101,    // 6
  0b00000111,    // 7
  0b01111111,    // 8
  0b01101111,    // 9
  0b01110111,    // A
  0b01111100,    // b
  0b01110001,    // F
  0b01010100     // n
};
                           // 12%,  25%,  38%,  50%,  63%,  75%,  88%, 100% 
const int Brightness[] = { 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f};

int Set_mode = 1; //<Set_mode=0; for 00:00 HH:MM> <Set_mode=1; for 00:00 MM:SS>

int hh=0, mm=0, ss=0;
int on_t1, on_t2;
int of_t1, of_t2;

word MilliSecond = 0; 
bool timerStart = false;

int flag1=0, flag2=0; 
int Set=0, Load=0, Mode=0;

void setup(){ // put your setup code here, to run once  

pinMode(bt_set, INPUT_PULLUP); 
pinMode(bt_up,  INPUT_PULLUP);
pinMode(bt_down,INPUT_PULLUP);
pinMode(bt_stop,INPUT_PULLUP);

pinMode(Relay, OUTPUT); digitalWrite(Relay, HIGH);

pinMode(buzzer, OUTPUT);

pinMode(clock_pin, OUTPUT);
pinMode(data_pin, OUTPUT);

start();
writeValue(Brightness[6]); // set Brightness 0 to 7
stop();

write_data(0x00, 0x00, 0x00, 0x00); // clear display

//Indicate that system is ready
for (int i = 9; i >=0; i--) {
write_data(digitToSegment[i%10], digitToSegment[i%10], digitToSegment[i%10], digitToSegment[i%10]);
 delay(1000); 
}
 
 noInterrupts();         // disable all interrupts
 TCCR1A = 0;             // set entire TCCR1A register to 0  //set timer1 interrupt at 1kHz  // 1 ms
 TCCR1B = 0;             // same for TCCR1B
 TCNT1  = 0;             // set timer count for 1khz increments
 OCR1A = 1999;           // = (16*10^6) / (1000*8) - 1
 //had to use 16 bit timer1 for this bc 1999>255, but could switch to timers 0 or 2 with larger prescaler
 // turn on CTC mode
 TCCR1B |= (1 << WGM12); // Set CS11 bit for 8 prescaler
 TCCR1B |= (1 << CS11);  // enable timer compare interrupt
 TIMSK1 |= (1 << OCIE1A);
 interrupts();           // enable

if(EEPROM.read(0)==0){}
else{
EEPROM.write(0, 0);  
EEPROM.write(1, 1);
EEPROM.write(11, 1);
EEPROM.write(12, 1);
EEPROM.write(21, 1);
EEPROM.write(22, 1);
}
delay(100); 
read_eeprom();
if(Mode==1){
mm=on_t1, hh=on_t2;  
timerStart = true; 
Load =0;  
}

}

void loop(){  

if(digitalRead (bt_set) == 0){ 
if(flag1==0 && Mode==0){ flag1=1;
Set = Set+1;

if(Set==1){
write_data(digitToSegment[10], 0x00, digitToSegment[0], digitToSegment[13]);
mm=on_t1,hh=on_t2;
delay(500); 
}

if(Set==3){
EEPROM.write(11, mm);
EEPROM.write(12, hh);
write_data(digitToSegment[11], 0x00, digitToSegment[0], digitToSegment[12]);
mm=of_t1,hh=of_t2;
delay(500); 
}

if(Set>4){Set=0;
EEPROM.write(21, mm);
EEPROM.write(22, hh);
write_data(digitToSegment[5], digitToSegment[13], digitToSegment[5], digitToSegment[12]);
read_eeprom();
mm=on_t1, hh=on_t2; 
delay(500); 
} 

 digitalWrite(buzzer, HIGH);
 delay(500);  
 }
}else{flag1=0;}

if(digitalRead (bt_up) == 0){
digitalWrite(buzzer, HIGH);
if(Set==1 || Set==3){mm = mm+1;}
if(Set==2 || Set==4){hh = hh+1;}
if(mm>59){mm=0;}
if(hh>99){hh=0;}
delay(200); 
}

if(digitalRead (bt_down) == 0){
digitalWrite(buzzer, HIGH);
if(Set==0){
mm=on_t1, hh=on_t2;  
timerStart = true; 
Mode=1; Load =0;
EEPROM.write(1, Mode);
}
if(Set==1 || Set==3){mm = mm-1;}
if(Set==2 || Set==4){hh = hh-1;}
if(mm<0){mm=59;}
if(hh<0){hh=99;}
delay(200); 
}

if(digitalRead (bt_stop) == 0){
if(flag2==0){ flag2=1;
digitalWrite(buzzer, HIGH);
Mode = !Mode;
if(Mode==1){timerStart = true;}
       else{timerStart = false;}
MilliSecond=0;
EEPROM.write(1, Mode);
delay(200); 
}
}else{flag2=0;}

if(hh==0 && mm==0 && Mode==1){ 
Load = !Load;
if(Load==0){mm=on_t1, hh=on_t2;}
       else{mm=of_t1, hh=of_t2;}
digitalWrite(buzzer, HIGH);
delay(200); 
}  

if(Set==0){
if(MilliSecond<500){
write_data(digitToSegment[(hh/10)%10], digitToSegment[hh%10] | 0x80, digitToSegment[(mm/10)%10], digitToSegment[mm%10] | 0x80);}
else{
write_data(digitToSegment[(hh/10)%10], digitToSegment[hh%10], digitToSegment[(mm/10)%10], digitToSegment[mm%10]);}
}

if(Set==1 || Set==3){write_data(0x00, 0x00, digitToSegment[(mm/10)%10], digitToSegment[mm%10]);}
if(Set==2 || Set==4){write_data(digitToSegment[(hh/10)%10], digitToSegment[hh%10], 0x00, 0x00);}

if(Mode==1){digitalWrite(Relay, Load);}
       else{digitalWrite(Relay, HIGH);}
delay(50); 
digitalWrite(buzzer, LOW);
}

void read_eeprom(){
Mode = EEPROM.read(1);   

on_t1 = EEPROM.read(11);
on_t2 = EEPROM.read(12);
 
of_t1 = EEPROM.read(21);
of_t2 = EEPROM.read(22); 
}


ISR(TIMER1_COMPA_vect){   
if(timerStart == true){MilliSecond++;
    if(MilliSecond >= 1000){MilliSecond = 0;
       if(Set_mode==0){ss--;
       if(ss<0){ss=59; mm--;}
       }else{mm--;}
       if(mm<0){mm=59; hh--;}
    }
  }  
}

void write_data(uint8_t dig1, uint8_t dig2, uint8_t dig3, uint8_t dig4){
  start();
  writeValue(0x40);
  stop();

  start();
  writeValue(0xc0);
  writeValue(dig1);
  writeValue(dig2);
  writeValue(dig3);
  writeValue(dig4);
  stop();
}

void start(void){
  digitalWrite(clock_pin,HIGH);//send start signal to TM1637
  digitalWrite(data_pin,HIGH);
  delayMicroseconds(5);

  digitalWrite(data_pin,LOW);
  digitalWrite(clock_pin,LOW);
  delayMicroseconds(5);
}

void stop(void){
  digitalWrite(clock_pin,LOW);
  digitalWrite(data_pin,LOW);
  delayMicroseconds(5);

  digitalWrite(clock_pin,HIGH);
  digitalWrite(data_pin,HIGH);
  delayMicroseconds(5);
}

bool writeValue(uint8_t value){
  for(uint8_t i = 0; i < 8; i++){
    digitalWrite(clock_pin, LOW);
    delayMicroseconds(5);
    digitalWrite(data_pin, (value & (1 << i)) >> i);
    delayMicroseconds(5);
    digitalWrite(clock_pin, HIGH);
    delayMicroseconds(5);
  }

  // wait for ACK
  digitalWrite(clock_pin,LOW);
  delayMicroseconds(5);
  pinMode(data_pin,INPUT);
  digitalWrite(clock_pin,HIGH);
  delayMicroseconds(5);

  bool ack = digitalRead(data_pin) == 0;
  pinMode(data_pin,OUTPUT);
  return ack;
}
