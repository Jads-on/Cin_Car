//Motor frente: VCC = DigitalPWM5 e GND = DigitalPWM6
//Motor re: VCC = DigitalPWM6 e GND = DigitalPWM5
//funcionamento das marchas: ao trocar acelerando aumenta a marcha, se nao estiver acelerando diminui e se freiar com a marcha ativa entra na re

#include <Arduino.h>
#include <Servo.h>
#include <BluetoothSerial.h>

//config bluetooth
// UUID (identificador do perfil bluetooth - perfil SPP) padrão do HC-05 — apps antigos procuram esse 
//#define SPP_UUID "00001101-0000-1000-8000-00805F9B34FB"

//definicao de portas
#define DIRECAO 4
#define FAROL 16
#define MOTOR_FRENTE 25
#define MOTOR_RE 27
#define LUZ_DE_RE 18
#define LUZ_DE_FREIO 19
#define SETA_ESQUERDA 21
#define SETA_DIREITA 22

//definicao de valores
int marcha = 1,
    marcha_re_ligada = 0,
    angulo_volante = 0,
    valor_velocidade = 0,
    velocidade = 0;

//temporizador
unsigned long int timer;

//flags
bool estado_acelerador,
     estado_embreagem_atual,
     estado_embreagem_anterior,
     estado_freio;

//var de processamento de comandos
char comando_sentido_luzes,
     comando_direcao;

String comando;

//instanciando as bibliotecas usadas
Servo direcao; 
BluetoothSerial modulo_bluetooth;

void recebimento_comandos(){ //recebimento e processamento dos comandos recebidos pelo bluetooth

  comando = modulo_bluetooth.readStringUntil('\n');

  //comando.trim(); //remove os espaços apos o comando (remover caso necessário)
  comando_sentido_luzes = comando.charAt(0);

  if(comando.length() > 1){ //comandos ex: luzes e freio brusco
    comando_direcao = comando.charAt(3);
  
    //recebe a intensidade do comando
    valor_velocidade = comando.substring(1, 3).toInt();
    angulo_volante = comando.substring(4, 6).toInt();
  }

  //logica direcao
  if(comando_direcao == 'R'){

    if(angulo_volante == 0){
      //retorno do volante ao centro
      angulo_volante = 90;
      digitalWrite(SETA_DIREITA, LOW);
      digitalWrite(SETA_ESQUERDA, LOW);
    }
    else{
      //retorno do volante ao centro
      angulo_volante += 90;
      digitalWrite(SETA_DIREITA, HIGH);
      digitalWrite(SETA_ESQUERDA, LOW);
    }

  }
  else{
      angulo_volante -= 90;
      digitalWrite(SETA_DIREITA, LOW);
      digitalWrite(SETA_ESQUERDA, HIGH);
    }

  //processamento
  switch(comando_sentido_luzes){

    //Freio
    case 'W':
      //executa imediatamente os comandos
      if(velocidade > 0){
        //frenagem dinamica (curto-circuita os fios (entradas) em low para causar uma forca contra-eletromatriz)
        digitalWrite(MOTOR_FRENTE, LOW);
        digitalWrite(MOTOR_RE, LOW);
        digitalWrite(LUZ_DE_FREIO, HIGH);
        estado_acelerador = LOW;
        velocidade = 0; //garante a execucao unica do freio
      }
    break;

    //acelerar para frente
    case 'F':
      //se o joystick esta direcionado para frente
      
      if(marcha_re_ligada){
        marcha_re_ligada = 0;
        marcha = 1;
      }

      if(valor_velocidade > 0){
        estado_acelerador = HIGH;
        estado_freio = LOW;
      }
      
      //se nao esta, ativa o freio dinamico
      else{
        estado_acelerador = LOW;
      }
    break;

    //Reverse
    case 'B':
      //desacelera antes de entrar na re
      if(velocidade > 0){ 
        estado_acelerador = LOW;
      }

      if(valor_velocidade > 0){
        marcha_re_ligada = 1;
        marcha = 1;
        estado_acelerador = HIGH;
        estado_freio = LOW;
      }
      else{
        estado_acelerador = LOW;
      }
    break;

    //embreagem pressionada
    case 'Z':
      estado_embreagem_atual = HIGH;
    break;

    //farol aceso
    case 'U':
      digitalWrite(FAROL, HIGH);
    break;

    //farol apagado    
    case 'u':
      digitalWrite(FAROL, LOW);
    break;

    default:
    break;
  }

  return;       
}

