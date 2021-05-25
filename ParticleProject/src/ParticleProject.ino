#include <../lib/google-maps-device-locator/src/google-maps-device-locator.h>

TCPClient client;
byte server[] = { 192, 168, 1, 6 };

float _latitude;
float _longitude;
float _accuracy;


GoogleMapsDeviceLocator locator;

//int i = 0;

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
  
}

/*
GOOGLE MAPS LOCATION INTEGRATION:

#include <google-maps-device-locator.h>

GoogleMapsDeviceLocator locator;

void setup() {
  Serial.begin(9600);
  // Scan for visible networks and publish to the cloud every 30 seconds
  // Pass the returned location to be handled by the locationCallback() method
  locator.withSubscribe(locationCallback).withLocatePeriodic(30);
}

void locationCallback(float lat, float lon, float accuracy) {
  // Handle the returned location data for the device. This method is passed three arguments:
  // - Latitude
  // - Longitude
  // - Accuracy of estimated location (in meters)
  Serial.printlnf("Current location: %f, %f (%f)", lat, lon, accuracy);
}

void loop() {
  locator.loop();
}
        
*/