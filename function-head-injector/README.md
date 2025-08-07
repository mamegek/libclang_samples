# Function Head Injector

libclangを使用してC/C++ソースコードの各関数の先頭に任意のコードを挿入するプログラム

## 機能

- C/C++ソースファイルの解析
- 各関数の定義を特定
- 指定されたコードを各関数の先頭に自動挿入
- 新しいソースファイルとして出力

## ビルド

```bash
make
```

## 使用方法

```bash
Usage: ./bin/function_injector <input_source_file> <injection_code_file> [-o <output_file>] [-e <exclude_pattern_file>]
  1st arg ... input_source_file   : C/C++ source file to analyze
  2nd arg ... injection_code_file : File containing code to inject at function starts
  -o output_file          : Output file (if not specified, writes to stdout)
  -e exclude_pattern_file : File containing regex patterns for functions to exclude (one per line)
```


## 使用例

### 例1: デバッグログの挿入

1. 挿入するコードを準備（`debug_printf.txt`）:
```c
printf(">> Function entry: %s:%d\n", __FILE__, __LINE__);
```

2. プログラムを実行:
```bash
./bin/function_injector my_program.c debug_log.txt
```

これにより、`my_program.c`の各関数に`debug_printf.txt`の内容が挿入されたプログラムがコンソールに出力される。　


## テストの実行

サンプルファイルでのテスト:
```bash
make run
```

## クリーンアップ

```bash
make clean
```

## 注意事項

- libclangがシステムにインストールされている必要がある