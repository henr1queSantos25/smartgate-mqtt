#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "led_5x5.h"

// ARQUIVO .pio
#include "build/ws2812.pio.h"


// INICIALIZAÇÃO E CONFIGURAÇÃO DO PIO
void setup_PIO() {
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    uint sm = pio_claim_unused_sm(pio, true);
    ws2812_program_init(pio, sm, offset, OUT_PIN);
    matrix_rgb(0.0, 0.0, 0.0); // Inicializa a matriz com todos os LEDs apagados
}

// FUNÇÃO PARA ENVIAR O VALOR RGB PARA A MATRIZ
uint32_t matrix_rgb(double r, double g, double b) {
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// ACENDE TODOS OS LEDS COM UMA COR ESPECÍFICA
void drawMatrix(uint cor) {
    uint32_t valor_led;

    /*
    "0. Azul", 
    "1. Verde", 
    "2. Vermelho",     
    "3. Desligado"
    */

    switch (cor)
    {
    case 0:
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(0.0, 0.0, 0.2);
            pio_sm_put_blocking(pio0, 0, valor_led);
        }
        break;
    case 1:
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(0.0, 0.2, 0.0);
            pio_sm_put_blocking(pio0, 0, valor_led);
        }
        break;
    case 2:
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(0.2, 0.0, 0.0);
            pio_sm_put_blocking(pio0, 0, valor_led);
        }
        break;
    case 3:
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(0.0, 0.0, 0.0);
            pio_sm_put_blocking(pio0, 0, valor_led);
        }
        break;
    }
    
    

}

void apagarMatriz() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t valor_led = matrix_rgb(0.0, 0.0, 0.0);
        pio_sm_put_blocking(pio0, 0, valor_led);
    }
}

// DESENHO NOS LEDS CENTRAIS
bool isCentroMatriz(int i) {
    return i == 6 || i == 7 || i == 8 || i == 11 || i == 12 || i == 13 || i == 16 || i == 17 || i == 18;
}
void desenharCorNaMatriz(float r, float g, float b) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t valor_led;

        valor_led = isCentroMatriz(i) ? matrix_rgb(r, g, b) : matrix_rgb(0.0, 0.0, 0.0);
        
        pio_sm_put_blocking(pio0, 0, valor_led);
    }
}


void desenhoCheck() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t valor_led;

        if (i == 3 || i == 5 || i == 7 || i == 11 || i == 19) {
            valor_led = matrix_rgb(0.0, 0.01, 0.0);
        } else {
            valor_led = matrix_rgb(0.0, 0.0, 0.0);
        }

        pio_sm_put_blocking(pio0, 0, valor_led);
    }
}

void desenhoX() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t valor_led;

        if (i == 6 || i == 8 || i == 12 || i == 16 || i == 18) {
            valor_led = matrix_rgb(0.2, 0.0, 0.0);
        } else {
            valor_led = matrix_rgb(0.0, 0.0, 0.0);
        }

        pio_sm_put_blocking(pio0, 0, valor_led);
    }
}


