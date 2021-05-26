//#include <../lib/google-maps-device-locator/src/google-maps-device-locator.h>

/*TCPClient client;
byte server[] = { 192, 168, 0, 106 };
//byte server[] = { 192, 168, 1, 6 };

float _latitude;
float _longitude;
float _accuracy;

GoogleMapsDeviceLocator locator;

int i = 0;

// setup() runs once, when the device is first turned on.
void setup() {
  Serial.begin(9600);
  // Put initialization like pinMode and begin functions here.

  waitFor(Serial.isConnected, 30000);
  Serial.println("Started");
  locator.withSubscribe(locationCallback).withLocatePeriodic(30);
}

void locationCallback(float lat, float lon, float accuracy) {
  // Handle the returned location data for the device. This method is passed three arguments:
  _latitude = lat;
  _longitude = lon;
  _accuracy = accuracy;

  Serial.println("Google Maps:");
  Serial.print("Latitude: ");
  Serial.println(_latitude);
  Serial.print("Longitude: ");
  Serial.println(_longitude);
  Serial.print("Accuracy: ");
  Serial.println(_accuracy);
}


// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  //Serial.printlnf("Counting: %d", i++);
  delay(1000);

  locator.loop();

  // POST Message
  if(client.connect(server, 3000))
  {
    // Print some information that we have connected to the server
    Serial.println("**********************************!");
    Serial.printlnf("New POST Connection!: %d", i);
    Serial.println("Connection OK!");

    char* postVal = ""
    "{"
      "\"windSpeed\": 420,"
      "\"temperature\": 23"
    "}";
            
    // Send our HTTP data!
    client.println("POST / HTTP/1.0");
    client.println("Host: 192.168.1.6:3000"); // ************ HARDCODED HOST IP??
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(strlen(postVal));
    client.println();
    client.print(postVal);

    // Read data from the buffer
    
    // while(receivedData.indexOf("\r\n\r\n") == -1)
    // {
    //     memset(dataBuffer, 0x00, sizeof(dataBuffer));
    //     client.read(dataBuffer, sizeof(dataBuffer));
    //     receivedData += (const char*)dataBuffer;
    // }
    
    // Print the string
    //Serial.println(receivedData);

    // Stop the current connection
    client.stop();
  }  
  else
  {
      Serial.println("Server connection failed. Trying again...");
  } 

}*/

#include <math.h>

const int lightSensor = A0;
const int humidSensor = D2;
const int windSensor = A2;
const int windSpeedSensor = A3;
const int rainSensor = D4;

int lastWindSpeedEventTime = 0;
int lastRainEventTime = 0;

uint8_t bht_sensor = 0x77;

// Oversampling rate and scale factors
int scale_factors[8] = {524288, 1572864, 3670016, 7864320, 253952, 516096, 1040384, 2088960};

void setup() {
	pinMode(lightSensor, OUTPUT);
	pinMode(humidSensor, OUTPUT);
	digitalWrite(humidSensor, HIGH); // DHT11 starts high

	attachInterrupt(windSpeedSensor, windSpeedEvent, RISING);
	attachInterrupt(rainSensor, rainEvent, RISING);

	Wire.begin();

	Serial.print("Setup complete.");
}

void readData(int device_addr, int reg, int num_bytes, uint8_t registerValues[]) {
	Wire.beginTransmission(device_addr);
	Wire.write(reg);
	Wire.endTransmission();

	Wire.requestFrom(bht_sensor, num_bytes);
	for (int i = 0; i < num_bytes; i++) {
		registerValues[i] = Wire.read();
	}
}

void printBin(char* text, int value) {
	Serial.print(text);
	Serial.print(value, BIN);
	Serial.println();
}

void printDec(char* text, int value) {
	Serial.print(text);
	Serial.print(value);
	Serial.println();
}

void printFloat(char* text, float value) {
	Serial.print(text);
	Serial.print(value);
	Serial.println();
}

int comp2(uint32_t binary_input, int num_bit) {

	// Check if negative number
	int binary_output;
	if (binary_input > (pow(2, num_bit - 1) - 1)) {
		binary_output = binary_input - pow(2, num_bit);
	}

	return binary_output;
}

