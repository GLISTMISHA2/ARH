#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstddef>
#include <chrono>
#include <vector>
#include <algorithm>

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
    return scalar(data, n);
}

struct BenchResult {
    size_t N;
    double scalar_us;
    double neon_us;
    double speedup;
};

double bench(const int32_t* data, size_t n, int runs, bool use_neon) {
    double total = 0;
    for (int r = 0; r < runs; r++) {
        auto start = std::chrono::high_resolution_clock::now();
        volatile int64_t s = use_neon ? neon(data, n) : scalar(data, n);
        auto end = std::chrono::high_resolution_clock::now();
        total += std::chrono::duration<double, std::micro>(end - start).count();
    }
    return total / runs;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1100, 700, "NEON Benchmark", NULL, NULL);
    glfwMakeContextCurrent(window);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImPlot::CreateContext();

    std::vector<BenchResult> results;
    size_t sizes[] = {100, 500, 1000, 5000, 10000, 50000, 100000, 500000, 1000000};
    int num_sizes = sizeof(sizes)/sizeof(sizes[0]);
    int runs = 10;
    bool done = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ВЕРХ
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1100, 55));
        ImGui::Begin("##bar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Runs:"); ImGui::SameLine();
        ImGui::PushItemWidth(70);
        ImGui::SliderInt("##r", &runs, 1, 50, "%d"); ImGui::SameLine(120);
        ImGui::PopItemWidth();

        if (ImGui::Button("RUN", ImVec2(120, 30))) {
            results.clear();
            for (int s = 0; s < num_sizes; s++) {
                size_t N = sizes[s];
                alignas(16) int32_t* data = new int32_t[N];
                for (size_t i = 0; i < N; i++)
                    data[i] = (i % 3 == 0) ? -(int32_t)i : (i % 3 == 1) ? (int32_t)i : 0;
                results.push_back({N, bench(data, N, runs, false), bench(data, N, runs, true), 0});
                results.back().speedup = results.back().scalar_us / results.back().neon_us;
                delete[] data;
            }
            done = true;
        }

        if (done) {
            double mx = 0;
            for (auto& r : results) if (r.speedup > mx) mx = r.speedup;
            ImGui::SameLine(260);
            ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "DONE | Max: %.2fx", mx);
        }

        ImGui::End();

        if (!results.empty()) {
            // ТАБЛИЦА
            ImGui::SetNextWindowPos(ImVec2(0, 55));
            ImGui::SetNextWindowSize(ImVec2(350, 645));
            ImGui::Begin("Table", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            if (ImGui::BeginTable("tbl", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("N");
                ImGui::TableSetupColumn("Scalar");
                ImGui::TableSetupColumn("NEON");
                ImGui::TableSetupColumn("Speedup");
                ImGui::TableHeadersRow();

                for (auto& r : results) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%zu", r.N);
                    ImGui::TableNextColumn(); ImGui::Text("%.1f us", r.scalar_us);
                    ImGui::TableNextColumn(); ImGui::Text("%.1f us", r.neon_us);
                    ImGui::TableNextColumn();
                    ImVec4 c = r.speedup >= 3 ? ImVec4(0,1,0,1) :
                               r.speedup >= 1.5 ? ImVec4(1,1,0,1) : ImVec4(1,0.3f,0.3f,1);
                    ImGui::TextColored(c, "%.2fx", r.speedup);
                }
                ImGui::EndTable();
            }
            ImGui::End();

            // ГРАФИКИ
            ImGui::SetNextWindowPos(ImVec2(350, 55));
            ImGui::SetNextWindowSize(ImVec2(750, 645));
            ImGui::Begin("Graphs", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            std::vector<double> xs, ys, yn, sp;
            for (auto& r : results) {
                xs.push_back((double)r.N);
                ys.push_back(r.scalar_us);
                yn.push_back(r.neon_us);
                sp.push_back(r.speedup);
            }

            // Время
            if (ImPlot::BeginPlot("##time", ImVec2(730, 300))) {
                ImPlot::SetupAxis(ImAxis_X1, "Size");
                ImPlot::SetupAxis(ImAxis_Y1, "Time (us)");
                ImPlot::PlotLine("Scalar", xs.data(), ys.data(), xs.size());
                ImPlot::PlotLine("NEON", xs.data(), yn.data(), xs.size());
                ImPlot::EndPlot();
            }

            // Speedup
            if (ImPlot::BeginPlot("##sp", ImVec2(730, 300))) {
                ImPlot::SetupAxis(ImAxis_X1, "Size");
                ImPlot::SetupAxis(ImAxis_Y1, "Speedup (x)");

                double mx = 0;
                for (auto& r : results) if (r.speedup > mx) mx = r.speedup;
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, std::max(4.0, mx * 1.3));

                ImPlot::PlotBars("Speedup", xs.data(), sp.data(), xs.size(), 0.12);
                ImPlot::PlotLine("Trend", xs.data(), sp.data(), xs.size());

                double tx[] = {xs.front(), xs.back()};
                double ty[] = {3.0, 3.0};
                ImPlot::PlotLine("3x Goal", tx, ty, 2);

                ImPlot::EndPlot();
            }

            ImGui::End();
        }

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.08f, 0.08f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}