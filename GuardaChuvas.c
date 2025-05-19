/*
 * GuardaChuvas: Estação de Alerta de Enchente com Simulação de Sensores
 * Autor: Daniel Silva de Souza
 * Data: 18/05/2025
 * Descrição: Sistema embarcado com FreeRTOS para monitoramento de nível de água
 * e volume de chuva, com alertas visuais (display OLED, LED RGB, matriz WS2812B 5x5)
 * e sonoros (buzzer). Usa apenas filas para comunicação, sem semáforos ou mutexes.
 */

/* === Inclusão de Bibliotecas === */
#include "pico/stdlib.h"           // Biblioteca padrão do Pico SDK para GPIOs e inicialização
#include "hardware/gpio.h"         // Funções para configuração e manipulação de GPIOs
#include "hardware/adc.h"          // Funções para conversão analógico-digital (ADC)
#include "hardware/i2c.h"          // Funções para comunicação I2C (usada pelo display OLED)
#include "hardware/pwm.h"          // Funções para modulação por largura de pulso (PWM) para LED RGB e buzzer
#include "lib/ssd1306.h"           // Biblioteca para controle do display OLED SSD1306
#include "lib/font.h"              // Fonte para exibição de caracteres no display OLED
#include "FreeRTOS.h"              // Núcleo do FreeRTOS para gerenciamento de tarefas e filas
#include "task.h"                  // Funções para criação e manipulação de tarefas
#include "queue.h"                 // Funções para criação e uso de filas
#include <stdio.h>                 // Funções padrão de entrada/saída (ex.: printf para depuração)
#include <string.h>                // Funções para manipulação de strings (ex.: snprintf)
#include "pico/bootrom.h"          // Funções para reinicialização em modo BOOTSEL
#include "lib/animacoes.h"         // Funções para animações na matriz WS2812B 5x5

/* === Definições de Hardware === */
// Pinos I2C para o display OLED SSD1306
#define I2C_PORT i2c1              // Porta I2C usada (i2c1)
#define I2C_SDA 14                 // Pino SDA (GPIO14)
#define I2C_SCL 15                 // Pino SCL (GPIO15)
#define ENDERECO_OLED 0x3C         // Endereço I2C do display OLED (0x3C)

// Pinos ADC para sensores simulados
#define ADC_SENSOR_CHUVA 26        // GPIO26 (ADC0) para sensor de volume de chuva
#define ADC_SENSOR_AGUA 27         // GPIO27 (ADC1) para sensor de nível de água

// Pinos PWM para LED RGB
#define LED_RGB_RED 13             // GPIO13 para canal vermelho do LED RGB
#define LED_RGB_GREEN 11           // GPIO11 para canal verde do LED RGB
#define LED_RGB_BLUE 12            // GPIO12 para canal azul do LED RGB

// Pino PIO para matriz WS2812B
#define MATRIZ_WS2812B 7           // GPIO7 para matriz de LEDs WS2812B 5x5

// Pino PWM para buzzer
#define BUZZER 21                  // GPIO21 para buzzer

// Pino de entrada para botão
#define BOTAO_B 6                  // GPIO6 para botão B (BOOTSEL)

// Definição redundante (já declarada acima, pode ser um erro no código original)
#define MATRIZ_WS2812B 7           // Repetição do pino da matriz WS2812B

/* === Estrutura de Dados === */
// Estrutura para armazenar leituras dos sensores
typedef struct
{
    uint16_t chuva;                // Valor bruto do sensor de chuva (0–4095)
    uint16_t agua;                 // Valor bruto do sensor de nível de água (0–4095)
} sensor_data_t;

/* === Enumeração de Estados === */
// Estados possíveis do sistema com base nas condições de risco
typedef enum
{
    SEGURO,                        // Condição segura (baixo risco)
    ALERTA,                        // Condição de alerta (risco moderado)
    ENCHENTE                       // Condição de enchente (alto risco)
} alert_state_t;

