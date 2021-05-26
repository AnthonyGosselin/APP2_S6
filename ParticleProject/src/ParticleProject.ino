#include <../lib/google-maps-device-locator/src/google-maps-device-locator.h>
#include <math.h>

//SYSTEM_MODE(SEMI_AUTOMATIC);

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
const int lightSensor = A0;
const int humidSensor = D2;
const int windSensor = A2;
const int windSpeedSensor = A3;
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
bool locationAcquiered = false;
time_t lastWifiPostTime = 0;

// Oversampling rate and scale factors
int scale_factors[8] = {524288, 1572864, 3670016, 7864320, 253952, 516096, 1040384, 2088960};

void setup() {
	WiFi.on();
	Particle.connect();
	Serial.begin(9600);

	waitFor(Serial.isConnected, 30000);
	locator.withSubscribe(locationCallback).withLocateOnce();
	// locator.withSubscribe(locationCallback).withLocatePeriodic(30);

	pinMode(lightSensor, OUTPUT);
	pinMode(humidSensor, OUTPUT);
	digitalWrite(humidSensor, HIGH); // DHT11 starts high

	attachInterrupt(windSpeedSensor, windSpeedEvent, RISING);
	attachInterrupt(rainSensor, rainEvent, RISING);

	Wire.begin();
	getBarometerSetup();

	Serial.println("Setup complete.");
}

void locationCallback(float lat, float lon, float accuracy) {

	locationAcquiered = true;
	_latitude = lat;
	_longitude = lon;
	_accuracy = accuracy;
}

void sendPost() {

	// Connect to server
	if(!client.connect(server, server_port)) {
		Serial.println("Server connection failed.");
		return;
	}

	Serial.println("New connection: creating POST");

	// Get geolocation
	geolocationCurrentValue = String(_latitude) + ", " + String(_longitude) + ", " + String(_accuracy);

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

	Serial.println("Post function done");
}


void readData(int device_addr, int reg, int num_bytes, uint8_t *registerValues) {
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
	Serial.printf("%.4f", value);
	Serial.println();
}

void comp2(int32_t *binary_input, int length)
{
	if (*binary_input > (pow(2, length - 1) - 1)) {
		*binary_input -= pow(2, length);
	}
}

int getScaleFact(int reg) {
	uint8_t oversample_register_data[1];
	readData(bht_sensor, reg, 1, oversample_register_data);
	int oversampling_rate = oversample_register_data[0] & 0b00000111;
	int scale_fact = scale_factors[oversampling_rate];

	return scale_fact;
}

void getBarometerSetup() {

	// Set pressure config
	Wire.beginTransmission(bht_sensor); // 0x77 -> addresse de barometer
	int temp_cfg = 0x06;
	Wire.write(temp_cfg);
	Wire.write(0b00000011);
	Wire.endTransmission();

	// Set temp config
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

void getValuesBarometer() {

	// Read barometer
	int psr_b2 = 0x00;
	uint8_t barometerValues[3];
	readData(bht_sensor, psr_b2, 3, barometerValues);

	uint32_t pressureValue = (barometerValues[0] << 16) | (barometerValues[1] << 8) | barometerValues[2];

	int32_t pressureSigned = pressureValue;
	comp2(&pressureSigned, 24);

	int barometer_scale_fact = getScaleFact(0x06);
	float pressureSignedF = pressureSigned;
	pressureSignedF /= barometer_scale_fact;


	// Read temperature
	int tmp_b2 = 0x03;
	uint8_t temperature_values[3];
	readData(bht_sensor, tmp_b2, 3, temperature_values);

	uint32_t temperature_value = (temperature_values[0] << 16) | (temperature_values[1] << 8) | temperature_values[2];

	int32_t tempSigned = temperature_value;
	comp2(&tempSigned, 24);

	int temperature_scale_fact = getScaleFact(0x07);
	float tempSignedF = tempSigned;
	tempSignedF /= (float)temperature_scale_fact;


	// Compute final compensated values
	float p_comp = c00 + pressureSignedF * (c10 + pressureSignedF * (c20 + pressureSignedF * c30)) + tempSignedF * (c01 + pressureSignedF * (c11 + pressureSignedF * c21));
	float t_comp = c0*0.5 + c1*tempSignedF;

	float p_comp_kPa = p_comp / 1000.0;
	// printFloat("Final pressure p_comp (kPa): ", p_comp_kPa);
	// printFloat("Final temperature t_comp: ", t_comp);

	pressureCurrentValue = p_comp_kPa;
}

void getValuesHumidity() {
	int dhtPin = humidSensor;

	noInterrupts();
	digitalWrite(dhtPin, HIGH);
	delay(250);

	// Start signal
	digitalWrite(dhtPin, LOW);
	delay(20);
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
	int humid = dataBits[0];
	int humidDecimal = dataBits[1];
	int temp = dataBits[2];
	int tempDecimal = dataBits[3];

	int checkSum = dataBits[0] + dataBits[1] + dataBits[2] + dataBits[3];
	if ((checkSum & 0b11111111) != dataBits[4]) {
		// printDec("CHECKSUM ERROR: Checksum obtained: ", checkSum & 0b11111111);
		// printDec("CHECKSUM ERROR: Checksum expected: ", dataBits[4]);
		Serial.println("CHECKSUM ERROR");
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

	// printDec("Voltage read: ", voltage_read);
	// printFloat("Final direction: ", direction_deg);

	windDirectionCurrentValue = direction_deg;
}

void windSpeedEvent() {

	int current_millis = millis();
	if (current_millis != 0) {
		int time_since_last_event = current_millis - lastWindSpeedEventTime;

		// Ignore bounce
		if (time_since_last_event < 20) {
			return;
		}

		//printDec("Wind speed new delta: ", time_since_last_event);

		// 2.4 km/h / s
		float wind_speed = (1000.0 / (float)time_since_last_event) * 2.4;
		printFloat("Current wind speed: ", wind_speed);

		windSpeedCurrentValue = wind_speed;
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

		lastRainEventTime = current_millis;
	}

}

void getValuesLight() {
	int result = analogRead(lightSensor);
	//Serial.printlnf("Light sensor: %d mV", result);
}

void sendData(){

	// WiFi.on();
	// WiFi.connect();
	// Particle.connect();
	// Serial.println("WiFi on");

	// while (!WiFi.ready()){
	// 	delay(100);
	// }
	sendPost();
	//delay(10000);

	// Particle.disconnect();
	// WiFi.disconnect();
	// WiFi.off();
	// Serial.println("WiFi off");
}


void loop() {

	delay(1000);

	getValuesLight();
	getValuesBarometer();
	getValuesHumidity();
	getValuesWindDirection();

	time_t currentTime = Time.now();
	if (locationAcquiered){
		if (currentTime - lastWifiPostTime > 10) {
			sendData();
			lastWifiPostTime = currentTime;
		}
	}
	else {
		locator.loop();
	}
	
}