#include <cstdint>

int64_t process_array_scalar(const int32_t* data, size_t n) {
    int64_t sum = 0;

    for (size_t i = 0; i < n; i++) {
        int32_t val = data[i];

        if (val > 0)
            sum += val;
        else if (val < 0)
            sum += -val; // модуль
        // если 0 — ничего не делаем
    }

    return sum;
}