/* === Variáveis Globais === */
volatile alert_state_t system_state = SEGURO; // Estado inicial do sistema (Seguro)
QueueHandle_t xQueueSensorData;               // Fila para comunicação de dados dos sensores

/* === Manipulador de Interrupção do Botão B === */
// Função chamada quando o botão B (BOOTSEL) é pressionado
void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == BOTAO_B && events & GPIO_IRQ_EDGE_FALL) // Verifica se é o botão B com borda de descida
    {
        printf("Botão B pressionado: entrando em modo BOOTSEL\n"); // Log de depuração
        reset_usb_boot(0, 0); // Reinicia a placa em modo BOOTSEL para upload de firmware
    }
}

/* === Tarefa de Leitura dos Sensores === */
// Tarefa responsável por ler os sensores de chuva e nível de água via ADC
void vSensorTask(void *params)
{
    // Configura os pinos ADC
    adc_gpio_init(ADC_SENSOR_AGUA);  // Inicializa GPIO27 como entrada ADC
    adc_gpio_init(ADC_SENSOR_CHUVA); // Inicializa GPIO26 como entrada ADC
    adc_init();                      // Inicializa o módulo ADC do RP2040

    sensor_data_t sensordata;        // Estrutura para armazenar leituras
    while (true)
    {
        // Lê sensor de nível de água (ADC0, GPIO26)
        adc_select_input(0);         // Seleciona canal ADC0
        sensordata.agua = adc_read(); // Lê valor bruto (0–4095)

        // Lê sensor de volume de chuva (ADC1, GPIO27)
        adc_select_input(1);         // Seleciona canal ADC1
        sensordata.chuva = adc_read(); // Lê valor bruto (0–4095)

        // Calcula percentuais para depuração
        uint8_t nivel_agua = (sensordata.agua * 100) / 4095;   // Converte para 0–100%
        uint8_t volume_chuva = (sensordata.chuva * 100) / 4095; // Converte para 0–100%

        // Log de depuração com valores brutos e percentuais
        printf("Sensor Chuva: %u (%d%%), Sensor Água: %u (%d%%)\n",
               sensordata.chuva, volume_chuva, sensordata.agua, nivel_agua);

        // Envia os dados brutos para a fila
        xQueueSend(xQueueSensorData, &sensordata, 0); // Envia sem espera
        vTaskDelay(pdMS_TO_TICKS(100));              // Executa a 10 Hz (100ms)
    }
}

