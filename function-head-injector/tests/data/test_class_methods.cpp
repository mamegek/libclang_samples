#include <iostream>
#include <string>

class Sample {
private:
    int value;
    std::string name;

public:
    // コンストラクタ
    Sample() : value(0), name("default") {}
    
    // 実装が一行だけのメソッド
    int getValue() const { return value; }
    void setValue(int v) { value = v; }
    
    std::string getName() const { return name; }
    
    void setName(const std::string& n) { name = n; }
    
    // 実装が存在しない（{}だけ）
    void emptyMethod() {}
    
    void printInfo() const {
        std::cout << "Name: " << name << std::endl; 
        std::cout << "Value: " << value << std::endl; 
    }
};

int main() {
    Sample obj;
    
    // メソッドの使用例
    obj.setValue(42);
    obj.setName("Test Object");
    obj.printInfo();
    
    // 空のメソッドも呼び出し可能
    obj.emptyMethod();
    
    return 0;
}
