#include <Arduino.h>
#include <EEPROM.h>


#define SAMPLING_PERIOD 1000 // Sampling pediod to take measurements, miliseconds
#define PUMP_WATERING_TIME 7000  // Miliseconds
#define WATER_SENSOR_STARTUP_TIME 5000  // Miliseconds
#define PUMP_SPEED_PWM 240 // Fine tune from 0 to 255
#define SECONDS_IN_A_DAY 86400
#define N_NIGHT_DETECTOR_SAMPLING_STEPS 5  // Sampling steps to assume is night
#define N_DAYS_WATERING_PERIOD  7
#define NO_WATER_SAMPLING_STEPS 600  // 1800 // 30 min * 60 s
#define LEDS_FADE_IN_DELTA_T_MILISECONDS 150
#define LEDS_FADE_IN_DELTA_INCREMENT 3

#define WATER_SENSOR_IN 2   // PD2
#define LIGHT_SENSOR_IN 6   // PD6
#define WATER_PUMP_OUT 3   // PD3
#define WATER_SENSOR_POWER 4 // PD4
#define LEDS_OUT 5   // PD5


// Variables globales
unsigned char sensor_value_holder = 0;
unsigned char out_of_water = 0;
unsigned char failed_watering_attempt = 0;
unsigned char blink_led_stauts = LOW;
unsigned char lights_state = 0;
unsigned char night_samples_counter = 0;
unsigned char days_counter = 0;
unsigned long seconds_counter = 0;
unsigned char no_water_samples_counter = 0;
unsigned char eeAddress = 0;