/* === Tarefa do Display OLED === */
// Tarefa responsável por exibir informações no display OLED SSD1306
void vDisplayTask(void *params)
{
    // Inicializa a comunicação I2C
    i2c_init(I2C_PORT, 400 * 1000); // Configura I2C a 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define GPIO14 como SDA
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define GPIO15 como SCL
    gpio_pull_up(I2C_SDA);                    // Ativa pull-up interno
    gpio_pull_up(I2C_SCL);                    // Ativa pull-up interno

    // Inicializa o display OLED
    ssd1306_t ssd;                            // Estrutura de controle do display
    ssd1306_init(&ssd, 128, 64, false, ENDERECO_OLED, I2C_PORT); // Inicializa: 128x64, sem VCC externo
    ssd1306_config(&ssd);                     // Configura parâmetros do display
    ssd1306_fill(&ssd, false);                // Limpa o buffer do display
    ssd1306_send_data(&ssd);                  // Atualiza o display (limpo)

    sensor_data_t sensordata;                  // Estrutura para receber dados
    char buffer[32];                          // Buffer para formatar strings
    const char *status;                       // Ponteiro para string de status
    while (true)
    {
        // Recebe dados da fila (bloqueia até receber)
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE)
        {
            // Converte valores brutos para percentuais
            uint8_t nivel_agua = (sensordata.agua * 100) / 4095;   // Nível de água (0–100%)
            uint8_t volume_chuva = (sensordata.chuva * 100) / 4095; // Volume de chuva (0–100%)

            // Limpa o buffer do display
            ssd1306_fill(&ssd, false);

            // Determina o estado do sistema e desenha elementos gráficos
            if (nivel_agua >= 70 || volume_chuva >= 80) // Condição de enchente
            {
                status = "Enchente";                   // Define status como "Enchente"
                ssd1306_rect(&ssd, 1, 1, 126, 62, true, false); // Borda externa
                ssd1306_rect(&ssd, 28, 10, 105, 12, true, false); // Borda para "Chuva"
            }
            else if (nivel_agua >= 50 || volume_chuva >= 50) // Condição de alerta
            {
                status = "Alerta";                     // Define status como "Alerta"
                ssd1306_rect(&ssd, 28, 10, 105, 12, true, false); // Borda para "Chuva"
            }
            else // Condição segura
            {
                status = "Seguro";                     // Define status como "Seguro"
            }

            // Desenha borda externa do display
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);

            // Exibe "Água: X%" no display
            snprintf(buffer, sizeof(buffer), "Agua: %d%%", nivel_agua); // Formata string
            ssd1306_draw_string(&ssd, buffer, 25, 4);                  // Desenha na posição (25,4)

            // Exibe "Chuva: Y%" no display
            snprintf(buffer, sizeof(buffer), "Chuva: %d%%", volume_chuva); // Formata string
            ssd1306_draw_string(&ssd, buffer, 25, 15);                   // Desenha na posição (25,15)

            // Exibe status do sistema
            snprintf(buffer, sizeof(buffer), "%s", status); // Formata string
            ssd1306_draw_string(&ssd, buffer, 35, 30);     // Desenha na posição (35,30)

            // Desenha barra gráfica para nível de água
            uint8_t barra_largura = nivel_agua; // Escala 0–100% para 0–100 pixels
            ssd1306_rect(&ssd, 48, 15, barra_largura, 8, true, true); // Barra preenchida
            ssd1306_rect(&ssd, 48, 15, 100, 8, true, false);          // Borda da barra

            // Atualiza o display com o conteúdo do buffer
            ssd1306_send_data(&ssd);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Atualiza a 10 Hz (100ms)
    }
}

