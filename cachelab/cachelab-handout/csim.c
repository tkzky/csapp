#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h> 

int s, S, E, b;
char *t;
FILE *pFile = NULL;
int hits_count, misses_count, evictions_count;

typedef struct Cache
{
    int valid_bits;
    int tag;
    int time_stamp;
} cache_self, *cache_line, **cache_group;

cache_group cache = NULL;

/*
 * read options from shell
 */
void get_opt(int argc, char* argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "v::s:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'v':
            break;
        case 's':
            s = atoi(optarg);
            S = 1 << s;
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            t = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-s s] [-E E] [-b b] [-t filepath]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if(optind > argc)
    {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * init the cache_group
 */
void init_cache()
{
    // free by main
    cache = (cache_group)malloc(sizeof(cache_line) * S);
    
    for(int i = 0; i < S; i++)
    {
        // free by main
        cache[i] = (cache_line)malloc(sizeof(cache_self) * E);
        for(int j = 0; j < E; j++)
        {
            cache[i][j].valid_bits = 0;
            cache[i][j].tag = 0;
            cache[i][j].time_stamp = 0;
        }
    }
}

void read_address(unsigned address)
{
    int group_idx = (address >> b) & (-1U >> (64 - s));
    int tag = (address >> (s + b));

    // whether in cache?
    for (int i = 0; i < E; i++)
    {
        if (cache[group_idx][i].valid_bits == 1 && cache[group_idx][i].tag == tag)
        {
            // update timestamp
            cache[group_idx][i].time_stamp = 0;
            ++hits_count;
            return;
        }
    }
    
    // not int cache, whether exist free cache?
    for (int i = 0; i < E; i++)
    {
        if (cache[group_idx][i].valid_bits == 0)
        {
            cache[group_idx][i].tag = tag;
            cache[group_idx][i].valid_bits = 1;
            cache[group_idx][i].time_stamp = 0;
            ++misses_count;
            return;
        }
    }

    // evict a cache
    int max_time_stamp = -1;
    int max_timestamp_idx = 0;
    for (int i = 0; i < E; i++)
    {
        if (cache[group_idx][i].time_stamp > max_time_stamp)
        {
            max_timestamp_idx = i;
            max_time_stamp = cache[group_idx][i].time_stamp;
        }
    }
    cache[group_idx][max_timestamp_idx].tag = tag;
    cache[group_idx][max_timestamp_idx].time_stamp = 0;
    ++misses_count;
    ++evictions_count;
}

void update_timestamp()
{
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            cache[i][j].time_stamp++;
        }
    }
}


/*
 * parse the file
 */
void parse_line()
{
    char identifier;
    unsigned address;
    int size;

    while (fscanf(pFile, "%c %x,%d", &identifier, &address, &size) > 0)
    {
        switch(identifier)
        {
        case 'I':
            continue;
        case 'L':
            read_address(address);
            break;
        case 'M':
            read_address(address);
        case 'S':
            read_address(address);
        }
        update_timestamp();
    }
}


int main(int argc, char *argv[])
{
    // read options from shell
    get_opt(argc, argv);

    // open traces/file.trace
    pFile = fopen(t, "r");
    if (pFile == NULL)
    {
        printf("Open file failed\n");
        exit(EXIT_FAILURE);
    }

    // init the cache
    init_cache();

    // parse file line by line
    parse_line();

    // free the resource
    fclose(pFile);
    for (int i = 0; i < S; i++)
    {
        free(cache[i]);
    }
    free(cache);

    printSummary(hits_count, misses_count, evictions_count);
    return 0;
}
