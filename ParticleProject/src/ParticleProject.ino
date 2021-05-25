#include <../lib/google-maps-device-locator/src/google-maps-device-locator.h>

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

uint8_t bht_sensor = 0x77;

// Oversampling rate and scale factors
int scale_factors[8] = {524288, 1572864, 3670016, 7864320, 253952, 516096, 1040384, 2088960};

void setup() {
	pinMode(lightSensor, OUTPUT);

	Wire.begin();
}

void readData(int device_addr, int reg, int num_bytes, int registerValues[]) {
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

int comp2(int binary_input, int num_bit) {

	// Check if negative number
	printDec("comp2 check in int: ", binary_input);
	printDec("comp2 max: ", (pow(2, num_bit - 1) - 1));
	if (binary_input > (pow(2, num_bit - 1) - 1)) {
		binary_input = binary_input - pow(2, num_bit);
		Serial.println("Comp2 limit passed");
		printDec("New number comp2: ", binary_input);
	}

	return binary_input;
}

int getScaleFact(int reg) {
	int oversample_register_data[1];
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
	int calib_coeffs[18];
	readData(bht_sensor, 0x10, 18, calib_coeffs);

	int c0 = comp2((calib_coeffs[0] << 4) | (calib_coeffs[1] >> 4), 12);
	int c1 = comp2(((calib_coeffs[1] & 0b00001111) << 8) | calib_coeffs[2], 12);
	int c00 = comp2((calib_coeffs[3] << 12) | (calib_coeffs[4] << 4) | (calib_coeffs[5] >> 4), 20);
	int c10 = comp2((calib_coeffs[5] & 0b00001111) << 16 | calib_coeffs[6] << 8 | calib_coeffs[7], 20);
	int c01 = comp2(calib_coeffs[8] << 8 | calib_coeffs[9], 16);
	int c11 = comp2(calib_coeffs[10] << 8 | calib_coeffs[11], 16);
	int c20 = comp2(calib_coeffs[12] << 8 | calib_coeffs[13], 16);
	int c21 = comp2(calib_coeffs[14] << 8 | calib_coeffs[15], 16);
	int c30 = comp2(calib_coeffs[16] << 8 | calib_coeffs[17], 16);

	Serial.printlnf("Coeff: %d, %d, %d, %d, %d, %d, %d, %d, %d", c0, c1, c00, c10, c01, c11, c20, c21, c30);

	// Read barometer
	int psr_b2 = 0x00;
	int barometerValues[3];
	readData(bht_sensor, psr_b2, 3, barometerValues);

	int pressureValue = barometerValues[0];
	pressureValue = (pressureValue << 8) | barometerValues[1];
	pressureValue = (pressureValue << 8) | barometerValues[2];

	printBin("Pressure: ", pressureValue);

	pressureValue = comp2(pressureValue, 24);
	printDec("Pressure dec: ", pressureValue);

	int barometer_scale_fact = getScaleFact(0x06);
	printDec("Scale factor barometer: ", barometer_scale_fact);
	int p_raw_sc = pressureValue / barometer_scale_fact;

	// Read temperature
	int tmp_b2 = 0x03;
	int temperature_values[3];
	readData(bht_sensor, tmp_b2, 3, temperature_values);

	int temperature_value = temperature_values[0];
	temperature_value = (temperature_value << 8) | temperature_values[1];
	temperature_value = (temperature_value << 8) | temperature_values[2];

	printBin("Temperature: ", temperature_value);

	temperature_value = comp2(temperature_value, 24);
	printDec("Temperature dec: ", temperature_value);

	int temperature_scale_fact = getScaleFact(0x07);
	printDec("Scale factor temp: ", temperature_scale_fact);
	int t_raw_sc = temperature_value / temperature_scale_fact;


	// Compute final pressure value
	int p_comp = c00 + p_raw_sc * (c10 + p_raw_sc * (c20 + p_raw_sc * c30)) + t_raw_sc * c01 + t_raw_sc * p_raw_sc * (c11 + p_raw_sc * c21);
	int t_comp = c0*0.5 + c1*t_raw_sc;

	printDec("Final pressure p_comp: ", p_comp);
	printDec("Final temperature t_comp: ", t_comp);
}

// float addDecimals(int integral, int decimal, int decimalPlaces) {
// 	float intToFloat = integral;
// 	for (int i = 1; i < decimalPlaces+1; i++) {
// 		int decimalPosition = decimal % pow(10, i);
// 		intToFloat += pow(decimalPosition, -i);
// 	}
// }

void getValuesHumidity() {
	int dhtPin = D4;
	// Start signal
	digitalWrite(dhtPin, LOW);
	delay(25);
	digitalWrite(dhtPin, HIGH);

	int dhtResponseLowTime = pulseIn(dhtPin, LOW);
	delayMicroseconds(100);

	uint8_t dataBits[5];
	int dataBitsIndex = 0;
	for (int i = 1; i < 41; i++) {
		int pulseTime = pulseIn(dhtPin, HIGH);

		if (pulseTime < 50) {
			dataBits[dataBitsIndex] = dataBits[dataBitsIndex] << 1;
		}
		else {
			dataBits[dataBitsIndex] = (dataBits[dataBitsIndex] << 1) | 1;
		}

		if (i % 8 == 0) {
			dataBitsIndex++;
		}
	}

	digitalWrite(dhtPin, HIGH);

	// Calculate values obtained from communication
	float temp = dataBits[0] + dataBits[1]/100;
	float humid = dataBits[2] + dataBits[3]/100;

	int checkSum = dataBits[0] + dataBits[1] + dataBits[2] + dataBits[3];
	if ((checkSum & 0b11111111) != dataBits[4]) {
		printDec("CHECKSUM ERROR: Checksum obtained: ", checkSum & 0b11111111);
		printDec("CHECKSUM ERROR: Checksum expected: ", dataBits[4]);
	}
	// ELSE

	printFloat("*DHT11* Temperature value: ", temp);
	printFloat("*DHT11* Humidity value: ", humid);
	
}

void loop() {

	// Light sensor
	//int result = analogRead(lightSensor);
	//Serial.printlnf("Light sensor: %d mV", result);


	//getValuesBarometer();
	getValuesHumidity();

	
	delay(1000);
}