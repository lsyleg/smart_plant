#include <OneWire.h>

#include <DallasTemperature.h>
#include <OneWire.h>
#include <SoftwareSerial.h>

//--------------------------------------------아두이노 설정
#define RX 3            //BT
#define TX 2

#define HumiP A0                       
#define LuxP A1                       
#define TemP 4

#define LED 12          //on off 
#define LED_state 11    //빛 세기

#define PumP 8          //on off
#define PumP_speed 9    //모터 세기
#define defaltsetHum 20
#define defaltsetLux 70

SoftwareSerial BTSerial(TX,RX); //BT
OneWire dt(TemP);             //temp          
DallasTemperature sensors(&dt);   
extern volatile unsigned long timer0_millis;
unsigned long timer = 0;                                
//-------------------------------------------------------



//-------------------class 정의-----------------------------

class Value_Data
{
  private: 
             static byte  setHumidity ;
             static byte  setLux ;
             static bool  emptyTank;
           
            
  
  public:
  Value_Data()
  {
    
  }
  byte getHumidity()  //현재토양수분 0~100까지 출력(일반 물70, 소금물85, 공기 0)                           //
  {                                                                                              //
    return (100-map(analogRead(HumiP),0,1023,0,100));
  }
  
  byte get_setHum() //get 설정수분
  {
    return setHumidity;
  }
  void set_setHum(byte a)  //set 설정수분                           //
  {                                                                                              //
    setHumidity = a;
  }

  byte getLux() //현재조도량 출력(0~100)
  {
   return (100-map(analogRead(LuxP),0,1023,0,100));
  }
  void set_setLux(byte a){
        setLux = a;
  }
  byte get_setLux(){
       return setLux;
  }

  float getTemperature() //소수점 2자리까지 실제 온도 출력                                           //
  {
   sensors.requestTemperatures();
   return sensors.getTempCByIndex(0);                                                         //
  }

  bool getemptyTank()
  {
    return emptyTank;
  }

  void setemptyTank(bool a)
  {
    emptyTank = a;
  }
  
};
byte Value_Data::setHumidity = defaltsetHum;
byte Value_Data::setLux = defaltsetLux;
bool Value_Data::emptyTank = false;



class Control_Humidity{
  private:
          Value_Data Data;
          
          byte PreHum;
          byte At_starting_Humidity;
          byte dealycount ;//물을 주고 공급 확인을 확인하는 카운트
          unsigned long deadline_Time ; //60초뒤 수분이 잘 전달 되었는지 확인함
          unsigned short Max_wateringTime ;
          bool State;                 //현재 물을 공급하는 알고리즘이 실행되고 있는지

