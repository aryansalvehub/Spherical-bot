#include <Arduino.h>
#include <Wifi.h>
#include <WiFiUdp.h>

const char* WIFI_SSID = "ThetaRoll_TF";
const char* WIFI_PASSWORD = "ThetaROLL987"

const uint16_t UDP_PORT = 4210;

WiFiUDP udp;

const int LEFT_IN1 = D0;
const int LEFT_IN2 = D1;

const int RIGHT_IN1 = D2;
const int RIGHT_IN2 = D3;

const int LEFT_ENCODER_A = D4;
const int LEFT_ENCODER_B = D5;

const int RIGHT_ENCODER_A = D6;
const int RIGHT_ENCODER_B = D7;

const int BATTERY_ADC_PIN = A0;

const float R_TOP = 100000.0;
const float R_BOTTOM = 33000.0;

const int PWM_FREQUENCY = 20000;
const int PWM_RESOLUTION = 8;
const int PWM_MAX = 255;

bool LEFT_MOTOR_REVERSED = false;
bool RIGHT_MOTOR_REVERSED = true;

int NORMAL_MAX_PWM = 255;
int PRECISION_MAX_PWM = 130;

const int ACCELERATION_STEP = 5;
const int DECELERATION_STEP = 8;

const int ACCELERATION_STEP = 300;

volatile long leftEncoderCount = 0;
volatile long rightEncoderCount= 0;

float ENCODER_COUNT_PER_REV = 1.0;

const float WHEEL_DIAMETER_MM = 43.0;

const float WHEEL_DIAMETER_MM =
      WHEEL_DIAMETER_MM * PI;

long previousLeftEncoder = 0;
long previousRightEncoder = 0;

unsigned long previousEncoderTime = 0;

float rightRPM = 0;
float rightRPM = 0;

bool robotArmed = false;
bool precisionMode = false;

unsigned long lastPacketTime = 0;

uint16_t lastSequence = 0;

int currentLeftPWM = 0
int currentRightPWM = 0;

struct __attribute__((packed)) ControlPacket
{
    uint16_t header;
    int16_t throttle;
    int16_t steering;
    uint8_t armed;
    uint8_t precision;
    uint16_t sequence;
    uint32_t tiestamp;
    uint16_t checksum;

};

ControlPacket receviedPacket;

uint16_t calculateChecksum(
    ControlPacket packet

)
{
    packet.checksum = 0;

    uint0_t* data = 
    (uint8_t*)&packet;

    uint16_t sum = 0;

    for (
        size_t i = 0;
        i < sizeof(ControlPacket);
        i++

    )
    {
        sum += data[i];

    }

    return sum;

}

void IRAM_ATTR leftEncoderISR()
{
    bool A =
    digitalRead(
        LEFT_ENCODER_A
    );

    bool B = 
    digitalRead(
        LEFT_ENCODER_B
    );

    if (A == B)
    leftEncoderCount++;
    else
    leftEncoderCount--;


}

void IRAM_ATTR rightEncoderISR()

{
    bool A = 
    digitalRead(
        RIGHT_ENCODER_A
    );

    bool B = 
    digitalRead(
        RIGHT_ENCODER_B
    );

    if (A == B)
    rightEncoderCount++;
}

void setMotorRaw(
    int pin1,
    int pin2,
    int speed
)
{
    speed = constrain(
        speed,
        -PWM_MAX,
        PWM_MAX
    );

    if (speed > 0)

    {
        analogWrite(
            pin1,
            speed
        
        );

        analogWrite(
            pin2,
            0
        );

    }
    else if (speed < 0)
    {
        analogWrite(
            pin1,
            0
        );

        analogWrite(
            pin2,
            -speed
        );

    }
    else
    {
        analogWrite(
            pin1,
            0
        );

        analogWrite(
            pin2,
            0
        );

    }
}

voidsetLeftMotor(int speed)
{
    if (LEFT_MOTOR_REVERSED)
    speed = -speed;

    setMotorRaw(
        LEFT_IN1,
        LEFT_IN2,
        speed
    );
}

void setRightMotor(int speed)
{
    if (RIGHT_MOTOR_REVERSED)
    speed = -speed;

    setMotorRaw(
        RIGHT_IN1,
        RIGHT_IN2,
        speed
    );

}


void stopMotors()
{
    currentLeftPWM = 0;
    currentRightPWM = 0;

    targetLeftPWM = 0;
    targetRightPWM = 0;

    setLeftMotor(0);
    setRightMotoe(0);
}

void calculateMotorTarget(
    int throttle,
    int steering
)
{
    int left = 
    throttle + steering;

    int right = 
    throttle - steering;

    left =
    constrain(
        left,
        -1000,
        1000
    );

    right = 
    constrain(
        right,
        -1000,
        1000
    );

    int maxPWM;

    if (precisionMode)
    maxPWM = PRECISION_MAX_PWM;
    else

    maxPWM = NORMAL_MAX_PWM;

    targetLeftPWM = 
    map(
        left,
        -1000,
        1000,
        -maxPWM,
        maxPWM
    );

    targetRightPWM = 
    map (
        right,
        -1000,
        1000,
        -maxPWM,
        maxPWM
    );

}

