//Motor frente: VCC = DigitalPWM5 e GND = DigitalPWM6
//Motor re: VCC = DigitalPWM6 e GND = DigitalPWM5
//funcionamento das marchas: ao trocar acelerando aumenta a marcha, se nao estiver acelerando diminui e se freiar com a marcha ativa entra na re

#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>

//definicao de portas
#define DIRECAO 3
#define FAROL 4
#define MOTOR_FRENTE 5
#define MOTOR_RE 6
#define LUZ_DE_FREIO_ESQUERDA 9
#define LUZ_DE_FREIO_DIREITA 8
#define RX_BLUETOOTH 10
#define TX_BLUETOOTH 11

//definicao de valores
int marcha = 1,
    marcha_re_ligada = 0,
    angulo_volante = 0,
    valor_velocidade = 0,
    velocidade = 0;

unsigned long int timer;

bool estado_acelerador,
     estado_embreagem_atual,
     estado_embreagem_anterior,
     estado_freio;

Servo direcao; //Controle do servo que comanda a direção do carro

SoftwareSerial modulo_bluetooth(RX_BLUETOOTH, TX_BLUETOOTH);

void enviarInterface() {
  // 1. Entra no modo de configuração de UI
  modulo_bluetooth.println("id set ui"); 
  delay(200); // Delay maior no início é vital

  // 2. Adiciona os componentes um por um com pausas
  modulo_bluetooth.println("add button x=0 y=4 w=10 h=20 text=Menu id=menu");
  delay(100);

  modulo_bluetooth.println("add button x=20 y=46 w=12 h=18 text=Embreagem id=E rid=e");
  delay(100);

  modulo_bluetooth.println("add touchpad x=4 y=60 w=16 h=34 id=D rid=A xmin=0 xmax=180 ymin=0 ymax=0 label=Direcao");
  delay(100);

  modulo_bluetooth.println("add touchpad x=82 y=58 w=16 h=36 id=W rid=S xmin=0 xmax=0 ymin=99 ymax=0 label=Acelerador");
  delay(100);

  modulo_bluetooth.println("add textlog x=82 y=4 w=16 h=36 id=0");
  delay(100);

  // 3. Finaliza e manda o app desenhar a interface
  modulo_bluetooth.println("id set gui");
  delay(200);

  // Agora sim, envia uma mensagem para o log que já deve estar criado
  modulo_bluetooth.println("0 Interface Renderizada!"); 
}

void setup() {
    //debugar
        modulo_bluetooth.begin(9600);
     
    //configuracao dos pinos
        pinMode(MOTOR_FRENTE, OUTPUT);
        pinMode(MOTOR_RE, OUTPUT);

    //pre comandos
        direcao.attach(DIRECAO); //anexa o servo em um pino digital
        direcao.write(90);
        estado_embreagem_atual = LOW;
}

