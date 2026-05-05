CXX = g++
CXXFLAGS = -O2 -Iimgui -Iimgui/backends -Iimplot -march=native
LIBS = -lglfw -lGL -ldl -lpthread

OBJ = main.o \
      imgui/imgui.o imgui/imgui_draw.o imgui/imgui_tables.o imgui/imgui_widgets.o \
      imgui/backends/imgui_impl_glfw.o imgui/backends/imgui_impl_opengl3.o \
      implot/implot.o implot/implot_items.o

all: neongui

neongui: $(OBJ)
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f neongui $(OBJ)