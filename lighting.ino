#define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
#endif

class PhotoCell
{
  int PIN;
  
  public:
  PhotoCell(int pinnum) {
    PIN = pinnum;
  }

  int Get() {
    return analogRead(PIN);    
  }
};

class MotionSensor {

    int PIN;

    public:
    MotionSensor(int pinnum) {
      PIN = pinnum;  
    }

    bool Get() {
        return digitalRead(PIN);
    }
};

class DistanceSensor {

	int PIN;
	int stackLocation;
    double triggerDistance;

    static const int stackSize = 4;

	double avg[stackSize];

	public:
	DistanceSensor(int pinnum, double triggerDist) {
		PIN = pinnum;
    	triggerDistance = triggerDist;
    	stackLocation = 0;
    	for (int i = 0; i < stackSize; i++) {
    		avg[i] = 100.0;
    	}
	}

	bool Get() {
		double distance = 6202.3*pow(analogRead(PIN),-1.056);
		DEBUG_PRINT("Distance = ");
		DEBUG_PRINT(distance);
		DEBUG_PRINT(" | Average = ");
		avg[stackLocation] = distance;
		if (stackLocation == stackSize -1) {
			stackLocation = 0;
		}else{
			stackLocation ++;
		}
		double temp;
		for (int i = 0; i < stackSize; i++) {
    		temp += avg[i];
    	}
    	temp = temp / stackSize;
		DEBUG_PRINTLN(temp);
		return temp <= triggerDistance;
	}
};

class LightBar
{

//If we need to support more than X inputs for a single bar, change this.
  static const int inputCount = 4;
  static const int updateInterval = 5; //how many ms between checks.  (if another check takes longer it could be more than 5ms, this is the minimum)

  static const int GUP = 2; //Short for going up
  static const int GDOWN = 3; //Short for going down
  
  int state;
  

  int LPWMPIN;
  int cdsThreshold;
  int fadeSpeed;
  int lightTime;
  unsigned long lastUpdate;
  int numOfPhotos;
  int numOfMotions;
  int numOfDistances;
  unsigned long timeUp;
  int currentFade;
  
  PhotoCell *photoCells[inputCount];
  MotionSensor *motionSensors[inputCount];
  DistanceSensor *distanceSensors[inputCount];
  
  
  public:
  LightBar(int lightpwmpin, int cdsthreshold, int fadeSpeed, int lighttime) {
    LPWMPIN = lightpwmpin;
    cdsThreshold = cdsthreshold;
    fadeSpeed = fadeSpeed;
    lightTime = lighttime;

	currentFade = 0;
	timeUp = 0;
    numOfPhotos = 0;
    numOfMotions = 0;
    numOfDistances = 0;

    state = LOW;

    for (int i = 0; i<= inputCount; i++ ){
      photoCells[i] = NULL;
    }

    for (int i = 0; i<= inputCount; i++ ){
      motionSensors[i] = NULL;
    }

    
    for (int i = 0; i<= inputCount; i++ ){
      distanceSensors[i] = NULL;
    }
  }

  void AttachPhoto(int pin) {
  		DEBUG_PRINT("Adding PhotoCell on PIN:");
  		DEBUG_PRINTLN(pin);
        photoCells[numOfPhotos] = new PhotoCell(pin);
        numOfPhotos++;
  }

  void AttachMotion(int pin) {
  	  DEBUG_PRINT("Adding Motion Detector on PIN:");
  	  DEBUG_PRINTLN(pin);
      motionSensors[numOfMotions] = new MotionSensor(pin);
      numOfMotions++;
  }

  void AttachDistance(int pin, double trigDistance) {
    DEBUG_PRINT("Adding Distance Detector on PIN:");
      DEBUG_PRINTLN(pin);
      distanceSensors[numOfMotions] = new DistanceSensor(pin, trigDistance);
      numOfDistances++;
  }
  void Update() {
     if ((millis() - lastUpdate) > updateInterval) {
        lastUpdate = millis();
        //DEBUG_PRINT("State is:");
        //DEBUG_PRINTLN(state);
        switch(state) {
          case HIGH:
          	if (Detect()) { //If we have motion, reset the timeUp
          		timeUp = lastUpdate;
          	}else{ //Check to see if the timeUp has exceeded the total time, if so, set the state to GDOWN
          		if ((lastUpdate - timeUp) > (lightTime * 1000)){
          			state = GDOWN; 
          		}
          	}
          break;
          case LOW:
          	if (Detect()) {
          		state = GUP;
          	}
          break;
          case GUP:
          	if (currentFade < 255) {
          		currentFade += 1;
				analogWrite(LPWMPIN, currentFade);
				if (currentFade == 255) {
					state = HIGH;
					timeUp = lastUpdate;
				}
          	}
          break;
          case GDOWN:
          	if (Detect()) {
				state = GUP;
          	}else{
          		if (currentFade > 0) {
          			currentFade -= 1;
          			analogWrite(LPWMPIN, currentFade);
          			if (currentFade == 0){
          				state = LOW;
          			}
          		}
          	}
          break;
        }
     }
  }


  bool Detect() {
  	bool isDarkEnough = 0;
  	int lightlevel = 0;
	for (int i = 0; i < numOfPhotos; i++) {
		lightlevel = photoCells[i]->Get();
	//	DEBUG_PRINT("Light Level = ");
	//	DEBUG_PRINTLN(lightlevel);
		if (lightlevel < cdsThreshold){
			isDarkEnough = 1;
		}
	}
	if (isDarkEnough) {
		for( int i = 0; i < numOfMotions; i++) {
			if (motionSensors[i]->Get()) {
				return true;
			}
		}

   for (int i = 0; i < numOfDistances; i++) {
      if (distanceSensors[i]->Get()) {
        return true; 
      }
   }
		
		return false;
	}else{
		return false;
	}
  	
  }

};

//Stairs(Light PWM Pin, Photo Sensor Threshold, Fade Speed, Illumination Duration)
LightBar Stairs(3, 1000, 5, 5);



void setup() {
  Serial.begin(9600);
  Stairs.AttachPhoto(0);
  //Stairs.AttachMotion(2);
  Stairs.AttachDistance(1, 30.00);
}

void loop() {
  Stairs.Update();
 //temp.Get();
 //delay(500);
}