void logica_embreagem(){

  //aplica o angulo da direcao ao servo
  direcao.write(angulo_volante);

  //desacelero suave
  if(estado_acelerador == LOW){
    if(velocidade-- > 1){
      digitalWrite(LUZ_DE_FREIO, HIGH);
    }
    else{
      //desliga o led de FREIO quando para:
      digitalWrite(LUZ_DE_FREIO, LOW);
    }
  }
  
  //funcionamento da embreagem
  if((estado_embreagem_atual == HIGH) && (estado_embreagem_anterior == LOW) && (estado_acelerador == HIGH)){ //aumenta a marcha se estiver acelerando
    if(marcha < 5){ //garante o limite maximo de marchas em 5
      marcha += 1;
      estado_embreagem_atual == LOW; //garante apenas 1 marcha por vez
    }
  }

  else if(velocidade < marcha * 51){// reduz a marcha se reduzir muito a velocidade
    if (marcha > 1){//garante o minimo de marchas em 1
      marcha -= 1;
    }
  }

  //velocidades das marchas
  // Marchas: De 1 - 5 cada uma possuindo 51 niveis de potencias cada, que no total dao 255
  //aceleracao gradual
  if (marcha > 0 & !marcha_re_ligada){
    if(estado_acelerador == HIGH){
      if(velocidade <= (marcha * 51)){
        if(velocidade < 45){ //tranco inicial
          velocidade = 45;
        }
        else if(velocidade < 250){
          velocidade++;
          digitalWrite(LUZ_DE_FREIO, LOW);
        }
      }
    }

    //verifica se teve troca de marchas para adiconar um delay, e simula a preca de rpm na troca de marchas
    if((estado_embreagem_anterior == HIGH) && (estado_embreagem_atual == LOW)){
      if(velocidade > 101){
        velocidade -= 30;
        delay(100);
      }
    }
    analogWrite(MOTOR_FRENTE, velocidade);
  }

  //deteccao da marcha re
  else if(marcha_re_ligada){ 
    if(estado_acelerador == HIGH){
      //tranco inicial
      if(velocidade < 45){ 
        velocidade = 45;
      }
      else if(velocidade < 70){
        velocidade++;
        digitalWrite(MOTOR_FRENTE, LOW);
        digitalWrite(LUZ_DE_RE, HIGH);
        analogWrite(MOTOR_RE, velocidade);
      }
    }
  } 
}

void setup() {

  Serial.begin(115200);   
  delay(1000);

    //feedback bluetooth
    if(modulo_bluetooth.begin("Cin_Car")) {
      Serial.println("Bluetooth OK!");
    } 
    else{
      Serial.println("ERRO Bluetooth");
    }

    //configuracao dos pinos
    pinMode(MOTOR_FRENTE, OUTPUT);
    pinMode(MOTOR_RE, OUTPUT);

    //pre comandos
    direcao.attach(DIRECAO); //anexa o servo em um pino digital
    direcao.write(90);
    estado_embreagem_atual = LOW;
}

void loop(){

  if(modulo_bluetooth.available()){ //executa se o bluetooth estiver ativo

    //primeiro recebe os comandos
    recebimento_comandos();
    
    //segundo aplica eles
    logica_embreagem();

    //detecta o acionamento da embreagem
    estado_embreagem_anterior = estado_embreagem_atual;
  }
}
