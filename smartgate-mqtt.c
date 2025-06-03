#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 

// Bibliotecas padrão do pico
#include "pico/stdlib.h"         
#include "pico/cyw43_arch.h" 
#include "pico/unique_id.h"

// Bibliotecas lwip
#include "lwip/apps/mqtt.h" // Biblioteca LWIP MQTT -  fornece funções e recursos para conexão MQTT
#include "lwip/apps/mqtt_priv.h" // Biblioteca que fornece funções e recursos para Geração de Conexões
#include "lwip/dns.h" // Biblioteca que fornece funções e recursos suporte DNS:
#include "lwip/altcp_tls.h" // Biblioteca que fornece funções e recursos para conexões seguras usando TLS:

// Bibliotecas de hardware
#include "lib/hcSR04.h"
#include "lib/ledRGB.h"
#include "lib/buzzer.h"
#include "lib/ssd1306.h"
#include "lib/led_5x5.h"
#include "lib/font.h"

//======================================================
// DEFINIÇÕES E CONFIGURAÇÕES GLOBAIS
//======================================================

// Definições de pinos
#define TRIGGER 16 // Pino trigger do sensor ultrassônico
#define ECHO 17 // Pino echo do sensor ultrassônico
#define I2C_PORT i2c1 // Porta I2C para display OLED
#define I2C_SDA 14 // Pino SDA da interface I2C
#define I2C_SCL 15 // Pino SCL da interface I2C
#define SSD1306_ADDRESS 0x3C // Endereço I2C do display OLED

// Configurações do MQTT e Wi-Fi
#define WIFI_SSID "SEU_SSID" // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASSWORD "SEU_PASSWORD_WIFI" // Substitua pela senha da sua rede Wi-Fi
#define MQTT_SERVER "SEU_HOST" // Substitua pelo endereço do host - broket MQTT: Ex: 192.168.1.107
#define MQTT_USERNAME "SEU_USERNAME_MQTT" // Substitua pelo nome da host MQTT - Username
#define MQTT_PASSWORD "SEU_PASSWORD_MQTT" // Substitua pelo Password da host MQTT - credencial de acesso - caso exista

// Definição da Máquina de Estados
typedef enum {
    ESPERANDO, // Estado inicial - portão fechado, sem presença
    PRESENCA_DETECTADA, // Presença detectada, portão ainda fechado
    PORTAO_ABERTO // Portão aberto para acesso
} EstadoSistema;

// Variáveis globais
EstadoSistema estadoAtual = ESPERANDO; // Estado inicial do sistema
ssd1306_t ssd; // Estrutura do display OLED
uint64_t distancia = 150; // Distância medida pelo sensor (cm)
volatile bool tocar_som_abertura = false; // Controla o som de da "Porta/Portão" que abriu
volatile bool tocar_som_fechamento = false; // Controla o som de da "Porta/Portão" que fechou


#ifndef MQTT_SERVER
#error Need to define MQTT_SERVER
#endif

// This file includes your client certificate for client server authentication
#ifdef MQTT_CERT_INC
#include MQTT_CERT_INC
#endif

#ifndef MQTT_TOPIC_LEN
#define MQTT_TOPIC_LEN 100
#endif

//Dados do cliente MQTT
typedef struct {
    mqtt_client_t* mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[MQTT_TOPIC_LEN];
    uint32_t len;
    ip_addr_t mqtt_server_address;
    bool connect_done;
    int subscribe_count;
    bool stop_client;
} MQTT_CLIENT_DATA_T;

#ifndef DEBUG_printf
#ifndef NDEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif
#endif

#ifndef INFO_printf
#define INFO_printf printf
#endif

#ifndef ERROR_printf
#define ERROR_printf printf
#endif

// Temporização da coleta de distância
#define DIST_WORKER_TIME_S 2
#define STATUS_WORKER_TIME_S 1 // Tempo em segundos para publicar o status do sistema


// Manter o programa ativo
#define MQTT_KEEP_ALIVE_S 60

