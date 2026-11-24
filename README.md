# FluxIA | Physical Computing (ESP32 no Wokwi)

Entrega de **Physical Computing: IoT & IoB** da Global Solution CitizenTech 2026.2 (FIAP, Engenharia de Software, 3º ano).

| Nome | RM |
|---|---|
| João Victor Alvarenga Cansanção | RM93752 |
| Wesley Zimmer | RM560264 |

* Projeto no Wokwi: **https://wokwi.com/projects/480996697843558401**
* Vídeo demonstrativo (até 4 min): [FluxIA_Physical_Computing.mp4](FluxIA_Physical_Computing.mp4) (3min04s, com legendas)

## O FluxIA em uma frase

Com o sinal verde e sem espaço na via à frente, veículos entram no cruzamento, param no meio dele e bloqueiam o tráfego transversal. O FluxIA recomenda quando segurar o fluxo antes do cruzamento.

## Como o circuito se encaixa na solução

A solução principal usa o simulador de tráfego SUMO e uma IA para decidir quando segurar o fluxo. Este circuito é uma **demonstração física separada**, sem conexão com o SUMO nesta etapa. Ele mostra, em pequena escala, a peça que ficaria no cruzamento: um semáforo que conta veículos e só abre ou mantém o verde quando há espaço do outro lado.

A **detecção de veículos é simulada** por botões. Nenhum carro real é detectado.

## Componentes

| Componente | Pino do ESP32 | Papel |
|---|---|---|
| LED vermelho + resistor 220 Ω | GPIO 25 | Semáforo: pare |
| LED amarelo + resistor 220 Ω | GPIO 26 | Semáforo: atenção |
| LED verde + resistor 220 Ω | GPIO 27 | Semáforo: siga |
| Botão verde "Sensor entrada" | GPIO 18 | Simula um veículo cruzando a faixa de retenção e entrando na via à frente |
| Botão azul "Sensor saída" | GPIO 19 | Simula um veículo deixando a via à frente |
| Chave deslizante | GPIO 23 | Escolhe o modo REFERÊNCIA ou FLUXIA |
| LCD 16x2 I2C | SDA 21, SCL 22 | Mostra estado, contagem e ocupação |

## Funcionamento

* **Ciclo seguro:** o semáforo sempre passa por verde, amarelo e vermelho, com tempos mínimos (verde 8 s, amarelo 2,5 s, vermelho 6 s). Nunca vai de verde direto para vermelho.
* **Contagem:** cada clique no sensor de entrada com verde ou amarelo conta uma passagem e ocupa uma vaga da via à frente, que tem capacidade para 4 veículos. Cada clique no sensor de saída libera uma vaga. No vermelho, o clique só registra um veículo retido.
* **Modo REFERÊNCIA:** tempos fixos. Se um carro entra com a via cheia, ele fica parado no cruzamento e o contador de **bloqueios** sobe.
* **Modo FLUXIA:** com a via cheia, o controlador encerra o verde (depois de pelo menos 3 s) passando pelo amarelo e mantém o vermelho até haver espaço. O contador de **bloqueios evitados** sobe.

### Leitura do LCD

```
VERDE     P:012     estado do semáforo e total de passagens
Via 3/4 FLX E01     ocupação da via, modo e bloqueios evitados (E)
Via 4/4 REF B02     no modo REFERÊNCIA, mostra os bloqueios (B)
```

### Saída no Serial Monitor

A cada evento, o ESP32 imprime uma linha JSON:

```json
{"evento":"fluxia_segura_fluxo","modo":"FLUXIA","semaforo":"AMARELO","passagens":5,"ocupacao":4,"capacidade":4,"bloqueios":0,"bloqueios_evitados":1,"t_ms":23514}
```

## Integração futura com o FluxIA (não implementada)

Esse pacote JSON é o formato que o ESP32 poderia enviar por Wi-Fi (HTTP ou MQTT) para a API do FluxIA. Assim, a contagem real e o estado do semáforo alimentariam o banco de dados e a IA, e a recomendação da IA poderia voltar ao controlador. Nesta entrega, o circuito funciona sozinho e não se conecta ao SUMO nem à API.

## Arquivos

* `sketch.ino`: código do ESP32
* `diagram.json`: circuito do Wokwi
* `libraries.txt`: biblioteca LiquidCrystal I2C
* `integrantes.txt`: nomes, RMs e links da entrega
* `FluxIA_Physical_Computing.mp4`: vídeo demonstrativo com legendas
