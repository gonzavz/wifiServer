#include <SoftwareSerial.h>

const char CWMODE_1[] = "AT+CWMODE=1";
const char CWMODE_3[] = "AT+CWMODE=3\r\n";
const char CWMODE_CHECK[] = "AT+CWMODE?";
const char CWMODE_OK[] = "+CWMODE:1";
const char ATRST[] = "AT+RST\r\n";
char IPD[] = "+IPD,";
char HTTP_READ[] = "/read";
const char EOL[] = "\r\n";
const bool DEBUG = true;
const char CHBAUDR[] = "AT+CIOBAUD=9600\r\n";
const int ACTION_UNKNOWN = 0;
const int ACTION_READ = 1;


SoftwareSerial esp8266(3,2);
Stream* serialIn;
Stream* serialOut;

int currentConnectionId;
int requestedAction;
float Irms;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  esp8266.begin(115200);
  
  serialIn = &esp8266;
  serialOut = &Serial;

  wifiRestart();
  wifiSetAPMode();
  Irms = 0.0;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  Irms += 0.005;
  /*
  if(serialOut->available()){
    char readOutValue = serialOut->read(); 
    serialIn->print(readOutValue);
  }
  if(serialIn->available()){
    char readInValue = serialIn->read();
    serialOut->print(readInValue);
    }
  */
  
  //wifiRestart();
  
  if(serialIn->available()){
        
        if(serialIn->find(IPD)){
          
          readIPD(500);
          String response = "";
          response+="70;";
          response+=Irms;
          
          if(requestedAction==ACTION_READ)
          {
            sendHTTPResponse(currentConnectionId,response);
          }
          //Send Response
          if(requestedAction==ACTION_UNKNOWN)
          {
            closeConnection(currentConnectionId);
          }
          //Close Conection
          
          
          
        }

        //readATResponse();
  
  }
  
}

void wifiRestart(void){

sendCommand(ATRST,1000,1000,DEBUG);


}

void wifiSetAPMode(void){
sendCommand(CWMODE_3,10,1000,DEBUG);
 sendCommand("AT+CWSAP=\"EMETER_001aaa\",\"0000453298\",5,3\r\n",10,1000,DEBUG);
  sendCommand("AT+CIFSR\r\n",10,1000,DEBUG); // get ip address
  sendCommand("AT+CIPMUX=1\r\n",10,1000,DEBUG); // configure for multiple connections
  sendCommand("AT+CIPSERVER=1,80\r\n",10,1000,DEBUG); // turn on server on port 80 
}


void sendCommand(String command, const int timeout, const int timedelay, boolean debug)
{
  if(debug)
  {
  serialOut->print("AT Sending -> ");
  serialOut->println(command);  
  }
  serialIn->print(command);
  delay(timedelay);
  long int time = millis();
    
    while( (time+timeout) > millis())
    {
    readATResponse();
    }
}




void sendCIPSEND(int connectionId, String data, int timeout){
//wait till response or Time Out  
   String cipsend = "AT+CIPSEND=";
   cipsend += connectionId;
   cipsend += ",";
   cipsend +=data.length();
   cipsend +="\r\n";
   serialIn->print(cipsend);

   //wait for ">"
   long int time = millis();
   char c;
   while(time+timeout>millis())
   {
    if(serialIn->available()){
      c = serialIn->read();
      if(DEBUG){
        serialOut->print(c);
      }
      if(c == '>')
        break;
     } 
   }

   int dataSize = data.length();
   char charData[dataSize];
   data.toCharArray(charData,dataSize);
   serialIn->write(charData,dataSize);
   
   time=millis();
   while(time+timeout>millis())
   {
    if(serialIn->available()){
      c = serialIn->read();
      if(DEBUG){
        serialOut->print(c);
      }
      
     } 
   }

  
      
}



void debugLogAT(String logMsg){
serialOut->print("Sending AT command -> ");
serialOut->println(logMsg);  
}

void lookForConnection(){

  //check if +IPD, messagge is receive, then push on serial
  if(serialIn->available()){

        if(serialIn->find("+IPD,")){
        //push to serial
        readATResponse();
          
        }
  
  }
  
  
}


void sendHTTPResponse(int connectionId, String content)
{
     
     // build HTTP response
     String httpResponse;
     String httpHeader;
     // HTTP Header
     httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n"; 
     httpHeader += "Content-Length: ";
     httpHeader += content.length();
     httpHeader += "\r\n";
     httpHeader +="Connection: close\r\n\r\n";
     httpResponse = httpHeader + content + " "; // There is a bug in this code: the last character of "content" is not sent, I cheated by adding this extra space
     sendCIPSEND(connectionId,httpResponse,500);
}
 
/*
* Name: sendCIPDATA
* Description: sends a CIPSEND=<connectionId>,<data> command
*
*/
void sendCIPData(int connectionId, String data)
{
   String cipSend = "AT+CIPSEND=";
   cipSend += connectionId;
   cipSend += ",";
   cipSend +=data.length();
   cipSend +="\r\n";
   sendCommand(cipSend,100,10,DEBUG);
   int dataSize = data.length();
   char charData[dataSize];
   data.toCharArray(charData,dataSize);
   serialIn->write(charData,dataSize);
   //closeConnection(connectionId);
}


void closeConnection(int connectionId){
     String closeCommand = "AT+CIPCLOSE="; 
     closeCommand+=connectionId; // append connection id
     closeCommand+="\r\n";
     sendCommand(closeCommand,10,10,DEBUG);
}







void readATResponse(void){
  char readValue;
  bool readSomething = false;
  while(serialIn->available() > 0)
  {
    readSomething=true;
    readValue = serialIn->read();
    if(DEBUG)
    { 
    serialOut->print(readValue);  
    }
  }
  if(readSomething)
  {
    serialOut->println();
  }
}


void readIPD(int timeout){
long int time = millis();
 String ipdMsg = "";
    char c;
    while( (time+timeout) > millis())
    {
      while(serialIn->available()){
      c = serialIn->read();
      ipdMsg += c;
      }
    }
int cnnDelimiter = ipdMsg.indexOf(",");
int readQueryIndex = ipdMsg.indexOf(HTTP_READ);
currentConnectionId = ipdMsg.substring(0,cnnDelimiter).toInt();

if(readQueryIndex!=-1){
requestedAction = ACTION_READ;  
}else{
  requestedAction = ACTION_UNKNOWN;
}

}

void rxEmpty(bool debug){
  char readValue ;
  
  while(serialIn->available()>0){
    readValue = serialIn->read();
    if(debug)
    {
      serialOut->print(readValue);
      }
    }

}

