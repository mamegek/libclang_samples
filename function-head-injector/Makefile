CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = -lclang

# libclangのヘッダとライブラリのパスを設定
LLVM_CONFIG = llvm-config
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags)
LDFLAGS += $(shell $(LLVM_CONFIG) --ldflags --libs --system-libs)

# ビルド生成物用ディレクトリ
BIN_DIR = bin

# ターゲットとソース
TARGET = $(BIN_DIR)/function_injector
SOURCES = function_injector.cpp
OBJECTS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(SOURCES))

# デフォルトターゲット
all: $(TARGET)

# 実行ファイルの生成
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# オブジェクトファイルの生成
$(BIN_DIR)/%.o: %.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ディレクトリの作成
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean run