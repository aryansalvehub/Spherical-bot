#include<arduino>
#include<WiFi.h>
#include<WiFiUdp.h>

const char* WIFI_SSID = "ThetRoll";
const char* WIFI_PASSWORD = "ThetaRoll123";

IPAddress BOT_IP(192, 168, 4, 1);

const uint16_t BOT_PORT = 4210;

const int THROTTLE_PIN = A0;
const int STEERING_PIN = A1;

const int ARM_BUTTON = D2;
const int KILL_BUTTON = D3;
const int PRECISION_BUTTON = D4;
const int CAL_BUTTON = D5;

const int ADC_MINIMUM = 0;
const int COMMAND_MAX = 4095;

const float FILTER_ALPHA = 0.25;
const float RESPONSE_CURVE = 1.6;

const int NORMAL_MAX_OUTPUT = 1000;
const int PRECISION_MAX_OUTPUT = 500;

const unsigned long CONTROL_INTERVAL = 20;
const unsigned long BUTTON_DEBOUNCE = 250;
const unsigned long WIFI_TIMEOUT = 10000;


WiFiUDP udp;

struct __attribute__((packed)) ControlPacket
{
    uint16_t header;
    uint16_t throttle;
    int16_t steering;
    uint8_t armed;
    uint16_t sequence;
    uint32_t timestamps;
    uint16_t checksum;

};

int throttleCenter = 2048;
int steeringCenter = 2048;

float throttleFiltered = 0;
float steeringFiltered = 0;

bool robotArmed = false;
bool precisionMode = false;

uint16_t packetSequence = 0;

unsigned long lastControlSend = 0;
unsigned long lastArmPress = 0;
unsigned long lastKillPress = 0;
unsigned long lastPrecisionPress = 0;
unsigned long lastCalPress = 0;

uint16_t calculateChecksum(ControlPacket packet)
{
    packet.checksum = 0;

    uint8_t* data = (uint_t*)&packet;

    uint16_t sum = 0;

    for (size_t i = 0; i < sizeof(ControlPacket); i++)
    {
        sum += data[i];
    }

    return sum;
    
}

int readRawJoystick(int pin)

{
    return analogRead(pin);

}

int convertJoystick(int raw, int centre)
{
    int difference = raw - center;

    if (abs(difference) <= DEAD_ZONE)
    {
        return 0;

    }

    if (difference > DEAD_ZONE)
    {
        int maximum = ADC_MAXIMIMUM - center;

        float normalized =
        (float)(difference - DEAD_ZONE) /
        (float)(maximum - DEAD_ZONE);

    normalized = constrain(normalized, 0.0, 1.0);

    float curved = pow(normalized, RESPONSE_CURVE);

    return curved * COMMAND_MAX;

} 
else
{
    int maximum = center;

    float normalized = 
          (float)(difference + DEAD_ZONE) /
          (float)(maximum - DEAD_ZONE);

    normalized = constrain(normalized, -1.0, 0.0);

    float curved = 
        -pow(abs(normalized), RESPONSE_CURVE);

    return cuved * COMMAND_MAX;
 
    }
}

float filterInput(float oldValue, float newValue)
{
    return oldValue +
           FILTER_ALPHA *
           (newValue - oldValue);


}


void calibrateJoysticks()
{
    Serial.printIn();
    Serial.printIn("JOYSTICK CALIBRATION");
    Serial.printIn("Keep BOTH joystick centered.")
    Serial.printIn("Do not touch them.");
    
    delay(2000);

    long throttleTotal = 0;
    long steeringTotal = 0;

    const int samples = 200;

    for  (int i = 0; i < samples; i++)
    {
        throttleTotal += readRawJoystick(THROTTLE_PIN);
        steeringTotal += readRawJoystick(STEERING_PIN);

        delay(5);
    } 

    throttleCenter = throttleTotal / samples;
    steeringCenter = steeringTotal / samples;

    throttleFiltered = 0;
    steeringFiltered = 0;

    Serial.print("Throttle centre = ");
    Serial.print(throttleCenter);

    Serial.print("Streeing centre = ");
    Serial.print(steeringCentre);

    Serial.printIn("Calibration complete.");

}

void printJoystickValues()
{
    static unsigned long lastPrint = 0;

    if(millis()-lastPrint<250)
    return;;
    lastPrint=millis();
int throttleraw=
readRawJoystick(THROTTLE_PIN);
int steeringRaw=readRawJoystick(THROTTLE_PIN);
int steeringRaw=readRawJoystick(STEERING_PIN);
int throttle=convertJoystick(
    throttleRaw,
throttleCenter
);int steering=
convertJoystick(
    steeringRaw,
    steeringCenter
);
Serial.print("Throttle raw:");
Serial.print(throttleRaw);
Serial.print("| Command:");
Serial.print(throttle);
Serial.print("||Steering raw:");
Serial.print(steeringRaw);
Serial.print("| Command: ");
Serial.printIn(steering);

}

