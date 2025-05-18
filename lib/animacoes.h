#include "matrizled.c"
#include <time.h>
#include <stdlib.h>

// Intensidade padrão para as animações
#define intensidade 1
void printNum(void) {
    npWrite();
    npClear();
}


// Sprites existentes
int OFF[5][5][3] = {
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}
};

int ATENCAO[5][5][3] = {
    {{0, 0, 0}, {128, 128, 0}, {128, 128, 0}, {128, 128, 0}, {0, 0, 0}},
    {{128, 128, 0}, {128, 128, 0}, {0, 0, 0}, {128, 128, 0}, {128, 128, 0}},
    {{128, 128, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {128, 128, 0}},
    {{128, 128, 0}, {0, 0, 0}, {128, 128, 0}, {0, 0, 0}, {128, 128, 0}},
    {{0, 0, 0}, {128, 128, 0}, {128, 128, 0}, {128, 128, 0}, {0, 0, 0}}
};

int SETA_VERDE[5][5][3] = {
    {{0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}},
    {{0, 128, 0}, {0, 128, 0}, {0, 128, 0}, {0, 128, 0}, {0, 128, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}, {0, 0, 0}}
};

int X_VERMELHO[5][5][3] = {
    {{128, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {128, 0, 0}},
    {{0, 0, 0}, {128, 0, 0}, {0, 0, 0}, {128, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {128, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {128, 0, 0}, {0, 0, 0}, {128, 0, 0}, {0, 0, 0}},
    {{128, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {128, 0, 0}}
};

// Funções existentes
void PedestreSIGA(void) {
    desenhaSprite(SETA_VERDE, intensidade);
    printNum();
}

void PedestrePARE(void) {
    desenhaSprite(X_VERMELHO, intensidade);
    printNum();
}

void Amarelo_Noturno(void) {
    desenhaSprite(ATENCAO, intensidade);
    printNum();
}

void DesligaMatriz(void) {
    desenhaSprite(OFF, intensidade);
    printNum();
}



// Animação para SEGURO: seta verde estática
void anim_seguro(void) {
    PedestreSIGA(); // Exibe seta verde
    vTaskDelay(pdMS_TO_TICKS(100)); // Sincronia a 10 Hz
}

// Animação para ALERTA: chuva piscante azul
void anim_alerta(void) {
    npClear();
    // Acende 3 LEDs aleatórios em azul
    for (int i = 0; i < 3; i++) {
        int x = rand() % 5;
        int y = rand() % 5;
        int posicao = getIndex(x, y);
        npSetLED(posicao, 0, 0, 100); // Azul
    }
    npWrite();
    vTaskDelay(pdMS_TO_TICKS(500)); // Piscar a 2 Hz
}

// Animação para ENCHENTE: onda vermelha
void anim_enchente(void) {
    static int row = 0;
    npClear();
    // Acende uma linha em vermelho
    for (int col = 0; col < 5; col++) {
        int posicao = getIndex(col, row);
        npSetLED(posicao, 100, 0, 0); // Vermelho
    }
    npWrite();
    row = (row + 1) % 5; // Avança linha
    vTaskDelay(pdMS_TO_TICKS(200)); // Onda a 5 Hz
}

