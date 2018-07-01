#ifndef _HeavyCache_H
#define _HeavyCache_H

#include <cstdint>
#include "BOBHash32.h"
#include <cstring>
#include <sstream>
#include <x86intrin.h>
#include <bmiintrin.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include "others/EMFSD.h"
using namespace std;


template<int key_length, int tot_memory_in_bytes, int bucket_num, int counter_per_bucket = 8, int group = 1,
    int d = 1, bool ci_strategy = true>
class alignas(64) HeavyCache
{
    typedef HeavyCache<key_length, tot_memory_in_bytes, bucket_num, counter_per_bucket, group,
     d, ci_strategy> MyType;
private:
    static constexpr int per_counter_size = key_length > 4 ? (key_length + 8) : 8;
    static constexpr int w = (tot_memory_in_bytes - bucket_num * counter_per_bucket * per_counter_size) * 8 / 16 / d;
    struct Bucket
    {
        uint32_t key[counter_per_bucket];
        uint32_t val[counter_per_bucket];
    } buckets[bucket_num];
    uint8_t key_stores[bucket_num][counter_per_bucket][key_length];
    int cur_pos;
    uint16_t counter[d][w];
    BOBHash32 *bobhash[d + 1];
    uint32_t cardinality = 0;
    uint32_t element_cnt = 0;

    EMFSD * em_fsd_algo = NULL;

public:
    string name;
    int stat_swap_cnt = 0;
    int stat_first_level_cnt = 0;

    HeavyCache()
    {
        memset(counter, 0, sizeof(counter));
        memset(buckets, 0, sizeof(buckets));
        cur_pos = 0;

        for (int i = 0; i <= d; i++)
        {
            bobhash[i] = new BOBHash32(i + 1000);
        }

        stringstream name_buffer;
        
        name_buffer << "HC@" << tot_memory_in_bytes << "@" << bucket_num << "@" << counter_per_bucket
                    << "@" << d << "@" << (ci_strategy ? 1 : 0);
        name = name_buffer.str();
    }

