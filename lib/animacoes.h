#include <matrizled.c>
#include <time.h>
// AQUI ESTA O MAPEAMENTO DE TODOS OS NUMEROS DE 0 A 9
// por padrao foi definido a exibicao dos nuemros na cor VERMELHA

#define intensidade 1

void printNum()
{
    npWrite();
    // sleep_ms(2000);
    npClear();
}

int OFF[5][5][3] = {
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}};

int ATENCAO[5][5][3] = {
    {{0, 0, 0}, {128, 128, 0}, {128, 128, 0}, {128, 128, 0}, {0, 0, 0}}, // Olhos e topo da exclamação
    {{128, 128, 0}, {128, 128, 0}, {0, 0, 0}, {128, 128, 0}, {128, 128, 0}},     // Corpo da exclamação
    {{128, 128, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {128, 128, 0}},     // Corpo da exclamação
    {{128, 128, 0}, {0, 0, 0}, {128, 128, 0}, {0, 0, 0}, {128, 128, 0}}, // Boca
    {{0, 0, 0}, {128, 128, 0}, {128, 128, 0}, {128, 128, 0}, {0, 0, 0}}, // Ponto da exclamação
};

int SETA_VERDE[5][5][3] = {
    {{0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}},
    {{0, 128, 0}, {0, 128, 0}, {0, 128, 0}, {0, 128, 0}, {0, 128, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 128, 0}, {0, 0, 0}, {0, 0, 0}}};

int X_VERMELHO[5][5][3] = {
    {{128, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {128, 0, 0}},
    {{0, 0, 0}, {128, 0, 0}, {0, 0, 0}, {128, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {128, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {128, 0, 0}, {0, 0, 0}, {128, 0, 0}, {0, 0, 0}},
    {{128, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {128, 0, 0}}};

void PedresteSIGA()
{
    desenhaSprite(SETA_VERDE, intensidade);
    printNum();
}

void PedrestePARE()
{
    desenhaSprite(X_VERMELHO, intensidade);
    printNum();
}

void DesligaMatriz()
{
    desenhaSprite(OFF, intensidade);
    printNum();
}

void Amarelo_Noturno()
{
    desenhaSprite(ATENCAO, intensidade);
    printNum();
}