          unsigned short wateringTime;   //물 펌프 가동시간 = 물주는량

       
public:
  Control_Humidity(){
    
  }
  Control_Humidity(Value_Data a){
    Data = a;
    dealycount = 0;
    deadline_Time = 240000; //물을 주고 4분뒤 확인
    Max_wateringTime = 20000;
    State = false;
    wateringTime = 5000;
}



public: void maintain_hum()// 물공급 함수
{
    Serial.println("<<maintain_hum 실행>>");
    Serial.print("현재 수분-->");Serial.println(Data.getHumidity());
    Serial.print("설정 수분-->");Serial.println(Data.get_setHum());
    Serial.print("설정 펌프 가동시간-->");Serial.println(wateringTime);
    Serial.print("현재 milli(): ");Serial.print(millis()); Serial.print("  stste:");Serial.print(State);Serial.print("  물탱크: ");Serial.println(Data.getemptyTank());
    Serial.println();
   if(millis()>deadline_Time&&(State == true))//처음 물을 주고 x분이 지났을때
    {
      
       dealycount++;
       Serial.print("물공급후 확인 ");Serial.print(dealycount);Serial.println("번째 확인");
      
       if(At_starting_Humidity-Data.getHumidity()>=0 )//물을 주고도 습도가 오르지 않을때
       { 
          Serial.println("습도가 오르지 않음");
          if(dealycount>3)//물을 3번이나 공급하고도 습도가 내려갈때
          {
            Data.setemptyTank(true);
            Serial.println("물탱크에 물이 없는거 같습니다");
          }
          else
          {
             wateringTime =  wateringTime*1.5;  //물주는량 30프로증가
             Data.setemptyTank(false);   
             Serial.print("물 주는 시간 증가 -->");Serial.println(wateringTime);
          }
          
         Serial.print("물공급함 -->"); Serial.println(wateringTime);
         WaterPump(wateringTime);
          
       }
   
       else// 습도가 증가 했을때
       {
        
        Serial.println("습도가 증가");
        Serial.print("차이 -->");Serial.println(Data.get_setHum()-Data.getHumidity());
         Data.setemptyTank(false);

         if(Data.get_setHum()-Data. getHumidity()==0)//설정습도에 정확히 도달시
         {
            Serial.println("습도가 정확히 오름");
            State = false; //물공급 알고리즘 끝을 전달
            dealycount = 0;
         }
             
         else if(Data.get_setHum()-Data.getHumidity()>0) //설정 습도까지는 미도달 했을때
         {
          Serial.println("습도가 정확히는 미도달");
          wateringTime =  wateringTime*1.4;

          Serial.print("물공급함 -->"); Serial.println(wateringTime);
          WaterPump(wateringTime);
           
         }
         else
         {
          Serial.println("습도가 설정값을 오버함");
          wateringTime =  wateringTime*0.8;
          State = false;//물공급 알고리즘 끝을 전달
          dealycount = 0;
         }

         
       }
       At_starting_Humidity = Data.getHumidity();
       timer0_millis= 0;
       
       
       
    }

    

   if(Data.get_setHum()-Data.getHumidity()>1 && (State == false)) // 처음 현재습도가 설정습도에 미도달하고 
   {
      Serial.print("처음으로 수분이 떨어져  "); Serial.print(wateringTime);Serial.println(" ms만큼 공급"); 
                                           
      At_starting_Humidity = Data.getHumidity();//물을 주기 직전 습도 저장
                                           // 처음 물을 주기전 습도 (다음 loop에서 습도가 At_starting_Humidity 보다 작으면 물공급이 잘 되지 않은것)
      
         //물공급
      WaterPump(wateringTime);                         //(-getHumidity()+seyHum)마큼 습도가 증가하기위해선 
                                                                               //(습도차이)*wateringTime(습도 1이 올라가기위한 필요한시간)초만큼 펌프가 가동되야함
      timer0_millis= 0;// 밀리 초기화
      Serial.print("타이머 초기화 확인 -->");Serial.println(millis());

      State = true;
   }

    Serial.print("현재 수분-->");Serial.println(Data.getHumidity());
    Serial.print("설정 수분-->");Serial.println(Data.get_setHum());
    Serial.print("설정 펌프 가동시간-->");Serial.println(wateringTime);
    Serial.println("<<maintain_hum 끝>>");
    Serial.println("");
    Serial.println("");
       

}


private:  void WaterPump(unsigned int times) // time [ms] 만큼 펌프 가동                                            //
{
  if(times>Max_wateringTime)
    wateringTime = Max_wateringTime;
  analogWrite(PumP_speed,255); //펌프세기 100(0~100);                                           //
  digitalWrite(PumP,HIGH);
  delay(times);
  digitalWrite(PumP,LOW);                                                                     //
}



};



class Control_Lux
{
  private: Value_Data Data;
           bool LED_on_off;
           byte LED_emiting_value;
  public:
  Control_Lux(Value_Data a)
  {
    Data = a;
    LED_on_off = true;
    LED_emiting_value = 30;
  }

  public: void maintain_lux(){

  Serial.println("<<maintain_lux 시작>>");
  Serial.print("LED_emiting_value: ");   Serial.println(LED_emiting_value);
  Serial.print("현재 조도: "); Serial.println(Data.getLux());
  Serial.print("설정조도: "); Serial.println(Data.get_setLux());

  if(Data.get_setLux()-3 >= Data.getLux() || Data.get_setLux()+3 <= Data.getLux())
  {
    Serial.println("설정 값을 벚어나 조도를 재설정합니다");

    //조도가 설정값이 될때까지 loop돌리기
    while(Data.get_setLux()-3 >= Data.getLux() || Data.get_setLux()+3 <= Data.getLux())
    {
        if (Data.get_setLux()-7>=Data.getLux() && LED_emiting_value <= 251) // 조도량이 과도하게 적을때
        { 
          Serial.print("설정 값보다 받는 조도량이 적습니다 ");
          if(LED_emiting_value==255)
          {
            Serial.println("LED가 최대 밝기지만 설정값보다 조도량이 작습니다");
            break;
          }
          else
          {
            LED_emiting_value+=4;
            Set_LED_Light(LED_emiting_value);
            Serial.print("LED 밝기를 +4 합니다");
          }
        }
       else if(Data.get_setLux()-3 >= Data.getLux())// 조도량이 적을때
        { 
          Serial.print("설정 값보다 받는 조도량이 적습니다 ");
          if(LED_emiting_value==255)
          {
            Serial.println("LED가 최대 밝기지만 설정값보다 조도량이 작습니다");
            break;
          }
          else
          {
            LED_emiting_value++;
            Set_LED_Light(LED_emiting_value);
            Serial.print("LED 밝기를 +1 합니다");
          }
        }

        else if (Data.get_setLux()+7 <= Data.getLux() && LED_emiting_value >= 4) // 조도가 과도하게 많을때
        {
           if(LED_emiting_value==0)
          { Serial.println("");
            Serial.println("LED가 최소 밝기지만 설정값보다 조도량이 많습니다");
            break;
          }
          else
          {
            LED_emiting_value-=4;
            Set_LED_Light(LED_emiting_value);
            Serial.print("LED 밝기를 -4 합니다");
          }
        }
        
        else if (Data.get_setLux()+3 <= Data.getLux())// 조도가 많을때
        {
           if(LED_emiting_value==0)
          { Serial.println("");
            Serial.println("LED가 최소 밝기지만 설정값보다 조도량이 많습니다");
            break;
          }
          else
          {
            LED_emiting_value--;
            Set_LED_Light(LED_emiting_value);
            Serial.print("LED 밝기를 -1 합니다");
          }
        }
        else Serial.print("Error\n");
    } 
    Serial.println("");
  }

  Serial.print("LED_emiting_value: ");   Serial.println(LED_emiting_value);
  Serial.print("현재 조도: "); Serial.println(Data.getLux());
  Serial.print("설정조도: "); Serial.println(Data.get_setLux());
  Serial.println("<<maintain_lux 끝>>");
  Serial.println("");Serial.println("");
  
 }
  
