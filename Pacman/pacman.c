#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h> 
#include <unistd.h>

#define MAX_FANTASMAS 4
#define RESET   "\x1b[0m"
#define AZUL    "\x1b[34m"
#define AMARELO "\x1b[33;1m" 
#define VERMELHO "\x1b[31;1m"
#define MAGENTA "\x1b[35;1m"
#define CIANO   "\x1b[36;1m"
#define VERDE   "\x1b[32;1m"
#define BRANCO  "\x1b[37m"

typedef struct {
    int x, y;
} Posicao;

typedef struct {
    Posicao pos;
    char background_anterior;
    int dir_x, dir_y; 
} Fantasma;

typedef struct {
    char **grade;
    int linhas, colunas;
    int pontos_totais, pontos_coletados;
} Mapa;

Mapa m;
Posicao jogador;
Fantasma fantasmas[MAX_FANTASMAS];
int qtd_fantasmas = 0;
bool jogo_rodando = true;

int jog_dir_x = 0;
int jog_dir_y = 0;

// ============================================================================
// FUNÇÕES DE TERMINAL (LINUX)
// ============================================================================

char ler_tecla() {
    char ch = 0;
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void limpar_tela() {
    system("clear"); 
}

// ============================================================================
// GERENCIAMENTO DE MAPA E ARQUIVOS
// ============================================================================

void liberar_mapa() {
    for (int i = 0; i < m.linhas; i++) free(m.grade[i]);
    free(m.grade);
}

void carregar_mapa() {
    srand(time(NULL));
    // Sorteio dinâmico atualizado para 5 mapas
    int num_mapa = (rand() % 5) + 1; 
    char nome_arquivo[20];
    sprintf(nome_arquivo, "mapa%d.txt", num_mapa);

    FILE *f = fopen(nome_arquivo, "r");
    if (f == NULL) {
        printf(VERMELHO "Erro fatal: Não foi possível abrir o arquivo %s!\n" RESET, nome_arquivo);
        printf("Certifique-se de executar no diretório raiz ou colocar o arquivo na pasta correta.\n");
        exit(1);
    }

    fscanf(f, "%d %d", &m.linhas, &m.colunas);
    m.grade = malloc(m.linhas * sizeof(char*));
    for (int i = 0; i < m.linhas; i++) {
        m.grade[i] = malloc((m.colunas + 2) * sizeof(char));
    }

    m.pontos_totais = 0;
    m.pontos_coletados = 0;
    qtd_fantasmas = 0;
    fgetc(f); 

    for (int i = 0; i < m.linhas; i++) {
        fgets(m.grade[i], m.colunas + 2, f);
        for (int j = 0; j < m.colunas; j++) {
            if (m.grade[i][j] == 'P') {
                jogador.x = i; jogador.y = j;
            } else if (m.grade[i][j] == 'G') {
                if (qtd_fantasmas < MAX_FANTASMAS) {
                    fantasmas[qtd_fantasmas].pos.x = i;
                    fantasmas[qtd_fantasmas].pos.y = j;
                    fantasmas[qtd_fantasmas].background_anterior = '.';
                    fantasmas[qtd_fantasmas].dir_x = 0;
                    fantasmas[qtd_fantasmas].dir_y = 0;
                    qtd_fantasmas++;
                    m.pontos_totais++; 
                }
            } else if (m.grade[i][j] == '.') {
                m.pontos_totais++;
            }
        }
    }
    fclose(f);
}

void imprimir_jogo() {
    limpar_tela();
    printf(AMARELO "=== PAC-MAN: TRABALHO DE CONSTRUÇÃO DE ALGORITMO. ===\n\n" RESET);
    
    // Motor de Renderização (Decoupling de lógica e visual)
    for (int i = 0; i < m.linhas; i++) {
        for (int j = 0; j < m.colunas; j++) {
            char c = m.grade[i][j];
            
            if (c == '#') {
                // Renderiza parede como um bloco sólido azul
                printf(AZUL "█" RESET);
            } else if (c == '.') {
                // Renderiza pontos
                printf(BRANCO "." RESET);
            } else if (c == 'P') {
                // Renderiza Pac-Man
                printf(AMARELO "C" RESET);
            } else if (c == 'G') {
                // Procura qual o ID deste fantasma especifico nesta coordenada
                int id_fantasma = 0;
                for (int k = 0; k < qtd_fantasmas; k++) {
                    if (fantasmas[k].pos.x == i && fantasmas[k].pos.y == j) {
                        id_fantasma = k;
                        break;
                    }
                }
                
                // Aplica a cor clássica baseada na IA do fantasma
                if (id_fantasma == 0) printf(VERMELHO "M" RESET); // Blinky
                else if (id_fantasma == 1) printf(MAGENTA "M" RESET);  // Pinky
                else if (id_fantasma == 2) printf(CIANO "M" RESET);    // Inky
                else printf(VERDE "M" RESET);                          // Clyde
            } else {
                printf(" "); // Espaço vazio
            }
        }
        printf("\n"); // Quebra de linha da matriz
    }
    
    printf(BRANCO "\nStatus da Missão:\n" RESET);
    printf(CIANO "-Pontos coletados: %d\n" RESET, m.pontos_coletados);
    printf(VERMELHO "-Pontos restantes: %d\n" RESET, m.pontos_totais - m.pontos_coletados);
    printf(BRANCO "\nControles: W (Cima), S (Baixo), A (Esquerda), D (Direita). Aperte 'Q' para Sair.\n" RESET);
}

// ============================================================================
// LÓGICA E REGRAS DO JOGO
// ============================================================================

bool posicao_valida(int x, int y) {
    if (x < 0 || x >= m.linhas || y < 0 || y >= m.colunas) return false;
    return (m.grade[x][y] != '#');
}

int calcular_distancia(int x1, int y1, int x2, int y2) {
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

void checar_colisao_com_fantasmas() {
    for (int i = 0; i < qtd_fantasmas; i++) {
        if (jogador.x == fantasmas[i].pos.x && jogador.y == fantasmas[i].pos.y) {
            jogo_rodando = false;
            limpar_tela();
            printf(VERMELHO "\n\nVOCÊ FOI PEGO PELO FANTASMA!\n\n" RESET);
        }
    }
}

void mover_jogador(char direcao) {
    int proximo_x = jogador.x;
    int proximo_y = jogador.y;
    jog_dir_x = 0; jog_dir_y = 0;

    switch (direcao) {
        case 'w': case 'W': proximo_x--; jog_dir_x = -1; break;
        case 's': case 'S': proximo_x++; jog_dir_x = 1;  break;
        case 'a': case 'A': proximo_y--; jog_dir_y = -1; break;
        case 'd': case 'D': proximo_y++; jog_dir_y = 1;  break;
        case 'q': case 'Q': jogo_rodando = false; return;
        default: return; 
    }

    if (posicao_valida(proximo_x, proximo_y)) {
        if (m.grade[proximo_x][proximo_y] == 'G') {
            jogador.x = proximo_x; jogador.y = proximo_y;
            checar_colisao_com_fantasmas();
            return;
        }

        if (m.grade[proximo_x][proximo_y] == '.') m.pontos_coletados++;
        m.grade[jogador.x][jogador.y] = ' ';
        jogador.x = proximo_x;
        jogador.y = proximo_y;
        m.grade[jogador.x][jogador.y] = 'P';
    }
}

void mover_fantasmas() {
    for (int i = 0; i < qtd_fantasmas; i++) {
        int tx = jogador.x, ty = jogador.y; 
        
        if (i == 0) {
            tx = jogador.x; ty = jogador.y;
        } else if (i == 1) {
            tx = jogador.x + (jog_dir_x * 4);
            ty = jogador.y + (jog_dir_y * 4);
        } else if (i == 2) {
            if (qtd_fantasmas > 0) {
                tx = jogador.x + (jogador.x - fantasmas[0].pos.x);
                ty = jogador.y + (jogador.y - fantasmas[0].pos.y);
            }
        } else {
            if (calcular_distancia(fantasmas[i].pos.x, fantasmas[i].pos.y, jogador.x, jogador.y) > 25) {
                tx = jogador.x; ty = jogador.y;
            } else {
                tx = m.linhas - 1; ty = 0; 
            }
        }

        int x_atual = fantasmas[i].pos.x;
        int y_atual = fantasmas[i].pos.y;
        
        int opcoes[4][2] = {
            {x_atual - 1, y_atual},
            {x_atual + 1, y_atual},
            {x_atual, y_atual - 1},
            {x_atual, y_atual + 1} 
        };

        int caminhos_validos = 0;
        for (int k = 0; k < 4; k++) {
            if (posicao_valida(opcoes[k][0], opcoes[k][1]) && m.grade[opcoes[k][0]][opcoes[k][1]] != 'G') {
                caminhos_validos++;
            }
        }

        int melhor_x = -1, melhor_y = -1;
        int menor_dist = 999999;
        int dx_oposto = -fantasmas[i].dir_x;
        int dy_oposto = -fantasmas[i].dir_y;

        for(int k = 0; k < 4; k++) {
            int nx = opcoes[k][0];
            int ny = opcoes[k][1];
            
            if (posicao_valida(nx, ny) && m.grade[nx][ny] != 'G') {
                bool eh_meia_volta = ((nx - x_atual) == dx_oposto && (ny - y_atual) == dy_oposto);
                if (eh_meia_volta && caminhos_validos > 1) {
                    continue; 
                }

                int dist = calcular_distancia(nx, ny, tx, ty);
                if (dist < menor_dist) {
                    menor_dist = dist;
                    melhor_x = nx;
                    melhor_y = ny;
                }
            }
        }

        if (melhor_x != -1) {
            m.grade[x_atual][y_atual] = fantasmas[i].background_anterior;
            
            if (m.grade[melhor_x][melhor_y] != 'P') {
                fantasmas[i].background_anterior = m.grade[melhor_x][melhor_y];
            } else {
                fantasmas[i].background_anterior = ' '; 
            }
            
            fantasmas[i].dir_x = melhor_x - x_atual;
            fantasmas[i].dir_y = melhor_y - y_atual;
            fantasmas[i].pos.x = melhor_x;
            fantasmas[i].pos.y = melhor_y;
            m.grade[melhor_x][melhor_y] = 'G';
        }
    }
    checar_colisao_com_fantasmas();
}

void checar_vitoria() {
    if (m.pontos_coletados == m.pontos_totais && jogo_rodando) {
        jogo_rodando = false;
        limpar_tela();
        printf(VERDE "\n\nPARABÉNS! VOCÊ COLETOU TODOS OS PONTOS E VENCEU O JOGO!\n\n" RESET);
    }
}

// ============================================================================
// MAIN - LOOP DE EXECUÇÃO
// ============================================================================

int main() {
    carregar_mapa();

    while (jogo_rodando) {
        imprimir_jogo();
        char comando = ler_tecla();
        mover_jogador(comando);
        
        if (jogo_rodando) mover_fantasmas();
        if (jogo_rodando) checar_vitoria();
    }

    liberar_mapa();
    return 0;
}