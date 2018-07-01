#ifndef _CMSKETCH_H
#define _CMSKETCH_H

#include "../BOBHash32.h"
#include <cstring>
#include <algorithm>
#include <cstdint>

using namespace std;

template <int key_len, int memory_in_bytes, int d>
class CMSketch
{
private:
    static constexpr int w = memory_in_bytes * 8 / 32;
	int counters[w];
	BOBHash32 * hash[d];
public:
    string name;
    CMSketch()
    {
        memset(counters, 0, sizeof(counters));
        for (int i = 0; i < d; i++)
            hash[i] = new BOBHash32(i + 750);

        stringstream name_buffer;
        name_buffer << "CM@" << memory_in_bytes << "@" << d;
        name = name_buffer.str();
    }

    void print_basic_info()
    {
        printf("CM sketch\n");
        printf("\tCounters: %d\n", w);
        printf("\tMemory: %.6lfMB\n", w * 4.0 / 1024 / 1024);
    }

    virtual ~CMSketch()
    {
        for (int i = 0; i < d; i++)
            delete hash[i];
    }

    void insert(uint8_t * key, int f = 1)
    {
        for (int i = 0; i < d; i++) {
            int index = (hash[i]->run((const char *)key, key_len)) % w;
            counters[index] += f;
        }
    }

	int query(uint8_t * key)
    {
        int ret = 1 << 30;
        for (int i = 0; i < d; i++) {
            int tmp = counters[(hash[i]->run((const char *)key, key_len)) % w];
            ret = min(ret, tmp);
        }
        return ret;
    }
};

#endif //_CMSKETCH_H