#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <future>

template <typename T>
std::vector<T> generate(int n, T start, T stop) {
    std::vector<T> v(n);
    std::random_device rd;
    if constexpr (std::is_same_v<T, int>)
    {
        std::uniform_int_distribution<T> dis(start, stop);
        for (auto& item: v) item = dis(rd);
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        std::uniform_real_distribution<T> dis(start, stop);
        for (auto& item: v) item = dis(rd);
    }
    return v;
}

template <typename Iterator, typename T = typename std::iterator_traits<Iterator>::value_type>
T summarize(Iterator start, Iterator stop, T initial) {
    T total = initial;
    for (auto it = start; it < stop; ++it) total += *it;
    return total;
}

template <typename Iterator, typename T = typename std::iterator_traits<Iterator>::value_type>
T summarize_par(Iterator start, Iterator stop, T initial) {
    T total = initial;
    auto nh = std::thread::hardware_concurrency();
    auto span = std::distance(start, stop) / nh;
    auto last_span = std::distance(start, stop) % nh;
    // vectors
    std::vector<std::jthread> vh(nh);
    std::vector<T> vr(nh);
    // fork
    for (int i = 0; i < nh; ++i) {
        vh[i] = std::jthread([&vr, i, start, span] {
            vr[i] = summarize(std::next(start, i*span), std::next(start, (i+1)*span), 0);
        });
    }
    // join
    for (int i = 0; i < nh; ++i) vh[i].join();
    // add last span
    if (last_span > 0)
        total += summarize(std::next(start, nh*span), stop, 0);
    total += summarize(vr.begin(), vr.end(), 0);
    return total;
}

void task_1(int a, int b, int& result) {
    result = a + b;
}

int task_async(int a, int b) {
    return a + b;
}

void ejemplo_de_async() {
    std::future<int> fut = std::async(task_async, 10, 20);
    std::cout << fut.get() << std::endl;
}

template <typename Iterator, typename T = typename std::iterator_traits<Iterator>::value_type>
T summarize_async(Iterator start, Iterator stop, T initial) {
    T total = initial;
    auto nh = std::thread::hardware_concurrency();
    auto span = std::distance(start, stop) / nh;
    auto last_span = std::distance(start, stop) % nh;
    // vectors
    std::vector<std::future<T>> vf(nh);
    // fork
    for (int i = 0; i < nh; ++i) {
        vf[i] = std::async([i, start, span]
                { return std::accumulate(std::next(start, i*span), std::next(start, (i+1)*span), 0);
        });
    }
    // add last span
    if (last_span > 0)
        total += std::accumulate(std::next(start, nh*span), stop, 0);
    total += std::accumulate(vf.begin(), vf.end(), 0, [](auto a, auto& b) { return a + b.get(); });
    return total;
}

void task_producer(std::string name, std::promise<std::string> prm, std::future<std::string> fut) {
    prm.set_value(name+": Hola");
    std::this_thread::sleep_for(std::chrono::seconds(4));
    std::cout << fut.get() << std::endl;
}

void task_consumer(std::string name, std::future<std::string> fut, std::promise<std::string> prm) {
    std::cout << fut.get() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    prm.set_value(name+": Hola");
}

void ejemplo() {
    auto prm1 = std::promise<std::string>();
    auto fut1 = prm1.get_future();

    auto prm2 = std::promise<std::string>();
    auto fut2 = prm2.get_future();

    std::jthread jt1(task_producer, "PEPE", std::move(prm1), std::move(fut2));
    std::jthread jt2(task_consumer, "LUCHO", std::move(fut1), std::move(prm2));

    jt1.join();
    jt2.join();
}

int main() {
    auto v1 = generate(400000, 1, 20);
    std::cout << summarize(v1.begin(), v1.end(), 0) << std::endl;
    std::cout << summarize_par(v1.begin(), v1.end(), 0) << std::endl;
    std::cout << summarize_async(v1.begin(), v1.end(), 0) << std::endl;
    ejemplo();

//    ejemplo_de_async();
    return 0;
}
