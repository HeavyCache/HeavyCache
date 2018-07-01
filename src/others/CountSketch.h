#ifndef _COUNT_SKETCH_H
#define _COUNT_SKETCH_H

#include "../BOBHash32.h"
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <vector>

using namespace std;

template <uint32_t keylen, int memory_in_bytes, int d>
class CountSketch
{
private:
    static constexpr int w = memory_in_bytes * 8 / 32 / d;
	int counters[d][w];
	BOBHash32 * hash[d];
    BOBHash32 * sign_hash[d];
public:
    string name = "Count Sketch";
    CountSketch()
    {
        memset(counters, 0, sizeof(counters));
        srand(time(0));
        for (int i = 0; i < d; i++) {
            hash[i] = new BOBHash32(rand() % MAX_PRIME32);
            sign_hash[i] = new BOBHash32(rand() % MAX_PRIME32);
        }

        stringstream name_buf;
        name_buf << "C@" << memory_in_bytes;
        name = name_buf.str();
    }

    void print_basic_info()
    {
        printf("Count sketch\n");
        printf("\tCounters: %d\n", w);
        printf("\tMemory: %.6lfMB\n", w * d * 4.0 / 1024 / 1024);
    }

    ~CountSketch()
    {
        for (int i = 0; i < d; i++) {
            delete hash[i];
            delete sign_hash[i];
        }
    }

    void insert(uint8_t * key, int f = 1)
    {
        int hash_val[d];
        int sign_val[d];

//        hash_r.get_hash_val(key, hash_val, d);
//        sign_hash_r.get_hash_val(key, sign_val, d);

        for (int i = 0; i < d; i++) {
            int index = (hash[i]->run((const char *)key, keylen)) % w;
            int sign = (sign_hash[i]->run((const char *)key, keylen)) % 2;
//            int index = hash_val[i] % w;
//            int sign = sign_val[i] % 2;
            counters[i][index] += sign ? f : -f;
        }
    }

	int query(uint8_t * key)
    {
        int hash_val[d];
        int sign_val[d];

//        hash_r.get_hash_val(key, hash_val, d);
//        sign_hash_r.get_hash_val(key, sign_val, d);

        vector<int> result(d);
        for (int i = 0; i < d; i++) {
            int index = (hash[i]->run((const char *)key, keylen)) % w;
            int sign = (sign_hash[i]->run((const char *)key, keylen)) % 2;
//            int index = hash_val[i] % w;
//            int sign = sign_val[i] % 2;

            result[i] = sign ? counters[i][index] : -counters[i][index];
        }

        sort(result.begin(), result.end());

        int mid = d / 2;
        int ret;
        if (d % 2 == 0) {
            ret = (result[mid] + result[mid - 1]) / 2;
        } else {
            ret = result[mid];
        }

//        if (ret <= 0)
//            return 1;
        return ret;
    }
};

#endif //_COUNT_SKETCH_H