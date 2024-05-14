
#include <cstdint>
#include <type_traits>
#include <utility>
#pragma onec


class TaskFunction
{
    class UndefinedClass;
    union Data
    {
        void* ptr;
        void (*func_ptr)();
        void (UndefinedClass::*class_func)();
    } data;
    static constexpr std::uint8_t max_size_ = sizeof(data);

    void (*const* vtable)() = nullptr;

    template<typename T>
    friend void FunConstruct(Data& dst, T&& src)
    {
        using U = typename std::decay<T>::type;
        if (sizeof(U) > TaskFunction::max_size_)
            *(U**)&dst = new U(std::forward<T>(src));

        else {
            new ((U*)&dst) U(std::forward<U>(src));
        }
    }
    template<typename T>
    friend inline void FunDestructor(Data& dst);
    template<typename T>
    friend inline void FunMove(Data& dst, Data& src);
    template<typename T>
    friend inline void FunInvoke(const Data& dst);

public:
    template<typename T>
    TaskFunction(T&& obj);
    ~TaskFunction()
    {
        if (vtable) ((void (*)(Data&))vtable[0])(data);
    }
    void operator()()
    {
        ((void (*)(const Data&))vtable[2])(data);
    }
};



template<typename T>
inline void FunDestructor(TaskFunction::Data& dst)
{
    using U = typename std::decay<T>::type;
    if (sizeof(U) > TaskFunction::max_size_)
        delete *(U**)&dst;
    else
        ((U*)&dst)->~U();
}

template<typename T>
inline void FunMove(TaskFunction::Data& dst, TaskFunction::Data& src)
{
    using U = typename std::decay<T>::type;
    if (sizeof(U) > TaskFunction::max_size_) {
        *(U**)&dst = *(U**)&src;
        *(U**)&src = nullptr;
    }
    else
        new ((U*)&dst) U(std::move(*(U*)&src));
}

template<typename T>
inline void FunInvoke(const TaskFunction::Data& dst)
{
    using U = typename std::decay<T>::type;
    if (sizeof(U) > TaskFunction::max_size_)
        (**(U**)&dst)();
    else
        (*(U*)&dst)();
}

template<typename T>
inline auto VtableInit() -> void (*const*)()
{
    using FunType = void();
    static void (*funs[3])() = {
        (void (*)())FunDestructor<T>,
        (void (*)())FunMove<T>,
        (void (*)())FunInvoke<T>,
    };
    return funs;
}

template<typename T>
inline TaskFunction::TaskFunction(T&& obj)
    : vtable(VtableInit<T>())
{
    FunConstruct(data, std::forward<T>(obj));
}

template<>
inline TaskFunction::TaskFunction(TaskFunction&& old)
    : vtable(old.vtable)
{
    old.vtable = nullptr;
    ((void (*)(Data& otd, Data& src))vtable[1])(this->data, old.data);
}