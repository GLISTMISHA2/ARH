#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>

#ifdef __arm__
    #include <arm_neon.h>
    #ifndef IMGUI_IMPL_OPENGL_ES3
    #define IMGUI_IMPL_OPENGL_ES3
    #endif
    const char* GLSL_VERSION = "#version 300 es";
#else
    const char* GLSL_VERSION = "#version 130";
#endif

// --- Скалярная реализация ---
int64_t scalar_sum_abs(const int32_t* data, size_t n) {
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t val = data[i];
        sum += (val > 0) ? val : -val;
    }
    return sum;
}

// --- Настоящая NEON реализация ---
int64_t neon_sum_abs(const int32_t* data, size_t n) {
#ifdef __arm__
    int64x2_t sum_v = vdupq_n_s64(0); // 64-битный аккумулятор для защиты от переполнения
    size_t i = 0;

    // Обработка по 4 элемента за итерацию
    for (; i + 3 < n; i += 4) {
        int32x4_t v = vld1q_s32(data + i);      // Загрузка [a, b, c, d]
        int32x4_t abs_v = vabsq_s32(v);         // Аппаратный модуль: [|a|, |b|, |c|, |d|]
        
        // Расширение до 64 бит и прибавление к аккумулятору
        sum_v = vaddw_s32(sum_v, vget_low_s32(abs_v));
        sum_v = vaddw_high_s32(sum_v, abs_v);
    }

    // Сборка суммы из вектора
    int64_t total = vgetq_lane_s64(sum_v, 0) + vgetq_lane_s64(sum_v, 1);

    // Доработка хвоста массива
    for (; i < n; i++) {
        int32_t val = data[i];
        total += (val > 0) ? val : -val;
    }
    return total;
#else
    return scalar_sum_abs(data, n); // Заглушка для x86
#endif
}

struct BenchResult {
    double N;
    double scalar_sec;
    double neon_sec;
};

double run_bench(const int32_t* data, size_t n, int runs, bool use_neon) {
    if (n == 0) return 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < runs; r++) {
        volatile int64_t s = use_neon ? neon_sum_abs(data, n) : scalar_sum_abs(data, n);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count() / runs;
}

int main() {
    if (!glfwInit()) return 1;

#ifdef __arm__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1100, 700, "Orange Pi 5+ NEON Benchmark", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    std::vector<BenchResult> results;
    int runs = 100;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Тулбар
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1100, 60));
        ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration);
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("Averaging Runs", &runs, 1, 1000); ImGui::SameLine();
        
        if (ImGui::Button("START NEON TEST", ImVec2(200, 40))) {
            results.clear();
            for (double e = 2.0; e <= 6.001; e += 0.1) {
                size_t N = static_cast<size_t>(std::pow(10.0, e));
                int32_t* data = new int32_t[N];
                for (size_t j = 0; j < N; j++) 
                    data[j] = (j % 3 == 0) ? -(int32_t)j : (int32_t)j;
                
                results.push_back({(double)N, run_bench(data, N, runs, false), run_bench(data, N, runs, true)});
                delete[] data;
            }
        }
        ImGui::End();

        // Таблица
        ImGui::SetNextWindowPos(ImVec2(0, 60));
        ImGui::SetNextWindowSize(ImVec2(300, 640));
        ImGui::Begin("Data", nullptr, ImGuiWindowFlags_NoDecoration);
        if (ImGui::BeginTable("T", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Size N"); ImGui::TableSetupColumn("Scalar(s)"); ImGui::TableSetupColumn("NEON(s)");
            ImGui::TableHeadersRow();
            for (auto& r : results) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%.0f", r.N);
                ImGui::TableNextColumn(); ImGui::Text("%.6f", r.scalar_sec);
                ImGui::TableNextColumn(); ImGui::Text("%.6f", r.neon_sec);
            }
            ImGui::EndTable();
        }
        ImGui::End();

        // График
        ImGui::SetNextWindowPos(ImVec2(300, 60));
        ImGui::SetNextWindowSize(ImVec2(800, 640));
        ImGui::Begin("Plot", nullptr, ImGuiWindowFlags_NoDecoration);
        if (!results.empty()) {
            if (ImPlot::BeginPlot("Performance: Scalar vs NEON", ImVec2(-1, -1))) {
                ImPlot::SetupAxis(ImAxis_X1, "Elements (N)");
                ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
                ImPlot::SetupAxis(ImAxis_Y1, "Time (Seconds)");

                std::vector<double> xs, ys, yn;
                for (auto& r : results) { xs.push_back(r.N); ys.push_back(r.scalar_sec); yn.push_back(r.neon_sec); }

                ImPlot::PlotLine("Scalar", xs.data(), ys.data(), xs.size());
                ImPlot::PlotLine("NEON", xs.data(), yn.data(), xs.size());
                ImPlot::PlotScatter("Scalar Pts", xs.data(), ys.data(), xs.size());
                ImPlot::PlotScatter("NEON Pts", xs.data(), yn.data(), xs.size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}