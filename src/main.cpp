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

//constantes
#define TAMANHO_STRING_COMANDO 6

//definicao de valores
int marcha = 1,
    marcha_re_ligada = 0,
    angulo_volante = 0,
    velocidade = 0;

int valor_rotacao_volante,
    valor_velocidade,
    ajuste_angular;

bool estado_acelerador,
     estado_embreagem_atual,
     estado_embreagem_anterior,
     estado_freio;

String string_velocidade,
       string_rotacao_volante;

Servo direcao; //Controle do servo que comanda a direção do carro

SoftwareSerial modulo_bluetooth(RX_BLUETOOTH, TX_BLUETOOTH);

void setup() {
    //debugar
        Serial.begin(9600);
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
            string_velocidade = comando.substring(4, 6);
            valor_velocidade = string_velocidade.toInt();
            if(valor_velocidade > 50){
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
            string_rotacao_volante = comando.substring(1, 3);
            valor_rotacao_volante = string_rotacao_volante.toInt();
            ajuste_angular = map(valor_rotacao_volante,0,  60, 0, 90); //ajusta os valores recebidos pra que sejam usados n     o servo
            angulo_volante = 90 - ajuste_angular; //de 0 a 90 graus
            angulo_volante = 90 + ajuste_angular; //de 90 a 180 graus
          break;

          case 'A':
            //retorno suave do volante ao centro
              if (angulo_volante < 90){
                  angulo_volante++;
              }
              else if (angulo_volante > 90){
                  angulo_volante--;
              }
              else{
                  angulo_volante = 90;
              }
          break;

          case 'E':
              estado_embreagem_atual = HIGH;
            break;
          
          case 'e':
              estado_embreagem_atual = LOW;
            break;
            
          default:
          break;
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

}