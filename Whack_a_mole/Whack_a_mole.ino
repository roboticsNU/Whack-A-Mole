#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>     


//-----------------------------------------------------------------------------------
//Macrodefinitions and constants
//First pin number for each pin group
#define SCORESTART 22
#define TIMESTART 30
#define RESETBTN 38
#define LEDSTART    40
#define BUTTONSTART 45

//Timer subsystem
//Period of timers update
//A small lyrical digression:
//It is assumed that after each delay exactly *PERIOD* milliseconds are passed,
//although, in fact, there are small delays caused by code executions and network sned delays(in most degree).
//That fact will lead to deviations in time management, if timer periods will be long enough, but who cares?
//In order to exclude these time deviations one shall use precise clocks, based on cpu base clocks(lol),
//which weren't found in this "high level" Arduino framework.
#define PERIOD 50
#define SWITCH_DELAY 2000
#define MAX_TIME     60000

//index of currently active game unit
int currentActive;
int score;
int timer1 = 0;
long timer2 = MAX_TIME;
bool ethernet = false;

void chooseNew();

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
//self IP
IPAddress ip(192, 168, 11, 17);
//ip of remote controller
IPAddress ipRem(192, 168, 11, 4);

unsigned int localPort = 8888;      // local port to listen on
//-----------------------------------------------------------------------------------

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back
EthernetUDP Udp;

//Class that encapsulates logic of one game unit(A pair of button and LED)
class Unit
{
  public:
    Unit(int bPin, int lPin) : buttonPin(bPin), ledPin(lPin), active(false), lastPushed(false)
    {
      pinMode(bPin, INPUT);
      pinMode(lPin, OUTPUT);
      deactivate();
    }

	//handles a button push down event
    void checkConditions(bool push)
    {
      bool pushed = push;

      if(!pushed)
      {
        lastPushed = pushed;
        return;
      }

      if(lastPushed){
        return;
      }

      lastPushed = pushed;

      if(!active)
      { 
        if(score > 0)
          score--;
          
        timer1 = 0;

        chooseNew();
        return;
      }
      
      timer1 = 0;
      score++;
      chooseNew();
    }

    void activate()
    {
      active = true;
      digitalWrite(ledPin, LOW);
    }

    void deactivate()
    {
      active = false;
      digitalWrite(ledPin, HIGH);
    }

    bool readButton()
    {
      return digitalRead(buttonPin);
    }
    
    bool active;
	//For rising edge mechanics
    bool lastPushed;
    int buttonPin;
    int ledPin;
};

Unit* units[5];

void setup() {  
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
   Udp.begin(localPort);
  
  gameStart();

  for(int i = SCORESTART; i < SCORESTART+8; i++)
    pinMode(i, OUTPUT);
  for(int i = TIMESTART; i < TIMESTART+8; i++)
    pinMode(i, OUTPUT);
}

//Sends a bcd signal for two decoders for score board
void renderScore()
{
  int high = score / 10;
  int low = score % 10;

  for(int i = 0; i < 4; i++)
  {
    digitalWrite(SCORESTART + i, ((high >> i)  & 0x01));
  }

  for(int i = 0; i < 4; i++)
  {
    digitalWrite(SCORESTART + i + 4, ((low >> i)  & 0x01));
  }
}

//Sends a bcd signal for two decoders for timer board
void renderTime()
{
  int high = (timer2 / 1000) / 10;
  int low = (timer2 / 1000) % 10;

  for(int i = 0; i < 4; i++)
  {
    digitalWrite(TIMESTART + i, ((high >> i)  & 0x01));
  }

  for(int i = 0; i < 4; i++)
  {
    digitalWrite(TIMESTART + i + 4, ((low >> i)  & 0x01));
  }
}


//Re-init game
void gameStart()
{
  for(int i = 0; i < 5; i++)
  {
    if(units[i] == 0)
      delete units[i];
      
    units[i] = new Unit(BUTTONSTART + i, LEDSTART + i);
  }

  currentActive = 0;
  score = 0;
  timer2 = MAX_TIME;
  timer1 = 0;

  chooseNew();
}

//Randomizing next active game unit
void chooseNew()
{
  units[currentActive]->deactivate();
  currentActive = random(0,5);
  units[currentActive]->activate();


  //info for remote manager
	if(ethernet){
	//229 - id of packet with notification of new active game unit
		ReplyBuffer[0] = 229;
		ReplyBuffer[1] = (byte)currentActive;
		Udp.beginPacket(ipRem, localPort);
		Udp.write(ReplyBuffer,2);
		Udp.endPacket();
	}
}

void loop() {
//Receiveing and handling packets for remote control
  if(ethernet){
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      Serial.println("Contents:");
      for(int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++){
        Serial.print((byte)packetBuffer[i]);
        Serial.print(" ");
      }
	  //228 - id of packet with reset command
      if((byte)packetBuffer[0] == 228)
      {
            for(int i = 0; i < 5; i++)
        units[i]->activate();
      
      delay(5000);
  
      gameStart();
      }
	  //230 - id of packet with remote button push notification
	  //second received byte is index of pressed button
	  //(Yes, you can cause segmentation fault with this packet)
      if((byte)packetBuffer[0] == 230)
      {
        int pressed = (byte)packetBuffer[1];
        units[pressed]->checkConditions(true);
      }
    }
  }
    

if(digitalRead(RESETBTN))
  {
    Serial.print(timer2);
    ethernet = !ethernet;
    
    for(int i = 0; i < 5; i++)
      units[i]->activate();
    
    delay(5000);

    gameStart();
    return;
  }
  
  //Reading and handling buttons presses
  for(int i = 0; i < 5; i++)
    units[i]->checkConditions(digitalRead(units[i]->buttonPin));

    renderScore();
    renderTime();
  
  //This executes, when time given to kick a mole passes
  if(timer1 >= SWITCH_DELAY)
  {
    if(score > 0)
      score--;

      chooseNew();

      timer1 = 0;
  }

  //This executes, when time for a one game ends
  if(timer2 <= 0)
  {
    Serial.print(timer2);
    
    for(int i = 0; i < 5; i++)
      units[i]->activate();
    
    delay(5000);

    gameStart();
    return;
  }
  
  //Time management
  delay(PERIOD);
  timer1 += PERIOD;
  timer2 -= PERIOD;
}