// QoS - mqtt_subscribe
// At most once (QoS 0)
// At least once (QoS 1)
// Exactly once (QoS 2)
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 1
#define MQTT_PUBLISH_RETAIN 0

// Tópico usado para: last will and testament
#define MQTT_WILL_TOPIC "/online"
#define MQTT_WILL_MSG "0"
#define MQTT_WILL_QOS 1

#ifndef MQTT_DEVICE_NAME
#define MQTT_DEVICE_NAME "pico"
#endif

// Definir como 1 para adicionar o nome do cliente aos tópicos, para suportar vários dispositivos que utilizam o mesmo servidor
#ifndef MQTT_UNIQUE_TOPIC
#define MQTT_UNIQUE_TOPIC 0
#endif

//======================================================
// PROTÓTIPOS DE FUNÇÕES
//======================================================

// Inicialização dos periféricos
void setup();

// Requisição para publicar
static void pub_request_cb(__unused void *arg, err_t err);

// Topico MQTT
static const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name);

// Controle do portão
static void control_gate(MQTT_CLIENT_DATA_T *state, bool on);

// Publicar distância
static void publish_distance(MQTT_CLIENT_DATA_T *state);

// Publicar status
static void publish_status(MQTT_CLIENT_DATA_T *state);

// Requisição de Assinatura - subscribe
static void sub_request_cb(void *arg, err_t err);

// Requisição para encerrar a assinatura
static void unsub_request_cb(void *arg, err_t err);

// Tópicos de assinatura
static void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub);

// Dados de entrada MQTT
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

// Dados de entrada publicados
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);

// Publicar distância
static void distance_worker_fn(async_context_t *context, async_at_time_worker_t *worker);
static async_at_time_worker_t distance_worker = { .do_work = distance_worker_fn };

// Publicar status do sistema
static void publish_status_worker_fn(async_context_t *context, async_at_time_worker_t *worker);
static async_at_time_worker_t publish_status_worker = { .do_work = publish_status_worker_fn };

// Conexão MQTT
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);

// Inicializar o cliente MQTT
static void start_client(MQTT_CLIENT_DATA_T *state);

// Call back com o resultado do DNS
static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);


