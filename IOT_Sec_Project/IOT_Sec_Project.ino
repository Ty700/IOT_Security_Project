//DATA LINES
#define WATER_LEVEL         A5
#define MOISTURE_PIN        A3

//POWER LINES
#define WATER_SENS_POWER    7
#define MOISTURE_POWER      4
#define PUMP_PIN            8

//USER DEFINED THRESHOLDS
#define RES_LEVEL_THRESHOLD 250
#define MOISTURE_THRESHOLD  500

//MOISTURE PERCENTAGE THRESHOLDS
#define MOISTURE_MAX 1019
#define MOISTURE_MIN 250

//SERIAL SETUP
#define BAUD_RATE           115200
#define SEND_DATA           "5"

//MOISTURE CAPTURE DELAY TIMEOUT/SETUP
unsigned long previousCaptureTime = millis();
#define CAPTURE_DELAY 10000//Every 10 seconds

//PING TIMEOUT SETUP
#define PING_DELAY 5000
unsigned long previousPingTime = millis();

void setup() {
  //PIN MODES

  //OUTPUTS
  pinMode(WATER_SENS_POWER,   OUTPUT);
  pinMode(MOISTURE_POWER,     OUTPUT);
  pinMode(PUMP_PIN,           OUTPUT);

  //INPUTS
  pinMode(WATER_LEVEL,        INPUT);
  pinMode(MOISTURE_PIN,       INPUT);

  //DEFAULTS
  digitalWrite(WATER_SENS_POWER,  LOW);
  digitalWrite(MOISTURE_POWER,    LOW);
  digitalWrite(PUMP_PIN,          LOW);
  
  //Serial SETUP
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

/* 
FUNCTION: 
* To avoid constant pinging that could destroy logic I implemented a ping timeout.
* User can only ping the arduino every PING_DELAY milliseconds.
* 
RETURN VALUES:
  - True  =>   If PING_DELAY milliseconds HAS passed since last ping.
  - False =>   If PING_DELAY milliseconds HAS NOT passed since last ping.
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


/* 
FUNCTION: 
* This is a check to see if CAPTURE_DELAY milliseconds has passed.
* If so the system will check the moisture.
*
RETURN VALUES:
  - True  => CAPTURE_DELAY milliseconds HAS passed
  - False => CAPTURE_DELAY milliseconds HAS NOT passed
*/ 
bool Time_To_Check_Moisture(){
  unsigned long timeNow = millis();
  if(timeNow - previousCaptureTime > CAPTURE_DELAY){
    previousCaptureTime += CAPTURE_DELAY;
    return true;
  } else {
    return false;
  }
}


/* 
FUNCTION: 
* Will read the current moisture level and return it.

RETURN VALUES:
  0 through 1024 based on moisture.
    - 0    => moist
    - 1024 => dry
*/ 
int Capture_Moisture(){

  digitalWrite(MOISTURE_POWER, HIGH);

  delay(500);
  
  int current_moisture = analogRead(MOISTURE_PIN);

  digitalWrite(MOISTURE_POWER, LOW);
  
  return current_moisture;
}

/* 
FUNCTION: 
* Will check the pump res level.

RETURN VALUES:
  False - If pump levels is greater than the refill threshold
  True - If pump levels are less than refill threshold

TODO: 
* CHANGE TO INTERRUPT
* If this is executing and the user pings to read moisture, it will miss the request since this
* (and every other delay) halts code execution. Again, super inefficent, maybe I will change it later.
*/ 
 
bool Res_Levels_Low(){
  bool status = false;

  //Turn sensor on
  digitalWrite(WATER_SENS_POWER, HIGH);
  
  //Delay of a second <= Turn this to an interrupt somehow so we don't stop running
  delay(1000);

  //Reads Water level sensor and compares it to the threshold we set
  if(analogRead(WATER_LEVEL) < RES_LEVEL_THRESHOLD){
    //If lower than threshold we have to refill 
    status = true;
  } 

  //Turns sensor off
  digitalWrite(WATER_SENS_POWER, LOW);

  return status;
  
}

/* 
FUNCTION: 
* Will constantly refill until moisture is above a threshold
* Refills for a half second interval
* Waits for water to drain to the bottom
* Reads it again, and determines if soil needs more water ot not
* 
RETURN VALUES:
  Void

TODO: 
* CHANGE TO INTERRUPT
* If this is executing and the user pings to read moisture, it will miss the request since this
* (and every other delay) halts code execution. Again, super inefficent, maybe I will change it later.
*/ 

void Water_Plant(){
  int current_moisture = 1024;
  bool res_levels_low = false;
  Serial.println("Starting pump");
  
  do {
    current_moisture = Capture_Moisture();
    res_levels_low = Res_Levels_Low();

    if(res_levels_low){
      Serial.println("WATERING RESULT: FAILED");
      Serial.println("    REASON: PUMP LEVELS TOO LOW.");

      //Fixes the spam of moisture checks if logic gets stuck in the do-while loop
      previousCaptureTime = millis();
      return;
    }

//###################DEBUG######################\\
//    Serial.print("Moisture Level: ");
//    Serial.println(current_moisture);
//
//    Serial.print("Res Levels Low: ");
//    
//    if(!res_levels_low){
//        Serial.println("False");
//    } else {
//      Serial.println("True");
//    }
//##############END OF DEGUG#################//

    digitalWrite(PUMP_PIN, HIGH);
    delay(500);
    digitalWrite(PUMP_PIN, LOW);
    delay(3000);
  }while(current_moisture > MOISTURE_THRESHOLD);

  //Fixes the spam of moisture checks if logic gets stuck in the do-while loop
  previousCaptureTime = millis();
  Serial.println("WATERING STATUS: SUCCESS");
}


/*
 * TODO:
 */

void loop() {
  int current_moisture = 0;

  //If there is an input detected on the serial buffer
  if(Serial.available()){
    //Ping timeout of PING_DELAY ms
    // Default: 5 seconds => 5000ms
      if(Allow_Ping()){
        String task = Serial.readString();
    
        //Check Moisture and send results to user
        if(task == SEND_DATA){
          current_moisture = Capture_Moisture();

          String output = "Current Moisture: " + (String)current_moisture;

          Serial.println(output);
          
          //Does the plant need to be watered?
          if(current_moisture > MOISTURE_THRESHOLD){
            //If moisture is less than the threshold water plant
            Serial.println("Plant needs watering.");
            Serial.println("Attempting to water plant.");
            Water_Plant();

            //After watering the plant, does it need to be refilled?
            if(Res_Levels_Low()){
              Serial.println("CRITICAL WARNING: PUMP NEEDS TO BE REFILLED.");
            }
          } else {
            Serial.println("Plant does not need watering.");
          }
        }  
     }
  }

  /*This will run every hour and is independent of the user
  *Example:
  * If the user pings it to run at 56 mins, this will run 4 mins later.
  * This is an ineffecient way of doing things, maybe I will change it.
  */
  
  if(Time_To_Check_Moisture()){
    current_moisture = Capture_Moisture();
    
    String output = "Current Moisture: " + (String)current_moisture;
    
    Serial.println(output);

    //Start pump?
    if(current_moisture > MOISTURE_THRESHOLD){
      Serial.println("Plant needs watering.");
      Serial.println("Attempting to water plant.");
      
      Water_Plant();
      
      //Check pump levels
      if(Res_Levels_Low()){
        Serial.println("CRITICAL WARNING: PUMP NEEDS TO BE REFILLED.");
      }
    } else {
      Serial.println("Plant does not need watering.");
    }
  }
}
