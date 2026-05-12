#include <arm_neon.h>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <chrono>
#include <iomanip>

int64_t scalar(const int32_t* data, size_t n) {
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t val = data[i];
        if (val > 0) sum += val;
        else if (val < 0) sum += -val;
    }
    return sum;
}

int64_t neon(const int32_t* data, size_t n) {
    int64_t sum = 0;
    int32x4_t acc = vdupq_n_s32(0);
    size_t i = 0;

    for (; i + 3 < n; i += 4) {
        int32x4_t vec = vld1q_s32(data + i);
        int32x4_t zero = vdupq_n_s32(0);
        uint32x4_t mask_pos = vcgtq_s32(vec, zero);
        uint32x4_t mask_neg = vcltq_s32(vec, zero);
        
        int32x4_t sign = vshrq_n_s32(vec, 31);
        int32x4_t abs_val = veorq_s32(vec, sign);
        abs_val = vsubq_s32(abs_val, sign);
        
        int32x4_t pos_part = vandq_s32(vec, vreinterpretq_s32_u32(mask_pos));
        int32x4_t neg_part = vandq_s32(abs_val, vreinterpretq_s32_u32(mask_neg));
        int32x4_t result = vorrq_s32(pos_part, neg_part);
        
        acc = vaddq_s32(acc, result);
        
        if (i % 64 == 60) {
            int32x2_t tmp = vadd_s32(vget_high_s32(acc), vget_low_s32(acc));
            tmp = vpadd_s32(tmp, tmp);
            sum += vget_lane_s32(tmp, 0);
            acc = vdupq_n_s32(0);
        }
    }
    
    int32x2_t sum32 = vadd_s32(vget_high_s32(acc), vget_low_s32(acc));
    sum32 = vpadd_s32(sum32, sum32);
    sum += vget_lane_s32(sum32, 0);
    
    for (; i < n; i++) {
        int32_t val = data[i];
        if (val > 0) sum += val;
        else if (val < 0) sum -= val;
    }
    return sum;
}

int main() {
    size_t sizes[] = {100, 1000, 5000, 10000, 50000, 100000, 500000, 1000000};
    int num = sizeof(sizes)/sizeof(sizes[0]);
    int runs = 5;

    std::cout << "+------------+----------+----------+----------+" << std::endl;
    std::cout << "|    N       | Scalar   | NEON     | Speedup  |" << std::endl;
    std::cout << "+------------+----------+----------+----------+" << std::endl;

    for (int s = 0; s < num; s++) {
        size_t N = sizes[s];
        alignas(16) int32_t* data = new int32_t[N];
        for (size_t i = 0; i < N; i++)
            data[i] = (i % 3 == 0) ? -(int32_t)i : (i % 3 == 1) ? (int32_t)i : 0;

        volatile int64_t w = scalar(data, N);
        w = neon(data, N);

        double t1 = 0, t2 = 0;
        for (int r = 0; r < runs; r++) {
            auto start = std::chrono::high_resolution_clock::now();
            volatile int64_t s1 = scalar(data, N);
            auto end = std::chrono::high_resolution_clock::now();
            t1 += std::chrono::duration<double, std::micro>(end - start).count();
        }
        t1 /= runs;

        for (int r = 0; r < runs; r++) {
            auto start = std::chrono::high_resolution_clock::now();
            volatile int64_t s2 = neon(data, N);
            auto end = std::chrono::high_resolution_clock::now();
            t2 += std::chrono::duration<double, std::micro>(end - start).count();
        }
        t2 /= runs;

        int64_t r1 = scalar(data, N);
        int64_t r2 = neon(data, N);
        bool ok = (r1 == r2);

        std::cout << "| " << std::setw(10) << N
                  << " | " << std::setw(8) << std::fixed << std::setprecision(1) << t1 << "us"
                  << " | " << std::setw(8) << t2 << "us"
                  << " | " << std::setw(6) << std::setprecision(2) << t1/t2 << "x"
                  << "  " << (ok ? "OK" : "FAIL") << " |" << std::endl;

        delete[] data;
    }
    std::cout << "+------------+----------+----------+----------+" << std::endl;
    return 0;
}