//======================================================
// FUNÇÃO PRINCIPAL
//======================================================
int main(void) {

    // Inicializa os periféricos
    setup();    

    INFO_printf("mqtt client starting\n");


    // Cria registro com os dados do cliente
    static MQTT_CLIENT_DATA_T state;

    // Inicializa a arquitetura do cyw43
    if (cyw43_arch_init()) {
        panic("Failed to inizialize CYW43");
    }

    // Usa identificador único da placa
    char unique_id_buf[5];
    pico_get_unique_board_id_string(unique_id_buf, sizeof(unique_id_buf));
    for(int i=0; i < sizeof(unique_id_buf) - 1; i++) {
        unique_id_buf[i] = tolower(unique_id_buf[i]);
    }

    // Gera nome único, Ex: pico1234
    char client_id_buf[sizeof(MQTT_DEVICE_NAME) + sizeof(unique_id_buf) - 1];
    memcpy(&client_id_buf[0], MQTT_DEVICE_NAME, sizeof(MQTT_DEVICE_NAME) - 1);
    memcpy(&client_id_buf[sizeof(MQTT_DEVICE_NAME) - 1], unique_id_buf, sizeof(unique_id_buf) - 1);
    client_id_buf[sizeof(client_id_buf) - 1] = 0;
    INFO_printf("Device name %s\n", client_id_buf);

    state.mqtt_client_info.client_id = client_id_buf;
    state.mqtt_client_info.keep_alive = MQTT_KEEP_ALIVE_S; // Keep alive in sec
#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
    state.mqtt_client_info.client_user = MQTT_USERNAME;
    state.mqtt_client_info.client_pass = MQTT_PASSWORD;
#else
    state.mqtt_client_info.client_user = NULL;
    state.mqtt_client_info.client_pass = NULL;
#endif
    static char will_topic[MQTT_TOPIC_LEN];
    strncpy(will_topic, full_topic(&state, MQTT_WILL_TOPIC), sizeof(will_topic));
    state.mqtt_client_info.will_topic = will_topic;
    state.mqtt_client_info.will_msg = MQTT_WILL_MSG;
    state.mqtt_client_info.will_qos = MQTT_WILL_QOS;
    state.mqtt_client_info.will_retain = true;
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    // TLS enabled
#ifdef MQTT_CERT_INC
    static const uint8_t ca_cert[] = TLS_ROOT_CERT;
    static const uint8_t client_key[] = TLS_CLIENT_KEY;
    static const uint8_t client_cert[] = TLS_CLIENT_CERT;
    // This confirms the indentity of the server and the client
    state.mqtt_client_info.tls_config = altcp_tls_create_config_client_2wayauth(ca_cert, sizeof(ca_cert),
            client_key, sizeof(client_key), NULL, 0, client_cert, sizeof(client_cert));
#if ALTCP_MBEDTLS_AUTHMODE != MBEDTLS_SSL_VERIFY_REQUIRED
    WARN_printf("Warning: tls without verification is insecure\n");
#endif
#else
    state->client_info.tls_config = altcp_tls_create_config_client(NULL, 0);
    WARN_printf("Warning: tls without a certificate is insecure\n");
#endif
#endif

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("Failed to connect");
    }
    INFO_printf("\nConnected to Wifi\n");

    //Faz um pedido de DNS para o endereço IP do servidor MQTT
    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(MQTT_SERVER, &state.mqtt_server_address, dns_found, &state);
    cyw43_arch_lwip_end();

    // Se tiver o endereço, inicia o cliente
    if (err == ERR_OK) {
        start_client(&state);
    } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
        panic("dns request failed");
    }

    // Som de inicialização do sistema
    somInicializacao(BUZZER2);

    // Loop condicionado a conexão mqtt
    while (!state.connect_done || mqtt_client_is_connected(state.mqtt_client_inst)) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));

        distancia = getCmFiltered(TRIGGER, ECHO, 6); // Mede a distância com filtragem para reduzir ruídos
        if (distancia < 2) distancia = 2; // Valor mínimo seguro para evitar travamento
        
        // Limpa o display para nova renderização
        ssd1306_fill(&ssd, false);
        
        // Toca o som da abertura da Porta/Portão quando necessário
        if (tocar_som_abertura) {
            tocar_som_abertura = false;
            buzzer_pwm_off(BUZZER2);
            somAberturaPortao(BUZZER2); 
        }
        
        // Toca o som de fechamento da Porta/Portão quando necessário
        if (tocar_som_fechamento) {
            apagarMatriz(); // Apaga a matriz LED
            tocar_som_fechamento = false;
            somFechamentoPortao(BUZZER2);
        }
        
        // Máquina de estados do sistema
        switch (estadoAtual) {
            case ESPERANDO:
            // Transição para PRESENCA_DETECTADA se detectar objeto próximo
            if (distancia <= 30) {
                estadoAtual = PRESENCA_DETECTADA;
            }
            setLeds(0, 0, 1); // LED Azul indica modo de espera
            drawImage(&ssd, cadeado_fechado); // Mostra ícone de cadeado fechado
            break;
            
            case PRESENCA_DETECTADA:
            // Retorna para ESPERANDO se não houver mais presença
            if (distancia > 30) {
                estadoAtual = ESPERANDO;
            }
            desenhoX(); // Desenha um "X" na matriz LED
            setLeds(1, 0, 0); // LED Vermelho indica alerta
            drawImage(&ssd, alerta); // Mostra ícone de alerta
            alarmePresencaPWM(BUZZER1); // Aciona o alarme sonoro
            apagarMatriz(); // Apaga a matriz LED
            break;
            
            case PORTAO_ABERTO:
            desenhoCheck(); // Desenha um "check" na matriz LED
            setLeds(0, 1, 0);  // LED Verde indica portão aberto
            drawImage(&ssd, cadeado_aberto);  // Mostra ícone de cadeado aberto
            break;
        }


        INFO_printf("Distância: %llu cm\n", distancia);
        
        ssd1306_send_data(&ssd); // Atualiza o display com as alterações
        sleep_ms(30); // Pequeno atraso para estabilidade
    }

    INFO_printf("mqtt client exiting\n");
    return 0;
}

