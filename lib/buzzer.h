#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define BUZZER1 10
#define BUZZER2 21

#define BUZZER1_PWM_SLICE pwm_gpio_to_slice_num(BUZZER1)
#define BUZZER2_PWM_SLICE pwm_gpio_to_slice_num(BUZZER2)


void init_buzzer();
void buzzer_on(uint buzzer_pin, float frequency, uint duration_ms);
void ativarAlarme();
void atualizar_buzzer_alarme();
void buzzer_confirmacao();
void init_pwm_buzzer(uint gpio);
void buzzer_pwm_on(float freq_hz, float duty, uint gpio);
void buzzer_pwm_off(uint gpio);
void somInicializacao(uint gpio);
void somAberturaPortao(uint gpio);
void somFechamentoPortao(uint gpio);
void alarmePresencaPWM(uint gpio);



