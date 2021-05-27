#include <../lib/google-maps-device-locator/src/google-maps-device-locator.h>
#include <math.h>

// Used for closing and opening WiFi module
SYSTEM_MODE(SEMI_AUTOMATIC);

// Server variables
TCPClient client;
byte server[] = { 192, 168, 0, 106 };
String ip = "192.168.0.106";
//byte server[] = { 192, 168, 1, 6 };
int server_port = 3000;

// Google maps variables
GoogleMapsDeviceLocator locator;
float _latitude;
float _longitude;
float _accuracy;

// Argon pins
uint8_t bht_sensor = 0x77;
const int lightSensor = A2;
const int humidSensor = D2;
const int windSensor = A0;
const int windSpeedSensor = A1;
const int rainSensor = D4;

// Current values (used to transmit)
float windSpeedCurrentValue = 0;
float windDirectionCurrentValue = 0;
float temperatureCurrentValue = 0;
float temperatureDecimalCurrentValue = 0;
float pressureCurrentValue = 0;
float humidityCurrentValue = 0;
float humidityDecimalCurrentValue = 0;
float rainCurrentValue = 0;
float luminosityCurrentValue = 0;

String geolocationCurrentValue = "no location";

// Global time counters 
int lastWindSpeedEventTime = 0;
int lastRainEventTime = 0;

// Barometer coefficients
int32_t c0;
int32_t c1;
int32_t c00;
int32_t c01;
int32_t c11;
int32_t c10;
int32_t c20;
int32_t c21;
int32_t c30;

// Wifi connection timer
bool locationAcquired = false;
time_t lastWifiPostTime = 0;

// Oversampling rate and scale factors
int scale_factors[8] = {524288, 1572864, 3670016, 7864320, 253952, 516096, 1040384, 2088960};

// Initial connections and setups
void setup() {
	// Connect WiFi and particle, since starting in semiauto mode
	WiFi.on();
	Particle.connect();
	Serial.begin(9600);
	
	//waitFor(Serial.isConnected, 30000);

	// Subscribe callback to google maps locator
	locator.withSubscribe(locationCallback).withLocateOnce();
	// locator.withSubscribe(locationCallback).withLocatePeriodic(30);

	// Set pin modes
	pinMode(lightSensor, INPUT);
	pinMode(windSensor, INPUT);
	pinMode(windSpeedSensor, INPUT);
	pinMode(rainSensor, INPUT);
	pinMode(humidSensor, OUTPUT);
	digitalWrite(humidSensor, HIGH); // DHT11 starts high

	// Attach interrupts for rain and wind speed sensors
	attachInterrupt(windSpeedSensor, windSpeedEvent, RISING);
	attachInterrupt(rainSensor, rainEvent, RISING);

	Wire.begin();
	getBarometerSetup();

	Serial.println("Setup complete.");
}

// Function called by Google Maps integration to return location
void locationCallback(float lat, float lon, float accuracy) {

	locationAcquired = true;
	_latitude = lat;
	_longitude = lon;
	_accuracy = accuracy;
}

// Connects to server and builds POST request to send to server
void sendPost() {

	// Connect to server
	if(!client.connect(server, server_port)) {
		Serial.println("Server connection failed.");
		return;
	}
	Serial.println("New connection: creating POST");

	// Get geolocation
	geolocationCurrentValue = String(_latitude) + ", " + String(_longitude) + " (" + String(_accuracy) + " m)";

	// Build JSON payload
	String postVal = ""
	"{" 
	"\"windSpeed\": " + String(windSpeedCurrentValue) + ","
	"\"windDirection\": " + String(windDirectionCurrentValue) + ","
	"\"temperature\": " + String(temperatureCurrentValue) + ","
	"\"temperatureDecimal\": " + String(temperatureDecimalCurrentValue) + ","
	"\"pressure\": " + String(pressureCurrentValue) + ","
	"\"humidity\": " + String(humidityCurrentValue) + ","
	"\"humidityDecimal\": " + String(humidityDecimalCurrentValue) + ","
	"\"rain\": " + String(rainCurrentValue) + ","
	"\"luminosity\": " + String(luminosityCurrentValue) + ","
	"\"location\": \"" + geolocationCurrentValue + "\""
	"}";

	String port_str = String(server_port);
	
	// Send our HTTP data!
	client.println("POST / HTTP/1.0");
	client.println("Host: " + ip + ":" + port_str);
	client.println("Content-Type: application/json");
	client.print("Content-Length: ");
	client.println(strlen(postVal));
	client.println();
	client.print(postVal);

	// Stop the current connection
	client.stop();

	//Serial.println("Post function done");
}