//======================================================
// FUNÇÕES DE INICIALIZAÇÃO
//======================================================
void setup() {
    stdio_init_all(); // Inicializa stdio
    setupUltrasonicPins(TRIGGER, ECHO); // Configura pinos do sensor ultrassônico
    setupLED(LED_RED); // Configura LED vermelho
    setupLED(LED_GREEN); // Configura LED verde
    setupLED(LED_BLUE); // Configura LED azul
    setup_I2C(I2C_PORT, I2C_SDA, I2C_SCL, 400 * 1000); // Configura I2C a 400kHz
    setup_ssd1306(&ssd, SSD1306_ADDRESS, I2C_PORT); // Inicializa display OLED
    setup_PIO(); // Configura matriz LED 5x5
    init_pwm_buzzer(BUZZER1); // Inicializa buzzer 1 com PWM
    init_pwm_buzzer(BUZZER2); // Inicializa buzzer 2 com PWM
}

// Requisição para publicar
static void pub_request_cb(__unused void *arg, err_t err) {
    if (err != 0) {
        ERROR_printf("pub_request_cb failed %d", err);
    }
}

//Topico MQTT
static const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name) {
#if MQTT_UNIQUE_TOPIC
    static char full_topic[MQTT_TOPIC_LEN];
    snprintf(full_topic, sizeof(full_topic), "/%s%s", state->mqtt_client_info.client_id, name);
    return full_topic;
#else
    return name;
#endif
}

// Controle do portão
static void control_gate(MQTT_CLIENT_DATA_T *state, bool open) {
    // Publica o estado do portão
    const char* message = open ? "Open" : "Close";
    if (open) {
        tocar_som_abertura = true;  // Seta flag
        estadoAtual = PORTAO_ABERTO;
    } else {
        tocar_som_fechamento = true;  // Seta flag
        estadoAtual = (distancia > 30) ? ESPERANDO : PRESENCA_DETECTADA;
    }
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/gate/state"), message, strlen(message), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
}

// Publicar distância
static void publish_distance(MQTT_CLIENT_DATA_T *state) {
    static float old_distance;
    const char *distance_key = full_topic(state, "/distance");
    uint64_t distance = distancia; // ← USA A VARIÁVEL GLOBAL
    if (distance != old_distance) {
        old_distance = distance;
        // Publish distance on /distance topic
        char dist_str[16];
        snprintf(dist_str, sizeof(dist_str), "%llu", distance); // Use %llu para uint64_t
        INFO_printf("Publishing %s to %s\n", dist_str, distance_key);
        mqtt_publish(state->mqtt_client_inst, distance_key, dist_str, strlen(dist_str), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    }
}

// Publicar status
static void publish_status(MQTT_CLIENT_DATA_T *state) {
    char status[64];
    const char *status_key = full_topic(state, "/status");
    if (estadoAtual == ESPERANDO)
        strcpy(status, "Portao fechado – sem presença detectada");
    else if (estadoAtual == PRESENCA_DETECTADA)
        strcpy(status, "Presença detectada – aguardando ação");
    else
        strcpy(status, "Portao aberto – acesso autorizado");

    INFO_printf("Publishing status: %s to %s\n", status, status_key);
    mqtt_publish(state->mqtt_client_inst, status_key, status, strlen(status), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
}

// Requisição de Assinatura - subscribe
static void sub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        panic("subscribe request failed %d", err);
    }
    state->subscribe_count++;
}

// Requisição para encerrar a assinatura
static void unsub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        panic("unsubscribe request failed %d", err);
    }
    state->subscribe_count--;
    assert(state->subscribe_count >= 0);

    // Stop if requested
    if (state->subscribe_count <= 0 && state->stop_client) {
        mqtt_disconnect(state->mqtt_client_inst);
    }
}

// Tópicos de assinatura
static void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub) {
    mqtt_request_cb_t cb = sub ? sub_request_cb : unsub_request_cb;
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/gate"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/print"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/ping"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/exit"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
}

