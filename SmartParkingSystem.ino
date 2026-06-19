/*****************************************************************************************************************************
**********************************    Author  : Ehab Magdy Abdullah                      *************************************
**********************************    Linkedin: https://www.linkedin.com/in/ehabmagdyy/  *************************************
**********************************    Youtube : https://www.youtube.com/@EhabMagdyy      *************************************
******************************************************************************************************************************/

/*******************************************          Includes         *******************************************************/
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <ESP32Servo.h>

/*******************************************          Defines          *******************************************************/
#define TRIGGER_PIN         18
#define ECHO_PIN            19
#define GREEN_LED           32
#define BLUE_LED            26
#define RED_LED             25
#define PHOTORESISTOR_PIN   33
#define LIGHTS_PIN1         12
#define LIGHTS_PIN2         14
#define SERVO_MOTOR_PIN     13
#define BUTTON_PIN          21
#define MAX_NUMBER_OF_CARS  5

/*******************************************          Classes           *******************************************************/
WiFiClient espClient;
PubSubClient client(espClient);
Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
Servo myservo;

/*******************************************     Wifi & MQTT Server     *******************************************************/
const char* ssid = "2001";                        /* Your Wifi SSID */
const char* password = "19821968";                /* Your Wifi Password */
const char* mqtt_server = "test.mosquitto.org";   /* Mosquitto Server URL */

/*******************************************         Variables          *******************************************************/
uint8_t num_of_cars = 0;
uint8_t num_of_blocked_cars = 0;
char buffer[3];
uint16_t distance;
uint16_t sensorValue;
uint8_t lightIntensity;

/*******************************************    Setup Wifi Connection   *******************************************************/
void setup_wifi()
{ 
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED) 
    { 
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

/*******************************************    Recconect To MQTT      *******************************************************/
void reconnect() 
{ 
    while(!client.connected()) 
    {
        Serial.println("Attempting MQTT connection...");

        if(client.connect("ESPClient")) 
        {
            Serial.println("Connected");
            client.subscribe("Ehab/Parking/NumNoPlaceCars");
            client.subscribe("Ehab/Parking/CarsInParking");
        } 
        else 
        {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

/*******************************************           Setup            *******************************************************/
void setup()
{
    Serial.begin(115200);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(PHOTORESISTOR_PIN, INPUT);
    pinMode(LIGHTS_PIN1, OUTPUT);
    pinMode(LIGHTS_PIN2, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);

    myservo.attach(SERVO_MOTOR_PIN);
    myservo.write(90);

    setup_wifi(); 
    client.setServer(mqtt_server, 1883);

    digitalWrite(RED_LED, HIGH);
}

/*******************************************           loop            *******************************************************/
void loop()
{
    /* Reconnect if disconnection happens */
    if(!client.connected()) { reconnect(); }

    /* Read analog value of PhotoResistor */
    sensorValue = analogRead(PHOTORESISTOR_PIN); // Read the value from the photoresistor
    lightIntensity = map(sensorValue, 4095, 0, 100, 0); // Map the voltage to a percentage of light intensity

    /* Turn ON/OFF lights at Night/Day */
    if(lightIntensity < 90)
    {
        digitalWrite(LIGHTS_PIN1, HIGH);
        digitalWrite(LIGHTS_PIN2, HIGH);
    }
    else
    {
        digitalWrite(LIGHTS_PIN1, LOW);
        digitalWrite(LIGHTS_PIN2, LOW);
    }

    /* if button pressed -> open the way, a car leaving */
    if(HIGH == digitalRead(BUTTON_PIN) && num_of_cars > 0)
    {
        while(digitalRead(BUTTON_PIN));
        num_of_cars--;
        dtostrf(num_of_cars, 3, 0, buffer);
        client.publish("Ehab/Parking/CarsInParking", buffer, false);
        myservo.write(180);
        delay(3000);
        myservo.write(90);
    }
    else{ /* Nothing */ }

    /* Reading Distance */
    distance = ultrasonic.read();
    /* if a car wanna enter, see if there is a place in the parking */
    if(distance < 20)
    {
        if(num_of_cars == MAX_NUMBER_OF_CARS)
        {
            char buff[4];
            num_of_blocked_cars++;
            dtostrf(num_of_blocked_cars, 4, 0, buff);
            client.publish("Ehab/Parking/NumNoPlaceCars", buff, false);
            for(uint8_t i = 0 ; i < 6 ; i++)
            {
                digitalWrite(RED_LED, !digitalRead(RED_LED));
                delay(500);
            }
        }
        else
        {
            num_of_cars++;
            dtostrf(num_of_cars, 3, 0, buffer);
            client.publish("Ehab/Parking/CarsInParking", buffer, false);
            digitalWrite(BLUE_LED, HIGH);
            digitalWrite(RED_LED, LOW);
            delay(1500);
            myservo.write(180);
            digitalWrite(GREEN_LED, HIGH);
            digitalWrite(BLUE_LED, LOW);
            delay(3000);
            myservo.write(90);
            digitalWrite(RED_LED, HIGH);
            digitalWrite(GREEN_LED, LOW);
        }
    }
    else{ /* Nothing */ }
    delay(100);
}
