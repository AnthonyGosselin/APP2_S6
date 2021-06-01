// APP2 - examen formatif pratique
// Philippe Mabilleau ing.
// mai 2021

// https://docs.particle.io/reference/device-os/firmware/#spi

// SPI slave simulation
static uint8_t rx_buffer[16];
static uint8_t tx_buffer[16];
static uint8_t canal = 0x00;
static uint32_t v =  0;
static uint32_t select_state = 0x00;
static uint32_t transfer_state = 0x00;


void onTransferFinished() {
    transfer_state = 1;
}

void onSelect(uint8_t state) {
    if (state)
        select_state = state;
}

void SPI_simulation_init() {
    for (int i = 0; i < sizeof(tx_buffer); i++)
      tx_buffer[i] = 0x55;
    SPI1.onSelect(onSelect);
    SPI1.begin(SPI_MODE_SLAVE, D5);     // CS (SS) pin of slave
    transfer_state = 0;
    SPI1.transfer(tx_buffer, rx_buffer, sizeof(rx_buffer), onTransferFinished); // https://docs.particle.io/reference/device-os/firmware/#transfer-
}

void SPI_simulation_exec() {
    while(transfer_state == 0);
//    if (SPI1.available() > 0) {
//        Serial.printf("Received %d bytes", SPI1.available());
//        Serial.println();
//        for (int i = 0; i < SPI1.available(); i++) {
//            Serial.printf("%02x ", rx_buffer[i]);
//        }
//    Serial.println();
//    }
    if(rx_buffer[0] == 0x02) {
        if(rx_buffer[1] == canal) {
            v = analogRead(A0);
            tx_buffer[0] = v / 256;
            tx_buffer[1] = v % 256;
            }
        }

    transfer_state = 0;
    SPI1.transfer(tx_buffer, rx_buffer, sizeof(rx_buffer), onTransferFinished);    
}

TCPServer server = TCPServer(9000);
TCPClient client;


/* executes once at startup */
void setup() {
    
    Serial.begin(9600);
    
    server.begin();
    
    IPAddress myIP = WiFi.localIP();
    Serial.println(myIP.toString().c_str());  // impression de l'adresse IP du module
    
    
    pinMode(D6, OUTPUT);
    
    // Initialisation du port SPI maitre 
    SPI.setClockSpeed(4, MHZ);
    SPI.begin(SPI_MODE_MASTER, A5); // CS (SS) pin of master
    digitalWrite(A5, HIGH);

    SPI_simulation_init();
    Serial.println("Init complete");
}

int loopCnt = 0;

/* executes continuously after setup() runs */
void loop() {
    Serial.println("Loop");
    int valeur = 0;
    
    
    
     while (1) {
        client = server.available();            // attente d'un client
          if (client.connected()) {             // Client connecte
                pinResetFast(A5);		        // selection du peripherique SPI1 (ss=0)
                valeur = SPI.transfer(0x02);    // envoi code = 0x02, reception MSB de la lecture
                valeur = (valeur * 256) + SPI.transfer(0x00); // envoi canal 0x00, reception de LSB de lecture
                pinSetFast(A5);			        // deselection du peripherique (ss=1)
            
            // pour la mise au point
            
                Serial.println("Transmissin lancee");
                Serial.printf(" Valeur = %04x", valeur);
                Serial.println();
            
                
                SPI_simulation_exec();	        // execution de la simulation du peripherique esclave
                server.println(valeur);         // envoi de la valeur lue
                //  (c'est en fait la valeur de la lecture precedente qui est envoyee)
                client.stop();		            // fermeture de la connexion
                delay(1000);
                
                if (loopCnt%2 == 0){
                    Serial.println("High");
                    digitalWrite(D6, HIGH);
                }
                else{
                    Serial.println("Low");
                    digitalWrite(D6, LOW);
                }
            
                loopCnt++;
        }
        
        delay(500);  // ces delais sont utiles pour avoir une fenetre permettant
        //  le telechargement d'un nouveau code sans retour au mode safe du module
        
        
  
    }
    
    
}
