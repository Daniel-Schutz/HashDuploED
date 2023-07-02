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

void ler_cidades(const char *nome_arquivo, thash *hash_table)
{
    // Abre o arquivo de texto para leitura
    FILE *arquivo;
    arquivo = fopen(nome_arquivo, "r");

    if (arquivo == NULL)
    {
        printf("Não foi possível abrir o arquivo.\n");
        return;
    }

    // Variáveis temporárias para armazenar os dados lidos
    char codigo_ibge[8];
    int capital, codigo_uf, siafi_id, ddd;
    char nome[100], fuso_horario[100];
    float latitude, longitude;

    // Lê as linhas do arquivo e insere as cidades na tabela hash
    while (!feof(arquivo))
    {
        fscanf(arquivo, " %[^,], %[^,], %f, %f, %d, %d, %d, %d, %[^,\n]",
               codigo_ibge, nome, &latitude,
               &longitude, &capital, &codigo_uf,
               &siafi_id, &ddd, fuso_horario);

        // Aloca a cidade e insere na tabela hash
        void *cidade = aloca_cidade(codigo_ibge, nome, latitude, longitude,
                                    capital, codigo_uf, siafi_id, ddd, fuso_horario);
        hash_insere(hash_table, cidade);
    }

    // Fecha o arquivo
    fclose(arquivo);
}

void print_hash_table(thash h)
{
    printf("Hash Table:\n");
    for (int i = 0; i < h.max; i++)
    {
        if (h.table[i] != 0 && h.table[i] != h.deleted)
        {
            tcidade *cidade = (tcidade *)h.table[i];
            printf("%s: %s\n", cidade->codigo_ibge, cidade->nome);
        }
    }
}

void print_cidade_info(const tcidade *cidade)
{
    printf("Codigo IBGE: %s\n", cidade->codigo_ibge);
    printf("Nome: %s\n", cidade->nome);
    printf("Latitude: %f\n", cidade->latitude);
    printf("Longitude: %f\n", cidade->longitude);
    printf("Capital: %s\n", cidade->capital ? "Sim" : "Não");
    printf("Codigo UF: %d\n", cidade->codigo_uf);
    printf("Siafi ID: %d\n", cidade->siafi_id);
    printf("DDD: %d\n", cidade->ddd);
    printf("Fuso Horario: %s\n", cidade->fuso_horario);
}

void exibir_menu()
{
    printf("\n======== Menu =======\n");
    printf("1. Buscar cidade por codigo IBGE\n");
    printf("2. Sair\n");
    printf("======================\n");
    printf("Escolha uma opcao: ");
}

int main()
{
    // Cria a tabela hash
    thash hash_table;
    hash_constroi(&hash_table, 7000, get_key);

    // Lê as cidades do arquivo e insere na tabela hash
    ler_cidades("cidades.txt", &hash_table);

    int opcao;
    char codigo_ibge[8];
    tcidade *cidade;

    do
    {
        exibir_menu();
        scanf("%d", &opcao);

        switch (opcao)
        {
        case 1:
            printf("Digite o codigo IBGE da cidade que deseja buscar: ");
            scanf("%s", codigo_ibge);

            // Busca a cidade pelo código IBGE na tabela hash
            cidade = (tcidade *)hash_busca(hash_table, codigo_ibge);

            if (cidade != NULL)
            {
                printf("\nCidade encontrada:\n");
                print_cidade_info(cidade);
            }
            else
            {
                printf("Cidade nao encontrada.\n");
            }
            break;
        case 2:
            printf("Encerrando o programa...\n");
            break;
        default:
            printf("Opção invalida. Tente novamente.\n");
            break;
        }
    } while (opcao != 2);

    // Libera a memória alocada pela tabela hash
    hash_apaga(&hash_table);

    return 0;
}