int getScaleFact(int reg) {
	uint8_t oversample_register_data[1];
	readData(bht_sensor, reg, 1, oversample_register_data);
	int oversampling_rate = oversample_register_data[0] & 0b00000111;
	int scale_fact = scale_factors[oversampling_rate];

	return scale_fact;
}

void getValuesBarometer() {
	// Set continuous reading for barometer and temperature
	Wire.beginTransmission(bht_sensor); // 0x77 -> addresse de barometer
	int meas_cfg = 0x08;
	Wire.write(meas_cfg);
	Wire.write(7);
	Wire.endTransmission();

	// Read calibration coefficients
	uint8_t calib_coeffs[18];
	readData(bht_sensor, 0x10, 18, calib_coeffs);

	int c0 = comp2(((uint32_t)calib_coeffs[0] << 4) | ((uint32_t)calib_coeffs[1] >> 4), 12);
	int c1 = comp2((((uint32_t)calib_coeffs[1] & 0b00001111) << 8) | (uint32_t)calib_coeffs[2], 12);
	int c00 = comp2(((uint32_t)calib_coeffs[3] << 12) | ((uint32_t)calib_coeffs[4] << 4) | ((uint32_t)calib_coeffs[5] >> 4), 20);
	int c10 = comp2(((uint32_t)calib_coeffs[5] & 0b00001111) << 16 | (uint32_t)calib_coeffs[6] << 8 | (uint32_t)calib_coeffs[7], 20);
	int c01 = comp2((uint32_t)calib_coeffs[8] << 8 | (uint32_t)calib_coeffs[9], 16);
	int c11 = comp2((uint32_t)calib_coeffs[10] << 8 | (uint32_t)calib_coeffs[11], 16);
	int c20 = comp2((uint32_t)calib_coeffs[12] << 8 | (uint32_t)calib_coeffs[13], 16);
	int c21 = comp2((uint32_t)calib_coeffs[14] << 8 | (uint32_t)calib_coeffs[15], 16);
	int c30 = comp2((uint32_t)calib_coeffs[16] << 8 | (uint32_t)calib_coeffs[17], 16);

	Serial.printlnf("Coeff: %d, %d, %d, %d, %d, %d, %d, %d, %d", c0, c1, c00, c10, c01, c11, c20, c21, c30);

	// Read barometer
	int psr_b2 = 0x00;
	uint8_t barometerValues[3];
	readData(bht_sensor, psr_b2, 3, barometerValues);

	uint32_t pressureValue = barometerValues[0];
	pressureValue = (pressureValue << 8) | barometerValues[1];
	pressureValue = (pressureValue << 8) | barometerValues[2];

	printBin("Pressure: ", pressureValue);

	int pressureValueSigned = comp2(pressureValue, 24);
	printDec("Pressure dec: ", pressureValueSigned);

	int barometer_scale_fact = getScaleFact(0x06);
	printDec("Scale factor barometer: ", barometer_scale_fact);
	int p_raw_sc = pressureValueSigned / barometer_scale_fact;

	// Read temperature
	int tmp_b2 = 0x03;
	uint8_t temperature_values[3];
	readData(bht_sensor, tmp_b2, 3, temperature_values);

	uint32_t temperature_value = temperature_values[0];
	temperature_value = (temperature_value << 8) | temperature_values[1];
	temperature_value = (temperature_value << 8) | temperature_values[2];

	printBin("Temperature: ", temperature_value);

	int temperature_value_signed = comp2(temperature_value, 24);
	printDec("Temperature dec: ", temperature_value_signed);

	int temperature_scale_fact = getScaleFact(0x07);
	printDec("Scale factor temp: ", temperature_scale_fact);
	int t_raw_sc = temperature_value_signed / temperature_scale_fact;


	// Compute final pressure value
	int p_comp = c00 + p_raw_sc * (c10 + p_raw_sc * (c20 + p_raw_sc * c30)) + t_raw_sc * c01 + t_raw_sc * p_raw_sc * (c11 + p_raw_sc * c21);
	int t_comp = c0*0.5 + c1*t_raw_sc;

	printDec("Final pressure p_comp: ", p_comp);
	printDec("Final temperature t_comp: ", t_comp);
}

