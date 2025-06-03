#include "hcSR04.h"

// Tempo máximo de espera pelo retorno do pulso (em microssegundos)
int timeout = 26100;


// Configura os pinos do sensor ultrassônico
void setupUltrasonicPins(uint trigPin, uint echoPin) {
    gpio_init(trigPin);
    gpio_init(echoPin);
    gpio_set_dir(trigPin, GPIO_OUT);
    gpio_set_dir(echoPin, GPIO_IN);
}


// Obtém a duração do pulso de eco em microssegundos
uint64_t getPulse(uint trigPin, uint echoPin) {
    // Envia pulso de trigger de 10μs
    gpio_put(trigPin, 1);
    sleep_us(10);
    gpio_put(trigPin, 0);

    // Aguarda até que o pino echo fique HIGH (com timeout melhorado)
    absolute_time_t timeout_start = get_absolute_time();
    while (gpio_get(echoPin) == 0) {
        if (absolute_time_diff_us(timeout_start, get_absolute_time()) > 30000) {
            return 0; // Timeout - sem eco detectado
        }
        tight_loop_contents();
    }
    
    // Marca o início do pulso
    absolute_time_t startTime = get_absolute_time();
    
    // Mede o tempo até que o pino echo volte para LOW
    while (gpio_get(echoPin) == 1) {
        if (absolute_time_diff_us(startTime, get_absolute_time()) > 25000) {
            return 0; // Timeout - pulso muito longo
        }
        tight_loop_contents();
    }
    
    absolute_time_t endTime = get_absolute_time();
    return absolute_time_diff_us(startTime, endTime);
}

// Obtém a distância em centímetros
uint64_t getCm(uint trigPin, uint echoPin) {
    uint64_t pulseLength = getPulse(trigPin, echoPin);
    return pulseLength / 29 / 2;  // Fórmula: (tempo em μs) / 29 / 2 = distância em cm
}


// Obtém a distância em polegadas
uint64_t getInch(uint trigPin, uint echoPin) {
    uint64_t pulseLength = getPulse(trigPin, echoPin);
    return (long)pulseLength / 74.f / 2.f;  // Fórmula: (tempo em μs) / 74 / 2 = distância em polegadas
}

// Obtém a distância filtrada em centímetros usando múltiplas amostras
uint64_t getCmFiltered(uint trigPin, uint echoPin, int samples) {
    uint64_t values[samples];
    int valid_readings = 0;
    
    // Coleta várias amostras válidas
    for (int i = 0; i < samples; i++) {
        uint64_t reading = getCm(trigPin, echoPin);
        // Só aceita leituras válidas (entre 2 e 400 cm)
        if (reading >= 2 && reading <= 400) {
            values[valid_readings] = reading;
            valid_readings++;
        }
        sleep_ms(15);  // Atraso entre leituras
    }
    
    // Se não tiver leituras válidas suficientes, retorna valor padrão
    if (valid_readings < samples/2) {
        return 400; // Valor padrão para "muito longe"
    }
    
    // Ordenar os valores válidos
    for (int i = 0; i < valid_readings - 1; i++) {
        for (int j = i + 1; j < valid_readings; j++) {
            if (values[i] > values[j]) {
                uint64_t temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }
    
    // Retornar a mediana
    return values[valid_readings / 2];
}