# Function Head Injector

libclangを使用してC/C++ソースコードの各関数の先頭に任意のコードを挿入するプログラム

## 機能

- C/C++ソースファイルの解析
- 各関数の定義を特定
- 指定されたコードを各関数の先頭に自動挿入
  - 以下のプレースホルダーを使用して、関数の情報を動的に置換可能:
    - `{{ function_name }}`: 関数名
    - `{{ class_name }}`: クラス名（メソッドの場合のみ）
    - `{{ arg_names }}`: カンマ区切りの引数名リスト（例: `a, b`）
    - `{{ arg_types }}`: カンマ区切りの引数の型リスト（例: `int, char *`）
    - `{{ num_args }}`: 引数の数
- 指定されたコードをソースファイルの先頭にヘッダとして追加可能（オプション）
- 新しいソースファイルとして出力

## ビルド

```bash
make
```

## 使用方法

```bash
Usage: ./bin/function_injector <input_source_file> <injection_code_file> [-o <output_file>] [-e <exclude_pattern_file>] [-H <header_injection_file>]
  1st arg ... input_source_file   : C/C++ source file to analyze
  2nd arg ... injection_code_file : File containing code to inject at function starts
                                    (Can use {{ function_name }} placeholder to insert function name dynamically)
  -o output_file          : Output file (if not specified, writes to stdout)
  -e exclude_pattern_file : File containing regex patterns for functions to exclude (one per line)
  -H header_injection_file: File containing code to inject at the top of the file
```


## 使用例

### 例1: 関数名を含むデバッグログの挿入

1. 挿入するコードを準備（`debug_printf.txt`）:
`{{ function_name }}` などのプレースホルダーを使うと、挿入先の関数情報に自動で置換されます（大文字・小文字問わず、またスペースを入れてもOKです）。
```c
printf(">> Function entry: {{ function_name }} (Args: {{ arg_names }}) at %s:%d\n", __FILE__, __LINE__);
```

2. 共通のヘッダファイルを追加するコードを準備（`header.txt`）:
```c
#include <stdio.h>
```

3. プログラムを実行:
```bash
./bin/function_injector my_program.c debug_printf.txt -H header.txt
```

これにより、`my_program.c`の先頭に`header.txt`の内容が挿入され、各関数に`debug_printf.txt`の内容が置換・挿入されたプログラムがコンソールに出力されます。


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