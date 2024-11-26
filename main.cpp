#include <iostream>
#include <map> // Подключаем библиотеку для использования std::map
#include <vector>
#include <memory> // Подключаем библиотеку для работы с умными указателями
#include <cassert>
#include <limits> // Подключаем библиотеку для работы с границами типов

// Шаблонный класс MyAllocator для пользовательского аллокатора памяти
template <typename T, size_t BlockSize = 10>
class MyAllocator
{
public:
    using value_type = T; // Определяем тип значения
    using pointer = T *;  // Определяем указатель на тип T
    using const_pointer = const T *;
    using reference = T &;                  // Определяем ссылку на тип T
    using const_reference = const T &;      // Определяем константную ссылку на тип T
    using size_type = std::size_t;          // Определяем тип для размера
    using difference_type = std::ptrdiff_t; // Определяем тип для разности указателей

    // Структура rebind для поддержки аллокации
    template <typename U>
    struct rebind
    {
        using other = MyAllocator<U, BlockSize>; // Переопределяем аллокатор для другого типа U
    };

    MyAllocator() noexcept = default;

    template <typename U>
    MyAllocator(const MyAllocator<U, BlockSize> &) noexcept {} // Конструктор копирования для другого типа

    pointer allocate(size_type n)
    { // Метод выделения памяти
        if (n > BlockSize)
        {                                                               // Если запрашиваемое количество больше размера блока
            return static_cast<pointer>(::operator new(n * sizeof(T))); // Используем стандартный оператор new
        }
        if (free_blocks.empty())
        {             // Если нет свободных блоков
            expand(); // Увеличиваем количество блоков
        }
        pointer result = free_blocks.back(); // Берем последний свободный блок
        free_blocks.pop_back();              // Удаляем его из списка свободных блоков
        return result;                       // Возвращаем указатель на выделенный блок
    }

    void deallocate(pointer p, size_type n) noexcept
    { // Метод освобождения памяти
        if (n > BlockSize)
        {                         // Если освобождаем больше блока
            ::operator delete(p); // Используем стандартный оператор delete
        }
        else
        {
            free_blocks.push_back(p); // Добавляем указатель в список свободных блоков
        }
    }

    template <typename U, typename... Args>
    void construct(U *p, Args &&...args)
    {                                           // Метод конструирования объекта в выделенной памяти
        new (p) U(std::forward<Args>(args)...); // Вызываем конструктор объекта U на адресе p с аргументами args
    }

    template <typename U>
    void destroy(U *p)
    {            // Метод разрушения объекта в выделенной памяти
        p->~U(); // Вызываем деструктор объекта U по адресу p
    }

private:
    void expand()
    { // Метод увеличения количества свободных блоков
        for (size_t i = 0; i < BlockSize; ++i)
        {                                                                           // Для каждого блока в размере BlockSize
            free_blocks.push_back(static_cast<pointer>(::operator new(sizeof(T)))); // Выделяем новый блок и добавляем его в список свободных блоков
        }
    }

    std::vector<pointer> free_blocks; // Вектор для хранения указателей на свободные блоки памяти
};

// Оператор сравнения для аллокаторов (равенство)
template <typename T, typename U, size_t BlockSize>
bool operator==(const MyAllocator<T, BlockSize> &, const MyAllocator<U, BlockSize> &) noexcept
{
    return true; // Всегда возвращает true, так как аллокаторы эквивалентны по определению
}

// Оператор сравнения для аллокаторов (неравенство)
template <typename T, typename U, size_t BlockSize>
bool operator!=(const MyAllocator<T, BlockSize> &, const MyAllocator<U, BlockSize> &) noexcept
{
    return false; // Всегда возвращает false, так как аллокаторы эквивалентны по определению
}

// Шаблонный класс MyContainer для хранения элементов типа T с использованием заданного аллокатора
template <typename T, typename Allocator = std::allocator<T>>
class MyContainer
{
public:
    using value_type = T;                                               // Определяем тип значения контейнера
    using allocator_type = Allocator;                                   // Определяем тип аллокатора контейнера
    using pointer = typename std::allocator_traits<Allocator>::pointer; // Определяем указатель на элементы контейнера

    MyContainer(const allocator_type &alloc = allocator_type()) : alloc_(alloc), size_(0) {} // Конструктор с параметром по умолчанию

