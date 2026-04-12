#include <iostream>
#include <algorithm>

//  基本的なテンプレート関数
template<typename T>
T max_of_three(T a, T b, T c) {
    return std::max({a, b, c});
}

// constexpr関数
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// インライン関数
inline int square(int x) {
    return x * x;
}


// メイン関数でデモ
int main() {
    // テンプレート関数の使用
    std::cout << "Max of 3, 7, 5: " << max_of_three(3, 7, 5) << std::endl;
    
    // constexpr
    constexpr int fact5 = factorial(5);
    std::cout << "5! = " << fact5 << std::endl;
    
    // インライン関数
    std::cout << "Square of 7: " << square(7) << std::endl;
    
    
    return 0;
}