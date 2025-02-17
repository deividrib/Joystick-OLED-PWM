#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "ssd1306.h"
#include "font.h"

// Definições dos pinos conforme especificado na atividade
#define LED_R_PIN      13   // LED Vermelho (controlado via PWM pelo eixo X)
#define LED_B_PIN      12   // LED Azul (controlado via PWM pelo eixo Y)
#define LED_G_PIN      11   // LED Verde (alternado pelo botão do joystick)

#define JOYSTICK_X_ADC 0    // ADC canal para o eixo X (GPIO26)
#define JOYSTICK_Y_ADC 1    // ADC canal para o eixo Y (GPIO27)
#define JOYSTICK_BTN   22   // Botão do Joystick (GPIO22)

#define BUTTON_A_PIN   5    // Botão A para ativar/desativar o PWM

// Parâmetros para debounce (300 ms em microsegundos) e precisão do joystick
#define DEBOUNCE_TIME  300000
#define JOYSTICK_CENTER 2048
#define DEAD_ZONE      100   // Variação menor que 100 é considerada como central

// Resolução PWM (8 bits)
#define PWM_WRAP       255

// Número de amostras para suavizar as leituras do ADC
#define NUM_SAMPLES    10

// Variáveis globais para controle de estado
volatile uint32_t last_time_btnA = 0;
volatile uint32_t last_time_jsBtn = 0;
volatile bool pwm_enabled = true;
volatile bool led_green_state = false; // Estado do LED Verde (GPIO13)
volatile uint8_t border_style = 0;       // Estilo de borda: 0 = sólida; 1 = pontilhada

uint slice_num_R, slice_num_B;
uint channel_R, channel_B;

// Instância do display SSD1306
ssd1306_t display;

// Callback de interrupção para os botões (joystick e botão A)
// Utiliza-se debounce para evitar acionamentos múltiplos
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = to_us_since_boot(get_absolute_time());
    // Botão do Joystick: alterna o LED Verde e o estilo da borda
    if (gpio == JOYSTICK_BTN) {
        if (now - last_time_jsBtn < DEBOUNCE_TIME) return;
        last_time_jsBtn = now;
        led_green_state = !led_green_state;
        gpio_put(LED_G_PIN, led_green_state);
        // Alterna entre os estilos de borda (0: sólida, 1: pontilhada)
        border_style = (border_style + 1) % 2;
    }
    // Botão A: ativa/desativa os LEDs PWM (vermelho e azul)
    else if (gpio == BUTTON_A_PIN) {
        if (now - last_time_btnA < DEBOUNCE_TIME) return;
        last_time_btnA = now;
        pwm_enabled = !pwm_enabled;
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando programa...\n");

    // Configuração dos LEDs:
    // LEDs vermelho e azul: configurados para saída PWM
    gpio_set_function(LED_R_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);
    // LED verde: saída digital simples
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_put(LED_G_PIN, led_green_state);

    // Configuração do PWM para o LED vermelho
    slice_num_R = pwm_gpio_to_slice_num(LED_R_PIN);
    channel_R = pwm_gpio_to_channel(LED_R_PIN);
    pwm_set_wrap(slice_num_R, PWM_WRAP);
    pwm_set_enabled(slice_num_R, true);

    // Configuração do PWM para o LED azul
    slice_num_B = pwm_gpio_to_slice_num(LED_B_PIN);
    channel_B = pwm_gpio_to_channel(LED_B_PIN);
    pwm_set_wrap(slice_num_B, PWM_WRAP);
    pwm_set_enabled(slice_num_B, true);

    // Inicializa o ADC para os eixos do joystick (GPIO26 e GPIO27)
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Inicializa o display SSD1306 via I2C (GPIO14 = SDA e GPIO15 = SCL)
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    ssd1306_init(&display, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&display);

    // Configuração dos botões com pull-up e interrupções
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    // Define o callback global para interrupções (para o JOYSTICK_BTN)
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    // Habilita a interrupção para o botão A (utilizando o mesmo callback)
    gpio_set_irq_enabled(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true);

    // Índice do quadrado 8x8 na fonte: assume que ele foi adicionado ao final do array 'font'
    uint16_t font_size = sizeof(font);
    uint16_t quadrado_index = font_size - 8;

    while (true) {
        // Realiza várias leituras do ADC para suavizar o valor
        uint32_t adc_x_total = 0, adc_y_total = 0;
        for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
            adc_select_input(JOYSTICK_X_ADC);
            adc_x_total += adc_read();
            adc_select_input(JOYSTICK_Y_ADC);
            adc_y_total += adc_read();
            sleep_ms(1);
        }
        // Note que aqui foi feito o "swap" dos valores para corrigir os sentidos:
        uint16_t adc_y = adc_x_total / NUM_SAMPLES;
        uint16_t adc_x = adc_y_total / NUM_SAMPLES;

        // Aplica uma zona morta para o joystick: se a variação em torno do centro for pequena, fixa no valor central
        if (abs((int)adc_x - JOYSTICK_CENTER) < DEAD_ZONE) {
            adc_x = JOYSTICK_CENTER;
        }
        if (abs((int)adc_y - JOYSTICK_CENTER) < DEAD_ZONE) {
            adc_y = JOYSTICK_CENTER;
        }

        // Controla o brilho dos LEDs via PWM com base na distância do valor central
        int diff_x = abs((int)adc_x - JOYSTICK_CENTER);
        int diff_y = abs((int)adc_y - JOYSTICK_CENTER);
        uint32_t duty_R = (pwm_enabled ? (diff_x * PWM_WRAP) / JOYSTICK_CENTER : 0);
        uint32_t duty_B = (pwm_enabled ? (diff_y * PWM_WRAP) / JOYSTICK_CENTER : 0);
        pwm_set_chan_level(slice_num_R, channel_R, duty_R);
        pwm_set_chan_level(slice_num_B, channel_B, duty_B);

        // Mapeamento CORRIGIDO:
        // Para o eixo X, mapeamos normalmente.
        // Para o eixo Y, invertemos a leitura para que "para cima" corresponda a menor valor.
        uint8_t square_x = (adc_x * (display.width - 8)) / 4095;
        uint8_t square_y = ((4095 - adc_y) * (display.height - 8)) / 4095;

        // Atualiza o display:
        ssd1306_fill(&display, 0);
        // Desenha a borda com o estilo atual (sólida ou pontilhada)
        draw_border(&display, border_style);
        // Desenha o quadrado 8x8 na posição mapeada
        ssd1306_draw_bitmap(&display, square_x, square_y, &font[quadrado_index]);
        // Exibe o status do PWM na parte inferior do display
        char status[20];
        snprintf(status, sizeof(status), "PWM: %s", pwm_enabled ? "ON" : "OFF");
        ssd1306_draw_string(&display, status, 40, 48);
        ssd1306_send_data(&display);

        sleep_ms(100);
    }
}