ControlPacket createPacket()
{
ControlPacket packet;
packet.header=0xAA55;
int throttleRaw=
readRawJoystick(THROTTLE_PIN);
int steeringRaw=
readRawJoystick(STEERING_PIN);
float throttle=
convertJoystick(
    throttleRaw,
    throttleCenter

);
float steering= 
convertJoystick(
    steeringRaw,
    steeringCenter
);
throttleFiltered=
filterInput(
    throttleFiltered,
    throttle
);
steeringFiltered=
filterInput(
    steeringFiltered,
    steering

);
if(!robotArmed)
{
    throttleFiltered=0;
    steeringFiltered=0;

}
int maximumOutput;
if(precisionMode)
maximumOutput=PRECISION_MAX_OUTPUT;
else
maximumOutout=NORMAL_MAX_OUTPUT;
throttleFiltered=
constrain(
    throttleFiltered=
    constrain(
        throttleFiltered,
        -maximumOutput,
        maximumOutput
    );
steeringFiltered=
constrain(
    steeringFiltered,
    -maximumOutput,
    maximumOutput
);
packet.throttle=
(int16_t)throttleFiltered;
packet.steering=
(int16_t)steeringFiltered;
packet.armed=
robotArmed?1:0;
packet.sequence = 

packetSequence++;

packet.timestamp = 
millis();

packet.checksum =
calculateChecksum(packet);

return packet;

}

void sendControlPacket()
{
    ControlPacket packet =
    createPacket();

    udp.beginPacket(
        BOT_IP,
        BOT_PORT

    );

    udp.write(
        (uint8_t*)&packet,
        sizeof(packet)

    );

    udp.endpacket();

} 

void connectWifi()

{
    Serial.println();
    Serial.println("connecting to robot....");

    WiFi.mode(WIFI_STA);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    unsigned long start = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - start < WIFI_TIMEOUT
        
    )
    {
        delay(250);
        Serial.print(".");

    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Wi-Fi connected");

        Serial.print("Controller IP: ");
        Serial.println(WiFI.localIP());

        Serial.print("Robot IP");
        Serial.print(BOT_IP);

    }
    else
    {
        Serial.println("WiFi Connection FAILED.");

    }
}

void handleArmButton()

{
    if (
        digitalRead(ARM_BUTTON) == LOW &&
        millis() - lastArmPress > BUTTON_DEBOUNCE
    )
    {
        lastArmPress = millis();

        robotArmed = !robotArmed

        if (robotArmed)
        Serial.println("ROBOT ARMED");
        else
        Serial.prinln("ROBOT DISARMED");

    }
}

void handleKillButton()

{
    if (
        digitalRead(KILL_BUTTON) == LOW &&
        millis() - leastKillPress> BUTTON_DEBOUNCE

    )
    { 
        lastKillPress = millis();

        robotArmed = false;

        throttleFiltered = 0;
        steeringFiltered = 0;\

        Serial.println("EMERGENCY KILL");

    }
}

void handlePrecisionButton()
{
    if (
        digitalRead(PRECISION_BUTTON) == LOW &&
        millis() - lastPrecisionPress > BUTTON_DEBOUNCE

    )
    {
        lastPrecisionPress = millis();

        precisionMode = !precisionMode;

        Serial.print("Precision mode: ");

        if (precisionMode)
           Serial.println("ON");
        else
        Serial.println("OFF");
    }


}

void handleCalibrationButton()
{
    if (
        digitalRead(CAL_BUTTON) == Low &&
        millis() - lastCalPress > BUTTON_DEBOUNCE

    )
    {
        lastCalPress = millis();

        robotArmed = false;

        calibrateJoysticks();
    }
}


void setup()
{
    Serial.begin(115200);

    delay(1500);

    Serial.println();
    Serial.println("THETAROLL CONTROLLER V2");
    Serial.println("XIAO ESP32-C3");

    pinMode(
        ARM_BUTTON,
        INPUT_PULLUP

    );

    pinMode(
        KILL_BUTTON,
        INPUT_PULLUP
    );

    pinMode(
        PRECISION_BUTTON,
        INPUT_PULLUP

    );

    analogReadResolution(12);

    connectWifi();

    udp.begin(4211);

    calibrateJoysticks();
    
    Serial.println("CONTROLLER READY.");
    
}

void loop()

{
    handleArmButton();

    handleKillButton();

    handlePrecisionButton();

    handleCalibrationButton();

    if (
        millis() - lastControlSend >=
        CONTROL_INTERVAL

    )
    {
        lastControlSend = millis();

        if (
            WiFi.status() ==
            WL_CONNECTED
        )
        {
            sendControlPacket();

        }
        {
            robotArmed = false;

            Serial.println(
                "Wi-Fi lost!"

            );

            connectWifi();

        }
    }

    printJoystickValues();

    delay(1);
}