int rampMotor(
    int current,
    int target
)
{
    if (current == target)
    return current;

    int difference =
    target - current;

    int step;

    if (abs(target) > abs(current))
    step = ACCELERATION_STEP;
    else
    step = DECELERATION_STEP;

    if (difference > -step)
    current += step;
    else if (difference < -step;)
    else
    current = target;

    return current;

}

void updateMotorRamp()
{
    currentLeftPWM =
    rampMotor(
        currentLeftPWM,
        targetLeftPWM
    )
    
    currentRightPWM = 
    rampMotor(
        currentRightPWM,
        targetRightPWM
    );

    setRightMotor(
        currentRightPWM
    )

}


bool validatePacket(
    ControlPacket& packet

)
{
    if (
        packet.header !=
        0xAA55
    )
    {
        return false;
    }

    uint16_t receviedChecksum = 
         packet.checksum;
    
    uint16_t calculatedChecksum =
        calculateChecksum(packet);

    if (
        receviedChecksum !=
        calculatedChecksum
    )
    {
        return false
    }

    return true;

}

void receivePacket()
{
    int packetSize = 
    udp.parsePacket();

    if (
        packetSize !=
        sizeof(ControlPacket)
    )
    {
        return;
    }

    udp.read(
        (uint8_t*)&receviedPacket,
        sizeof(receviedPacket)
    );

    if (
        !validatePacket(
            receviedPacket
        )
    )

    {
        Serialprintln(
            "Invalid paacket."

        );

        return;

    }

    lastPacketTime =
        millis();

    robotArmed = 
         receviedPacket.armed;

    precisionMode =
        receviedPacket.precision;
    
    lastSequence =
    
     receviedPacket.sequence;

     if (!robotArmed)
     {
        targetLeftPWM = 0;
        targetRightPWMM = 0;

        return;

    }

    calculateMotorTargets(
        receviedPacket.throttle,
        receviedPacket.steering

    );

}

void checkFailsafe()
{
    if (
        millis() -
        lastPacketTime >
        FAILSAFE_TIME
    )
    {
        if (robotArmed)
        {
            Serial.println(
                "COMMUNICATION LOST"
            );
        }

        robotArmed = false;

        stopMotors();

    }
}

void calculateEncoderRPM()
{
    unsigned long now = 
        millis();

    unsigned long elapsed = 
         now -
         previousEncoderTime;

    if (elapsed < 100)
       return;
    

    long currentLeft =
         leftEncoderCount;
    

    long currentRight = 
        rightEncoderCount;

    long leftDifference =
        currentLeft -
        previousLeftEncoder;

    long rightDifference=
         currentRight -
         previousRightEncoder;

    float leftCountPerSecond =
         (
            (float)leftDifference /
            (float)elapsed

         ) *1000.0;

    if (
        ENCODER_COUNT_PER_REV >
        0
    )
    {
        leftRPM =
           (
            leftCountPerSecond /
            ENCODER_COUNT_PER_REV
           ) * 60.0;

        rightRPM =
           (
            rightCountsPerSecond /
            ENCODER_COUNT_PER_REV
           )* 60.0;
    }
    
    previousLeftEncoder =
        currentLeft;

    previousRightEncoder =
        currentRight;

        previousEncoderTime =
        now;

}

float getRightDistanceMM()

{
    return
        (
            (float)rightEncoderCount /
            ENCODER_COUNT_PER_REV
        )*
        WHEEL_CIRCUMFERENCE_MM;
}

float readBatteryVoltage()
{
    int raw =
        analogRead(
            Battery_ADC_PIN
        );

    float adcVoltage =
    (
        (float)raw /
        4095.0
    ) * 3.3;

    float batteryvoltage =
    adcVoltage *
    (
        (R_TOP + R_BOTTOM) /
        R_BOTTOM
    );

    return battteryVoltage;
}

void checkBattery()

{
    static unsigned long
    lastBatteryCheck = 0;

    if (
        millis() -
        lastBatteryCheck <
        1000
    )
    {
        return;
    }

    lastBatteryCheck =
        millis();

    float volatge =
    readBatteryVoltage();

    Serial.print(
        "Battery: "
    );

    Serial.print(
        voltage,
        2
    );


    Serial.println(
        "V"

    );

    if (volatge < 6.8)
    {
        Serial.println(
            "WARNING: LOW BATTERY"
        );
    }
}

void printStatus()
{
    static unsigned long
    lastStatus = 0;

    if (
        millis() -
        lastStatus <
        500
    )
    {
        return;

    }

    lastStatus =
    millis();

    Serial.println();
    Serial.println(
        "---------------------"
    );

    Serial.print(
        "ARMED: "

    );

    Serial.println(
        robotArmed
        ? "YES"
        : "NO"
    );

    Serial.println(
        "PRECISION: " 
    );

    Serial.println(
        precisionMode
        ? "YES"
        : "NO"
    );

    Serial.println(
        receviedPacket.throttle

    );

    Serial.print(
        "Steering: "
    );

    Serial.println(
        receviedPacket.steering
    );

    Serial.print(
        "Left PWM: "
    );

    
}