/* === Tarefa do LED RGB === */
// Tarefa responsável por controlar as cores do LED RGB com base no estado do sistema
void vLedRgbTask(void *params)
{
    // Configura os pinos como PWM
    gpio_set_function(LED_RGB_RED, GPIO_FUNC_PWM);   // GPIO13 para PWM
    gpio_set_function(LED_RGB_GREEN, GPIO_FUNC_PWM); // GPIO11 para PWM
    gpio_set_function(LED_RGB_BLUE, GPIO_FUNC_PWM);  // GPIO12 para PWM

    // Obtém os slices PWM correspondentes aos pinos
    uint slice_red = pwm_gpio_to_slice_num(LED_RGB_RED);     // Slice para vermelho
    uint slice_green = pwm_gpio_to_slice_num(LED_RGB_GREEN); // Slice para verde
    uint slice_blue = pwm_gpio_to_slice_num(LED_RGB_BLUE);   // Slice para azul

    // Configura PWM: ~1 kHz, resolução de 8 bits
    pwm_set_clkdiv(slice_red, 100.0f);   // Divisor de clock para ~4.88 kHz, ajustado
    pwm_set_clkdiv(slice_green, 100.0f);
    pwm_set_clkdiv(slice_blue, 100.0f);
    pwm_set_wrap(slice_red, 255);        // Resolução de 0–255
    pwm_set_wrap(slice_green, 255);
    pwm_set_wrap(slice_blue, 255);

    // Obtém os canais PWM (A ou B) para cada pino
    uint chan_red = pwm_gpio_to_channel(LED_RGB_RED);     // Canal para vermelho
    uint chan_green = pwm_gpio_to_channel(LED_RGB_GREEN); // Canal para verde
    uint chan_blue = pwm_gpio_to_channel(LED_RGB_BLUE);   // Canal para azul

    // Inicializa com duty cycle 0 (LED desligado)
    pwm_set_chan_level(slice_red, chan_red, 0);
    pwm_set_chan_level(slice_green, chan_green, 0);
    pwm_set_chan_level(slice_blue, chan_blue, 0);

    // Habilita os slices PWM
    pwm_set_enabled(slice_red, true);
    pwm_set_enabled(slice_green, true);
    pwm_set_enabled(slice_blue, true);

    sensor_data_t sensordata; // Estrutura para receber dados
    while (true)
    {
        // Recebe dados da fila (bloqueia até receber)
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE)
        {
            // Converte valores brutos para percentuais
            uint8_t nivel_agua = (sensordata.agua * 100) / 4095;   // Nível de água
            uint8_t volume_chuva = (sensordata.chuva * 100) / 4095; // Volume de chuva

            // Define a cor do LED RGB com base no estado
            if (nivel_agua >= 70 || volume_chuva >= 80) // Condição de enchente
            {
                pwm_set_chan_level(slice_red, chan_red, 255);     // Vermelho: 100%
                pwm_set_chan_level(slice_green, chan_green, 0); // Verde: 0%
                pwm_set_chan_level(slice_blue, chan_blue, 0);   // Azul: 0%
                printf("vLedRgbTask: Vermelho (Enchente)\n");   // Log de depuração
            }
            else if (nivel_agua >= 50 || volume_chuva >= 50) // Condição de alerta
            {
                pwm_set_chan_level(slice_red, chan_red, 255);     // Vermelho: 100%
                pwm_set_chan_level(slice_green, chan_green, 255); // Verde: 100%
                pwm_set_chan_level(slice_blue, chan_blue, 0);   // Azul: 0% (amarelo)
                printf("vLedRgbTask: Amarelo (Alerta)\n");      // Log de depuração
            }
            else // Condição segura
            {
                pwm_set_chan_level(slice_red, chan_red, 0);     // Vermelho: 0%
                pwm_set_chan_level(slice_green, chan_green, 255); // Verde: 100%
                pwm_set_chan_level(slice_blue, chan_blue, 0);   // Azul: 0%
                printf("vLedRgbTask: Verde (Seguro)\n");        // Log de depuração
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Atualiza a 10 Hz (100ms)
    }
}

/* === Tarefa do Buzzer === */
// Tarefa responsável por controlar o buzzer com sons distintos para cada estado
void vBuzzerTask(void *params)
{
    // Configura o pino do buzzer como PWM
    gpio_set_function(BUZZER, GPIO_FUNC_PWM); // GPIO21 para PWM
    uint slice = pwm_gpio_to_slice_num(BUZZER); // Obtém slice PWM
    uint chan = pwm_gpio_to_channel(BUZZER);   // Obtém canal PWM

    // Configura PWM para 500 Hz com duty cycle de 50%
    uint clock = 125000000;             // Clock do RP2040 (125 MHz)
    uint divider = 4;                   // Divisor de clock
    uint top = clock / (divider * 500); // Calcula TOP para 500 Hz
    pwm_set_clkdiv(slice, divider);     // Aplica divisor
    pwm_set_wrap(slice, top);           // Define resolução
    pwm_set_chan_level(slice, chan, top / 2); // Duty cycle 50%
    pwm_set_enabled(slice, false);       // Inicia com PWM desligado

    sensor_data_t sensordata;            // Estrutura para receber dados
    while (true)
    {
        // Recebe dados da fila (bloqueia até receber)
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE)
        {
            // Converte valores brutos para percentuais
            uint8_t nivel_agua = (sensordata.agua * 100) / 4095;   // Nível de água
            uint8_t volume_chuva = (sensordata.chuva * 100) / 4095; // Volume de chuva

            // Controla o buzzer com base no estado
            if (nivel_agua >= 70 || volume_chuva >= 80) // Condição de enchente
            {
                // Beeps rápidos: 200ms ligado, 200ms desligado
                pwm_set_enabled(slice, true);           // Liga o buzzer
                printf("vBuzzerTask: Beep rápido (Enchente)\n"); // Log de depuração
                vTaskDelay(pdMS_TO_TICKS(200));        // Espera 200ms
                pwm_set_enabled(slice, false);          // Desliga o buzzer
                vTaskDelay(pdMS_TO_TICKS(200));        // Espera 200ms
            }
            else if (nivel_agua >= 50 || volume_chuva >= 50) // Condição de alerta
            {
                // Beeps curtos: 500ms ligado, 500ms desligado
                pwm_set_enabled(slice, true);           // Liga o buzzer
                printf("vBuzzerTask: Beep curto (Alerta)\n"); // Log de depuração
                vTaskDelay(pdMS_TO_TICKS(500));        // Espera 500ms
                pwm_set_enabled(slice, false);          // Desliga o buzzer
                vTaskDelay(pdMS_TO_TICKS(500));        // Espera 500ms
            }
            else // Condição segura
            {
                pwm_set_enabled(slice, false);          // Mantém buzzer desligado
                printf("vBuzzerTask: Silêncio (Seguro)\n"); // Log de depuração
                vTaskDelay(pdMS_TO_TICKS(100));        // Mantém sincronia a 10 Hz
            }
        }
    }
}