void setup() {
  // Outputs
  pinMode(WATER_PUMP_OUT, OUTPUT);      // Water Pump
  pinMode(LEDS_OUT, OUTPUT);            // LEDs
  pinMode(WATER_SENSOR_POWER, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(WATER_PUMP_OUT, LOW);   // 
  analogWrite(WATER_PUMP_OUT, 0);

  digitalWrite(LEDS_OUT, LOW);   // 
  analogWrite(LEDS_OUT, 0);

  digitalWrite(WATER_SENSOR_POWER, LOW);   // 


  // Inputs
  pinMode(WATER_SENSOR_IN, INPUT);     // Water sensor
  pinMode(LIGHT_SENSOR_IN, INPUT);      // Light sensor
  
  // Recover variable days_counter from EEPROM
  Serial.begin(9600);           //  setup serial
  delay(5000);
  Serial.println("Recuperando variable days_counter desde la EEPROM");
  eeAddress = 0; //EEPROM address to start reading from
  EEPROM.get(eeAddress, days_counter);
  Serial.println(days_counter);
}

void loop() {
  /* Prueba 1: Medir presencia de agua con sensor
  water_sensor_bol = digitalRead(WATER_SENSOR_IN);
  if(!water_sensor_bol){ // La salida del sensor de agua esta invertida
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  */
  
  /*
  // PRUEBA 2: Bomba con potenciometro
  water_sensor_bol = digitalRead(WATER_SENSOR_IN);
  if(!water_sensor_bol){  // La salida del sensor de agua esta invertida
    analogWrite(WATER_PUMP_OUT, PUMP_SPEED_PWM);
  } else {
    analogWrite(WATER_PUMP_OUT, 0); // Apagar bomba si no hay agua
  }
  delay(PUMP_SAMPLING_PERIOD);
  */

  /*
  // Prueba 3: Probando LEDs
  analogWrite(LEDS_OUT, 60);
  delay(1000);
  analogWrite(LEDS_OUT, 255);
  delay(1000);
  */

  // Verificacion diaria de agua en el tanque y riego de la planta
  if (seconds_counter >= SECONDS_IN_A_DAY) {
    Serial.println(F("Verificacion diaria de agua en el tanque y riego de la planta"));
    //Serial.println(sizeof(sensor_value_holder));
    //Serial.println(sizeof(seconds_counter));
    days_counter = days_counter + 1;
    seconds_counter = 0;
    eeAddress = 0;
    EEPROM.put(eeAddress, days_counter);
    Serial.println("Guardando valor de variable days_counter en la EEPROM");
    Serial.println(days_counter);

    // Mirar si tengo agua en el tanque 
    // Energizar el sensor de liquido
    digitalWrite(WATER_SENSOR_POWER, HIGH);  
    delay(WATER_SENSOR_STARTUP_TIME);

    // Leer sensor de liquido
    sensor_value_holder = digitalRead(WATER_SENSOR_IN);
    if (!sensor_value_holder) { // La salida del sensor de agua esta invertida
      out_of_water = 0;
      Serial.println(F("Se detecto agua en la verificacion del dia"));
    } else {
      Serial.println(F("No se detecto agua en la verificacion del dia"));
      out_of_water = 1;
      //analogWrite(WATER_PUMP_OUT, 0); // Apagar bomba si no hay agua
    }
    
    // Desenergizar el sensor de liquido
    digitalWrite(WATER_SENSOR_POWER, LOW);   

    // Verificar si ya es dia de riego de la planta
    if(days_counter >= N_DAYS_WATERING_PERIOD){  
      Serial.println(F("Es hora de regar la planta"));
      // Es hora de regar la planta
      if (out_of_water == 0) {
        Serial.println(F("Se riega la planta"));
        // Si ya es momento de regar, entonces energizar la bomba
        analogWrite(WATER_PUMP_OUT, PUMP_SPEED_PWM);
        delay(PUMP_WATERING_TIME);
        analogWrite(WATER_PUMP_OUT, 0); // Apagar bomba
        days_counter = 0;               // Reiniciar el contador de dias
      } else {
        Serial.println(F("No se riega la planta porque no hay agua"));
        failed_watering_attempt = 1;
      }
    } 
  }

  // Si no se detecto agua en el tanque verificar que se vuelva a llenar para apagar la senal de luces
  if (out_of_water == 1) {
    // Controlar el blinking LED
    if (lights_state == 0) {
      analogWrite(LEDS_OUT, 255);
      delay(SAMPLING_PERIOD);
      analogWrite(LEDS_OUT, 0);
    }
    else {
      analogWrite(LEDS_OUT, 0);    
      delay(SAMPLING_PERIOD);
      analogWrite(LEDS_OUT, 255);
    }
    seconds_counter = seconds_counter + 1; // Tomar en cuenta el segundo que me tome en pestanear las luces
    

    // Verificar numero de muestras para energizar el sensor de liquido y mirar si ya hay agua
    no_water_samples_counter = no_water_samples_counter + 1;
    if (no_water_samples_counter >= NO_WATER_SAMPLING_STEPS) {
      no_water_samples_counter = 0; // Reiniciar el contador
      
      // Energizar el sensor de liquido
      digitalWrite(WATER_SENSOR_POWER, HIGH);   
      delay(WATER_SENSOR_STARTUP_TIME);

      // Leer sensor de liquido
      sensor_value_holder = digitalRead(WATER_SENSOR_IN);
      if(!sensor_value_holder){  // La salida del sensor de agua esta invertida
        out_of_water = 0;     // Tanque con agua

        if (failed_watering_attempt == 1) {
          // Regar la planta inmediatamente, ya que para entrar aqui tuvo que haber intentado regar la planta antes sin exito
          Serial.println(F("Se riega la planta inmediatamente despues de proveer agua al tanque"));
          delay(PUMP_WATERING_TIME); // Esperar un tiempo prudencial para que el usuario (Jeni) llene el tanque
          analogWrite(WATER_PUMP_OUT, PUMP_SPEED_PWM);
          delay(PUMP_WATERING_TIME);
          analogWrite(WATER_PUMP_OUT, 0); // Apagar bomba
          days_counter = 0;               // Reiniciar el contador de dias para regar la planta
          failed_watering_attempt = 0;
        }
      }
      
      // Desenergizar el sensor de liquido
      digitalWrite(WATER_SENSOR_POWER, LOW);   
    }
  }
    
  
  // Verificar si es de noche
  sensor_value_holder = digitalRead(LIGHT_SENSOR_IN);
  //Serial.println(sensor_value_holder);
  if (sensor_value_holder) {
    // Existe un bajo nivel de luz
    //Serial.println(F("Existe un bajo nivel de luz"));
    if (lights_state == 0) {
      Serial.println(F("Existe un bajo nivel de luz, luces apagadas"));
      night_samples_counter = night_samples_counter + 1;
      if (night_samples_counter >= N_NIGHT_DETECTOR_SAMPLING_STEPS) {
        Serial.println(F("Iniciar encendido de luces"));
        for (unsigned char duty_cycle=0; duty_cycle < 240; duty_cycle+=LEDS_FADE_IN_DELTA_INCREMENT) {
          analogWrite(LEDS_OUT, duty_cycle);
          delay(LEDS_FADE_IN_DELTA_T_MILISECONDS);
          Serial.println(duty_cycle);
        }
        analogWrite(LEDS_OUT, 255);
        lights_state = 1;
        night_samples_counter = 0;
      } 
    } 
  } else {
    // Existe un alto nivel de luz
    //Serial.println(F("Existe un alto nivel de luz"));
    if (lights_state == 1) {
      Serial.println(F("Existe un alto nivel de luz, luces encendidas"));
      night_samples_counter = night_samples_counter + 1;
      if (night_samples_counter >= N_NIGHT_DETECTOR_SAMPLING_STEPS) {
        // Entonces apago las luces
        Serial.println(F("Apagar luces"));
        analogWrite(LEDS_OUT, 0);
        night_samples_counter = 0;
        lights_state = 0;
      }
    }
  }

  
  delay(SAMPLING_PERIOD);
  seconds_counter = seconds_counter + 1;

}

void reportStatusSerial(){
  Serial.println(F("--------------"));
  Serial.println(F("Estado actual:"));
}
