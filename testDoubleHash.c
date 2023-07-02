#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define SEED 0x12345678

typedef struct _cidade
{
    char codigo_ibge[8];
    char nome[100];
    float latitude;
    float longitude;
    int capital;
    int codigo_uf;
    int siafi_id;
    int ddd;
    char fuso_horario[100];
} tcidade;

char *get_key(void *reg)
{
    return (*((tcidade *)reg)).codigo_ibge;
}

void *aloca_cidade(char *codigo_ibge, char *nome, float latitude, float longitude, int capital, int codigo_uf, int siafi_id, int ddd, char *fuso_horario)
{
    tcidade *cidade = malloc(sizeof(tcidade));

    strcpy(cidade->codigo_ibge, codigo_ibge);
    strcpy(cidade->nome, nome);
    cidade->latitude = latitude;
    cidade->longitude = longitude;
    cidade->capital = capital;
    cidade->codigo_uf = codigo_uf;
    cidade->siafi_id = siafi_id;
    cidade->ddd = ddd;
    strcpy(cidade->fuso_horario, fuso_horario);

    return cidade;
}

typedef struct
{
    uintptr_t *table;
    int size;
    int max;
    uintptr_t deleted;
    char *(*get_key)(void *);
} thash;

uint32_t hashf(const char *str, uint32_t h)
{
    /* One-byte-at-a-time Murmur hash
    Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
    for (; *str; ++str)
    {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint32_t hashf2(const char *str)
{
    /* Auxiliary hash function for double hashing */
    uint32_t hash = 0;
    for (; *str; ++str)
    {
        hash = (hash * 31) + *str;
    }
    return hash;
}

int hash_insere(thash *h, void *bucket)
{
    uint32_t hash = hashf(h->get_key(bucket), SEED);
    uint32_t hash2 = hashf2(h->get_key(bucket));
    int pos = hash % h->max;
    int step = (hash2 % (h->max - 1)) + 1;
    tcidade *cidade = (tcidade *)bucket;
    int count = 0;
    while (h->table[pos] != 0)
    {
        if (h->table[pos] == h->deleted)
            break;
        pos = (pos + step) % h->max;
        count++;
        if (count == h->max)
        {
            free(bucket);
            return EXIT_FAILURE;
        }
    }
    h->table[pos] = (uintptr_t)bucket;
    h->size += 1;
    return EXIT_SUCCESS;
}

int hash_constroi(thash *h, int nbuckets, char *(*get_key)(void *))
{
    h->table = calloc(sizeof(uintptr_t), nbuckets);
    if (h->table == NULL)
    {
        return EXIT_FAILURE;
    }
    h->max = nbuckets;
    h->size = 0;
    h->deleted = (uintptr_t) & (h->size);
    h->get_key = get_key;
    return EXIT_SUCCESS;
}

void *hash_busca(thash h, const char *key)
{
    uint32_t hash = hashf(key, SEED);
    uint32_t hash2 = hashf2(key);
    int pos = hash % h.max;
    int step = (hash2 % (h.max - 1)) + 1;
    int count = 0;
    while (h.table[pos] != 0)
    {
        if (strcmp(h.get_key((void *)h.table[pos]), key) == 0)
        {
            return (void *)h.table[pos];
        }
        else
        {
            pos = (pos + step) % h.max;
            count++;
            if (count == h.max)
                break;
        }
    }
    return NULL;
}

int hash_remove(thash *h, const char *key)
{
    uint32_t hash = hashf(key, SEED);
    uint32_t hash2 = hashf2(key);
    int pos = hash % h->max;
    int step = (hash2 % (h->max - 1)) + 1;
    int count = 0;
    while (h->table[pos] != 0)
    {
        if (strcmp(h->get_key((void *)h->table[pos]), key) == 0)
        {
            free((void *)h->table[pos]);
            h->table[pos] = h->deleted;
            h->size -= 1;
            return EXIT_SUCCESS;
        }
        else
        {
            pos = (pos + step) % h->max;
            count++;
            if (count == h->max)
                break;
        }
    }
    return EXIT_FAILURE;
}

void hash_apaga(thash *h)
{
    for (int pos = 0; pos < h->max; pos++)
    {
        if (h->table[pos] != 0 && h->table[pos] != h->deleted)
        {
            free((void *)h->table[pos]);
        }
    }
    free(h->table);
}

