
ESCOPO DA IDEIA A SER IMPLEMENTADA
# GuardaChuvas ☔

**Estação de Alerta de Enchente com Simulação por Joystick**

Bem-vindo ao **GuardaChuvas**, um sistema embarcado desenvolvido com FreeRTOS na plataforma BitDogLab (Raspberry Pi Pico RP2040). O projeto simula o monitoramento de enchentes em tempo real, usando um joystick para ajustar o nível de água e o volume de chuva, e exibe alertas visuais interativos no display OLED, LED RGB e (opcionalmente) matriz de LEDs. O nome "GuardaChuvas" é um trocadilho criativo que reflete a proteção contra chuvas intensas e enchentes! 🌊

Este projeto foi criado para consolidar conhecimentos em sistemas embarcados e FreeRTOS, com foco em comunicação via filas, organização de código e criatividade nas saídas visuais.

---

## 📋 Sobre o Projeto

O **GuardaChuvas** é uma estação de alerta que:
- **Lê** os eixos X (volume de chuva) e Y (nível de água) de um joystick via ADC.
- **Processa** os dados para determinar o risco de enchente com base em limiares (Seguro, Alerta, Enchente).
- **Exibe** informações no display OLED SSD1306, incluindo percentuais de água e chuva, status de alerta e uma barra gráfica de nível.
- **Controla** um LED RGB para indicar o estado: verde (seguro), amarelo (alerta), vermelho (enchente).
- **Anima** (opcionalmente) uma matriz de LEDs para mostrar padrões de chuva ou ondas.

O sistema usa **FreeRTOS** com filas para comunicação entre tarefas, garantindo operação em tempo real sem semąco ou mutexes, conforme exigido.

---

## 🎯 Funcionalidades

- **Leitura do Joystick**: Eixos X e Y mapeados para volume de chuva (0-100%) e nível de água (0-100%).
- **Display OLED**: Mostra nível de água, volume de chuva, status de alerta e uma barra gráfica interativa.
- **LED RGB**: Indica o risco de enchente com cores (verde, amarelo, vermelho).
- **Matriz de LEDs** (se disponível): Animações de chuva ou ondas baseadas no volume de chuva.
- **FreeRTOS**: Gerenciamento de tarefas com filas para comunicação eficiente.
- **Código Organizado**: Estrutura modular, comentários claros e uso de branches no Git para desenvolvimento.

---

## 🛠️ Tecnologias e Periféricos

- **Plataforma**: BitDogLab com Raspberry Pi Pico (RP2040).
- **Microcontrolador**: RP2040 (dual-core, 264KB SRAM).
- **Periféricos**:
  - Joystick (ADC em GPIO26 e GPIO27).
  - Display OLED SSD1306 (I2C em GPIO14 e GPIO15).
  - LED RGB (PWM em GPIO11, GPIO12, GPIO13) ou LEDs verde/azul (se RGB não disponível).
  - Matriz de LEDs (ex.: MAX7219 via SPI, pinos a confirmar).
- **Software**:
  - FreeRTOS para gerenciamento de tarefas.
  - Pico SDK para configuração de hardware.
  - Bibliotecas: `ssd1306.h` (display), `font.h` (textos), e (opcionalmente) matrizled.c e animacoes.h para matriz.

---

## 📂 Estrutura do Repositório

```plaintext
GuardaChuvas/
├── src/
│   ├── GuardaChuvas.c      # Código principal
│   ├── ssd1306.h           # Biblioteca do display
│   ├── font.h              # Biblioteca de fontes
│   └── CMakeLists.txt      # Configuração do Pico SDK
│   └── matrizled.c
│   └─animacoes.h
├── docs/
│   ├── Ficha_Proposta.md   # Ficha de proposta do projeto
│   └── Instrucoes_Compilacao.md # Instruções de compilação
├── README.md               # Este arquivo
└── .gitignore              # Ignora arquivos gerados
