#include "../include/latency.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


struct timespec string_para_timespec(char *tempo_varchar) {
    struct timespec tempo;
    char *ponto = strchr(tempo_varchar, '.');
    if (ponto != NULL) {
        *ponto = '\0'; // separa os segundos dos nanossegundos
        tempo.tv_sec = atol(tempo_varchar);
        tempo.tv_nsec = atol(ponto + 1);
    } else {
        tempo.tv_sec = atol(tempo_varchar);
        tempo.tv_nsec = 0;
    }
    return tempo;
}

long long tempo_atual_nanossegundos() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);
    // Converter segundos para nanossegundos e adicionar nanossegundos
    return tempo_atual.tv_sec * BILLION + tempo_atual.tv_nsec;
}

struct timespec tempo_atual_timespec() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);
    return tempo_atual;
}

char *tempo_para_varchar() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);
    // Convertendo o tempo para uma string legível
    char *tempo_varchar = (char *)malloc(MAX_STR_LEN * sizeof(char));
    if (tempo_varchar == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }
    snprintf(tempo_varchar, MAX_STR_LEN, "%ld.%09ld", tempo_atual.tv_sec, tempo_atual.tv_nsec);
    // Retornando a string de tempo
    return tempo_varchar;
}

long long diferenca_tempo(struct timespec tempo1, struct timespec tempo2) {
    long long diff_sec = (long long)(tempo1.tv_sec - tempo2.tv_sec);
    long long diff_nsec = (long long)(tempo1.tv_nsec - tempo2.tv_nsec);
    return diff_sec * BILLION + diff_nsec;
}

char *diferenca_para_varchar(long long diferenca) {
    char *tempo_varchar = (char *)malloc(MAX_STR_LEN * sizeof(char));
    if (tempo_varchar == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }
    snprintf(tempo_varchar, MAX_STR_LEN, "%lld.%09lld", diferenca / BILLION, diferenca % BILLION);
    return tempo_varchar;
}