    void insert(uint8_t * key, int f = 1)
    {
        element_cnt++;

        // first select a bucket
        uint32_t fp;
        if (key_length == 4) {
            fp = *((uint32_t *)key);
        } else {
            fp = bobhash[d]->run((const char *)key, key_length);
        }
        uint32_t pos = ((fp * 2654435761u) >> 15) % (bucket_num - group + 1);

        // try find element cache
        for (int j = 0; j < group; ++j) {
            if (counter_per_bucket == 1) {
                if (buckets[pos + j].key[0] == fp) {
                    buckets[pos + j].val[0] += f;
                    stat_first_level_cnt++;
                    return;
                }
            }
            else if (counter_per_bucket == 2) {
                const __m64 item = _mm_set1_pi32((int)fp);
                __m64 *keys_p = (__m64 *)(buckets[pos + j].key);
                int matched = 0;

                __m64 a_comp = _mm_cmpeq_pi32(item, keys_p[0]);
                matched = _mm_movemask_pi8((__m64)a_comp);

                if (matched != 0)
                {
                    //return 32 if input is zero;
                    int matched_index = _tzcnt_u32((uint32_t)matched) / 4;
                    buckets[pos + j].val[matched_index] += f;
                    stat_first_level_cnt++;
                    return;
                }
            }
            else if (counter_per_bucket == 4) {
                const __m128i item = _mm_set1_epi32((int)fp);
                __m128i *keys_p = (__m128i *)(buckets[pos + j].key);
                int matched = 0;

                __m128i a_comp = _mm_cmpeq_epi32(item, keys_p[0]);
                matched = _mm_movemask_ps((__m128)a_comp);

                if (matched != 0)
                {
                    //return 32 if input is zero;
                    int matched_index = _tzcnt_u32((uint32_t)matched);
                    buckets[pos + j].val[matched_index] += f;
                    stat_first_level_cnt++;
                    return;
                }
            }
            else if (counter_per_bucket == 8) {
                const __m256i item = _mm256_set1_epi32((int)fp);
                __m256i *keys_p = (__m256i *)(buckets[pos + j].key);
                int matched = 0;

                __m256i a_comp = _mm256_cmpeq_epi32(item, keys_p[0]);
                matched = _mm256_movemask_ps((__m256)a_comp);

                if (matched != 0)
                {
                    //return 32 if input is zero;
                    int matched_index = _tzcnt_u32((uint32_t)matched);
                    buckets[pos + j].val[matched_index] += f;
                    stat_first_level_cnt++;
                    return;
                }
            }
            else if (counter_per_bucket == 16) {
                const __m256i item = _mm256_set1_epi32((int)fp);
                __m256i *keys_p = (__m256i *)(buckets[pos + j].key);
                int matched = 0;

                __m256i a_comp = _mm256_cmpeq_epi32(item, keys_p[0]);
                __m256i b_comp = _mm256_cmpeq_epi32(item, keys_p[1]);
                matched = _mm256_movemask_ps(*(__m256*)&a_comp);
                matched += (_mm256_movemask_ps(*(__m256*)&b_comp)) << 8;

                if (matched != 0)
                {
                    //return 32 if input is zero;
                    int matched_index = _tzcnt_u32((uint32_t)matched);
                    buckets[pos + j].val[matched_index] += f;
                    stat_first_level_cnt++;
                    return;
                }
            }
            else if (counter_per_bucket == 32) {
                const __m256i item = _mm256_set1_epi32((int)fp);
                __m256i *keys_p = (__m256i *)(buckets[pos + j].key);
                uint32_t matched = 0;

                __m256i a_comp = _mm256_cmpeq_epi32(item, keys_p[0]);
                __m256i b_comp = _mm256_cmpeq_epi32(item, keys_p[1]);
                __m256i c_comp = _mm256_cmpeq_epi32(item, keys_p[2]);
                __m256i d_comp = _mm256_cmpeq_epi32(item, keys_p[3]);
                matched = _mm256_movemask_ps((__m256)d_comp);
                matched <<= 8;
                matched += _mm256_movemask_ps((__m256)c_comp);
                matched <<= 8;
                matched += _mm256_movemask_ps((__m256)b_comp);
                matched <<= 8;
                matched += _mm256_movemask_ps((__m256)a_comp);

                if (matched != 0)
                {
                    //return 32 if input is zero;
                    int matched_index = _tzcnt_u32((uint32_t)matched);
                    buckets[pos + j].val[matched_index] += f;
                    stat_first_level_cnt++;
                    return;
                }
            }
            else {
                for (int i = 0; i < counter_per_bucket; ++i) {
                    if (buckets[pos + j].key[i] == fp) {
                        buckets[pos + j].val[i] += f;
                        stat_first_level_cnt++;
                        return;
                    }
                }
            }
        }

        // if not found, insert e into sketch, and do a query
        uint16_t sketch_report_val = 0;
        uint32_t sketch_poses[d];
        
        uint16_t sketch_min_val = 0xFFFFu;
        for (int i = 0; i < d; ++i) {
            uint32_t sk_pos = bobhash[i]->run((const char *)&fp, 4) % w;
            sketch_poses[i] = sk_pos;
            counter[i][sk_pos] += f;
            sketch_min_val = std::min(counter[i][sk_pos], sketch_min_val);
        }

        if (sketch_min_val == 0) {
            cardinality += 1;
        }

        sketch_report_val = sketch_min_val;

        // if this val > min_val of cache
        uint32_t min_val = 1u << 30;
        uint32_t min_pos = 0;
        for (uint32_t j = 0; j < group; ++j) {
            for (uint16_t i = 0; i < counter_per_bucket; ++i) {
                min_val = std::min(min_val, buckets[pos + j].val[i]);
                min_pos =
                    (min_val == buckets[pos + j].val[i]) ?
                    uint32_t(i + j * counter_per_bucket) :
                    min_pos;
            }
        }

        constexpr uint16_t delta = 1;
        if (sketch_report_val >= min_val + delta) {
            // swap
            uint32_t bucket_id = pos + min_pos / counter_per_bucket;
            uint32_t counter_id = min_pos % counter_per_bucket;
            uint32_t & k_ref = buckets[bucket_id].key[counter_id];
            uint32_t & val_ref = buckets[bucket_id].val[counter_id];

            for (int i = 0; i < d; ++i) {
                uint32_t hash_pos = bobhash[i]->run((const char *)&k_ref, 4) % w;

                if (counter[i][hash_pos] < val_ref) {
                    counter[i][hash_pos] = uint16_t(val_ref);
                }
                counter[i][sketch_poses[i]] -= f;
            }
       
            k_ref = fp;
            if (key_length > 4) {
                memcpy(key_stores[bucket_id][counter_id], key, key_length);
            }
            if (val_ref == 0) {
                val_ref = f;
                return;
            }
            stat_swap_cnt++;
            if (!ci_strategy) {
                val_ref = sketch_report_val;
            } else {
                val_ref = std::min(sketch_report_val, uint16_t(min_val + delta));
            }
        }
    }

