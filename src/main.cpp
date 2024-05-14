#include "ThreadPool.hpp"
#include <functional>
#include <iostream>

int freeFunction(int a)
{
    return a * 2;
}

struct Test
{
    int i = 10;
    int operator()(int a)
    {
        return a + i;
    }
    int memberFunction(int a) const
    {
        return a + i;
    }
};

int main()
{
    auto& thr = ThreadPool::Init();

    // Free function
    auto future1 = thr.submit(freeFunction, 10);
    std::cout << "Free function result: " << future1.get() << std::endl;

    // Lambda function
    auto future2 = thr.submit([](int a) { return a * 3; }, 10);
    std::cout << "Lambda function result: " << future2.get() << std::endl;

    // Function object
    Test test;
    auto future3 = thr.submit(test, 10);
    std::cout << "Function object result: " << future3.get() << std::endl;

    // Member function
    auto future4 = thr.submit(&Test::memberFunction, test, 10);
    std::cout << "Member function result: " << future4.get() << std::endl;

    std::function<int(int)> std_function = [](int a) {
        return a + 10;
    };
    auto future5 = thr.submit(std::move(std_function), 10);
    std::cout << "std::function result: " << future5.get() << std::endl;

    return 0;
}