    void push_back(const T &value)
    {                                                                        // Метод добавления элемента в конец контейнера
        pointer ptr = std::allocator_traits<Allocator>::allocate(alloc_, 1); // Выделяем память под один элемент с помощью аллокатора
        std::allocator_traits<Allocator>::construct(alloc_, ptr, value);     // Конструируем элемент в выделенной памяти с заданным значением value
        elements_.push_back(ptr);                                            // Добавляем указатель на элемент в вектор элементов контейнера
        ++size_;                                                             // Увеличиваем размер контейнера на 1
    }

    void print() const
    { // Метод вывода всех элементов контейнера на экран
        for (const auto &ptr : elements_)
        {                             // Для каждого указателя в векторе элементов
            std::cout << *ptr << " "; // Выводим значение элемента по этому указателю
        }
        std::cout << std::endl; // Переход на новую строку после вывода всех элементов
    }

    size_t size() const { return size_; }     // Метод получения текущего размера контейнера
    bool empty() const { return size_ == 0; } // Метод проверки пустоты контейнера

    ~MyContainer()
    { // Деструктор контейнера
        for (auto ptr : elements_)
        {                                                                 // Для каждого указателя в векторе элементов
            std::allocator_traits<Allocator>::destroy(alloc_, ptr);       // Вызываем деструктор элемента
            std::allocator_traits<Allocator>::deallocate(alloc_, ptr, 1); // Освобождаем память под элемент
        }
    }

private:
    allocator_type alloc_;          // Аллокатор контейнера
    std::vector<pointer> elements_; // Вектор указателей на элементы контейнера
    size_t size_;                   // Текущий размер контейнера
};

// Рекурсивная функция вычисления факториала числа n
int factorial(int n)
{
    if (n <= 1)
        return 1;                // Базовый случай: факториал 0 и 1 равен 1
    return n * factorial(n - 1); // Рекурсивный вызов: n! = n * (n-1)!
}

int main()
{
    // 1. Создани е экземпwля ра st d::map<int, int>;
    std::map<int, int> map1;

    // 2. Заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа;
    for (int i = 0; i < 10; ++i)
    {
        map1[i] = factorial(i); // Заполняем map1 ключами от 0 до 9 и значениями - факториал этих ключей.
    }

    // 3. Создание экземпляра std::map<int, int> с новым аллокатором, ограниченным 10 элементами;
    std::map<int, int, std::less<int>, MyAllocator<std::pair<const int, int>, 10>> map2;

    // 4. Заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа;
    for (int i = 0; i < 10; ++i)
    {
        map2[i] = factorial(i); // Заполняем map2 ключами от 0 до 9 и значениями - факториал этих ключей.
    }

    // 5. Вывод на экран всех значений (ключ и значение разделены пробелом), хранящихся в контейнере;
    std::cout << "map1:" << std::endl;
    for (const auto &[key, value] : map1)
    {
        std::cout << "Key: " << key << ", Value: " << value << std::endl; // Выводим пары ключ-значение из map1.
    }

    std::cout << "map2:" << std::endl;
    for (const auto &[key, value] : map2)
    {
        std::cout << "Key: " << key << ", Value: " << value << std::endl; // Выводим пары ключ-значение из map2.
    }

    // 6. Создание экземпляра своего контейнера для хранения значений типа int;
    MyContainer<int> container1;

    // 7. Заполнение 10 элементами от 0 до 9;
    for (int i = 0; i < 10; ++i)
    {
        container1.push_back(i); // Добавляем значения от 0 до 9 в container1.
    }

    // 8. Создание экземпляра своего контейнера для хранения значений типа int с новым аллокатором, ограниченным 10 элементами;
    MyContainer<int, MyAllocator<int, 10>> container2;

    // 9. Заполнение 10 элементами от 0 до 9;
    for (int i = 0; i < 10; ++i)
    {
        container2.push_back(i); // Добавляем значения от 0 до 9 в container2.
    }

    // 10. Вывод на экран всех значений, хранящихся в контейнере;
    std::cout << "container1: ";
    container1.print(); // Выводим все элементы из container1.

    std::cout << "container2: ";
    container2.print(); // Выводим все элементы из container2.

    return 0; // Завершаем программу.
}