  private: void Set_LED_Light(byte a) //Led 밝기 조절(0~255);
{
  analogWrite(LED_state,a);
}
};




class Delay_Loop_Time
{
  private:
          byte preLux;
          byte preHum;
          float preTemp;
          
        
          Value_Data Data;
public:
    byte Looptime[8] = {6,6,6,6,6,6,6,6}; //1~5분 delay
     byte Tempset = 7;
    
    Delay_Loop_Time(){
      
    }
    Delay_Loop_Time(Value_Data a,byte a1 , byte b, byte c )
    {
      Data = a;
      preLux = a1;
      preHum = b;
      preTemp = c;
      Tempset = 7;
    }
          
 /*  public: void delaytime()
   {
     unsigned long times = (unsigned long)Looptime[Tempset]*10000;
     Serial.println("<<delaytime 시작>>");
     Serial.print("Looptime[Tempset]: ");Serial.println(Looptime[Tempset]);
     Serial.print(times);Serial.println("ms 만큼 아두이노가 정지됩니다");

     delay(times);
     Serial.println("<<delaytime 끝>>");
     Serial.println("");Serial.println("");

   }*/
   
   void calculate_temp_point() //0~8까지 존재 point 가 클수록 변화량이 높다는뜻
   {  
    
    //변화량
      byte variation_Lux;
      byte variation_Hum;
      float variation_Temp;

      byte point;
      Serial.println("<<calculate point 시작>>");
      Serial.print("이전 loop 습도: ");Serial.print(preHum);
      Serial.print(" 이전 loop 조도: ");Serial.print(preLux);
      Serial.print(" 이전 loop 온도: ");Serial.println(preTemp);
       Serial.print("현재 loop 습도: ");Serial.print(Data.getHumidity());
      Serial.print(" 현재 loop 조도: ");Serial.print(Data.getLux());
      Serial.print(" 현재 loop 온도: ");Serial.println(Data.getTemperature());
   

      
      
      //변화량 계산
      variation_Lux = abs(preLux-Data.getLux());
      variation_Hum = abs(preHum-Data.getHumidity());
      variation_Temp = abs(preTemp-Data.getTemperature());

      //temp 포인트 계산
      point = variation_Lux/5 + variation_Hum/2 + (byte)(variation_Temp*20);
      Serial.print("point: ");Serial.println(point);
      
      point = point/(Looptime[Tempset]/6);// 분당 포인트 변화량
      Serial.print("point: ");Serial.println(point);
      if(point >7)
        {
          Serial.print("point: ");Serial.println(point);
          Serial.println("point 최댓 값(7) 넘어감 7 로변경");
          point = 7;
          
        }
        Serial.print("point: ");Serial.println(point);
        Serial.println("pre data들 현재 값으로 입력");
        preLux = Data.getLux();
        preHum = Data.getHumidity();
        preTemp = Data.getTemperature();

      Serial.println("<<calculate point 끝>>");
      Serial.println("");Serial.println("");
      Tempset = point;
   }
};



//----------------class 선언-----------------------------------------------------------------
Value_Data Data;

