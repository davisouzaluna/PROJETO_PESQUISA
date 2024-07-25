#ifndef TESTLIBRARY_INCLUDE_LATENCY_H
#define TESTLIBRARY_INCLUDE_LATENCY_H

#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define BILLION 1000000000


#include <time.h>


// Retorna o tempo atual em timespec
struct timespec string_para_timespec(char *tempo_varchar);

// Retorna o tempo de nanosegundos em long long
long long tempo_atual_nanossegundos();

// Retorna o tempo atual em timespec
struct timespec tempo_atual_timespec();

// Retorna o tempo atual em varchar
char *tempo_para_varchar();

// Retorna a diferença de tempo em nanosegundos (sendo utilizada para ter uma ideia da latência)
long long diferenca_tempo(struct timespec tempo1, struct timespec tempo2);

// Função para converter diferença (método anterior) de tempo em string
char *diferenca_para_varchar(long long diferenca);

#endif // TESTLIBRARY_INCLUDE_LATENCY_H
