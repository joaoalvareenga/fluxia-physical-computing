/*
  FluxIA | Physical Computing (ESP32 no Wokwi)
  Global Solution CitizenTech 2026.2, FIAP, Engenharia de Software
  João Victor Alvarenga Cansanção (RM93752) e Wesley Zimmer (RM560264)

  O que o circuito demonstra
  Um semáforo com LEDs vermelho, amarelo e verde controla a entrada de veículos em um cruzamento.
  Dois botões simulam sensores de passagem (não há detecção de carros reais):
    SENSOR DE ENTRADA (botão verde): um veículo cruzou a faixa de retenção e entrou na via à frente.
    SENSOR DE SAÍDA (botão azul): um veículo deixou a via à frente, liberando espaço.
  O ESP32 conta as passagens e acompanha quantos veículos ocupam a via à frente.

  A chave seleciona o modo de controle:
    REFERÊNCIA: o semáforo cumpre tempos fixos. Se a via à frente estiver cheia e um carro entrar,
                ele fica parado no cruzamento e o sistema registra um BLOQUEIO.
    FLUXIA:     antes de manter ou abrir o verde, o controlador confere se há espaço na via à frente.
                Sem espaço, segura o fluxo (amarelo e depois vermelho) até a via esvaziar.
                Cada vez que isso acontece, registra um BLOQUEIO EVITADO.

  Integração futura (não implementada nesta etapa): a cada mudança, o ESP32 imprime no Serial uma
  linha JSON com o estado do semáforo e as contagens. Esse mesmo pacote poderia ser enviado por
  Wi-Fi (HTTP ou MQTT) para a API do FluxIA, alimentando o banco e a IA com dados de campo.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pinos
const int PINO_VERMELHO = 25;
const int PINO_AMARELO  = 26;
const int PINO_VERDE    = 27;
const int PINO_ENTRADA  = 18;   // botão: veículo entrou na via à frente
const int PINO_SAIDA    = 19;   // botão: veículo saiu da via à frente
const int PINO_MODO     = 23;   // chave: LOW = FLUXIA, HIGH = REFERÊNCIA

// Parâmetros do cruzamento simulado
const int CAPACIDADE_VIA = 4;   // quantos veículos cabem entre o cruzamento e o próximo semáforo

// Tempos (ms). Verde, amarelo e vermelho têm duração mínima para as transições serem seguras.
const unsigned long T_VERDE        = 8000;
const unsigned long T_AMARELO      = 2500;
const unsigned long T_VERMELHO     = 6000;   // fase do tráfego transversal
const unsigned long T_DEBOUNCE     = 20;
const unsigned long T_VERDE_MINIMO = 3000;   // o FluxIA não interrompe um verde antes disso

LiquidCrystal_I2C lcd(0x27, 16, 2);

enum Estado { VERDE, AMARELO, VERMELHO, SEGURANDO };
Estado estado = VERMELHO;
unsigned long inicioEstado = 0;

bool modoFluxia = true;
int passagens = 0;          // veículos que cruzaram a faixa de retenção
int ocupacao = 0;           // veículos na via à frente
int bloqueios = 0;          // veículos parados no cruzamento (via cheia)
int bloqueiosEvitados = 0;  // vezes em que o FluxIA segurou o fluxo
bool fluxoSeguro = false;   // evita contar duas vezes o mesmo episódio de retenção

struct Botao {
  int pino;
  bool estadoEstavel;
  bool ultimaLeitura;
  unsigned long ultimaMudanca;
};
Botao btEntrada = { PINO_ENTRADA, HIGH, HIGH, 0 };
Botao btSaida   = { PINO_SAIDA,   HIGH, HIGH, 0 };

// Retorna true uma única vez a cada clique (borda de descida, com debounce)
bool clicou(Botao &b) {
  bool leitura = digitalRead(b.pino);
  if (leitura != b.ultimaLeitura) {
    b.ultimaMudanca = millis();
    b.ultimaLeitura = leitura;
  }
  if (millis() - b.ultimaMudanca > T_DEBOUNCE && leitura != b.estadoEstavel) {
    b.estadoEstavel = leitura;
    return leitura == LOW;
  }
  return false;
}

const char* nomeEstado(Estado e) {
  switch (e) {
    case VERDE:     return "VERDE";
    case AMARELO:   return "AMARELO";
    case VERMELHO:  return "VERMELHO";
    case SEGURANDO: return "SEGURANDO";
  }
  return "?";
}

void acenderLuzes() {
  digitalWrite(PINO_VERDE,    estado == VERDE);
  digitalWrite(PINO_AMARELO,  estado == AMARELO);
  digitalWrite(PINO_VERMELHO, estado == VERMELHO || estado == SEGURANDO);
}

void atualizarDisplay() {
  char linha1[17];
  char linha2[17];
  snprintf(linha1, sizeof(linha1), "%-9s P:%03d", nomeEstado(estado), passagens);
  if (modoFluxia) {
    snprintf(linha2, sizeof(linha2), "Via %d/%d FLX E%02d", ocupacao, CAPACIDADE_VIA, bloqueiosEvitados);
  } else {
    snprintf(linha2, sizeof(linha2), "Via %d/%d REF B%02d", ocupacao, CAPACIDADE_VIA, bloqueios);
  }
  lcd.setCursor(0, 0);
  lcd.print(linha1);
  lcd.setCursor(0, 1);
  lcd.print(linha2);
}

// Pacote que no futuro seria enviado à API do FluxIA
void publicarEstado(const char* evento) {
  Serial.printf("{\"evento\":\"%s\",\"modo\":\"%s\",\"semaforo\":\"%s\",\"passagens\":%d,"
                "\"ocupacao\":%d,\"capacidade\":%d,\"bloqueios\":%d,\"bloqueios_evitados\":%d,\"t_ms\":%lu}\n",
                evento, modoFluxia ? "FLUXIA" : "REFERENCIA", nomeEstado(estado),
                passagens, ocupacao, CAPACIDADE_VIA, bloqueios, bloqueiosEvitados, millis());
  atualizarDisplay();
}

void mudarPara(Estado novo, const char* motivo) {
  estado = novo;
  inicioEstado = millis();
  acenderLuzes();
  publicarEstado(motivo);
}

bool viaCheia() {
  return ocupacao >= CAPACIDADE_VIA;
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_VERMELHO, OUTPUT);
  pinMode(PINO_AMARELO, OUTPUT);
  pinMode(PINO_VERDE, OUTPUT);
  pinMode(PINO_ENTRADA, INPUT_PULLUP);
  pinMode(PINO_SAIDA, INPUT_PULLUP);
  pinMode(PINO_MODO, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("FluxIA  ESP32");
  lcd.setCursor(0, 1);
  lcd.print("Sensor simulado");
  delay(1500);
  lcd.clear();

  modoFluxia = digitalRead(PINO_MODO) == LOW;
  Serial.println("FluxIA | semaforo com contagem simulada iniciado");
  mudarPara(VERMELHO, "inicio");
}

void loop() {
  unsigned long agora = millis();
  unsigned long noEstado = agora - inicioEstado;

  // Troca de modo pela chave
  bool modoLido = digitalRead(PINO_MODO) == LOW;
  if (modoLido != modoFluxia) {
    modoFluxia = modoLido;
    publicarEstado("troca_de_modo");
  }

  // Sensor de entrada: só conta passagem com verde ou amarelo (no vermelho o veículo está retido)
  if (clicou(btEntrada)) {
    if (estado == VERDE || estado == AMARELO) {
      passagens++;
      if (viaCheia()) {
        // Sem espaço do outro lado: o veículo fica parado em cima do cruzamento
        bloqueios++;
        publicarEstado("bloqueio_no_cruzamento");
      } else {
        ocupacao++;
        publicarEstado("passagem");
      }
    } else {
      publicarEstado("veiculo_retido_no_vermelho");
    }
  }

  // Sensor de saída: um veículo deixou a via à frente
  if (clicou(btSaida)) {
    if (ocupacao > 0) {
      ocupacao--;
      publicarEstado("saida_da_via");
    }
  }

  // Máquina de estados do semáforo
  switch (estado) {
    case VERDE:
      if (modoFluxia && viaCheia() && noEstado >= T_VERDE_MINIMO) {
        bloqueiosEvitados++;
        fluxoSeguro = true;
        mudarPara(AMARELO, "fluxia_segura_fluxo");
      } else if (noEstado >= T_VERDE) {
        mudarPara(AMARELO, "fim_do_verde");
      }
      break;

    case AMARELO:
      if (noEstado >= T_AMARELO) mudarPara(VERMELHO, "fim_do_amarelo");
      break;

    case VERMELHO:
      if (noEstado >= T_VERMELHO) {
        if (modoFluxia && viaCheia()) {
          if (!fluxoSeguro) bloqueiosEvitados++;
          fluxoSeguro = true;
          mudarPara(SEGURANDO, "fluxia_aguarda_espaco");
        } else {
          fluxoSeguro = false;
          mudarPara(VERDE, "abre_verde");
        }
      }
      break;

    case SEGURANDO:
      // Mantém o vermelho até haver espaço na via à frente (ou até o modo REFERÊNCIA ser escolhido)
      if (!viaCheia() || !modoFluxia) {
        fluxoSeguro = false;
        mudarPara(VERDE, "espaco_liberado");
      }
      break;
  }

  delay(5);
}
