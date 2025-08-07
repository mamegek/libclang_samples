CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = -lclang

# libclangのヘッダとライブラリのパスを設定
LLVM_CONFIG = llvm-config
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags)
LDFLAGS += $(shell $(LLVM_CONFIG) --ldflags --libs --system-libs)

# ディレクトリ構成
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)

# ターゲットとソース
TARGET = $(BIN_DIR)/function_injector
SOURCES = function_injector.cpp
OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

# デフォルトターゲット
all: $(TARGET)

# 実行ファイルの生成
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# オブジェクトファイルの生成
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ディレクトリの作成
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# クリーンアップ
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# 依存関係の自動生成 (必須ではない)
-include $(OBJECTS:.o=.d)

$(OBJ_DIR)/%.d: %.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< > $@

.PHONY: all clean run