#define DEBUG


// Set the mux to the proper pin
void setMuxPin(int pin) {
	/*
	Serial.print(bitRead(pin,0));
	Serial.print(bitRead(pin,1));
	Serial.print(bitRead(pin,2));
	Serial.println(bitRead(pin,3));
	*/
	digitalWrite(9, bitRead(pin,0));
	digitalWrite(10, bitRead(pin,1));
	digitalWrite(12, bitRead(pin,2));
	digitalWrite(13, bitRead(pin,3));
};


int analogMuxRead(int pin) {
	setMuxPin(pin);
	return analogRead(0);
};

bool digitalMuxRead(int pin) {
	setMuxPin(pin);
	return digitalRead(8);
}

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
  	int lightlevel = analogMuxRead(PIN);
  	
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
    	bool state = digitalMuxRead(PIN);
    	
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
    double total;

    static const int stackSize = 4;

	double avg[stackSize];

	public:
	DistanceSensor(int pinnum, double triggerDist, bool enableDebug) {
		PIN = pinnum;
    	triggerDistance = triggerDist;
    	stackLocation = 0;
		debug = enableDebug;
		total = 0;

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
    		total += 100;
    	}
	}

	//Keep a running total of the distance as it relates to stack size.  
	//On each read, it will add the newest amount to the total, move the pointer forward one and remove that distance from the total.  
	//This means we don't need to loop through the stack on each read.
	bool Get() {
		double distance = 6202.3*pow(analogMuxRead(PIN),-1.056);
		
		avg[stackLocation] = distance;
		total += distance;
		
		//Move us forward one
		if (stackLocation == stackSize -1) {
			stackLocation = 0;
		}else{
			stackLocation ++;
		}
		//Remove the distance from the total for the next.  (basically removing the oldest distance)
		total -= avg[stackLocation];
		
		double temp = total / stackSize;

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
LightBar Stairs(3, 5, 1, false);



void setup() {
  Serial.begin(9600);
  pinMode(9, OUTPUT); 
  pinMode(10, OUTPUT); 
  pinMode(12, OUTPUT); 
  pinMode(13, OUTPUT); 
  Stairs.AttachPhoto(new PhotoCell(1, 1000, true));
  Stairs.AttachSensor(new MotionSensor(0, false));
}

void loop() {
  Stairs.Update();
}

