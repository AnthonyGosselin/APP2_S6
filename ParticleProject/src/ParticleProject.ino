/*
 * Project ParticleProject
 * Description:
 * Author:
 * Date:
 */

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
}