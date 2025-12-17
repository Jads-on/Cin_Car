//Motor frente: VCC = DigitalPWM5 e GND = DigitalPWM6
//Motor re: VCC = DigitalPWM6 e GND = DigitalPWM5
//funcionamento das marchas: ao trocar acelerando aumenta a marcha, se nao estiver acelerando diminui e se freiar com a marcha ativa entra na re
#include <Arduino.h>
#include <Servo.h>

//definicao de portas
#define VOLANTE 0
#define DIRECAO 3
#define FREIO 4
#define MOTOR_FRENTE 5
#define MOTOR_RE 6
#define ACELERADOR 7
#define EMBREAGEM 8
#define LUZ_DE_FREIO_ESQUERDA 9
#define LUZ_DE_FREIO_DIREITA 10

//definicao de valores
int marcha = 1,
    marcha_re_ligada = 0,
    angulo_volante = 0,
    velocidade = 0;
bool estado_acelerador,
     estado_embreagem_atual,
     estado_embreagem_anterior,
     estado_freio;

Servo direcao; //Controle do servo que comanda a direção do carro

void setup() {
    //debugar
        Serial.begin(9600);

    //configuracao dos pinos
        pinMode(MOTOR_FRENTE, OUTPUT);
        pinMode(MOTOR_RE, OUTPUT);
        pinMode(ACELERADOR, INPUT);
        pinMode(EMBREAGEM, INPUT);

    //pre comandos
        estado_embreagem_atual = digitalRead(EMBREAGEM);
        direcao.attach(DIRECAO); //anexa o servo em um pino digital
}

void loop() {
//comandos
    estado_acelerador = digitalRead(ACELERADOR);
    estado_embreagem_atual = digitalRead(EMBREAGEM);
    estado_freio = digitalRead(FREIO);

//leitura da direcao
    angulo_volante = analogRead(VOLANTE);
    angulo_volante = map(angulo_volante, 0, 1023, 0 , 180); //ajusta a o valor lido pra que ele fique entre 0 e 180 graus
    direcao.write(angulo_volante);

//funcionamento da embreagem
    if((estado_embreagem_atual == HIGH) && (estado_embreagem_anterior == LOW) && (estado_acelerador == HIGH)){ //aumenta a marcha se estiver acelerando
        if(marcha < 5){ //garante o limite maximo de marchas em 5
        marcha += 1;
        }
    }

    else if((estado_embreagem_atual == HIGH) && (estado_embreagem_anterior == LOW) && (estado_acelerador == LOW)){// reduz a marcha se soltar o ACELERADOR
        if (marcha == 1){//garante o minimo de marchas em 1
        marcha -= 1;
        }
    }

//detecta o acionamento da embreagem
    estado_embreagem_anterior = estado_embreagem_atual;

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

//frenagem dinamica (curto-circuita os fios (entradas) em low para causar uma forca contra-eletromatriz)
    if(estado_freio == HIGH){
        digitalWrite(MOTOR_FRENTE, LOW);
        digitalWrite(MOTOR_RE, LOW);
        digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
        digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
        velocidade = 0;
    }

//desliga o led de FREIO quando para:
      if (velocidade == 0){
        digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
        digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
      }

//aceleracao gradual
      if (marcha > 0){
        analogWrite(MOTOR_FRENTE, velocidade);
      }

//verifica se teve troca de marchas para adiconar um delay, e simula a preca de rpm na troca de marchas
      if(estado_embreagem_anterior == HIGH){
        if (velocidade > 101){
        velocidade -= 30;
        digitalWrite(MOTOR_RE, LOW);
        analogWrite(MOTOR_FRENTE, velocidade);
        delay(200);
        }
      }

//velocidades das marchas
// Marchas: De 1 - 5 cada uma possuindo 51 niveis de potencias cada, que no total dao 255
    if (estado_acelerador == HIGH){ // tranco inicial pra sair do lugar
        if (velocidade == 0){
            velocidade = 45;
        }
    }
    switch (marcha){
        case 0:
          if((velocidade < 70) && (estado_acelerador == HIGH)){
            velocidade++;
            analogWrite(MOTOR_RE, velocidade);
            digitalWrite(MOTOR_FRENTE, LOW);
            digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
            digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
          }
          else if ((velocidade > 0) && (estado_acelerador == LOW)){
            velocidade--;
            digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
            digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
          }
        break;

        case 1:
            if((velocidade < 51) && (estado_acelerador == HIGH)){
              velocidade++;
              digitalWrite(MOTOR_RE, LOW);
              analogWrite(MOTOR_FRENTE, velocidade);
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
              digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
            }

        else if ((velocidade > 0) && (estado_acelerador == LOW)){
              velocidade--;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
              digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
            }
        break;

        case 2:
            if((velocidade < 102) && (estado_acelerador == HIGH)){
              velocidade++;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
              digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
            }

            else if ((velocidade > 0) && (estado_acelerador == LOW)){
              velocidade--;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
              digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
            }
        break;

        case 3:
            if((velocidade < 153) && (estado_acelerador == HIGH)){
              velocidade++;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
              digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
            }

            else if ((velocidade > 0) && (estado_acelerador == LOW)){
              velocidade--;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
              digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
            }
        break;

        case 4:
            if((velocidade < 204) && (estado_acelerador == HIGH)){
              velocidade++;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
              digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
            }

            else if ((velocidade > 0) && (estado_acelerador == LOW)){
              velocidade--;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
              digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
            }
        break;

        case 5:
            if((velocidade < 255) && (estado_acelerador == HIGH)){
              velocidade++;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, LOW);
              digitalWrite(LUZ_DE_FREIO_DIREITA, LOW);
            }

            else if ((velocidade > 0) && (estado_acelerador == LOW)) {
              velocidade--;
              digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
              digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
            }
        break;

        default:
          if (velocidade > 0){
            velocidade--;
            digitalWrite(LUZ_DE_FREIO_ESQUERDA, HIGH);
            digitalWrite(LUZ_DE_FREIO_DIREITA, HIGH);
          }
        break;
    }

        delay(500); //normal em 100
        Serial.print("Velocidade atual: ");
        Serial.println(velocidade);
        Serial.print("\n");
        Serial.print("Marcha atual: ");
        Serial.print(marcha);
        Serial.print("\n");
        Serial.print("Status Freio: ");
        Serial.print(estado_freio);
        Serial.print("\n");
        Serial.print("Status re: ");
        Serial.print(marcha_re_ligada);
        Serial.print("\n");

}