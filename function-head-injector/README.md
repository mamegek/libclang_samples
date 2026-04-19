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

## アーキテクチャ

### モジュール構成

```
src/
  main.cpp       -- エントリポイント。CXRewriterを使ったソース書き換えの制御
  ast_parser.cpp -- libclangを使ったAST解析。関数定義の検出
  code_gen.cpp   -- 注入コードの生成（テンプレート/printf/USDT）
  utils.cpp      -- ファイルI/O、引数解析、除外パターン処理
  types.h        -- 共通データ構造の定義
```

### 処理の流れ

```
1. コマンドライン引数の解析
2. 注入コード・ヘッダコードのファイル読み込み
3. libclangでソースファイルをパースしASTを構築
4. ASTを走査して関数定義の位置情報を収集
5. CXRewriterを作成し、各関数の本体先頭にコードを挿入
6. ヘッダコードがあればファイル先頭に挿入
7. CXRewriterが書き換え済みソースを出力
```

### ソース書き換えの仕組み（CXRewriter）

ソースコードの書き換えにはlibclangの `CXRewriter` API (`<clang-c/Rewrite.h>`) を使用しています。

CXRewriterはTranslationUnit（AST）に紐づくソース書き換えエンジンで、以下のAPIを提供します:

| API | 用途 |
|-----|------|
| `clang_CXRewriter_create(TU)` | TranslationUnitからRewriterを作成 |
| `clang_CXRewriter_insertTextBefore(Rew, Loc, text)` | 指定位置にテキストを挿入 |
| `clang_CXRewriter_replaceText(Rew, Range, text)` | 指定範囲のテキストを置換 |
| `clang_CXRewriter_removeText(Rew, Range)` | 指定範囲のテキストを削除 |
| `clang_CXRewriter_writeMainFileToStdOut(Rew)` | 書き換え結果をstdoutへ出力 |
| `clang_CXRewriter_overwriteChangedFiles(Rew)` | 変更されたファイルを上書き保存 |
| `clang_CXRewriter_dispose(Rew)` | Rewriterのリソースを解放 |

本プログラムでは以下のように使っています:

```cpp
// ASTからRewriterを作成
CXRewriter rewriter = clang_CXRewriter_create(translationUnit);

// ファイル先頭にヘッダコードを挿入
CXSourceLocation fileStart = clang_getLocationForOffset(TU, file, 0);
clang_CXRewriter_insertTextBefore(rewriter, fileStart, headerCode);

// 各関数の本体先頭（ '{' の直後）にフックコードを挿入
CXSourceLocation loc = clang_getLocationForOffset(TU, file, bodyStartOffset);
clang_CXRewriter_insertTextBefore(rewriter, loc, hookCode);

// 結果を出力
clang_CXRewriter_writeMainFileToStdOut(rewriter);
```

CXRewriterはオフセットの管理を内部で行うため、挿入順序を気にする必要がありません。手動で文字列を操作する場合と比べて、安全で簡潔なコードになります。

### 注入モード

| モード | 説明 | 引数 |
|--------|------|------|
| `template`（デフォルト） | テンプレートファイルのプレースホルダーを関数情報で置換 | 注入コードファイルが必要 |
| `printf` | 関数名と引数値を出力するprintf文を自動生成 | 注入コードファイル不要 |
| `usdt` | USDT（User Statically-Defined Tracing）プローブを自動生成 | 注入コードファイル不要 |

## ビルド

```bash
make
```

前提条件:
- libclangがシステムにインストールされていること
- `llvm-config` がPATHに存在すること

## 使用方法

```bash
Usage: ./bin/function_injector <input_source_file> [-m <mode>] [injection_code_file] [-o <output_file>]
         [-e <exclude_pattern_file>] [-H <header_injection_file>]
  input_source_file     : C/C++ source file to analyze
  injection_code_file   : File containing code to inject at function starts
                          (required for template mode, ignored otherwise)
  -m mode               : Generation mode: template (default), printf, usdt
  -o output_file        : Output file (if not specified, writes to stdout)
  -e exclude_pattern_file : File containing regex patterns for functions to exclude (one per line)
  -H header_injection_file: File containing code to inject at the top of the file
```

## 使用例

### 例1: テンプレートモードでデバッグログを挿入

挿入するコードを準備（`debug_printf.txt`）:
```c
printf(">> Function entry: {{ function_name }} (Args: {{ arg_names }}) at %s:%d\n", __FILE__, __LINE__);
```

ヘッダコードを準備（`header.txt`）:
```c
#include <stdio.h>
```

実行:
```bash
./bin/function_injector my_program.c debug_printf.txt -H header.txt -o output.c
```

### 例2: printfモードで自動的に引数値をログ出力

```bash
./bin/function_injector my_program.c -m printf
```

各関数に対して、引数の型に応じたprintf文が自動生成されます。

### 例3: USDTプローブを挿入

```bash
./bin/function_injector my_program.c -m usdt -H usdt_header.txt -o instrumented.c
```

### 例4: 特定の関数を除外

除外パターンファイル（`exclude.txt`、1行1正規表現）:
```
^main$
^test_.*
```

```bash
./bin/function_injector my_program.c debug_printf.txt -e exclude.txt
```

## テスト

```bash
bash tests/run_tests.sh
```

期待出力の更新:
```bash
bash tests/run_tests.sh --update
```

## クリーンアップ

```bash
make clean
```

## 注意事項

- libclangがシステムにインストールされている必要がある
- CXRewriter APIはLLVM 7.0以降で利用可能
