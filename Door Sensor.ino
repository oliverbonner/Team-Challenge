//SYSTEM_MODE(MANUAL);

int led = D7;
int sensor = D3;
volatile int door_state = LOW;
char publishString[255];

void setup() 
{
    pinMode(led, OUTPUT);
    pinMode(sensor, INPUT);
}

void loop()
{
    //when we turn on or finish publishing data, go to sleep
    System.sleep(sensor, CHANGE);
    
    //when we wake up check the sensor and (in debug mode) write to the LED
    wakeup();
    digitalWrite(led, door_state);
    
    //connect to the cloud, we need to wait to be connected before publishing
    Spark.connect();
    while(!Spark.connected())
    {
        Spark.process();
        delay(50);
    }
    delay(500);
    
    //publish the door state
    publish_data();
}

void wakeup(void)
{
    if(digitalRead(sensor) == 1)
    {
        door_state = HIGH;
    }
    else
    {
        door_state = LOW;
    }
}

void publish_data(void)
{
    sprintf(publishString,"door_state: %d", door_state);
    Spark.publish("Door State", publishString);
    //delay(PUBLISH_DELAY);
}
