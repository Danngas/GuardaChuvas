/*
 * GuardaChuvas: Estação de Alerta de Enchente com Simulação de Sensores
 * Autor: Daniel Silva de Souza
 * Data: 18/05/2025
 * Descrição: Sistema embarcado com FreeRTOS para monitoramento de nível de água
 * e volume de chuva, com alertas visuais (display OLED, LED RGB, matriz WS2812B 5x5)
 * e sonoros (buzzer). Usa apenas filas para comunicação, sem semáforos ou mutexes.
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include "pico/bootrom.h" // Biblioteca para reinicialização via USB

// Configurações de hardware
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_OLED 0x3C
#define ADC_SENSOR_CHUVA 26
#define ADC_SENSOR_AGUA 27
#define LED_RGB_RED 13
#define LED_RGB_GREEN 11
#define LED_RGB_BLUE 12
#define MATRIZ_WS2812B 7
#define BUZZER 10
#define BOTAO_B 6

// Estrutura para dados dos sensores
typedef struct {
    uint16_t chuva; // Volume de chuva
    uint16_t agua;  // Nível de água
} sensor_data_t;

// Enum para estados de alerta
typedef enum {
    SEGURO,
    ALERTA,
    ENCHENTE
} alert_state_t;

// Variável global para o estado do sistema
volatile alert_state_t system_state = SEGURO;

QueueHandle_t xQueueSensorData;

// Manipulador de interrupção para Botão B (BOOTSEL)
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BOTAO_B && events & GPIO_IRQ_EDGE_FALL) {
        printf("Botão B pressionado: entrando em modo BOOTSEL\n");
        reset_usb_boot(0, 0);
    }
}

// Tarefa de leitura dos sensores
void vSensorTask(void *params) {
    adc_gpio_init(ADC_SENSOR_AGUA); // Configura GPIO27 como ADC
    adc_gpio_init(ADC_SENSOR_CHUVA); // Configura GPIO26 como ADC
    adc_init(); // Inicializa o conversor ADC

    sensor_data_t sensordata;
    while (true) {
        adc_select_input(0); // GPIO26 = ADC0 (nível de água)
        sensordata.agua = adc_read();
        adc_select_input(1); // GPIO27 = ADC1 (volume de chuva)
        sensordata.chuva = adc_read();

        // Mapeia valores ADC (0-4095) para percentuais (0-100)
        uint8_t nivel_agua = (sensordata.agua * 100) / 4095;
        uint8_t volume_chuva = (sensordata.chuva * 100) / 4095;

        // Depuração: imprime valores brutos e percentuais
        printf("Sensor Chuva: %u (%d%%), Sensor Água: %u (%d%%)\n",
               sensordata.chuva, volume_chuva, sensordata.agua, nivel_agua);

        // Envia dados para a fila
        xQueueSend(xQueueSensorData, &sensordata, 0);
        vTaskDelay(pdMS_TO_TICKS(100)); // Leitura a 10 Hz
    }
}

// Tarefa de lógica de alerta
void vAlertLogicTask(void *params) {
    sensor_data_t sensordata;
    while (true) {
        // Usa xQueuePeek para não consumir a mensagem
        if (xQueuePeek(xQueueSensorData, &sensordata, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Mapeia valores para percentuais
            uint8_t nivel_agua = (sensordata.agua * 100) / 4095;
            uint8_t volume_chuva = (sensordata.chuva * 100) / 4095;

            // Determina o estado
            alert_state_t new_state;
            if (nivel_agua >= 80 || volume_chuva >= 80) {
                new_state = ENCHENTE;
            } else if (nivel_agua >= 50 || volume_chuva >= 50) {
                new_state = ALERTA;
            } else {
                new_state = SEGURO;
            }

            // Atualiza o estado global
            system_state = new_state;

            // Depuração: imprime estado atualizado
            printf("vAlertLogicTask: Estado atualizado: %d (0=Seguro, 1=Alerta, 2=Enchente)\n", (int)new_state);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Processa a 10 Hz
    }
}

// Tarefa do display OLED
void vDisplayTask(void *params) {
    // Inicializa I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa display
    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, ENDERECO_OLED, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd);

    sensor_data_t sensordata;
    char buffer[32];
    while (true) {
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE) {
            // Mapeia valores para percentuais
            uint8_t nivel_agua = (sensordata.agua * 100) / 4095;
            uint8_t volume_chuva = (sensordata.chuva * 100) / 4095;

            // Limpa o display
            ssd1306_fill(&ssd, false);

            // Consulta o estado global
            const char *status;
            if (system_state == ENCHENTE) {
                status = "Enchente";
                ssd1306_rect(&ssd, 1, 1, 126, 62, true, false);
                ssd1306_rect(&ssd, 28, 10, 105, 12, true, false);
            } else if (system_state == ALERTA) {
                status = "Alerta";
                ssd1306_rect(&ssd, 28, 10, 105, 12, true, false);
            } else {
                status = "Seguro";
            }
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);

            // Exibe "Água: X%"
            snprintf(buffer, sizeof(buffer), "Agua: %d%%", nivel_agua);
            ssd1306_draw_string(&ssd, buffer, 25, 4);

            // Exibe "Chuva: Y%"
            snprintf(buffer, sizeof(buffer), "Chuva: %d%%", volume_chuva);
            ssd1306_draw_string(&ssd, buffer, 25, 15);

            // Exibe status
            snprintf(buffer, sizeof(buffer), "%s", status);
            ssd1306_draw_string(&ssd, buffer, 35, 30);

            // Desenha barra gráfica para nível de água (100 pixels de largura, 8 pixels de altura)
            uint8_t barra_largura = nivel_agua; // Escala 0-100% para 0-100 pixels
            ssd1306_rect(&ssd, 48, 15, barra_largura, 8, true, true);
            ssd1306_rect(&ssd, 48, 15, 100, 8, true, false);

            // Atualiza o display
            ssd1306_send_data(&ssd);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Atualiza a 10 Hz
    }
}

// Tarefa de controle do LED RGB
void vLedRgbTask(void *params) {
    // Configura GPIOs como PWM
    gpio_set_function(LED_RGB_RED, GPIO_FUNC_PWM);
    gpio_set_function(LED_RGB_GREEN, GPIO_FUNC_PWM);
    gpio_set_function(LED_RGB_BLUE, GPIO_FUNC_PWM);

    // Obtém os slices PWM
    uint slice_red = pwm_gpio_to_slice_num(LED_RGB_RED);
    uint slice_green = pwm_gpio_to_slice_num(LED_RGB_GREEN);
    uint slice_blue = pwm_gpio_to_slice_num(LED_RGB_BLUE);

    // Configura PWM: ~1 kHz, resolução de 8 bits
    pwm_set_clkdiv(slice_red, 100.0f); // 125 MHz / 100 / 256 = ~4.88 kHz, ajustado para ~1 kHz
    pwm_set_clkdiv(slice_green, 100.0f);
    pwm_set_clkdiv(slice_blue, 100.0f);
    pwm_set_wrap(slice_red, 255); // Resolução 0–255
    pwm_set_wrap(slice_green, 255);
    pwm_set_wrap(slice_blue, 255);

    // Define canais PWM (A ou B dependendo do GPIO)
    uint chan_red = pwm_gpio_to_channel(LED_RGB_RED);
    uint chan_green = pwm_gpio_to_channel(LED_RGB_GREEN);
    uint chan_blue = pwm_gpio_to_channel(LED_RGB_BLUE);

    // Duty inicial: 0 (desligado)
    pwm_set_chan_level(slice_red, chan_red, 0);
    pwm_set_chan_level(slice_green, chan_green, 0);
    pwm_set_chan_level(slice_blue, chan_blue, 0);

    // Habilita PWM
    pwm_set_enabled(slice_red, true);
    pwm_set_enabled(slice_green, true);
    pwm_set_enabled(slice_blue, true);

    while (true) {
        // Ajusta cores com base no system_state
        switch (system_state) {
            case SEGURO:
                pwm_set_chan_level(slice_red, chan_red, 0);     // Vermelho: 0
                pwm_set_chan_level(slice_green, chan_green, 1); // Verde: 100%
                pwm_set_chan_level(slice_blue, chan_blue, 0);    // Azul: 0
                printf("vLedRgbTask: Verde (Seguro)\n");
                break;
            case ALERTA:
                pwm_set_chan_level(slice_red, chan_red, 1);   // Vermelho: 100%
                pwm_set_chan_level(slice_green, chan_green, 1); // Verde: 100%
                pwm_set_chan_level(slice_blue, chan_blue, 0);    // Azul: 0
                printf("vLedRgbTask: Amarelo (Alerta)\n");
                break;
            case ENCHENTE:
                pwm_set_chan_level(slice_red, chan_red, 1);   // Vermelho: 100%
                pwm_set_chan_level(slice_green, chan_green, 0);  // Verde: 0
                pwm_set_chan_level(slice_blue, chan_blue, 0);    // Azul: 0
                printf("vLedRgbTask: Vermelho (Enchente)\n");
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Atualiza a 10 Hz
    }
}

int main() {
    // Configura Botão B (BOOTSEL)
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();
    xQueueSensorData = xQueueCreate(5, sizeof(sensor_data_t));

    // Criação das tarefas
    xTaskCreate(vSensorTask, "Sensor Task", 256, NULL, 2, NULL);
    xTaskCreate(vAlertLogicTask, "Alert Logic Task", 256, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);
    xTaskCreate(vLedRgbTask, "LED RGB Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}