void test_hash()
{
    // Cria uma tabela hash
    thash hash_table;
    hash_constroi(&hash_table, 10, get_key);

    // Cria dados de cidades exemplo
    char codigo_ibge1[8] = "1234567";
    char nome1[100] = "Cidade Exemplo 1";
    float latitude1 = 1.23;
    float longitude1 = 4.56;
    int capital1 = 1;
    int codigo_uf1 = 10;
    int siafi_id1 = 20;
    int ddd1 = 30;
    char fuso_horario1[100] = "UTC+1";

    char codigo_ibge2[8] = "9876543";
    char nome2[100] = "Cidade Exemplo 2";
    float latitude2 = 7.89;
    float longitude2 = 5.43;
    int capital2 = 0;
    int codigo_uf2 = 20;
    int siafi_id2 = 30;
    int ddd2 = 40;
    char fuso_horario2[100] = "UTC-3";

    // Testa a função hash_insere
    void *cidade1 = aloca_cidade(codigo_ibge1, nome1, latitude1, longitude1,
                                 capital1, codigo_uf1, siafi_id1, ddd1, fuso_horario1);
    int resultado1 = hash_insere(&hash_table, cidade1);
    assert(resultado1 == EXIT_SUCCESS);

    void *cidade2 = aloca_cidade(codigo_ibge2, nome2, latitude2, longitude2,
                                 capital2, codigo_uf2, siafi_id2, ddd2, fuso_horario2);
    int resultado2 = hash_insere(&hash_table, cidade2);
    assert(resultado2 == EXIT_SUCCESS);

    // Libera a memória alocada
    free(cidade1);
    free(cidade2);
    hash_apaga(&hash_table);

    printf("Teste de insercao aprovado com sucesso!\n");
}

void test_search()
{
    // Cria uma tabela hash
    thash hash_table;
    hash_constroi(&hash_table, 10, get_key);

    // Cria uma cidade exemplo
    char codigo_ibge[8] = "1234567";
    char nome[100] = "Cidade Exemplo";
    float latitude = 1.23;
    float longitude = 4.56;
    int capital = 1;
    int codigo_uf = 10;
    int siafi_id = 20;
    int ddd = 30;
    char fuso_horario[100] = "UTC+1";

    // Insere a cidade na tabela hash
    void *cidade = aloca_cidade(codigo_ibge, nome, latitude, longitude,
                                capital, codigo_uf, siafi_id, ddd, fuso_horario);
    hash_insere(&hash_table, cidade);

    // Testa a função hash_busca
    tcidade *cidade_buscada = (tcidade *)hash_busca(hash_table, codigo_ibge);
    assert(cidade_buscada != NULL);
    assert(strcmp(cidade_buscada->codigo_ibge, codigo_ibge) == 0);

    // Libera a memória alocada
    free(cidade);
    hash_apaga(&hash_table);

    printf("Teste de busca aprovado com sucesso!\n");
}

void test_remove()
{
    // Cria uma tabela hash
    thash hash_table;
    hash_constroi(&hash_table, 10, get_key);

    // Cria uma cidade exemplo
    char codigo_ibge[8] = "1234567";
    char nome[100] = "Cidade Exemplo";
    float latitude = 1.23;
    float longitude = 4.56;
    int capital = 1;
    int codigo_uf = 10;
    int siafi_id = 20;
    int ddd = 30;
    char fuso_horario[100] = "UTC+1";

    // Insere a cidade na tabela hash
    void *cidade = aloca_cidade(codigo_ibge, nome, latitude, longitude,
                                capital, codigo_uf, siafi_id, ddd, fuso_horario);
    hash_insere(&hash_table, cidade);

    // Testa a função hash_remove
    int resultado = hash_remove(&hash_table, codigo_ibge);
    assert(resultado == EXIT_SUCCESS);

    // Tenta buscar a cidade removida
    tcidade *cidade_removida = (tcidade *)hash_busca(hash_table, codigo_ibge);
    assert(cidade_removida == NULL);

    // Libera a memória alocada
    free(cidade);
    hash_apaga(&hash_table);

    printf("Teste de remocao aprovado com sucesso!\n");
}

int main()
{
    test_hash();
    test_search();
    test_remove();

    return 0;
}
