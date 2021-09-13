
#include "PwmPin.h"

///////////////////

LedC::LedC(uint8_t pin, uint16_t freq){

  if(freq==0)
    freq=DEFAULT_PWM_FREQ;

  for(int nMode=0;nMode<LEDC_SPEED_MODE_MAX;nMode++){
    for(int nChannel=0;nChannel<LEDC_CHANNEL_MAX;nChannel++){
      for(int nTimer=0;nTimer<LEDC_TIMER_MAX;nTimer++){
        if(!channelList[nChannel][nMode]){
          
          if(!timerList[nTimer][nMode]){                                // if this timer slot is free, use it
            timerList[nTimer][nMode]=new ledc_timer_config_t;           // create new timer instance
            timerList[nTimer][nMode]->speed_mode=(ledc_mode_t)nMode;
            timerList[nTimer][nMode]->timer_num=(ledc_timer_t)nTimer;
            timerList[nTimer][nMode]->freq_hz=freq;
          
            int res=20;                                                 // find the maximum possible resolution
            while(getApbFrequency()/(freq*pow(2,res))<1)
              res--;      
              
            timerList[nTimer][nMode]->duty_resolution=(ledc_timer_bit_t)res;
            ledc_timer_config(timerList[nTimer][nMode]);
          }
          
          if(timerList[nTimer][nMode]->freq_hz==freq){                      // if timer matches desired frequency (always true if newly-created above)
            channelList[nChannel][nMode]=new ledc_channel_config_t;         // create new channel instance
            channelList[nChannel][nMode]->speed_mode=(ledc_mode_t)nMode;
            channelList[nChannel][nMode]->channel=(ledc_channel_t)nChannel;
            channelList[nChannel][nMode]->timer_sel=(ledc_timer_t)nTimer;
            channelList[nChannel][nMode]->intr_type=LEDC_INTR_DISABLE;
            channelList[nChannel][nMode]->hpoint=0;
            channelList[nChannel][nMode]->gpio_num=pin;
            timer=timerList[nTimer][nMode];
            channel=channelList[nChannel][nMode];
            return;
          }       
        }
      }
    }
  }
}

///////////////////

LedPin::LedPin(uint8_t pin, float level, uint16_t freq) : LedC(pin, freq){

  if(!channel)
    Serial.printf("\n*** ERROR:  Can't create LedPin(%d) - no open PWM channels and/or Timers ***\n\n",pin);
      
  set(level);
   
}

///////////////////

void LedPin::set(float level){

  if(!channel)
    return;

  if(level>100)
    level=100;
    
  channel->duty=level*(pow(2,timer->duty_resolution)-1);
  channel->duty/=100;
  ledc_channel_config(channel);
  
}

///////////////////

void LedPin::HSVtoRGB(float h, float s, float v, float *r, float *g, float *b ){

  // The algorithm below was provided on the web at https://www.cs.rit.edu/~ncs/color/t_convert.html
  // h = [0,360]
  // s = [0,1]
  // v = [0,1]

  int i;
  float f, p, q, t;
  
  if( s == 0 ){
    *r = *g = *b = v;
    return;
  }
  
  h /= 60;
  i = floor( h ) ;
  f = h - i;
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );
  switch( i % 6 ) {
    case 0:
      *r = v;
      *g = t;
      *b = p;
      break;
    case 1:
      *r = q;
      *g = v;
      *b = p;
      break;
    case 2:
      *r = p;
      *g = v;
      *b = t;
      break;
    case 3:
      *r = p;
      *g = q;
      *b = v;
      break;
    case 4:
      *r = t;
      *g = p;
      *b = v;
      break;
    case 5:
      *r = v;
      *g = p;
      *b = q;
      break;
  }
}

////////////////////////////

ServoPin::ServoPin(uint8_t pin, double initDegrees, uint16_t minMicros, uint16_t maxMicros, double minDegrees, double maxDegrees) : LedC(pin, 50){
  
  if(!channel)
    Serial.printf("\n*** ERROR:  Can't create ServoPin(%d) - no open PWM channels and/or Timers ***\n\n",pin);
      
  this->minMicros=minMicros;
  this->maxMicros=maxMicros;
  this->minDegrees=minDegrees;
  microsPerDegree=(double)(maxMicros-minMicros)/(maxDegrees-minDegrees);

  set(initDegrees);
      
}

///////////////////

void ServoPin::set(double degrees){

  if(!channel)
    return;

  double usec=(degrees-minDegrees)*microsPerDegree+minMicros;
  
  if(usec<minMicros)
    usec=minMicros;
  else if(usec>maxMicros)
    usec=maxMicros;

  usec*=timer->freq_hz/1e6*(pow(2,timer->duty_resolution)-1);

  channel->duty=usec;  
  ledc_channel_config(channel);
}

////////////////////////////

ledc_channel_config_t *LedC::channelList[LEDC_CHANNEL_MAX][LEDC_SPEED_MODE_MAX]={};
ledc_timer_config_t *LedC::timerList[LEDC_TIMER_MAX][LEDC_SPEED_MODE_MAX]={};

////////////////////////////

//*******************************************************
// DEPRECATED - INCLUDED FOR BACKWARDS COMPATIBILITY ONLY
//*******************************************************

PwmPin::PwmPin(uint8_t channel, uint8_t pin){
  this->channel=channel & 0x0F;
  this->pin=pin;

  ledc_timer_config_t ledTimer;
  ledTimer.timer_num=LEDC_TIMER_0;
  ledTimer.speed_mode=(this->channel)<8?LEDC_HIGH_SPEED_MODE:LEDC_LOW_SPEED_MODE;
  ledTimer.duty_resolution=LEDC_TIMER_10_BIT;
  ledTimer.freq_hz=5000;
  ledc_timer_config(&ledTimer);

  ledChannel.gpio_num=pin;
  ledChannel.speed_mode=(this->channel)<8?LEDC_HIGH_SPEED_MODE:LEDC_LOW_SPEED_MODE;
  ledChannel.channel=(ledc_channel_t)(this->channel&0x07);
  ledChannel.intr_type=LEDC_INTR_DISABLE;
  ledChannel.timer_sel=LEDC_TIMER_0;
  ledChannel.duty=0;
  ledChannel.hpoint=0;
  ledc_channel_config(&ledChannel);
  
}

///////////////////

void PwmPin::set(uint8_t channel, uint8_t level){
  ledChannel.duty=level*1023;
  ledChannel.duty/=100;
  ledChannel.duty&=0x03FF;
  ledc_channel_config(&ledChannel);
  
}

///////////////////

void PwmPin::HSVtoRGB(float h, float s, float v, float *r, float *g, float *b ){

  // The algorithm below was provided on the web at https://www.cs.rit.edu/~ncs/color/t_convert.html
  // h = [0,360]
  // s = [0,1]
  // v = [0,1]

  int i;
  float f, p, q, t;
  
  if( s == 0 ){
    *r = *g = *b = v;
    return;
  }
  
  h /= 60;
  i = floor( h ) ;
  f = h - i;
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );
  switch( i % 6 ) {
    case 0:
      *r = v;
      *g = t;
      *b = p;
      break;
    case 1:
      *r = q;
      *g = v;
      *b = p;
      break;
    case 2:
      *r = p;
      *g = v;
      *b = t;
      break;
    case 3:
      *r = p;
      *g = q;
      *b = v;
      break;
    case 4:
      *r = t;
      *g = p;
      *b = v;
      break;
    case 5:
      *r = v;
      *g = p;
      *b = q;
      break;
  }
}
