#include "buzzer.h"
#include "pico/stdlib.h"


// VARIÁVEIS GLOBAIS
bool buzzer_tom_alto = false;
absolute_time_t proximo_bipe;
bool alarme_sonando = false;


void init_buzzer() {
    gpio_init(BUZZER1);
    gpio_set_dir(BUZZER1, GPIO_OUT);
}

// Função para gerar som no buzzer por um tempo específico e com a frequência desejada
void buzzer_on(uint buzzer_pin, float frequency, uint duration_ms) {
    float period = 1.0f / frequency;   // Calcula o período da onda
    float half_period = period / 2;    // Meio período em segundos
    uint cycles = (duration_ms * frequency) / 1000; // Número de ciclos necessários

    for (uint i = 0; i < cycles; i++)
    {
        gpio_put(buzzer_pin, 1);            // Liga o buzzer (HIGH)
        sleep_us((int)(half_period * 1e6)); // Aguarda meio período
        gpio_put(buzzer_pin, 0);            // Desliga o buzzer (LOW)
        sleep_us((int)(half_period * 1e6)); // Aguarda meio período
    }
}

// SOM DE ALARME
void ativarAlarme(){
    alarme_sonando = true;
    proximo_bipe = make_timeout_time_ms(0); // dispara imediatamente
}

void atualizar_buzzer_alarme() {
    if (!alarme_sonando) return;

    if (!time_reached(proximo_bipe)) return;

    float freq = buzzer_tom_alto ? 1000.0f : 500.0f;
    buzzer_tom_alto = !buzzer_tom_alto;

    // Gera um bipe de 50ms com a frequência escolhida
    buzzer_on(BUZZER1, freq, 50);

    // Define o próximo bipe daqui a 200ms
    proximo_bipe = make_timeout_time_ms(100);

}

// Função para gerar um som curto de confirmação
void buzzer_confirmacao() {
    float freq_inicial = 800.0f;  // Frequência inicial
    float freq_final = 1200.0f;   // Frequência final
    uint duracao_bip = 80;        // Duração de cada bip em ms
    uint intervalo = 50;          // Pequena pausa entre bipes

    // Três bipes curtos e crescentes
    for (int i = 0; i < 3; i++) {
        buzzer_on(BUZZER1, freq_inicial + (i * 100), duracao_bip);
        sleep_ms(intervalo);
    }
}

// ============================================================================
//                               PWM
// ============================================================================

// Função para inicializar o PWM do buzzer
void init_pwm_buzzer(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM); // Define o pino como saída PWM
    uint slice = pwm_gpio_to_slice_num(gpio); // Obtém o slice do PWM para o pino
    pwm_set_clkdiv(slice, 4.0f); // Divide o clock base (ajuste fino se necessário)
    pwm_set_enabled(slice, true); // Habilita o PWM no slice
}

// Função para ativar o PWM do buzzer com frequência e ciclo de trabalho específicos
void buzzer_pwm_on(float freq_hz, float duty_cycle, uint gpio) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint32_t clock_hz = 125000000; // Clock padrão do RP2040
    uint32_t wrap = clock_hz / (4 * freq_hz); // PWM_CLK_DIV = 4.0
    if (wrap < 10) wrap = 10;
    if (wrap > 65535) wrap = 65535;

    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), wrap * duty_cycle);
    pwm_set_enabled(slice, true);
}

// Função para desativar o PWM do buzzer
void buzzer_pwm_off(uint gpio) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), 0);
    pwm_set_enabled(slice, false);
}

// SONS

void somInicializacao(uint gpio) {
    float frequencias[] = {600.0f, 900.0f, 1200.0f};
    for (int i = 0; i < 3; i++) {
        buzzer_pwm_on(frequencias[i], 0.5f, gpio);
        sleep_ms(100);
        buzzer_pwm_off(gpio);
        sleep_ms(100);
    }
}

void somAberturaPortao(uint gpio) {
    float frequencias[] = {800.0f, 1000.0f};
    for (int i = 0; i < 2; i++) {
        buzzer_pwm_on(frequencias[i], 0.5f, gpio);
        sleep_ms(90);
        buzzer_pwm_off(gpio);
        sleep_ms(30);
    }
}

void somFechamentoPortao(uint gpio) {
    buzzer_pwm_on(500.0f, 0.5f, gpio);
    sleep_ms(150);
    buzzer_pwm_off(gpio);
}

void alarmePresencaPWM(uint gpio) {
    float frequencias[] = {1200.0f, 800.0f};
    for (int i = 0; i < 6; i++) {
        buzzer_pwm_on(frequencias[i % 2], 0.3f, gpio); // 30% duty
        sleep_ms(20);
        buzzer_pwm_off(gpio);
        sleep_ms(20);
    }
}

