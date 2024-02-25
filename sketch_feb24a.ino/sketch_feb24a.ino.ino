#define WATER_LEVEL         A5
#define MOISTURE_PIN        A3
#define PUMP_PIN            8

#define WATER_SENS_POWER    7
#define MOISTURE_POWER      4

#define RES_LEVEL_THRESHOLD 250
#define MOISTURE_THRESHOLD  500
#define REFILL_THRESHOLD    1000

#define BAUD_RATE           115200
#define MOISTURE_ADDR       1

#define SEND_DATA           "5"

unsigned long previousCaptureTime = millis();
//#define CAPTURE_DELAY 3600000 //Every hour
#define CAPTURE_DELAY 30000    //Every 30 seconds

#define PING_DELAY 5000
unsigned long previousPingTime = millis();

void setup() {
  pinMode(WATER_SENS_POWER,   OUTPUT);
  pinMode(MOISTURE_POWER,     OUTPUT);

  //Turn off everything
  digitalWrite(WATER_SENS_POWER,  LOW);
  digitalWrite(MOISTURE_POWER,    LOW);
  
  pinMode(WATER_LEVEL,        INPUT);
  pinMode(MOISTURE_PIN,       INPUT);
  
  //Serial
  Serial.begin(BAUD_RATE);
  Serial.setTimeout(10);
}
/*
//============================================\\

  Every X mins check moisture
  
    IF LOW:
      TURN ON PUMP
      CHECK RES LEVEL
        IF RES LEVEL LOW:
          ALERT USER
        IF RES LEVEL HIGH:
          NOTHING
          
    IF HIGH:
      NOTHING


  IF RASP PI PING ARDUINO:
    Check moisture & send results to user
        AFTER DATA SENT:
          CHECK MOISTURE LOGIC

          
\\=============================================//
*/    

bool Allow_Ping(){
  unsigned long timeNow = millis();
  if((timeNow - previousPingTime) > PING_DELAY){
    previousPingTime += PING_DELAY;
    return true;
  } else {
    return false;
  }
}

bool Time_To_Check_Moisture(){
  unsigned long timeNow = millis();
  if(timeNow - previousCaptureTime > CAPTURE_DELAY){
    previousCaptureTime += CAPTURE_DELAY;
    return true;
  } else {
    return false;
  }
}

int Capture_Moisture(){

  digitalWrite(MOISTURE_POWER, HIGH);

  delay(500);
  
  int current_moisture = analogRead(MOISTURE_PIN);

  digitalWrite(MOISTURE_POWER, LOW);

  return current_moisture;
}

//TODO: FINISH && CHANGE TO INTERRUPT
void Water_Plant(int current_moisture){
  while(current_moisture < REFILL_THRESHOLD){
    digitalWrite(PUMP_PIN, HIGH);
    delay(500);
    digitalWrite(PUMP_PIN, LOW);
    delay(3000);
  }
}

void loop() {
  int current_moisture = 0;
    
  if(Serial.available()){
      if(Allow_Ping()){
        String task = Serial.readString();
    
        //Check Moisture and send results to user
        if(task == SEND_DATA){
          current_moisture = Capture_Moisture();
          Serial.print("From user: ");
          Serial.println(current_moisture);

          //Start pump?
          if(current_moisture < MOISTURE_THRESHOLD){
              Water_Plant(current_moisture);

              //Check pump levels
          }
        }  
     }
  }

  if(Time_To_Check_Moisture()){
    current_moisture = Capture_Moisture();
    Serial.print("TIME TO CHECK: ");
    Serial.println(current_moisture);  

    //Start pump?
    if(current_moisture < MOISTURE_THRESHOLD){
      Water_Plant(current_moisture);
      
      //Check pump levels
    }
  }
}
