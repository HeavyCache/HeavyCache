#ifndef _CUSKETCH_H
#define _CUSKETCH_H

#include "../BOBHash32.h"
#include <cstring>
#include <algorithm>

using namespace std;

template <int key_len, int memory_in_bytes, int d>
class CUSketch
{
private:
    static constexpr int w = memory_in_bytes * 8 / 32;
	int counters[w];
	BOBHash32 * hash[d];
public:
    string name;
    CUSketch()
    {
        memset(counters, 0, sizeof(counters));
        for (int i = 0; i < d; i++)
            hash[i] = new BOBHash32(i + 750);

        stringstream name_buffer;
        name_buffer << "CU@" << memory_in_bytes << "@" << d;
        name = name_buffer.str();
    }

    void print_basic_info()
    {
        printf("CU sketch\n");
        printf("\tCounters: %d\n", w);
        printf("\tMemory: %.6lfMB\n", w * 4.0 / 1024 / 1024);
    }

    virtual ~CUSketch()
    {
        for (int i = 0; i < d; i++)
            delete hash[i];
    }

    void insert(uint8_t * key, int f = 1)
    {
        int index[d];
        int value[d];
        int min_val = 1 << 30;

        for (int i = 0; i < d; i++) {
            index[i] = (hash[i]->run((const char *)key, key_len)) % w;
            value[i] = counters[index[i]];
            min_val = min(min_val, value[i]);
        }

        int temp = min_val + f;
        for (int i = 0; i < d; i++) {
            counters[index[i]] = max(counters[index[i]], temp);
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

#endif //_CUSKETCH_H