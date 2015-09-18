#define DEBUG


class Sensor {
	public:
		virtual bool Get();
};

class PhotoCell : public Sensor
{
  int PIN;
  int cdsThreshold;
  bool debug;

  
  public:
  PhotoCell(int pinnum, int threshold, bool enableDebug) {
    PIN = pinnum;
    cdsThreshold = threshold;
    debug = enableDebug;
    
	#ifdef DEBUG
	if (debug) {
		Serial.print("Adding Light Detector on PIN:");
	  	Serial.println(pinnum);
	}
	#endif
	
  }

  bool Get() {
  	int lightlevel = analogRead(PIN);
  	
  	#ifdef DEBUG
  	if (debug) {
  		Serial.print("Light Level = ");
		Serial.println(lightlevel);
  	}
	#endif
	
    return  lightlevel < cdsThreshold;    
  }
};



class MotionSensor : public Sensor {

    int PIN;
    bool debug;

    public:
    MotionSensor(int pinnum, bool enableDebug) {
      PIN = pinnum;  
      debug = enableDebug;
      
      #ifdef DEBUG
      if (debug) {
      	Serial.print("Adding Motion Detector on PIN:");
  	  	Serial.println(pinnum);
      }
      #endif
    }

    bool Get() {
    	bool state = digitalRead(PIN);
    	
    	#ifdef DEBUG
      	if (debug) {
      		Serial.print("Motion Sensor State:");
  	  		Serial.println(state);
      	}
    	#endif
    	
        return state;
    }
};

class DistanceSensor : public Sensor{

	int PIN;
	int stackLocation;
    double triggerDistance;
    bool debug;

    static const int stackSize = 4;

	double avg[stackSize];

	public:
	DistanceSensor(int pinnum, double triggerDist, bool enableDebug) {
		PIN = pinnum;
    	triggerDistance = triggerDist;
    	stackLocation = 0;
		debug = enableDebug;

      #ifdef DEBUG
      if (debug) {
      	Serial.print("Adding Motion Detector on PIN:");
  	  	Serial.print(pinnum);
  	  	Serial.print(" | Trigger Distance:");
  	  	Serial.println(triggerDist);
      }
      #endif
    	
    	for (int i = 0; i < stackSize; i++) {
    		avg[i] = 100.0;
    	}
	}

	bool Get() {
		double distance = 6202.3*pow(analogRead(PIN),-1.056);
		
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
    	
    	#ifdef DEBUG
		if (debug) {
			Serial.print("Distance = ");
			Serial.print(distance);
			Serial.print(" | Average = ");
			Serial.println(temp);
		}
		#endif
		
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
  bool debug;

  int LPWMPIN;
  int fadeSpeed;
  int lightTime;
  unsigned long lastUpdate;
  int numOfPhotos;
  int numOfSensors;
  unsigned long timeUp;
  int currentFade;
  
  Sensor *photoCells[inputCount];
  Sensor *sensors[inputCount];
   
  public:
  LightBar(int lightpwmpin, int fadeSpeed, int lighttime, bool enableDebug) {
    LPWMPIN = lightpwmpin;
    fadeSpeed = fadeSpeed;
    lightTime = lighttime;
	debug = enableDebug;
	
	currentFade = 0;
	timeUp = 0;
    numOfPhotos = 0;
	numOfSensors = 0;
	
    state = LOW;
  }

  void AttachPhoto(Sensor *photoCell) {
        photoCells[numOfPhotos] = photoCell;
        numOfPhotos++;
  }

  void AttachSensor(Sensor *sensor) {
  	 sensors[numOfSensors] = sensor;
  	 numOfSensors++;
  }
  
  void Update() {
     if ((millis() - lastUpdate) > updateInterval) {
        lastUpdate = millis();
        #ifdef DEBUG
        if (debug) {
        	Serial.print("State is:");
        	Serial.println(state);
        }
        #endif
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
		if (photoCells[i]->Get()){
			isDarkEnough = 1;
			break;
		}
	}
	if (isDarkEnough) {
	   for (int i = 0; i < numOfSensors; i++) {
    	  if (sensors[i]->Get()) {
	        return true; 
    	  }
   		}
		
		return false;
	}else{
		return false;
	}
  	
  }

};

//Stairs(Light PWM Pin, Fade Speed, Illumination Duration, Enable Debug)
LightBar Stairs(3, 5, 5, false);



void setup() {
  Serial.begin(9600);
  Stairs.AttachPhoto(new PhotoCell(0, 1000, false));
  Stairs.AttachSensor(new MotionSensor(2, false));
}

void loop() {
  Stairs.Update();
}

