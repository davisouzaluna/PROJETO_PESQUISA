
#ifndef TESTLIBRARY_INCLUDE_SAVE_DATA_H
#define TESTLIBRARY_INCLUDE_SAVE_DATA_H

//Chave padrão para salvar no redis
const char * g_redis_key; 

typedef struct {
    const char *value;
    const char *redis_key;
} RedisParams;

void* store_in_redis_async(void *params);

void store_in_redis_async_call(const char *value, const char *redis_key);

void store_in_redis(const char *value, const char *redis_key);

#endif // SAVE_DATA_H