void getValuesHumidity() {

	Serial.println("start");
	int dhtPin = humidSensor;

	noInterrupts();
	digitalWrite(dhtPin, HIGH);
	delay(250);

	// Start signal
	Serial.println("low");
	digitalWrite(dhtPin, LOW);
	delay(20);
	Serial.println("high");
	digitalWrite(dhtPin, HIGH);
	delayMicroseconds(40);
	pinMode(humidSensor, INPUT);

	// DHT low then high, maybe replace with a pulseIn(HIGH)
	pulseIn(dhtPin, HIGH);

	int pulseTimes[40];
	for (int i = 0; i < 40; i++) {
		pulseTimes[i] = pulseIn(dhtPin, HIGH); // Do not execute anything in loop (will throw off timing)
	}
	interrupts();

	// Print delays
	// for (int i = 0; i < 40; i++) {
	// 	printf("%d", i);
	// 	printDec(" Delay time: ", delayArrayForPrint[i]);
	// }

	pinMode(humidSensor, OUTPUT);
	digitalWrite(dhtPin, LOW);

	uint8_t dataBits[5] = {0};
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 8; j++) {
			dataBits[i] <<= 1;
			if (pulseTimes[i*8 + j] > 50) {
				dataBits[i] |= 1;
			}
		}
	}

	// Calculate values obtained from communication
	float humid = dataBits[0]; // + dataBits[1]/100;
	float temp = dataBits[2]; // + dataBits[3]/100;

	int checkSum = dataBits[0] + dataBits[1] + dataBits[2] + dataBits[3];
	if ((checkSum & 0b11111111) != dataBits[4]) {
		printDec("CHECKSUM ERROR: Checksum obtained: ", checkSum & 0b11111111);
		printDec("CHECKSUM ERROR: Checksum expected: ", dataBits[4]);
	}
	// ELSE

	printFloat("*DHT11* Temperature value: ", temp);
	printFloat("*DHT11* Humidity value: ", humid);
	
}

void getValuesWindDirection() {
	int voltage_read = analogRead(windSensor) * (5000/4096.0);

	int voltage_array[16] = {3840, 1980, 2250, 410, 450, 320, 900, 620, 1400, 1190, 3080, 2930, 4620, 4040, 4330, 3430};

	// Find value that is closest and save index
	int closest_ind = 0;
	int smallest_delta = 99999;
	for (int i = 0; i < 16; i++) {
		int delta = abs(voltage_array[i] - voltage_read);
		if (delta < smallest_delta) {
			smallest_delta = delta;
			closest_ind = i;
		}
	}

	// Get direction
	float direction_array[16] = {0, 22.5, 45, 67.5, 90, 112.5, 135, 157.5, 180, 202.5, 225, 247.5, 270, 292.5, 315, 337.5};
	float direction_deg = direction_array[closest_ind];

	printDec("Voltage read: ", voltage_read);
	printDec("smallest delta: ", smallest_delta);
	printDec("closest ind: ", closest_ind);
	printFloat("Final direction: ", direction_deg);
}

void windSpeedEvent() {

	int current_millis = millis();
	if (current_millis != 0) {
		int time_since_last_event = current_millis - lastWindSpeedEventTime;

		// Ignore bounce
		if (time_since_last_event < 20) {
			return;
		}

		printDec("Wind speed new delta: ", time_since_last_event);

		// 2.4 km/h / s
		float wind_speed = (1000.0 / (float)time_since_last_event) * 2.4;
		printFloat("Current wind speed: ", wind_speed);
	}
	lastWindSpeedEventTime = current_millis;
}

void rainEvent() {
	// One event every 0.2794 mm
	int current_millis = millis();

	// Ignore bounce
	int time_since_last_event = current_millis - lastRainEventTime;
	if (time_since_last_event > 600) {
		printDec("Time since last rain event: ", time_since_last_event);
	}

	lastRainEventTime = current_millis;
}


void loop() {

	// Light sensor
	//int result = analogRead(lightSensor);
	//Serial.printlnf("Light sensor: %d mV", result);

	delay(1000);

	//getValuesBarometer();
	
	//getValuesHumidity();

	//getValuesWindDirection();

}