    int query(uint8_t * key)
    {
        uint32_t fp;
        uint32_t bucket_min_val = 1u << 30;
        if (key_length == 4) {
            fp = *((uint32_t *)key);
        } else {
            fp = bobhash[d]->run((const char *)key, key_length);
        }
        uint32_t pos = ((fp * 2654435761u) >> 15) % (bucket_num - group + 1);
        for (int j = 0; j < group; ++j) {
            for (int i = 0; i < counter_per_bucket; i++) {
                if (buckets[pos + j].key[i] == fp)
                    return buckets[pos + j].val[i];
                bucket_min_val = std::min(buckets[pos + j].val[i], bucket_min_val);
            }
        }

        uint32_t hash_value, temp;
        uint32_t estimate_value = (1u << 30);
        for(int i = 0; i < d; i++)
        {
            hash_value = (bobhash[i]->run((const char *)&fp, 4)) % w;
            temp = counter[i][hash_value];
            estimate_value = estimate_value < temp ? estimate_value : temp;
        }
        if (!ci_strategy) {
            return estimate_value;
        } else {
            return bucket_min_val < estimate_value ? bucket_min_val : estimate_value;
        }
    }

    double get_cardinality()
    {
        uint16_t * mirror = new uint16_t[w];
        memcpy(mirror, counter[0], sizeof(counter[0]));
        for (int i = 0; i < bucket_num; i++) {
            for (int j = 0; j < counter_per_bucket; j++) {
                uint32_t fp = buckets[i].key[j];
                uint32_t hash_value = (bobhash[0]->run((const char *)&fp, 4)) % w;
                mirror[hash_value]++;
            }
        }

        int zero_cnt = 0;
        for (int i = 0; i < w; ++i) {
            if (mirror[i] == 0)
                zero_cnt++;
        }

        return (w * std::log(w / double(zero_cnt)));
    }

    double get_entropy()
    {
        collect_fsd();
        for (int i = 0; i < 5; ++i) {
            next_epoch();
        }

        vector<double> fsd;
        get_distribution(fsd);
        double s = 0;
        double sum = 0;
        for (int i = 1; i < fsd.size(); ++i) {
            s += (fsd[i]) * i * log2(i);
            sum += fsd[i] * i;
        }
        std::cout << element_cnt - sum << endl;

        return log2(element_cnt) - s / element_cnt;
    }

#pragma GCC push_options
#pragma GCC optimize ("O0")
    void get_top_k(uint32_t k, std::vector<pair<string, uint32_t> >& ret)
    {
        std::vector<pair<string, uint32_t>> all_elements(uint32_t(bucket_num * counter_per_bucket));
        for (int i = 0; i < bucket_num; ++i) {
            for (int j = 0; j < counter_per_bucket; ++j) {
                int idx = i * counter_per_bucket + j;
                if (key_length == 4) {
                    all_elements[idx].first.assign((const char *)&buckets[i].key[j], 4);
                } else {
                    all_elements[idx].first.assign((const char *)key_stores[i][j], key_length);
                }
                all_elements[idx].second = buckets[i].val[j];
            }
        }

        std::sort(all_elements.begin(), all_elements.end(), [](const std::pair<string, int> &left, const std::pair<string, int> &right) {
            return left.second > right.second;
        });

        ret.resize(k);

        std::copy(all_elements.begin(), all_elements.begin() + k, ret.begin());
    }
#pragma GCC pop_options

    void get_heavy_hitters(uint32_t threshold, std::vector<pair<string, uint32_t> >& ret)
    {
        int max_k = bucket_num * counter_per_bucket;
        ret.resize(max_k);
        get_top_k(max_k, ret);
        for (int i = 0; i < max_k; ++i) {
            if (ret[i].second < threshold) {
                ret.resize(i);
                return;
            }
        }
    }

    void collect_fsd()
    {
        if (!em_fsd_algo) {
            em_fsd_algo = new EMFSD();
        }
        if (!em_fsd_algo->inited) {
            // try another approach
            uint16_t * mirror = new uint16_t[w];
            memcpy(mirror, counter[0], w * sizeof(uint16_t));
            em_fsd_algo->set_counters(w, mirror);
            delete[] mirror;
        }
    }

    void next_epoch()
    {
        em_fsd_algo->next_epoch();
    }

    void get_distribution(vector<double> & fsd)
    {
        fsd = em_fsd_algo->ns;

        for (int i = 0; i < bucket_num; i++) {
            for (int j = 0; j < counter_per_bucket; ++j) {
                if (buckets[i].val[j] + 1> fsd.size()) {
                    fsd.resize(buckets[i].val[j] + 1);
                }
                fsd[buckets[i].val[j]]++;
            }
        }
    }

    string get_custom_string()
    {
        stringstream buf;
        buf << stat_swap_cnt / double(element_cnt) << "," << stat_first_level_cnt / double(element_cnt);
        return buf.str();
    }

    ~HeavyCache()
    {
        for(int i = 0; i < d; i++)
        {
            delete bobhash[i];
        }
    }
};

#endif//_HeavyCache_H