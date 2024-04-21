CC=clang
CXX=clang++
C_FLAGS=-Wall -Wextra -Werror -Wpedantic -std=c23 -O3 -g
CXX_FLAGS=-Wall -Wextra -Werror -Wpedantic -std=c++20 -O3 -g
OUT_DIR=bin

VULKAN_INC=$(VULKAN_SDK)/Include

CMAKE_GEN_FLAGS=\
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON\
	-DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release

GLM_SRC=glm
GLM_GEN_FLAGS=-DGLM_BUILD_TESTS=OFF

GLFW_SRC=glfw
GLFW_GEN_FLAGS=\
	-DGLFW_LIBRARY_TYPE=STATIC\
	-DGLFW_BUILD_EXAMPLES=OFF\
	-DGLFW_BUILD_TESTS=OFF\
	-DGLFW_BUILD_DOCS=OFF

LINK_FLAGS=-static -m64 -lm
WINDOWS_LIBS=-lgdi32 -lwinmm 

.PHONY: all clean run\
	glm glm_clean\
	glfw glfw_clean

all: glm glfw 
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXX_FLAGS) -v\
		-I$(VULKAN_INC)\
		-I$(GLM_SRC) -L$(GLM_SRC)/build/glm/Debug -lglm\
		-I$(GLFW_SRC)/include  -L$(GLFW_SRC)/build/src/Debug -lglfw3\
		$(LINK_FLAGS) $(WINDOWS_LIBS)\
		-o $(OUT_DIR)/vulkan_tutorial vulkan_tutorial.cc

run: all
	$(OUT_DIR)/vulkan_tutorial

clean: glm_clean glfw_clean
	rm -rf $(OUT_DIR)

## GLM

$(GLM_SRC)/build/:
	cd $(GLM_SRC) && cmake -S . -B build $(GLM_GEN_FLAGS) $(CMAKE_GEN_FLAGS)

glm: $(GLM_SRC)/build/
	cd $(GLM_SRC) && cmake --build build

glm_clean:
	rm -rf $(GLM_SRC)/build

## GLFW

$(GLFW_SRC)/build/:
	cd $(GLFW_SRC) && cmake -S . -B build $(GLFW_GEN_FLAGS) $(CMAKE_GEN_FLAGS)

glfw: $(GLFW_SRC)/build/
	cd $(GLFW_SRC) && cmake --build build

glfw_clean:
	rm -rf $(GLFW_SRC)/build
