# Function Head Injector

libclangを使用してC/C++ソースコードの各関数の先頭に任意のコードを挿入するプログラムです。

## 機能

- C/C++ソースファイルの解析
- 各関数の開始位置（開き中括弧の直後）を特定
- 指定されたコードを各関数の先頭に自動挿入
- 新しいソースファイルとして出力

## ビルド

```bash
make
```

## 使用方法

```bash
./bin/function_injector <入力ソースファイル> <挿入コードファイル> <出力ファイル>
```

### パラメータ
- `入力ソースファイル`: 解析対象のC/C++ソースファイル
- `挿入コードファイル`: 各関数の先頭に挿入するコードが記載されたテキストファイル
- `出力ファイル`: コードが挿入された新しいソースファイルの出力先

## 使用例

### 例1: デバッグログの挿入

1. 挿入するコードを準備（`debug_log.txt`）:
```c
printf(">> Function entry: %s:%d\n", __FILE__, __LINE__);
```

2. プログラムを実行:
```bash
./bin/function_injector my_program.c debug_log.txt my_program_debug.c
```

### 例2: パフォーマンス計測コードの挿入

1. 挿入するコードを準備（`perf_measure.txt`）:
```c
clock_t start_time = clock();
```

2. プログラムを実行:
```bash
./bin/function_injector application.cpp perf_measure.txt application_perf.cpp
```

### 例3: カスタムトレーシング

1. 挿入するコードを準備（`trace.txt`）:
```c
trace_enter(__FUNCTION__);
```

2. プログラムを実行:
```bash
./bin/function_injector source.c trace.txt source_traced.c
```

## テストの実行

サンプルファイルでのテスト:
```bash
make run
```

これにより、`test_input.c`に`injection_code.txt`の内容が挿入され、`test_output.c`が生成されます。

## クリーンアップ

```bash
make clean
```

## 注意事項

- libclangがシステムにインストールされている必要があります
- C++11以上のコンパイラが必要です
- 挿入されるコードは各関数の開き中括弧`{`の直後に配置されます
- システムヘッダーファイル内の関数定義も検出される場合があります