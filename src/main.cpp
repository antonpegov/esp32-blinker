#include <Arduino.h>

#define led1HighTimeMS 1000
#define led1LowTimeMS 5000
#define led2HighTimeMS 1500
#define led2LowTimeMS 3000

u_int count1 = 0;
u_int count2 = 0;
int outPin1 = 1; // External LED1
int outPin2 = 2; // External LED2
void blink(int pin, u_int highTime, u_int lowTime, u_int count);

void blinkLed1Task(void *parameter);
void blinkLed2Task(void *parameter);

// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only one Task is accessing this resource at any
// time.
SemaphoreHandle_t mutex;

// Task functions
void blinkLed1Task(void *parameter) {
    for (;;) {
        blink(outPin1, led1HighTimeMS, led1LowTimeMS, ++count1);
    }
}

void blinkLed2Task(void *parameter) {
    for (;;) {
        blink(outPin2, led2HighTimeMS, led2LowTimeMS, ++count2);
    }
}

void runRGBIlluminationTask(void *parameter) {
    for (;;) {
        runRGBIllumination();
    }
}

void setup() {
    Serial.begin(9600);
    // Run timout for 1 second
    delay(10000);
    Serial.println("Serial port ready");

    pinMode(outPin1, OUTPUT);
    pinMode(outPin2, OUTPUT);

    // Create a mutex semaphore which we will use to manage the Serial Port
    mutex = xSemaphoreCreateMutex();

    // Create tasks for blink and runRGBIllumination
    xTaskCreate(blinkLed1Task, "Blink LED 1", 10000, NULL, 3, NULL);
    xTaskCreate(blinkLed2Task, // Task function
                "Blink LED 2", // Name of task
                10000,         // Stack size of task
                NULL,          // Parameter of the task
                3,             // Priority of the task
                NULL);         // Task handle to keep track of created task
}

void loop() {
    // Empty. All tasks are running in FreeRTOS.
}

void blink(int pin, u_int highTime, u_int lowTime, u_int count) {
    // Only print if we can take the mutex
    if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
        // String pinStr = pin == outPin1 ? "First" : "Second";
        // TODO: Can't print pin nimber
        // Serial.print(pinStr);
        Serial.println(" LED blinked " + String(count) + " times...");
        xSemaphoreGive(mutex);
    }

    digitalWrite(pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(highTime));
    digitalWrite(pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(lowTime));
}

#pragma endregion