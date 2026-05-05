CXX = g++
CXXFLAGS = -O2 -Iimgui -Iimgui/backends -Iimplot
LIBS = -lGL -lglfw -lpthread -ldl

SRC = main.cpp \
      imgui/imgui.cpp \
      imgui/imgui_draw.cpp \
      imgui/imgui_tables.cpp \
      imgui/imgui_widgets.cpp \
      imgui/backends/imgui_impl_glfw.cpp \
      imgui/backends/imgui_impl_opengl3.cpp \
      implot/implot.cpp \
      implot/implot_items.cpp

all:
	$(CXX) $(CXXFLAGS) -o neongui $(SRC) $(LIBS)

clean:
	rm -f neongui