Control_Humidity Hum(Data);
Delay_Loop_Time looptime(Data,Data.getLux(),Data.getHumidity(),Data.getTemperature());
Control_Lux Lux(Data) ;

 


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  BTSerial.begin(9600);
  
  pinMode(HumiP,INPUT);
  pinMode(LuxP,INPUT); 
  pinMode(TemP,INPUT); 
  
  pinMode(LED,INPUT); 
  pinMode(LED_state,INPUT); 
  
  pinMode(PumP,INPUT); 
  pinMode(PumP_speed,INPUT); 


  //LED 켜기
  analogWrite(LED_state,20);
  digitalWrite(LED,HIGH); 

  //최초 동작
   looptime.calculate_temp_point();

 
   Hum.maintain_hum();  
   Lux.maintain_lux();

//   looptime.delaytime();
  // delay(10000*looptime.Looptime[looptime.Tempset]);
   timer = millis();
 
  Serial.print(10*looptime.Looptime[looptime.Tempset]);Serial.println("[s] 뒤에 동작");
}

void loop() {
  // put your main code here, to run repeatedly:

   bluetooth_input_output();
    
   if( timer+(unsigned long)10000*looptime.Looptime[looptime.Tempset]<millis())
   {
      Serial.println("-----------------------------------------------------");
      looptime.calculate_temp_point();
       
      Hum.maintain_hum();   
      Lux.maintain_lux();
      

 
      //looptime.delaytime();

      //Data.set_setHum(50);
      timer = millis();
      Serial.print(10*looptime.Looptime[looptime.Tempset]);Serial.println("[s] 뒤에 동작");
      Serial.println("-----------------------------------------------------");
   }
   
   delay(5000);
   
   

}

void bluetooth_input_output()
{
  String value = "";
  char code;
  if(BTSerial.available())
  {
    // Serial.println("첫 if문");
    code = BTSerial.read();
    if(code == '0')//------------------------------------첫번째 인자 '1' input
    {
      Serial.println("BT at 0 ");
      delay(10);
      code = BTSerial.read(); //공백받기
      int valueNumber = 1; //3번째 인자까지만 받기 위해
      delay(15);
      while(BTSerial.available())
      {
        code = BTSerial.read();
        if(code == ' ')//공백을 받으면
        {
          if(valueNumber==1)
          {
               //Serial.print("valueNumber==1-->");Serial.println(value);
             Data.set_setHum(value.toInt());// hum = value.toInt();
              value = "";
              valueNumber++;
          }
          else if(valueNumber==2) 
          {
          //  Serial.print("valueNumber==2-->");Serial.println(value);
             Data.set_setLux(value.toInt()); //lux = value.toInt();  
              value = "";
           // code = BTSerial.read();//"\n"제거 --------------------------------------------------수정
              break;//연속된 통신 제거
          }
        }
        else
        {
           Serial.print(" value+=code;");
          value+=code;
          Serial.println(value);
        }
        delay(10);//문자 끊김 방지
      }
    }
    else if(code == '1')//------------------------------------첫번째 인자 '0' output
    {
      while(BTSerial.available())
      {
           Serial.print(code);
            Serial.println("-->문자제거");
            code = BTSerial.read();
            delay(10);
      }
       BTSerial.print((int)Data.getTemperature());
       //    Serial.print(code);
    //  code = BTSerial.read();//엔터빼기
     // Serial.print(code);
    
     BTSerial.print(" ");

     BTSerial.print(Data.getLux());
     BTSerial.print(" ");

   //  BTSerial.print(Data.getemptyTank());
    // BTSerial.print(" ");
     
    BTSerial.println(Data.getHumidity());
    
    // print(" ");
     
    }
    else if(code == '2')//------------------------------------첫번째 인자 '0' output
    {
      while(BTSerial.available())
      {
           Serial.print(code);
            Serial.println("-->문자제거");
            code = BTSerial.read();
            delay(10);
      }
       //BTSerial.print((int)Data.setTemperature());
       //    Serial.print(code);
    //  code = BTSerial.read();//엔터빼기
     // Serial.print(code);
    
    // BTSerial.print(" ");

     BTSerial.print(Data.get_setLux());
     BTSerial.print(" ");

   //  BTSerial.print(Data.getemptyTank());
    // BTSerial.print(" ");
     
    BTSerial.println(Data.get_setHum());
    
    // print(" ");
     
    }
    else{
      
     Serial.println("알수 없는 명령입니다.");
      delay(10);
     while(BTSerial.available())
      {
           Serial.print(code);
            Serial.println("-->문자제거");
            code = BTSerial.read();
            delay(10);
      }
    // return 0;
    }
   Serial.println("Lux: "); Serial.println(Data.get_setLux());
   Serial.println("Hum: ");Serial.println(Data.get_setHum());
  }
   
   
}