// Dados de entrada MQTT
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
#if MQTT_UNIQUE_TOPIC
    const char *basic_topic = state->topic + strlen(state->mqtt_client_info.client_id) + 1;
#else
    const char *basic_topic = state->topic;
#endif
    strncpy(state->data, (const char *)data, len);
    state->len = len;
    state->data[len] = '\0';

    DEBUG_printf("Topic: %s, Message: %s\n", state->topic, state->data);
    if (strcmp(basic_topic, "/gate") == 0)
    {
        if (lwip_stricmp((const char *)state->data, "Open") == 0 || strcmp((const char *)state->data, "1") == 0)
            control_gate(state, true);
        else if (lwip_stricmp((const char *)state->data, "Close") == 0 || strcmp((const char *)state->data, "0") == 0)
            control_gate(state, false);
    } else if (strcmp(basic_topic, "/print") == 0) {
        INFO_printf("%.*s\n", len, data);
    } else if (strcmp(basic_topic, "/ping") == 0) {
        char buf[11];
        snprintf(buf, sizeof(buf), "%u", to_ms_since_boot(get_absolute_time()) / 1000);
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/uptime"), buf, strlen(buf), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    } else if (strcmp(basic_topic, "/exit") == 0) {
        state->stop_client = true; // stop the client when ALL subscriptions are stopped
        sub_unsub_topics(state, false); // unsubscribe
    }
}

// Dados de entrada publicados
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(state->topic, topic, sizeof(state->topic));
}

// Publicar distância
static void distance_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)worker->user_data;
    publish_distance(state);
    async_context_add_at_time_worker_in_ms(context, worker, DIST_WORKER_TIME_S * 1000);
}

// publicar status do sistema
static void publish_status_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)worker->user_data;
    publish_status(state);
    async_context_add_at_time_worker_in_ms(context, worker, STATUS_WORKER_TIME_S * 800);
}

// Conexão MQTT
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        state->connect_done = true;
        sub_unsub_topics(state, true); // subscribe;

        // indicate online
        if (state->mqtt_client_info.will_topic) {
            mqtt_publish(state->mqtt_client_inst, state->mqtt_client_info.will_topic, "1", 1, MQTT_WILL_QOS, true, pub_request_cb, state);
        }

        // Publish distance every 10 sec if it's changed
        distance_worker.user_data = state;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &distance_worker, 0);

        // Adicione esta linha para ativar o worker de status:
        publish_status_worker.user_data = state;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &publish_status_worker, 0);
    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        if (!state->connect_done) {
            panic("Failed to connect to mqtt server");
        }
    }
    else {
        panic("Unexpected status");
    }
}

// Inicializar o cliente MQTT
static void start_client(MQTT_CLIENT_DATA_T *state) {
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    const int port = MQTT_TLS_PORT;
    INFO_printf("Using TLS\n");
#else
    const int port = MQTT_PORT;
    INFO_printf("Warning: Not using TLS\n");
#endif

    state->mqtt_client_inst = mqtt_client_new();
    if (!state->mqtt_client_inst) {
        panic("MQTT client instance creation error");
    }
    INFO_printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    INFO_printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&state->mqtt_server_address));

    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(state->mqtt_client_inst, &state->mqtt_server_address, port, mqtt_connection_cb, state, &state->mqtt_client_info) != ERR_OK) {
        panic("MQTT broker connection error");
    }
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    // This is important for MBEDTLS_SSL_SERVER_NAME_INDICATION
    mbedtls_ssl_set_hostname(altcp_tls_context(state->mqtt_client_inst->conn), MQTT_SERVER);
#endif
    mqtt_set_inpub_callback(state->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, state);
    cyw43_arch_lwip_end();
}

// Call back com o resultado do DNS
static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    MQTT_CLIENT_DATA_T *state = (MQTT_CLIENT_DATA_T*)arg;
    if (ipaddr) {
        state->mqtt_server_address = *ipaddr;
        start_client(state);
    } else {
        panic("dns request failed");
    }
}