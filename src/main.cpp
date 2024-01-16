#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define stripPin 38 // Internal RGB LED panel
#define led1HighTimeMS 1000
#define led1LowTimeMS 5000
#define led2HighTimeMS 1500
#define led2LowTimeMS 3000

u_int count1 = 0;
u_int count2 = 0;
int outPin1 = 1; // External LED1
int outPin2 = 2; // External LED2
void blink(int pin, u_int highTime, u_int lowTime, u_int count);
uint16_t pixels;

void runRGBIllumination();
void rainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);
void rainbowCycle(uint8_t wait);
void theaterChaseRainbow(uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);

void blinkLed1Task(void *parameter);
void blinkLed2Task(void *parameter);

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, stripPin, NEO_GRB + NEO_KHZ800);

// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only one Task is accessing this resource at any time.
SemaphoreHandle_t mutex;

// Task functions
void blinkLed1Task(void *parameter)
{
  for (;;)
  {
    blink(outPin1, led1HighTimeMS, led1LowTimeMS, ++count1);
  }
}

void blinkLed2Task(void *parameter)
{
  for (;;)
  {
    blink(outPin2, led2HighTimeMS, led2LowTimeMS, ++count2);
  }
}

void runRGBIlluminationTask(void *parameter)
{
  for (;;)
  {
    runRGBIllumination();
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("Hello from ESP32-S3!");

  pinMode(outPin1, OUTPUT);
  pinMode(outPin2, OUTPUT);

  // Create a mutex semaphore which we will use to manage the Serial Port
  mutex = xSemaphoreCreateMutex();

  strip.begin();
  strip.setBrightness(10);
  strip.show(); // Initialize all pixels to 'off'

  // Create tasks for blink and runRGBIllumination
  xTaskCreate(runRGBIlluminationTask, "Illumination", 100000, NULL, 1, NULL);
  xTaskCreate(blinkLed1Task, "Blink LED 1", 1000, NULL, 3, NULL);
  xTaskCreate(
      blinkLed2Task, // Task function
      "Blink LED 2", // Name of task
      1000,          // Stack size of task
      NULL,          // Parameter of the task
      3,             // Priority of the task
      NULL);         // Task handle to keep track of created task
}

void loop()
{
  // Empty. All tasks are running in FreeRTOS.
}

void blink(int pin, u_int highTime, u_int lowTime, u_int count)
{
  // Only print if we can take the mutex
  if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE)
  {
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

void runRGBIllumination()
{
  // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(255, 0, 0), 50);     // Red
  colorWipe(strip.Color(0, 255, 0), 100);    // Green
  colorWipe(strip.Color(0, 0, 255), 200);    // Blue
  colorWipe(strip.Color(0, 0, 0, 255), 100); // White RGBW
  //  Send a theater pixel chase in...
  // theaterChase(strip.Color(127, 127, 127), 50); // White
  // theaterChase(strip.Color(127, 0, 0), 50);     // Red
  // theaterChase(strip.Color(0, 0, 127), 50);     // Blue

  rainbow(20);
  rainbowCycle(20);
  theaterChaseRainbow(50);
}

#pragma region RGB Illumination
void colorWipe(uint32_t c, uint8_t wait)
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
    strip.show();
    vTaskDelay(pdMS_TO_TICKS(wait));
  }
}

void rainbow(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256; j++)
  {
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    vTaskDelay(pdMS_TO_TICKS(wait));
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++)
  { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    vTaskDelay(pdMS_TO_TICKS(wait));
  }
}

// Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait)
{
  for (int j = 0; j < 10; j++)
  { // do 10 cycles of chasing
    for (int q = 0; q < 3; q++)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, c); // turn every third pixel on
      }
      strip.show();

      vTaskDelay(pdMS_TO_TICKS(wait));

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait)
{
  for (int j = 0; j < 256; j++)
  { // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, Wheel((i + j) % 255)); // turn every third pixel on
      }
      strip.show();

      vTaskDelay(pdMS_TO_TICKS(wait));

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
#pragma endregion