void loop() {

  if(modulo_bluetooth.available()){ //executa se o bluetooth estiver ativo

    //recebimento e processamento dos comandos recebidos pelo bluetooth

        String comando = modulo_bluetooth.readStringUntil('\n');

        comando.trim(); //remove os espaços apos o comando
            char comando_atual = comando.charAt(0);

        //processamento
        switch(comando_atual){
          case 'W':
            valor_velocidade = comando.substring(2).toInt();
            if(valor_velocidade > 80){
                estado_acelerador = HIGH;
                estado_freio = LOW;
            }

            else{
              estado_acelerador = LOW;
              estado_freio = HIGH;
            }
          break;

          case 'S':
            estado_acelerador = LOW;
            estado_freio = LOW;
          break;

          case 'D':
            angulo_volante = comando.substring(2).toInt();
          break;

          case 'A':
            //retorno do volante ao centro
            angulo_volante = 90;
          break;

          case 'E':
              estado_embreagem_atual = HIGH;
          break;

          case 'e':
              if(estado_embreagem_atual == HIGH){
                estado_embreagem_atual = LOW;
              }
          break;

          case 'u':
            enviarInterface();
          break;

          default:
          break;
        }
  }
  direcao.write(angulo_volante);

  //funcionamento da embreagem
    if((estado_embreagem_atual == HIGH) && (estado_embreagem_anterior == LOW) && (estado_acelerador == HIGH)){ //aumenta a marcha se estiver acelerando
      if(marcha < 5){ //garante o limite maximo de marchas em 5
        marcha += 1;
      }
    }

    else if((estado_embreagem_atual == HIGH) && (estado_embreagem_anterior == LOW) && (estado_acelerador == LOW)){// reduz a marcha se soltar o ACELERADOR
      if (marcha > 1){//garante o minimo de marchas em 1
        marcha -= 1;
        }
    }

    //caso esteja de re
    if((estado_freio == HIGH) && (estado_embreagem_atual == HIGH)){
      if (marcha_re_ligada == 0){
        marcha_re_ligada = 1;
        marcha = 0;
      }
      else{
        marcha = 1;
        marcha_re_ligada = 0; //sai da marcha re
      }
    }

    //desliga o led de FREIO quando para:
    if (velocidade == 0){
      digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
      digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
    }

    if((velocidade > 0) && (estado_acelerador == LOW)){
        velocidade--;
        digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
        digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
    }

    //verifica se teve troca de marchas para adiconar um delay, e simula a preca de rpm na troca de marchas
    if((estado_embreagem_anterior == HIGH) && (estado_embreagem_atual == LOW)){
      if (velocidade > 101){
        velocidade -= 30;
        digitalWrite(MOTOR_RE, LOW);
        analogWrite(MOTOR_FRENTE, velocidade);
        delay(100);
        }
    }

    //velocidades das marchas
    // Marchas: De 1 - 5 cada uma possuindo 51 niveis de potencias cada, que no total dao 255
    if(millis() - timer > 50){
      timer = millis();

      //frenagem dinamica (curto-circuita os fios (entradas) em low para causar uma forca contra-eletromatriz)
      if(estado_freio == HIGH){
          digitalWrite(MOTOR_FRENTE, LOW);
          digitalWrite(MOTOR_RE, LOW);
          digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
          digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
          velocidade = 0;
      }

      //aceleracao gradual
      if (marcha > 0){
        if(estado_acelerador == HIGH){
          if(velocidade <= (marcha * 51)){
            if(velocidade < 45){ //tranco inicial
              velocidade = 45;
            }
            else if(velocidade < 250){
              velocidade++;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
              digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
            }
          }
        }
        analogWrite(MOTOR_FRENTE, velocidade);
      }

      else if(marcha == 0){ //deteccao da marcha re
        if(estado_acelerador == HIGH){
          if(velocidade < 45){ //tranco inicial
            velocidade = 45;
          }
          else if(velocidade < 70){
            velocidade++;
            analogWrite(MOTOR_RE, velocidade);
            digitalWrite(MOTOR_FRENTE, LOW);
            digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
            digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
          }
        }
      }

      if ((velocidade > 0) && (estado_acelerador == LOW)){
          velocidade--;
          digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
          digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
        }
    }
    /*
      Serial.print("\nVelocidade atual\n");
      Serial.println(velocidade);
      Serial.print("\nMarcha atual: \n");
      Serial.print(marcha);
      Serial.print("");
      Serial.print("\nStatus Freio: \n");
      Serial.print(estado_freio);
      Serial.print("\nStatus re: \n");
      Serial.print(marcha_re_ligada);
    */
   if( estado_embreagem_anterior != estado_embreagem_atual){
    modulo_bluetooth.print("0 Marcha atual: ");
    modulo_bluetooth.println(marcha == 0 ? "RE" : String(marcha));
   }
    //detecta o acionamento da embreagem
    estado_embreagem_anterior = estado_embreagem_atual;
}