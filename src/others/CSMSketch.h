#ifndef CSMSKETCH_H
#define CSMSKETCH_H

#include "../BOBHash32.h"
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

using namespace std;

template <int key_len, int memory_in_bytes, int d>
class CSMSketch
{
private:
    static constexpr int w = memory_in_bytes / 4 / d;
	int counters[d][w];
    BOBHash32 * primary_hash;
	BOBHash32 * hash[d];
    int element_num = 0;
public:
    string name;
    CSMSketch()
    {
        memset(counters, 0, sizeof(counters));
        srand(time(0));
        for (int i = 0; i < d; i++)
            hash[i] = new BOBHash32(i + 750);
        primary_hash = new BOBHash32(rand() % MAX_PRIME32);

        stringstream name_buffer;
        name_buffer << "CSM@" << memory_in_bytes << "@" << d;
        name = name_buffer.str();
    }

    void print_basic_info()
    {
        printf("CSM sketch\n");
        printf("\tCounters: %d\n", w);
        printf("\tMemory: %.6lfMB\n", w * 4.0 / 1024 / 1024);
    }

    ~CSMSketch()
    {
        delete primary_hash;
        for (int i = 0; i < d; i++)
            delete hash[i];
    }

    void insert(uint8_t * key, int f = 1)
    {
        uint32_t idx = primary_hash->run((const char *)&element_num, 4) % d;
        int index = (hash[idx]->run((const char *)key, key_len)) % w;
        counters[idx][index] += f;
    }

	int query(uint8_t * key)
    {
        uint32_t sum = 0;
        for (int i = 0; i < d; ++i) {
            uint32_t index = (hash[i]->run((const char *)key, key_len)) % w;
            sum += counters[i][index];
        }

        return sum;
    }
};

#endif //CSMSKETCH_H