/* === Tarefa da Matriz WS2812B === */
// Tarefa responsável por controlar as animações na matriz WS2812B 5x5
void vMatrixTask(void *params)
{
    npInit(MATRIZ_WS2812B);                       // Inicializa a matriz (GPIO7, PIO)
    srand(to_ms_since_boot(get_absolute_time())); // Inicializa semente para números aleatórios
    sensor_data_t sensordata;                     // Estrutura para receber dados
    while (true)
    {
        // Recebe dados da fila (bloqueia até receber)
        if (xQueueReceive(xQueueSensorData, &sensordata, portMAX_DELAY) == pdTRUE)
        {
            // Converte valores brutos para percentuais (usado apenas para depuração)
            uint8_t nivel_agua = (sensordata.agua * 100) / 4095;   // Nível de água
            uint8_t volume_chuva = (sensordata.chuva * 100) / 4095; // Volume de chuva

            // Executa animação de enchente com base no nível de água
            anim_enchente(sensordata.agua); // Passa valor bruto para a função
        }
    }
}

/* === Função Principal === */
int main()
{
    // Configura o botão B (BOOTSEL)
    gpio_init(BOTAO_B);                       // Inicializa GPIO6
    gpio_set_dir(BOTAO_B, GPIO_IN);          // Configura como entrada
    gpio_pull_up(BOTAO_B);                   // Ativa pull-up interno
    // Habilita interrupção na borda de descida
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();                        // Inicializa comunicação serial (UART) para printf
    // Cria fila para dados dos sensores (6 elementos, tamanho de sensor_data_t)
    xQueueSensorData = xQueueCreate(6, sizeof(sensor_data_t));

    // Cria tarefas do FreeRTOS
    xTaskCreate(vSensorTask, "Sensor Task", 256, NULL, 1, NULL);   // Tarefa de sensores
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 2, NULL); // Tarefa do display
    xTaskCreate(vLedRgbTask, "LED RGB Task", 256, NULL, 2, NULL);  // Tarefa do LED RGB
    xTaskCreate(vBuzzerTask, "Buzzer Task", 256, NULL, 2, NULL);   // Tarefa do buzzer
    xTaskCreate(vMatrixTask, "Matriz Task", 256, NULL, 2, NULL);   // Tarefa da matriz

    vTaskStartScheduler();                   // Inicia o escalonador do FreeRTOS
    panic_unsupported();                     // Caso o escalonador falhe
}