// Read data from a register on a device (I2C)
void readData(int device_addr, int reg, int num_bytes, uint8_t *registerValues) {
	Wire.beginTransmission(device_addr);
	Wire.write(reg);
	Wire.endTransmission();

	Wire.requestFrom(bht_sensor, num_bytes);
	for (int i = 0; i < num_bytes; i++) {
		registerValues[i] = Wire.read();
	}
}

// Print number in binary form
void printBin(char* text, int value) {
	Serial.print(text);
	Serial.print(value, BIN);
	Serial.println();
}

// Print number in decimal (base 10) form
void printDec(char* text, int value) {
	Serial.print(text);
	Serial.print(value);
	Serial.println();
}

// Print float
void printFloat(char* text, float value) {
	Serial.print(text);
	Serial.printf("%.4f", value);
	Serial.println();
}

// 2's complement conversion of binary with defined number of bits 'length'
void comp2(int32_t *binary_input, int length)
{
	if (*binary_input > (pow(2, length - 1) - 1)) {
		*binary_input -= pow(2, length);
	}
}

// Useful to read scale factor on barometer sensor
int getScaleFact(int reg) {
	uint8_t oversample_register_data[1];
	readData(bht_sensor, reg, 1, oversample_register_data);
	int oversampling_rate = oversample_register_data[0] & 0b00000111;
	int scale_fact = scale_factors[oversampling_rate];

	return scale_fact;
}

// Initial setup for barometer sensor
void getBarometerSetup() {

	// Set pressure config (oversampling value)
	Wire.beginTransmission(bht_sensor); // 0x77 -> addresse de barometer
	int temp_cfg = 0x06;
	Wire.write(temp_cfg);
	Wire.write(0b00000011);
	Wire.endTransmission();

	// Set temp config (coeff source and oversampling value)
	uint8_t temp_coeff_src[1];
	readData(bht_sensor, 0x28, 1, temp_coeff_src);
	uint8_t temp_config = (temp_coeff_src[0] & 0b10000000) | 0b00000011;

	Wire.beginTransmission(bht_sensor); // 0x77 -> addresse de barometer
	int prs_cfg = 0x07;
	Wire.write(prs_cfg);
	Wire.write(temp_config);
	Wire.endTransmission();

	// Set continuous reading for barometer and temperature
	Wire.beginTransmission(bht_sensor); // 0x77 -> addresse de barometer
	int meas_cfg = 0x08;
	Wire.write(meas_cfg);
	Wire.write(0b00000111);
	Wire.endTransmission();


	// Read calibration coefficients
	uint8_t calib_coeffs[18];
	readData(bht_sensor, 0x10, 18, calib_coeffs);

	c0 = ((uint32_t)calib_coeffs[0] << 4) | (((uint32_t)calib_coeffs[1] >> 4) & 0x0F);
	comp2(&c0, 12);
	c1 = (((uint32_t)calib_coeffs[1] & 0x0F) << 8) | (uint32_t)calib_coeffs[2];
	comp2(&c1, 12);

	c00 = (((uint32_t)calib_coeffs[3] << 12) | ((uint32_t)calib_coeffs[4] << 4) | (((uint32_t)calib_coeffs[5] >> 4)) & 0x0F);
	comp2(&c00, 20);
	c10 = ((uint32_t)calib_coeffs[5] & 0x0F) << 16 | (uint32_t)calib_coeffs[6] << 8 | (uint32_t)calib_coeffs[7];
	comp2(&c10, 20);
	c01 = (uint32_t)calib_coeffs[8] << 8 | (uint32_t)calib_coeffs[9];
	comp2(&c01, 16);
	c11 = (uint32_t)calib_coeffs[10] << 8 | (uint32_t)calib_coeffs[11];
	comp2(&c11, 16);
	c20 = (uint32_t)calib_coeffs[12] << 8 | (uint32_t)calib_coeffs[13];
	comp2(&c20, 16);
	c21 = (uint32_t)calib_coeffs[14] << 8 | (uint32_t)calib_coeffs[15];
	comp2(&c21, 16);
	c30 = (uint32_t)calib_coeffs[16] << 8 | (uint32_t)calib_coeffs[17];
	comp2(&c30, 16);

	//Serial.printlnf("Coeff: %d, %d, %d, %d, %d, %d, %d, %d, %d", c0, c1, c00, c10, c01, c11, c20, c21, c30);
}

