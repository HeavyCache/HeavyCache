#include <cmath>
#include <string>
#include <cstring>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <iterator>
#include <random>

using namespace std;

long double alpha;
long double zetan;
long double eta;

random_device rd;

long double zeta(int n, long double skew) {
    long double res = 0;
    for (int i = 1; i <= n; i++) {
        res += pow(1.0 / i, skew);
    }
    return res;
}

int popzipf(int n, long double skew) {
    // popZipf(skew) = wss + 1 - floor((wss + 1) ** (x ** skew))
    long double u = rand() / (long double) (RAND_MAX);
    return (int) (n + 1 - floor(pow(n + 1, pow(u, skew))));
}

int trace_gen(uint32_t * data, int flow_num, int packet_num, double skew = 1.0) {
    srand(time(NULL));

    alpha = 1 / (1 - skew);
    zetan = zeta(flow_num, skew);
    eta = (1 - pow(2.0 / flow_num, 1 - skew)) / (1 - zeta(2, skew) / zetan);

    vector<uint32_t> vec_flow(flow_num);

    for (int i = 0; i < flow_num; i++) {
        vec_flow[i] = rd();
    }

    int next_value;

    for (int i = 0; i < packet_num; i++) {
        next_value = popzipf(flow_num, skew);
        data[i] = vec_flow[next_value - 1];
    }

    random_shuffle(data, data + packet_num);

    unordered_map<uint32_t, int> mp;
    for (int i = 0; i < packet_num; ++i) {
        mp[data[i]]++;
    }

    return mp.size();
}
