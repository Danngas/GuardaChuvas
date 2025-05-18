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
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
// MODO BOOTSEL (reset via botão B)
#include "pico/bootrom.h" // Biblioteca para reinicialização via USB

// Configurações de hardware
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_OLED 0x3C
#define ADC_SENSOR_CHUVA 26
#define ADC_SENSOR_AGUA 27
#define LED_RGB_RED 11
#define LED_RGB_GREEN 12
#define LED_RGB_BLUE 13
#define MATRIZ_WS2812B 7
#define BUZZER 10
#define BOTAO_B 6
#define TAM_QUAD 10

// Estrutura para dados dos sensores
typedef struct {
    uint16_t chuva; // Volume de chuva
    uint16_t agua;  // Nível de água
} sensor_data_t;

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

        // Envia dados para a fila (único mecanismo de comunicação)
        xQueueSend(xQueueSensorData, &sensordata, 0);
        vTaskDelay(pdMS_TO_TICKS(100)); // Leitura a 10 Hz
    }
}

// Tarefas do código base (serão substituídas em branches futuras)
void vDisplayTask(void *params) {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_OLED, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    sensor_data_t sensordata;
    bool cor = true;
    while (true) {
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE) {
            uint8_t x = (sensordata.chuva * (128 - TAM_QUAD)) / 4095;
            uint8_t y = (sensordata.agua * (64 - TAM_QUAD)) / 4095;
            y = (64 - TAM_QUAD) - y;
            ssd1306_fill(&ssd, !cor);
            ssd1306_rect(&ssd, y, x, TAM_QUAD, TAM_QUAD, cor, !cor);
            ssd1306_send_data(&ssd);
        }
    }
}

void vLedGreenTask(void *params) {
    gpio_set_function(LED_RGB_GREEN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED_RGB_GREEN);
    pwm_set_wrap(slice, 100);
    pwm_set_chan_level(slice, PWM_CHAN_B, 0);
    pwm_set_enabled(slice, true);

    sensor_data_t sensordata;
    while (true) {
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE) {
            int16_t desvio_centro = (int16_t)sensordata.chuva - 2000;
            if (desvio_centro < 0) desvio_centro = -desvio_centro;
            uint16_t pwm_value = (desvio_centro * 100) / 2048;
            pwm_set_chan_level(slice, PWM_CHAN_B, pwm_value);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vLedBlueTask(void *params) {
    gpio_set_function(LED_RGB_BLUE, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED_RGB_BLUE);
    pwm_set_wrap(slice, 100);
    pwm_set_chan_level(slice, PWM_CHAN_A, 0);
    pwm_set_enabled(slice, true);

    sensor_data_t sensordata;
    while (true) {
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE) {
            int16_t desvio_centro = (int16_t)sensordata.agua - 2048;
            if (desvio_centro < 0) desvio_centro = -desvio_centro;
            uint16_t pwm_value = (desvio_centro * 100) / 2048;
            pwm_set_chan_level(slice, PWM_CHAN_A, pwm_value);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
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
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);
    xTaskCreate(vLedGreenTask, "LED Green Task", 256, NULL, 1, NULL);
    xTaskCreate(vLedBlueTask, "LED Blue Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}