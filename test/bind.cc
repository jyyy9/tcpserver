#include <functional>
#include <iostream>

int add(int num1, int num2)
{
    return num1 + num2;
}

int main()
{
    auto functor = std::bind(add, 11, 22);
    auto functor2 = std::bind(add, 11, std::placeholders::_1);
    std::cout << functor() << std::endl;
    std::cout << functor2(88) << std::endl;
    // std::cout << add(10, 20) << std::endl;
    return 0;
}