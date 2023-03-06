// Author: Bruno Guerreiro (bj.guerreiro@fct.unl.pt)
// Context: This work is part of a course on Introcuction of Electrical and Computer Engineering, at 
// DEEC - NOVA School of Science and Technology, where students are learning for the first time the use of 
// feedback dynamic systems, in this case, the difference of a well tuned controller, with no overshoot 
// (when A0 is low), or with overshoot (when A0 is maximum).
// 
// Acknowledgement: based on https://github.com/curiores/ArduinoTutorials/tree/main/SpeedControl

#include <util/atomic.h> // For the ATOMIC_BLOCK macro

#define POTA A0 // potenciometer
#define ENCA 2 // YELLOW
#define ENCB 3 // WHITE
#define PWM 5 // ORANGE
#define INDIR 6 // BROWN
//#define INDIR2 7 // TURQUOISE (only on Tinkercad)

volatile int posi = 0; // specify posi as volatile: https://www.arduino.cc/reference/en/language/variables/variable-scope-qualifiers/volatile/
long prevT = 0;
float eprev = 0;
float eintegral = 0;

void setup() {
  Serial.begin(9600);

  pinMode(POTA,INPUT);
  pinMode(ENCA,INPUT);
  pinMode(ENCB,INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCA),readEncoder,RISING);
  
  pinMode(PWM,OUTPUT);
  pinMode(INDIR,OUTPUT);
  
  Serial.println("target, pos, ctrl");
}

void readEncoder(){
  int b = digitalRead(ENCB);
  if(b > 0){
    posi++;
  }
  else{
    posi--;
  }
}

void setMotor(int u_sat){
  bool CCW_dir = (u_sat>=0)?HIGH:LOW;
  float pwmVal = min(abs(u_sat),255);
  analogWrite(PWM,pwmVal);
  digitalWrite(INDIR,!CCW_dir);
}

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void loop() {

  // Reference: set sinusoid/square wave as target position reference
  int MaxDeg = 250;
  int target = MaxDeg*( (sin(prevT/1e6) > 0) ? 1:-1);
  // int target = 1200; // fixed position alternative
  
  // PID gains
  float kp = 5;
  float kd = 0;//.025;
  float ki = 0.0;
  
  // set k and kd gains proportionally to A0 input
  int potA = analogRead(A0);
  kp = fmap(potA, -1023, 1024, 0.5, 25);
  kd = fmap(potA, -1023, 1024, 0.5, 0);

  // time difference
  long currT = micros();
  float deltaT = ((float) (currT - prevT))/( 1.0e6 );
  prevT = currT;

  // Read the position in an atomic block to avoid a potential
  // misread if the interrupt coincides with this code running
  // see: https://www.arduino.cc/reference/en/language/variables/variable-scope-qualifiers/volatile/
  int pos = 0; 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    pos = posi;
  }
  
  // error
  int e = pos - target;

  // derivative
  float dedt = (e-eprev)/(deltaT);

  // integral
  eintegral = eintegral + e*deltaT;

  // control signal
  float u = kp*e + kd*dedt + ki*eintegral;
  int u_sat = map(max(min(u,1024),-1024), -1024, 1024, -255, 255);

  // signal the motor
  setMotor(u_sat);

  // store previous error
  eprev = e;

  Serial.print(target);
  Serial.print(", ");
  Serial.print(pos);
  Serial.print(", ");
  Serial.print(u_sat);
//  Serial.print(", ");
//  Serial.print(kd);
//  Serial.print(", ");
//  Serial.print(e);
//  Serial.print(", ");
//  Serial.print(dedt);
//  Serial.print(", ");
//  Serial.print(eintegral);
  Serial.println();

  delay(10);
}