// Read, compute and store a reading from barometer sensor
void getValuesBarometer() {

	// Read barometer
	int psr_b2 = 0x00;
	uint8_t barometerValues[3];
	readData(bht_sensor, psr_b2, 3, barometerValues);

	// Adjust pressure value from multiple registers to one number
	uint32_t pressureValue = (barometerValues[0] << 16) | (barometerValues[1] << 8) | barometerValues[2];
	int32_t pressureSigned = pressureValue;
	comp2(&pressureSigned, 24);

	// Adjust pressure value with scale factor
	int barometer_scale_fact = getScaleFact(0x06);
	float pressureSignedF = pressureSigned;
	pressureSignedF /= barometer_scale_fact;


	// Read temperature
	int tmp_b2 = 0x03;
	uint8_t temperature_values[3];
	readData(bht_sensor, tmp_b2, 3, temperature_values);

	// Adjust temperature value from multiple registers to one number
	uint32_t temperature_value = (temperature_values[0] << 16) | (temperature_values[1] << 8) | temperature_values[2];
	int32_t tempSigned = temperature_value;
	comp2(&tempSigned, 24);

	// Adjust temperature value with scale factor
	int temperature_scale_fact = getScaleFact(0x07);
	float tempSignedF = tempSigned;
	tempSignedF /= (float)temperature_scale_fact;


	// Compute final compensated values
	float p_comp = c00 + pressureSignedF * (c10 + pressureSignedF * (c20 + pressureSignedF * c30)) + tempSignedF * (c01 + pressureSignedF * (c11 + pressureSignedF * c21));
	float t_comp = c0*0.5 + c1*tempSignedF;

	// Change units to kPa
	float p_comp_kPa = p_comp / 1000.0;
	pressureCurrentValue = p_comp_kPa;

	// printFloat("Final pressure p_comp (kPa): ", p_comp_kPa);
	// printFloat("Final temperature t_comp: ", t_comp);
	
}

// Read, compute and store a reading from humidity sensor
void getValuesHumidity() {
	int dhtPin = humidSensor;

	// Put pin high to send signal after
	digitalWrite(dhtPin, HIGH);
	delay(250);

	// Remove interrupts to ensure correct timings
	noInterrupts();

	// Start signal
	digitalWrite(dhtPin, LOW);
	delay(20);
	digitalWrite(dhtPin, HIGH);
	delayMicroseconds(40);

	// Read GHT acknowledgment to start communication
	pinMode(humidSensor, INPUT);
	pulseIn(dhtPin, HIGH);

	// Get data
	int pulseTimes[40];
	for (int i = 0; i < 40; i++) {
		pulseTimes[i] = pulseIn(dhtPin, HIGH); // Do not execute anything in loop (will throw off timing)
	}
	interrupts();

	pinMode(humidSensor, OUTPUT);
	digitalWrite(dhtPin, LOW);

	// Check pulse lengths saved and add correct bits into correct arrays
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
	int humid = dataBits[0];
	int humidDecimal = dataBits[1];
	int temp = dataBits[2];
	int tempDecimal = dataBits[3];

	// Check checksum, if wrong, dont update
	int checkSum = dataBits[0] + dataBits[1] + dataBits[2] + dataBits[3];
	if ((checkSum & 0b11111111) != dataBits[4]) {
		// printDec("CHECKSUM ERROR: Checksum obtained: ", checkSum & 0b11111111);
		// printDec("CHECKSUM ERROR: Checksum expected: ", dataBits[4]);
		Serial.println("CHECKSUM ERROR: humidity sensor");
	}
	else {
		humidityCurrentValue = humid;
		humidityDecimalCurrentValue = humidDecimal;
		temperatureCurrentValue = temp;
		temperatureDecimalCurrentValue = tempDecimal;
	}

	// printFloat("*DHT11* Temperature value: ", temp);
	// printFloat("*DHT11* Humidity value: ", humid);
}

