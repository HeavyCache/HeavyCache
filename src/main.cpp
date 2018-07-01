// A simple demo of HeavyCache
// show how HeavyCache records packets
// and how HeavyCache does 
// 1. flow size estimation
// 2. heavy hitter detecion
// 3. flow size distribution estimation
// 4. entropy estimation
// 5. cardinality estimation

#include <chrono>
#include "HeavyCache.h"
#include "trace_gen.h"

int flow_num = 1000000;
constexpr int packet_num = 20000000;

// HC with keylen=4, 2MB memory in total, 5000*8 cells in the cache part
#define HC_TYPE HeavyCache<4, 2 * 1024 * 1024, 5000, 8>

uint32_t trace_data[packet_num];

void test_flow_size_estimation(HC_TYPE * hc)
{
    unordered_map<uint32_t, int> mp;
    for (int i = 0; i < packet_num; ++i) {
        mp[trace_data[i]]++;
    }

    double aae = 0;
    for (auto & kv: mp) {
        uint32_t query_result = hc->query((uint8_t*)&kv.first);
        aae += std::abs(double(query_result) - double(kv.second));
    }
//    cout << mp.size() << endl;
    cout << "Flow size estimation" << endl;
    cout << "\tAAE: " << aae / flow_num << endl;
    cout << endl;
}

void test_heavy_hitter_detection(HC_TYPE * hc)
{
    constexpr int threshold = 5000;
    vector<pair<string, uint32_t>> est_result;
    hc->get_heavy_hitters(threshold, est_result);

    unordered_map<uint32_t, int> mp;
    for (int i = 0; i < packet_num; ++i) {
        mp[trace_data[i]]++;
    }

    double aae = 0;
    int hit = 0;
    for (auto & kv: est_result) {
        int true_val = mp[*(uint32_t *)(kv.first.c_str())];
        if (true_val >= threshold) {
            hit++;
        }
        aae += std::abs(double(kv.second) - double(true_val));
    }
    int report_size = est_result.size();
    int true_size = 0;
    for (auto & kv: mp) {
        if (kv.second >= threshold) {
            true_size++;
        }
    }

    cout << "Heavy hitter detection" << endl;
    cout << "\tAAE: " << aae / report_size << endl;
    cout << "\tPrecision: " << double(hit) / report_size << "(" << hit << "/" << report_size << ")" << endl;
    cout << "\tRecall: " << double(hit) / true_size << "(" << hit << "/" << true_size << ")" << endl;
    cout << endl;
}

void test_cardinality(HC_TYPE * hc)
{
    double est_card = hc->get_cardinality();
    double re = (est_card - flow_num) / flow_num;
    cout << "Cardinality estimation" << endl;
    cout << "\tRE: " << re << endl;
    cout << endl;
}

void test_fsd(HC_TYPE * hc)
{
    vector<double> est_dist, true_dist;

    unordered_map<uint32_t, int> mp;
    int max_cnt = 0;
    for (int i = 0; i < packet_num; ++i) {
        mp[trace_data[i]]++;
        max_cnt = std::max(mp[trace_data[i]], max_cnt);
    }
    true_dist.resize(max_cnt);
    for (auto & kv: mp) {
        true_dist[kv.second]++;
    }

    hc->collect_fsd();
    for (int i = 0; i < 5; ++i) {
        hc->next_epoch();
    }
    hc->get_distribution(est_dist);

    max_cnt = std::max(true_dist.size(), est_dist.size());
    true_dist.resize(max_cnt);
    est_dist.resize(max_cnt);

    double a = 0, b = 0;
    for (uint32_t i = 1; i < max_cnt; ++i) {
        double est = est_dist[i];
        double gt = true_dist[i];
        a += std::abs(est - gt);
        b += (est + gt) / 2;
    }
    double wmrd = a / b;

    cout << "Flow size distribution estimation" << endl;
    cout << "\tWMRD: " << wmrd << endl;
    cout << endl;
}

void test_entropy(HC_TYPE * hc)
{
    double est_entropy = hc->get_entropy();
    double true_entropy = 0;

    unordered_map<uint32_t, int> mp;
    int max_cnt = 0;
    for (int i = 0; i < packet_num; ++i) {
        mp[trace_data[i]]++;
        max_cnt = std::max(mp[trace_data[i]], max_cnt);
    }
    for (auto & kv: mp) {
        double p = kv.second / double(packet_num);
        true_entropy += -p * log2(p);
    }

    double re = (est_entropy - true_entropy) / true_entropy;

    cout << "Entropy estimation" << endl;
    cout << "\tRE: " << re << endl;
    cout << endl;
}

int main()
{
	HC_TYPE * hc = new HC_TYPE();

    flow_num = trace_gen(trace_data, flow_num, packet_num);
	cout << "Random trace generated." << endl;

    auto st = std::chrono::system_clock::now();
	for (int i = 0; i < packet_num; ++i) {
		hc->insert((uint8_t *)&trace_data[i], 1);
	}
    auto ed = std::chrono::system_clock::now();
    long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(ed - st).count();
    double build_th = packet_num * 1000.0 / duration;
    std::cout << "Recording Throughput: " << build_th << "Mips" << endl;

    test_flow_size_estimation(hc);
    test_heavy_hitter_detection(hc);
    test_cardinality(hc);
    test_fsd(hc);
    test_entropy(hc);

	return 0;
}