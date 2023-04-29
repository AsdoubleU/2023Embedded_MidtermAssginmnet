#include "mbed.h"
#include "Adafruit_SSD1306.h"

I2C i2c(I2C_SDA, I2C_SCL);
Adafruit_SSD1306_I2c myOled(i2c,D4,0x78,32,128);
DigitalOut led(D4);
PwmOut buz(D2);
PwmOut rcServo(D3);
Ticker tic,led_tic;

void turn(PwmOut &rc, float deg);
void blink();

enum status { UP, DOWN, EXTEND, SHRINK };
static const char *stat_str[] = { "UP          ","DOWN      ", "EXTEND    ", "SHRINK    " };

class Debounce
{
    InterruptIn btn;
    Timeout tmo;
    int interval;
    int state,ready;

    void btn_isr()
    {
        if(ready) { ready = 0; tmo.attach_us(callback(this,&Debounce::decide),interval);}
    }

    void decide() { state=0; ready=1; tmo.detach();}

    public:
    Debounce(PinName pin,int intv = 100000) : btn(pin)
    {
        btn.fall(callback(this,&Debounce::btn_isr));
        state=1; ready=1; interval=intv;
    }
    
    int read() { int _state = state; state = 1; return _state;}
    operator int(){ return read();}
};

int main(void)
{
    //// Initial declaration in the main function ////
    myOled.begin();
    Debounce btn(USER_BUTTON,100000);
    rcServo.period_ms(10);
    i2c.frequency(400000);
    rcServo.period_ms(10);

    //// Initial Settings ////
    enum status stat = SHRINK;
    float ang=0., inc=0;
    turn(rcServo,0);
    led = 0;

    //// loop function ////
    while(true){

        turn(rcServo,ang);

        if(!btn){
            buz.write(0.2);
            buz.period_us(100000/262);
            wait(0.1);
            buz.write(0);
            switch(stat)
            {
                case SHRINK: tic.attach(&blink,0.1);
                inc = 1; stat = UP; break;

                case EXTEND: tic.attach(&blink,0.1);
                inc = -1; stat = DOWN; break;

                case UP: 
                ang = 0; inc = 0; stat = SHRINK; tic.detach(); led = 0; break; 

                case DOWN: 
                ang = 180; inc = 0; stat = EXTEND; tic.detach(); led = 1; break;
            }
        }

        ang+=inc;
        wait_ms(20);

        myOled.clearDisplay();
        myOled.printf("%s\r",stat_str[stat]);
        myOled.display();
        if (ang>180.f){  tic.detach();led = 1; stat = EXTEND; inc = 0;}
        else if (ang<0.f){ tic.detach(); led = 0; stat = SHRINK; inc = 0;}

    } //// the end of the loop function
}

void turn(PwmOut &rc, float deg)
{
    float pulseW = 1000*(0.01*deg+0.6);
    rc.pulsewidth_us(pulseW);
}

void blink(){
    led = !led;
}