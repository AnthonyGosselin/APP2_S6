/*
 * Project ParticleProject
 * Description:
 * Author:
 * Date:
 */

TCPClient client;
byte server[] = { 192, 168, 0, 106 };

int i = 0;

// setup() runs once, when the device is first turned on.
void setup() {
  Serial.begin(9600);
  // Put initialization like pinMode and begin functions here.

  waitFor(Serial.isConnected, 30000);
  Serial.println("Started");
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  Serial.printlnf("Counting: %d", i++);
  delay(1000);

  // POST Message
  if(client.connect(server, 3000))
  {
    // Print some information that we have connected to the server
    Serial.println("**********************************!");
    Serial.println("New POST Connection!");
    Serial.println("Connection OK!");
            
    // Send our HTTP data!
    // client.println("POST /submit.php HTTP/1.0");
    // client.println("Host: maker-io-iot.atwebpages.com");
    // client.println("Content-Type: application/x-www-form-urlencoded");
    // client.println("Content-Length: 9");
    // client.println();
    // client.print("number=10");

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