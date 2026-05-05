#include <arm_neon.h>
#include <cstdint>
#include <cstddef>  // ДЛЯ size_t !!!

int64_t process_array_scalar(const int32_t* data, size_t n) {
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t val = data[i];
        if (val > 0)
            sum += val;
        else if (val < 0)
            sum += -val;
    }
    return sum;
}

int64_t process_array_neon(const int32_t* data, size_t n) {
    int32x4_t acc = vdupq_n_s32(0);
    size_t i = 0;

    for (; i + 3 < n; i += 4) {
        int32x4_t vec = vld1q_s32(data + i);
        
        // ИСПРАВЛЕНИЕ 1: zero должен быть int32x4_t
        int32x4_t zero = vdupq_n_s32(0);
        
        // ИСПРАВЛЕНИЕ 2: vcgtq и vcltq возвращают uint32x4_t, но нам нужна маска
        uint32x4_t mask_pos = vcgtq_s32(vec, zero);  // uint32x4_t!
        uint32x4_t mask_neg = vcltq_s32(vec, zero);  // uint32x4_t!
        
        // Баррельный шифтер для модуля
        int32x4_t sign = vshrq_n_s32(vec, 31);
        int32x4_t abs_val = veorq_s32(vec, sign);
        abs_val = vsubq_s32(abs_val, sign);
        
        // vandq_s32 ожидает int32x4_t, а маски у нас uint32x4_t
        // ИСПРАВЛЕНИЕ 3: используем vreinterpretq_s32_u32 для преобразования
        int32x4_t pos_part = vandq_s32(vec, vreinterpretq_s32_u32(mask_pos));
        int32x4_t neg_part = vandq_s32(abs_val, vreinterpretq_s32_u32(mask_neg));
        int32x4_t result = vorrq_s32(pos_part, neg_part);
        
        acc = vaddq_s32(acc, result);
    }
    
    // ИСПРАВЛЕНИЕ 4: vaddlvq_s32 → vpaddlq_s32 для ARMv7
    // Горизонтальное сложение вручную
    int32x2_t sum32 = vadd_s32(vget_high_s32(acc), vget_low_s32(acc));
    sum32 = vpadd_s32(sum32, sum32);
    int64_t sum = vget_lane_s32(sum32, 0);
    
    // Обработка остатка
    for (; i < n; i++) {
        int32_t val = data[i];
        if (val > 0) sum += val;
        else if (val < 0) sum -= val;
    }
    
    return sum;
}