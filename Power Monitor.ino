// -----------------
// This software reads the oscillating voltage from two Hall effect sensors (one with 5A range, the other 20A)
// Peak detection is performed on one oscillation of the 50Hz mains wave, and the amplitude is calculated
// Finally, a conversion to power in Watts is performed and published at 1Hz to the cloud
//
// Oliver Bonner 2015
// ob298@cam.ac.uk
// -----------------
#include "math.h"

#define WAVE_AVERAGING_SAMPLES 10
#define WAVE_SAMPLE_TIME 250
#define ON_THRESHOLD 50
#define SENSOR_SWITCH 500
#define PUBLISH_DELAY 1000
#define VOLTAGE_OFFSET_5A 3099
#define VOLTAGE_OFFSET_20A 2060

int amplitude = 0;
int amplitude_20A = 0;
int amplitude_5A = 0;
int RMS_5A = 0;
int RMS_20A = 0;
int min_5A = 0;
int min_20A = 0;
int max_5A = 0;
int max_20A = 0;
int _power_indication = 0;
char publishString[255];
int maximum_value = 0;
int _power_value = 0;
int square_array_5A[250];
int square_array_20A[250];

int test = 0;

void setup()
{
  // Register the variables for sending data to
  Spark.variable("test", &test, INT);
  Spark.variable("20A_Amp", &amplitude_20A, INT);
  Spark.variable("5A_Amp", &amplitude_5A, INT);
  Spark.variable("20A_RMS", &RMS_20A, INT);
  Spark.variable("5A_RMS", &RMS_5A, INT); 
  Spark.variable("Final_Amp", &amplitude, INT);
  Spark.variable("Power_Ind", &_power_indication, INT);
  Spark.variable("Power_Value", &_power_value, INT);

  // A0 is the analog in from the 20A Hall sensor
  pinMode(A0, INPUT);
  // A5 is the analog in from the 5A Hall sensor
  pinMode(A5, INPUT);
  //Digital Out for debugging
  pinMode(D0, OUTPUT);
  
  //Turn on serial port
  Serial.begin(9600);
}

void loop()
{
    //zero the sampling variables
    int reading_5A = 0;
    int reading_20A = 0;
    int max_average_5A = 0;
    int min_average_5A = 0;
    int max_average_20A = 0;
    int min_average_20A = 0;
    
    RMS_5A = 0;
    RMS_20A = 0;
    
    //continuously sample the analogue pin, A1.  Get the min and max values during that sample period.
    for(int j = 0; j < WAVE_AVERAGING_SAMPLES; j++)
    {
        //set the max too low and the min too high, so the next reading will set them both
        max_5A = 0;
        max_20A = 0;
        min_5A = 10000;
        min_20A = 10000;
        
        //take a sample of one 50Hz wave
        for(int i = 0; i < WAVE_SAMPLE_TIME; i++)
        {
            reading_5A = analogRead(A5);
            reading_20A = analogRead(A0);
            
            //zero the reading (using the voltage offset), then square it and add it to the array
            square_array_5A[i] = pow((reading_5A - VOLTAGE_OFFSET_5A), 2);
            square_array_20A[i] = pow((reading_20A - VOLTAGE_OFFSET_20A), 2);
            
            //find the min and max of the wave for both the 5A and 20A sensors
            if(reading_5A > max_5A)
            {
                max_5A = reading_5A;
            }
            if(reading_5A < min_5A)
            {
                min_5A = reading_5A;
            }
            
            if(reading_20A > max_20A)
            {
                max_20A = reading_20A;
            }
            if(reading_20A < min_20A)
            {
                min_20A = reading_20A;
            }
        }
        
        //keep a running total of the RMSs
        RMS_5A = RMS_5A + sqrt(mean_5A());
        RMS_20A = RMS_20A + sqrt(mean_20A());
        
        min_average_5A = min_average_5A + min_5A;        
    }

    //calculate the average RMS over the averaging period
    RMS_5A = RMS_5A / WAVE_AVERAGING_SAMPLES;
    RMS_20A = RMS_20A / WAVE_AVERAGING_SAMPLES;
    
    min_average_5A = min_average_5A / WAVE_AVERAGING_SAMPLES;    

    //if the 5A sensor is saturating (or close to), then use the 20A amplitude
    //otherwise use the 5A sensor for calculations (it's more sensitive)
    if(min_average_5A < SENSOR_SWITCH)
    {
        amplitude = RMS_20A;
        
        //convert ADC counts to a power (W) value
        _power_value = amplitude / 0.3249;
    }
    else
    {
        amplitude = RMS_5A;
        
        //convert ADC counts to a power (W) value
        _power_value = (amplitude - 7.4442) / 1.7656;
    }

    //test against the on/off threshold, then send out an event if it is on
    //set the front-facing LED accordingly
    //sprintf converts the INT to a string (you can only publish strings)
    if(amplitude > ON_THRESHOLD)
    {
        digitalWrite(D0, HIGH);
        
        if(_power_indication == 0)
        {
            _power_indication = 1;
        }
        
        publish_data();
    }
    else
    {
        digitalWrite(D0, LOW);
        
        _power_value = 0;
        
        if(_power_indication == 1)
        {
            _power_indication = 0;
            publish_data();
        }
    }
}

int mean_5A(void)
{
    int mean_total = 0;
    
    for(int i = 0; i < WAVE_SAMPLE_TIME; i++)
    {
        mean_total = mean_total + square_array_5A[i];
    }
    
    test = (mean_total / WAVE_SAMPLE_TIME);
    
    return (mean_total / WAVE_SAMPLE_TIME);
}

int mean_20A(void)
{
    int mean_total = 0;
    
    for(int i = 0; i < WAVE_SAMPLE_TIME; i++)
    {
        mean_total = mean_total + square_array_20A[i];
    }
    
    return (mean_total / WAVE_SAMPLE_TIME);
}

void publish_data(void)
{
    sprintf(publishString,"power_on_off: %d, power_value: %d",_power_indication, _power_value);
    Spark.publish("Power Indication", publishString);
    delay(PUBLISH_DELAY);
}
