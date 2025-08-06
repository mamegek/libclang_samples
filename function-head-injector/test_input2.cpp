#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include <numeric>
#include <type_traits>
#include <memory>

// 1. 基本的なテンプレート関数
template<typename T>
T max_of_three(T a, T b, T c) {
    return std::max({a, b, c});
}

// 2. 可変長テンプレート
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);  // C++17 fold expression
}

// 3. テンプレート特殊化
template<typename T>
std::string type_name() {
    return "unknown";
}

template<>
std::string type_name<int>() {
    return "int";
}

template<>
std::string type_name<double>() {
    return "double";
}

// 4. constexpr関数
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// 5. インライン関数
inline int square(int x) {
    return x * x;
}

// 6. 関数テンプレートとSFINAE
template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type
multiply_by_two(T value) {
    return value * 2;
}

// 7. ラムダ式を返す関数
auto make_adder(int x) {
    return [x](int y) { return x + y; };
}

// 8. 高階関数
template<typename Container, typename Func>
void apply_to_all(Container& cont, Func f) {
    std::for_each(cont.begin(), cont.end(), f);
}

// 9. ジェネリックラムダを使う関数 (C++14)
auto create_generic_lambda() {
    return [](auto x, auto y) { return x + y; };
}

// 10. std::functionを使った関数
std::function<int(int, int)> get_operation(char op) {
    switch(op) {
        case '+': return [](int a, int b) { return a + b; };
        case '-': return [](int a, int b) { return a - b; };
        case '*': return [](int a, int b) { return a * b; };
        default:  return [](int a, int b) { return 0; };
    }
}

// 11. テンプレートエイリアス
template<typename T>
using Vec = std::vector<T>;

// 12. 完全転送を使った関数
template<typename T, typename... Args>
std::unique_ptr<T> make_unique_wrapper(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// 13. コンセプトを使った関数 (C++20)
#if __cplusplus >= 202002L
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

template<Numeric T>
T half(T value) {
    return value / 2;
}
#endif

// メイン関数でデモ
int main() {
    // 1. テンプレート関数の使用
    std::cout << "Max of 3, 7, 5: " << max_of_three(3, 7, 5) << std::endl;
    
    // 2. 可変長テンプレート
    std::cout << "Sum of 1, 2, 3, 4, 5: " << sum(1, 2, 3, 4, 5) << std::endl;
    
    // 3. テンプレート特殊化
    std::cout << "Type name: " << type_name<int>() << std::endl;
    
    // 4. constexpr
    constexpr int fact5 = factorial(5);
    std::cout << "5! = " << fact5 << std::endl;
    
    // 5. インライン関数
    std::cout << "Square of 7: " << square(7) << std::endl;
    
    // 6. SFINAE
    std::cout << "Double of 21: " << multiply_by_two(21) << std::endl;
    
    // 7. ラムダを返す関数
    auto add5 = make_adder(5);
    std::cout << "5 + 3 = " << add5(3) << std::endl;
    
    // 8. 高階関数とラムダ
    Vec<int> numbers = {1, 2, 3, 4, 5};
    apply_to_all(numbers, [](int& n) { n *= 2; });
    std::cout << "Doubled numbers: ";
    for(int n : numbers) std::cout << n << " ";
    std::cout << std::endl;
    
    // 9. ジェネリックラムダ
    auto generic_add = create_generic_lambda();
    std::cout << "Generic add: " << generic_add(10, 20) << std::endl;
    std::cout << "Generic add strings: " << generic_add(std::string("Hello "), std::string("World")) << std::endl;
    
    // 10. std::function
    auto add_op = get_operation('+');
    std::cout << "10 + 5 = " << add_op(10, 5) << std::endl;
    
    // ラムダ式のバリエーション
    // キャプチャなし
    auto simple = []() { return 42; };
    
    // 値キャプチャ
    int x = 10;
    auto by_value = [x]() { return x * 2; };
    
    // 参照キャプチャ
    auto by_ref = [&x]() { x += 5; return x; };
    
    // 全キャプチャ
    int y = 20;
    auto capture_all_by_value = [=]() { return x + y; };
    auto capture_all_by_ref = [&]() { return x + y; };
    
    // mutableラムダ
    auto mutable_lambda = [x]() mutable { return ++x; };
    
    // trailing return type
    auto complex_lambda = [](auto a, auto b) -> decltype(a + b) { return a + b; };
    
    std::cout << "Simple lambda: " << simple() << std::endl;
    std::cout << "By value lambda: " << by_value() << std::endl;
    std::cout << "By ref lambda: " << by_ref() << ", x is now: " << x << std::endl;
    std::cout << "Mutable lambda: " << mutable_lambda() << ", x is still: " << x << std::endl;
    
    return 0;
}