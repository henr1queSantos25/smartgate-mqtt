# SmartGate: Controle Inteligente de Acesso via MQTT

O **SmartGate MQTT** é um sistema embarcado de controle de acesso inteligente desenvolvido para a plataforma **BitDogLab** com o microcontrolador **Raspberry Pi Pico W**. O sistema detecta aproximação de pessoas ou veículos usando um sensor ultrassônico HC-SR04 e permite monitoramento e controle remoto através do protocolo **MQTT** (Message Queuing Telemetry Transport). O projeto integra múltiplos periféricos para fornecer feedback visual e sonoro sobre o estado atual do sistema, criando uma solução completa de automação residencial e IoT.

---

## Vídeo de Demonstração

[Assista à demonstração do SmartGate MQTT](https://drive.google.com/file/d/1-Eo-eUEAC_75lqYn9iqdvpu7PxwAeI3g/view?usp=sharing)

---

## Funcionalidades Principais

- **Detecção de Presença em Tempo Real**: Utiliza sensor ultrassônico HC-SR04 para detectar aproximação de pessoas ou veículos com distância programável (≤ 30 cm).
- **Comunicação MQTT**: Protocolo leve e escalável ideal para aplicações IoT, permitindo comunicação bidirecional em tempo real.
- **Monitoramento Remoto via Smartphone**: Interface através do aplicativo **IoT MQTT Panel** para visualização de dados e controle do sistema.
- **Sistema de Notificação Multi-sensorial**:
  - Indicações visuais através de LED RGB e matriz de LEDs 5x5.
  - Alertas sonoros distintos para diferentes situações via buzzer com PWM.
  - Informações detalhadas e iconografia no display OLED.
- **Operação baseada em Máquina de Estados**:
  - **Modo Esperando**: Estado padrão quando não há presença detectada.
  - **Presença Detectada**: Acionado quando alguém se aproxima do portão.
  - **Portão Aberto**: Estado ativado após comando remoto via MQTT.
- **Broker MQTT Local**: Executado em dispositivos locais como smartphone ou computador usando Mosquitto.

---

## Tecnologias Utilizadas

- **Linguagem de Programação**: C  
- **Microcontrolador**: Raspberry Pi Pico W (RP2040 + CYW43439)  
- **Protocolo de Comunicação**: MQTT (Message Queuing Telemetry Transport)
- **Conectividade**: Wi-Fi embutido (modo Station)
- **Broker MQTT**: Mosquitto (executado localmente)
- **Componentes Utilizados**:
  - Sensor ultrassônico HC-SR04 para detecção de presença
  - Display OLED SSD1306 128x64 (I2C) para informações visuais
  - LED RGB para indicação de estado
  - Matriz de LEDs 5x5 para alertas visuais dinâmicos
  - Buzzers com PWM para alertas sonoros
- **Bibliotecas**:
  - lwIP MQTT para comunicação MQTT
  - Pico SDK para acesso ao hardware (GPIO, ADC, PWM e I2C)
  - CYW43 para controle do módulo Wi-Fi
  - Bibliotecas personalizadas para controle dos periféricos

---

## Tópicos MQTT

O sistema utiliza uma estrutura bem definida de tópicos MQTT para comunicação:

### `/distance` 
- **Tipo**: Publicação automática
- **Frequência**: A cada 2 segundos
- **Função**: Transmite a distância medida pelo sensor HC-SR04 em centímetros
- **Formato**: Valor numérico (ex: "25")
- **Uso**: Alimenta gráficos dinâmicos no aplicativo móvel

### `/status`
- **Tipo**: Publicação automática  
- **Frequência**: A cada 0,8 segundos
- **Função**: Informa o estado atual do sistema
- **Formatos**:
  - `"Portao fechado – sem presença detectada"`
  - `"Presença detectada – aguardando ação"`
  - `"Portao aberto – acesso autorizado"`
- **Uso**: Exibição de status em tempo real na interface

### `/gate`
- **Tipo**: Comando bidirecional
- **Função**: Controla abertura e fechamento do portão
- **Comandos de entrada**:
  - `"Open"` ou `"1"` → Abre o portão
  - `"Close"` ou `"0"` → Fecha o portão
- **Resposta**: Publicação em `/gate/state` confirmando a ação

### Tópicos Auxiliares
- `/online`: Last Will and Testament (indica se o dispositivo está conectado)
- `/ping`: Responde com tempo de atividade do sistema
- `/uptime`: Tempo desde a inicialização em segundos
- `/print`: Recebe mensagens para debug via terminal
- `/exit`: Comando para desconexão controlada

---

## Aplicativos Utilizados

### **IoT MQTT Panel** (Android/iOS)
- **Função**: Interface gráfica principal para controle do sistema
- **Recursos utilizados**:
  - **Gráfico dinâmico**: Visualização da distância em tempo real
  - **Indicadores de status**: Texto mostrando estado atual
  - **Botões de controle**: "Abrir Portão" e "Fechar Portão"
  - **Dashboard personalizado**: Layout configurável com widgets
- **Configuração**: Conexão ao broker MQTT local via IP da rede

### **Termux** (Android)
- **Função**: Terminal Linux no smartphone para executar o broker MQTT
- **Uso**: Instalação e execução do Mosquitto MQTT Broker
- **Comandos utilizados**:
  ```bash
  pkg install mosquitto
  mosquitto -d
  ```

### **Mosquitto MQTT Broker**
- **Função**: Servidor MQTT local executado no smartphone
- **Características**:
  - Leve e eficiente
  - Suporte a QoS (Quality of Service)
  - Retain messages para persistência de dados
  - Compatibilidade com Last Will and Testament

---

## Como Funciona

### Detecção de Presença
- O sensor ultrassônico HC-SR04 mede continuamente a distância entre o portão e qualquer objeto à sua frente.
- Leituras são filtradas para reduzir ruídos e garantir precisão (múltiplas amostragens com `getCmFiltered()`).
- Quando a distância medida é ≤ 30 cm, o sistema considera que há uma presença detectada.

### Máquina de Estados
- **ESPERANDO**:
  - LED RGB: Azul
  - Display OLED: Ícone de cadeado fechado
  - Matriz LED: Desligada
  - Buzzer: Desligado
  - Publicação MQTT: Status "Portao fechado – sem presença detectada"
  - Transição: Passa para PRESENCA_DETECTADA se distância ≤ 30 cm

- **PRESENCA_DETECTADA**:
  - LED RGB: Vermelho
  - Display OLED: Ícone de alerta
  - Matriz LED: Símbolo "X" piscante
  - Buzzer: Alarme sonoro ativo
  - Publicação MQTT: Status "Presença detectada – aguardando ação"
  - Transição: Retorna para ESPERANDO se distância > 30 cm ou avança para PORTAO_ABERTO via comando MQTT

- **PORTAO_ABERTO**:
  - LED RGB: Verde
  - Display OLED: Ícone de cadeado aberto
  - Matriz LED: Símbolo "✓" (check)
  - Buzzer: Som de abertura (momentâneo)
  - Publicação MQTT: Status "Portao aberto – acesso autorizado"
  - Transição: Retorna para estado apropriado via comando MQTT "Close"

### Comunicação MQTT
- O Raspberry Pi Pico W atua como **cliente MQTT**, conectando-se ao broker local.
- **Workers assíncronos** garantem publicação periódica sem bloquear o loop principal.
- **QoS 1** (At least once) garante entrega confiável das mensagens.
- **Retain flags** mantêm último estado conhecido disponível para novos clientes.

---

## Configuração do Hardware

| Componente | Pino do RP2040 | Função |
|------------|----------------|--------|
| Sensor HC-SR04 Trigger | GP16 | Dispara pulso ultrassônico |
| Sensor HC-SR04 Echo | GP17 | Recebe resposta do pulso ultrassônico |
| Display OLED (I2C) | GP14 (SDA), GP15 (SCL) | Exibição de informações e ícones |
| LED RGB | Pinos dedicados (LED_RED, LED_GREEN, LED_BLUE) | Indicação visual de estado |
| Matriz de LEDs 5x5 | PIO | Exibição de símbolos de alerta e confirmação |
| Buzzer 1 | BUZZER1 (PWM) | Alarme para presença detectada |
| Buzzer 2 | BUZZER2 (PWM) | Sons de inicialização, abertura e fechamento |
| Wi-Fi (CYW43439) | Integrado ao Pico W | Comunicação MQTT via Wi-Fi |

---

## Configuração do Sistema

### Credenciais Wi-Fi e MQTT
No arquivo `smartgate-mqtt.c`, configure:

```c
#define WIFI_SSID "SEU_SSID"
#define WIFI_PASSWORD "SEU_PASSWORD_WIFI"
#define MQTT_SERVER "SEU_HOST"  // Ex: 192.168.1.107
#define MQTT_USERNAME "SEU_USERNAME_MQTT"
#define MQTT_PASSWORD "SEU_PASSWORD_MQTT"
```

### Broker MQTT Local (Smartphone)
1. Instale o Termux no Android
2. Execute os comandos:
   ```bash
   pkg update && pkg upgrade
   pkg install mosquitto
   mosquitto -d
   ```
3. Anote o IP do smartphone na rede Wi-Fi
4. Configure o IP no código do Pico W

### Aplicativo IoT MQTT Panel
1. Instale o aplicativo na Play Store/App Store
2. Configure conexão com o broker:
   - **Host**: IP do smartphone
   - **Port**: 1883
   - **Client ID**: Nome único
3. Adicione widgets:
   - **Gráfico**: Tópico `/distance`
   - **Text Display**: Tópico `/status`
   - **Botões**: Tópico `/gate` com payloads "Open"/"Close"

---

## Estrutura do Repositório

- **`smartgate-mqtt.c`**: Código-fonte principal do projeto.
- **`CMakeLists.txt`**: Arquivo de configuração para o sistema de build CMake.
- **`lwipopts.h`**: Configurações personalizadas da stack lwIP para MQTT.
- **`mbedtls_config.h`**: Configurações para TLS (se usado).
- **`lib/hcSR04.h` e `lib/hcSR04.c`**: Biblioteca para o sensor ultrassônico HC-SR04.
- **`lib/ssd1306.h` e `lib/ssd1306.c`**: Biblioteca para controle do display OLED.
- **`lib/led_5x5.h` e `lib/led_5x5.c`**: Biblioteca para controle da matriz de LEDs 5x5 via PIO.
- **`lib/buzzer.h` e `lib/buzzer.c`**: Biblioteca para geração de sons via PWM.
- **`lib/ledRGB.h` e `lib/ledRGB.c`**: Biblioteca para controle do LED RGB.
- **`lib/font.h`**: Definição da fonte e ícones utilizados no display OLED.
- **`README.md`**: Documentação do projeto.

---

## Fluxo de Operação

1. **Inicialização**: O sistema configura todos os periféricos, conecta-se à rede Wi-Fi e estabelece conexão com o broker MQTT.
2. **Registro de Tópicos**: O cliente MQTT se inscreve nos tópicos de controle e publica tópicos de dados.
3. **Workers Assíncronos**: Iniciam publicação periódica de distância e status do sistema.
4. **Loop Principal**: O sistema continuamente:
   - Lê e filtra a distância medida pelo sensor ultrassônico
   - Atualiza o estado da máquina de estados com base nas medições e comandos MQTT
   - Gerencia os periféricos (LED RGB, OLED, matriz LED, buzzers) de acordo com o estado atual
   - Processa mensagens MQTT recebidas nos tópicos subscritos
   - Publica dados atualizados via workers temporizados

---

## Conceitos Aplicados

- **Protocolo MQTT**: Comunicação leve e eficiente para IoT com QoS e retain.
- **Máquina de Estados**: Gerenciamento de diferentes modos de operação do sistema.
- **Workers Assíncronos**: Processamento não-bloqueante para publicação periódica.
- **Broker Local**: Infraestrutura MQTT executada em dispositivo móvel.
- **Medição Ultrassônica**: Detecção de presença sem contato físico.
- **Filtragem de Sinais**: Técnicas para redução de ruído nas leituras do sensor.
- **Interface Móvel**: Controle via aplicativo dedicado com widgets personalizáveis.
- **Modulação por Largura de Pulso (PWM)**: Geração de diferentes padrões sonoros.
- **Interface I2C**: Comunicação com o display OLED.
- **Programable IO (PIO)**: Controle eficiente da matriz de LEDs 5x5.
- **Feedback Multi-sensorial**: Combinação de estímulos visuais e auditivos para alertas.

---

## Objetivos Alcançados

- **Automação Residencial**: Sistema prático para controle de acesso em residências.
- **Interface Móvel Intuitiva**: Controle via smartphone sem necessidade de desenvolvimento de app personalizado.
- **Comunicação IoT Robusta**: Implementação completa do protocolo MQTT com QoS e retain.
- **Infraestrutura Local**: Broker MQTT executado no próprio smartphone, sem dependência de nuvem.
- **Monitoramento em Tempo Real**: Visualização contínua de distância via gráficos dinâmicos.
- **Segurança**: Alerta em tempo real para presença detectada com múltiplas formas de notificação.
- **Usabilidade**: Combinação de indicadores visuais, sonoros e interface mobile para feedback completo.
- **Modularidade**: Código organizado em bibliotecas reutilizáveis com arquitetura assíncrona.
- **Escalabilidade**: Estrutura preparada para múltiplos dispositivos e sensores adicionais.
- **Prototipação Realista**: Simulação funcional de um sistema automatizado de controle de portão com protocolo industrial.

---

## Desenvolvido por

Henrique Oliveira dos Santos  
[LinkedIn](https://www.linkedin.com/in/dev-henriqueo-santos/)
