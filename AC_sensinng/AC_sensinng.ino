#define AC_SENSE_PIN 25  // H_PF

volatile uint32_t pulseCount = 0;
unsigned long lastCheck = 0;

void IRAM_ATTR pulseISR()
{
    pulseCount++;
}

void setup()
{
    Serial.begin(115200);

    pinMode(AC_SENSE_PIN, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(AC_SENSE_PIN),
        pulseISR,
        FALLING);

    Serial.println("AC Detection Test");
}

void loop()
{
    if (millis() - lastCheck >= 1000)
    {
        noInterrupts();
        uint32_t pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        if (pulses > 0)
        {
            Serial.println("AC ON");
        }
        else
        {
            Serial.println("AC OFF");
        }

        lastCheck = millis();
    }
}