#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <map>
#include <initializer_list>
#include <thread>
#include <chrono>

// 1. デフォルト引数にラムダ式を使用
void process_with_default(
    int value,
    std::function<int(int)> processor = [](int x) { return x * 2; }
) {
    std::cout << "Processed: " << processor(value) << std::endl;
}

// 2. 関数引数に直接ラムダを記述
template<typename Func>
void execute_with_retry(
    int max_retries,
    Func action = []() { std::cout << "Default action\n"; }
) {
    for (int i = 0; i < max_retries; ++i) {
        action();
    }
}

// 3. 初期化リストを引数に取る関数
void process_numbers(
    std::initializer_list<int> numbers = {1, 2, 3, 4, 5}
) {
    std::cout << "Processing numbers: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
}

// 4. 構造体の初期化を含む引数
struct Config {
    int timeout;
    std::string name;
    std::function<void()> callback;
};

void setup_system(
    Config config = {
        .timeout = 1000,
        .name = "default",
        .callback = []() { std::cout << "Default callback\n"; }
    }
) {
    std::cout << "Setup with timeout: " << config.timeout 
              << ", name: " << config.name << std::endl;
    config.callback();
}

// 5. 複数のラムダを引数に取る関数
void transform_and_filter(
    std::vector<int>& data,
    std::function<int(int)> transformer = [](int x) { return x * x; },
    std::function<bool(int)> filter = [](int x) { return x > 10; }
) {
    std::transform(data.begin(), data.end(), data.begin(), transformer);
    data.erase(
        std::remove_if(data.begin(), data.end(), 
                      [&filter](int x) { return !filter(x); }),
        data.end()
    );
}

// 6. ネストしたラムダを含む関数
auto create_nested_processor(
    std::function<int(int)> outer = [](int x) {
        auto inner = [](int y) { return y + 10; };
        return inner(x) * 2;
    }
) {
    return [outer](int value) {
        auto result = outer(value);
        std::cout << "Nested processing result: " << result << std::endl;
        return result;
    };
}

// 7. マップの初期化を含む引数
void process_with_map(
    std::map<std::string, std::function<void()>> actions = {
        {"start", []() { std::cout << "Starting...\n"; }},
        {"stop", []() { std::cout << "Stopping...\n"; }},
        {"pause", []() { std::cout << "Pausing...\n"; }}
    }
) {
    for (const auto& [name, action] : actions) {
        std::cout << "Executing " << name << ": ";
        action();
    }
}

// 8. 可変長テンプレートとラムダ
template<typename... Funcs>
void execute_all(Funcs... funcs) {
    // 初期化リストトリックを使って各関数を実行
    auto dummy = {
        (funcs(), 0)...
    };
    (void)dummy; // 未使用警告を回避
}

// 9. ラムダ内でクラスを定義
void create_with_local_class(
    std::function<void()> creator = []() {
        struct LocalHelper {
            void help() { std::cout << "Local class helper\n"; }
        };
        LocalHelper helper;
        helper.help();
    }
) {
    creator();
}

// 10. 複雑なラムダチェーン
auto create_pipeline(
    std::function<int(int)> stage1 = [](int x) { return x + 1; },
    std::function<int(int)> stage2 = [](int x) { return x * 2; },
    std::function<int(int)> stage3 = [](int x) { return x - 3; }
) {
    return [=](int input) {
        auto temp1 = stage1(input);
        auto temp2 = stage2(temp1);
        auto result = stage3(temp2);
        std::cout << input << " -> " << temp1 << " -> " 
                  << temp2 << " -> " << result << std::endl;
        return result;
    };
}

// 11. スレッドとラムダ
void async_execute(
    std::function<void()> task = []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Async task completed\n";
    }
) {
    std::thread t(task);
    t.join();
}

// 12. ラムダを返す関数にラムダを渡す
auto make_validator(
    std::function<bool(int)> condition = [](int x) { 
        return x > 0 && x < 100; 
    }
) -> std::function<std::string(int)> {
    return [condition](int value) -> std::string {
        if (condition(value)) {
            return "Valid: " + std::to_string(value);
        } else {
            return "Invalid: " + std::to_string(value);
        }
    };
}

int main() {
    std::cout << "=== Function definitions with nested braces ===\n\n";
    
    // 1. デフォルトラムダ
    process_with_default(5);
    process_with_default(5, [](int x) { return x * x; });
    
    // 2. テンプレート関数
    execute_with_retry(3);
    execute_with_retry(2, []() { std::cout << "Custom action\n"; });
    
    // 3. 初期化リスト
    process_numbers();
    process_numbers({10, 20, 30});
    
    // 4. 構造体初期化
    setup_system();
    setup_system({
        .timeout = 5000,
        .name = "custom",
        .callback = []() { std::cout << "Custom callback\n"; }
    });
    
    // 5. 変換とフィルタ
    std::vector<int> data = {1, 2, 3, 4, 5, 6};
    transform_and_filter(data);
    std::cout << "Filtered data: ";
    for (int n : data) std::cout << n << " ";
    std::cout << std::endl;
    
    // 6. ネストしたプロセッサ
    auto processor = create_nested_processor();
    processor(5);
    
    // 7. マップ処理
    process_with_map();
    
    // 8. 可変長引数
    execute_all(
        []() { std::cout << "First\n"; },
        []() { std::cout << "Second\n"; },
        []() { std::cout << "Third\n"; }
    );
    
    // 9. ローカルクラス
    create_with_local_class();
    
    // 10. パイプライン
    auto pipeline = create_pipeline();
    pipeline(10);
    
    // 11. 非同期実行
    async_execute();
    
    // 12. バリデータ
    auto validator = make_validator();
    std::cout << validator(50) << std::endl;
    std::cout << validator(150) << std::endl;
    
    return 0;
}