// Read, compute and store a reading from wind direction sensor
void getValuesWindDirection() {
	int voltage_read = analogRead(windSensor) * (5000/4096.0);

	int voltage_array[16] = {3840, 1980, 2250, 410, 450, 320, 900, 620, 1400, 1190, 3080, 2930, 4620, 4040, 4330, 3430};

	// Find predefined voltage that is closest and save index
	int closest_ind = 0;
	int smallest_delta = 99999;
	for (int i = 0; i < 16; i++) {
		int delta = abs(voltage_array[i] - voltage_read);
		if (delta < smallest_delta) {
			smallest_delta = delta;
			closest_ind = i;
		}
	}

	// Get direction for index found
	float direction_array[16] = {0, 22.5, 45, 67.5, 90, 112.5, 135, 157.5, 180, 202.5, 225, 247.5, 270, 292.5, 315, 337.5};
	float direction_deg = direction_array[closest_ind];

	windDirectionCurrentValue = direction_deg;
}

// Read, compute and store a reading from wind speed sensor
void windSpeedEvent() {

	int current_millis = millis();

	// Check if first time having a windSpeedEvent (or timer was reset)
	if (lastWindSpeedEventTime != 0) {
		int time_since_last_event = current_millis - lastWindSpeedEventTime;

		// Ignore bounce
		if (time_since_last_event < 20) {
			return;
		}

		// Calculate wind speed in km/h
		float wind_speed = (1000.0 / (float)time_since_last_event) * 2.4;

		windSpeedCurrentValue = wind_speed;
	}

	lastWindSpeedEventTime = current_millis;
}

// Read, compute and store a reading from anemometer sensor (rainfall)
void rainEvent() {

	int current_millis = millis();
	
	// Check if first time having a rainEvent (or timer was reset)
	if (lastRainEventTime != 0){

		int time_since_last_event = current_millis - lastRainEventTime;

		// Ignore bounce
		if (time_since_last_event < 600) {
			return;
		}

		// printDec("Time since last rain event: ", time_since_last_event);

		// Calculate rain in mm/min from last time to current time
		float rain_rate = 0.2794 / time_since_last_event * 1000 * 60;

		rainCurrentValue = rain_rate;
		// Serial.println(rainCurrentValue);
	}

	lastRainEventTime = current_millis;
}

// Read, compute and store a reading from light sensor
void getValuesLight() {
	float vIn = analogRead(lightSensor) * 3.3 / 4096.0;
	Serial.printlnf("Light sensor: %f V", vIn);

	float resistor = 68000.0;

	// float vcc = 3.3;
	// float satV = 2;
	// float maxV = vcc - satV;

	// float lux = vIn / maxV * 100;

	float b = 10.0 * pow(10, -9);
	float m = (100.0 - 0.0) / (239.5*pow(10, -6) - b);
	float currentValue = (float)vIn / resistor;
	float lux = currentValue*m + b;

	luminosityCurrentValue = lux;
	Serial.printlnf("Light sensor: %f lux", lux);
}

// Handles enabling and disabling WIFI for sending data
void sendData(){

	Serial.println("WiFi on");

	sendPost();

	// Turn WiFi off
	Particle.disconnect();
	WiFi.off();
	Serial.println("WiFi off");
}

void resetInterruptTimes(){
	int currentTimeMs = millis();

	// Check if rain event happened more than one hour ago
	if (currentTimeMs - lastRainEventTime >  (1*1000*60*60)){
		lastRainEventTime = 0;
	}
	// Check if wind event happened more than one hour ago
	if (currentTimeMs - lastWindSpeedEventTime >  (1*1000*60*60)){
		lastWindSpeedEventTime = 0;
	}

}

// Main program loop
void loop() {

	// Sensor polling delay
	delay(1000);

	// Main loop to get values from sensors that are not on interrupts
	getValuesLight();
	getValuesBarometer();
	getValuesHumidity();
	getValuesWindDirection();

	// Reset sensors depending on time (windSpeed, rain) if last time was very long ago
	resetInterruptTimes();


	// Every x seconds (10 or 60), connect to server through WiFi and send JSON data
	time_t currentTime = Time.now();
	if (locationAcquired){
		if (currentTime - lastWifiPostTime > 10) {

			// Activate WiFi module
			WiFi.on();
			Particle.connect();
			Serial.println("WiFi starting");

			// When WiFi is ready, send Data. If not, keep looping.
			if (WiFi.ready()){
				sendData();
				lastWifiPostTime = currentTime;
			}
		}
	}
	else {
		// If location not yet acquiered, keep "giving time" to locator
		locator.loop();
	}
	
}