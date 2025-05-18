```markdown
# GuardaChuvas ☔

**Estação de Alerta de Enchente com Simulação de Sensores**

Bem-vindo ao **GuardaChuvas**, um sistema embarcado desenvolvido com FreeRTOS na plataforma BitDogLab (Raspberry Pi Pico RP2040). O projeto monitora nível de água e volume de chuva em tempo real, usando sensores simulados via ADC, e exibe alertas visuais no display OLED, LED RGB, matriz WS2812B 5x5 e sonoros no buzzer. O nome "GuardaChuvas" reflete a proteção contra enchentes! 🌊

Autor: **Daniel Silva de Souza**

---

## 📋 Sobre o Projeto

O **GuardaChuvas** é uma estação de alerta que:
- **Lê** sensores de volume de chuva (GPIO 26, ADC0) e nível de água (GPIO 27, ADC1) via ADC.
- **Processa** os dados para determinar o risco de enchente (Seguro, Alerta, Enchente).
- **Exibe** informações no display OLED SSD1306 (nível de água, chuva, status, barra gráfica).
- **Controla** um LED RGB para indicar o estado: verde (Seguro), amarelo (Alerta), vermelho (Enchente).
- **Anima** uma matriz WS2812B 5x5 com padrões de chuva ou ondas.
- **Emite** sons distintos no buzzer para acessibilidade (silêncio, beeps, tom contínuo).

O sistema usa **FreeRTOS** com filas (`xQueueSensorData`, `xQueueAlertState`) para comunicação, sem semáforos ou mutexes.

---

## 🎯 Funcionalidades

- **Leitura de Sensores**: Volume de chuva e nível de água mapeados para 0-100%.
- **Display OLED**: Mostra nível de água, chuva, status e barra gráfica.
- **LED RGB**: Indica risco com cores (verde, amarelo, vermelho).
- **Matriz WS2812B 5x5**: Animações de chuva ou ondas.
- **Buzzer**: Sons distintos para inclusão (deficiência visual).
- **FreeRTOS**: Tarefas com filas para operação em tempo real.

---

## 🛠️ Tecnologias e Periféricos

- **Plataforma**: BitDogLab (RP2040).
- **Periféricos**:
  - Sensores: ADC (GPIO 26: chuva, GPIO 27: água).
  - Display OLED SSD1306: I2C (GPIOs 14, 15).
  - LED RGB: PWM (GPIOs 11, 12, 13).
  - Matriz WS2812B 5x5: PIO (GPIO 7).
  - Buzzer: PWM (GPIO 10).
  - Botão B: GPIO 6 (BOOTSEL).
- **Software**:
  - FreeRTOS (filas).
  - Pico SDK.
  - Bibliotecas: `ssd1306.h`, `font.h`, `matrizled.c`, `animacoes.h`, `ws2818b.pio`.

---

## 📂 Estrutura do Repositório

```plaintext
GuardaChuvas/
├── src/
│   ├── GuardaChuvas.c
│   ├── CMakeLists.txt
├── lib/
│   ├── font.h
│   ├── FreeRTOSConfig.h
│   ├── matrizled.c
│   ├── animacoes.h
│   ├── ssd1306.c
│   ├── ssd1306.h
│   ├── ws2818b.pio
├── docs/
│   ├── Ficha_Proposta.md
│   └── Instrucoes_Compilacao.md
├── README.md
└── .gitignore
```

---

## 🚀 Como Compilar e Executar

### Pré-requisitos
- Pico SDK instalado.
- Compilador ARM (`arm-none-eabi-gcc`).
- CMake e Make.
- Placa BitDogLab (RP2040).
- Terminal serial (`minicom`).

### Passos
1. **Clone o repositório**:
   ```bash
   git clone [https://github.com/seu-usuario/GuardaChuvas.git](https://github.com/Danngas/GuardaChuvas.git)
   cd GuardaChuvas
   ```

2. **Configure o Pico SDK**:
   ```bash
   export PICO_SDK_PATH=/caminho/para/pico-sdk
   ```

3. **Compile**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

4. **Carregue o firmware**:
   - Conecte a placa em modo BOOTSEL.
   - Copie `GuardaChuvas.uf2` para a placa.

5. **Teste**:
   - Use `minicom -b 115200 -o -D /dev/ttyACM0` para depuração.
   - Simule sensores e observe display, LEDs, matriz e buzzer.

---

## 📝 Autor

- **Nome**: Daniel Silva de Souza
- **Polo**: Bom Jesus da Lapa
- **Data**: 18/05/2025

---

**GuardaChuvas: Monitorando enchentes com inclusão e tecnologia!** ☔
`
