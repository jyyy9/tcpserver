#include <functional>
#include <iostream>

template<T>
class enable_shared_from_this {
    public:
        enable_shared_from_this(std::shared_ptr<T> obj);

        std::shared_ptr<T> shared_from_this() {
            return _ptr.lock();
        }
    private:
        std::weak_ptr<T> _ptr;
};

std::bind(&TcpConnection::sendInLoop, this, buf)

class temp {
    public:
        operator()(params1, params2) {

        }
    private:
        functor;
}

int add(int num1, int num2)
{
    return num1 + num2;
}

int main()
{
    auto f = &add;
    f(11, 22);
    auto functor = std::bind(add, 11, 22);
    auto functor2 = std::bind(add, 11, std::placeholders::_1);
    std::cout << functor() << std::endl;
    std::cout << functor2(88) << std::endl;
    // std::cout << add(10, 20) << std::endl;
    return 0;
}