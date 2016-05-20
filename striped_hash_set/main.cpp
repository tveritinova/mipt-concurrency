//
//  main.cpp
//  striped_hash_set
//
//  Created by Евгения Тверитинова on 20.05.16.
//  Copyright © 2016 Евгения Тверитинова. All rights reserved.
//

#include <iostream>
#include <thread>
#include "striped_hash_set.h"

int main()
{
    striped_hash_set<int> a(2);
    
    int number_of_threads = 20;
    std::vector<std::thread> threads;
    for (int i = 0; i < number_of_threads; ++i) {
        threads.emplace_back(std::thread([&a, i]() {
            std::cout << i << " entering\n";
            a.add(i);
            std::cout << i << " pass\n";
        }));
    }
    for (int i = 0; i < number_of_threads; ++i) {
        if (threads[i].joinable())
        {
            threads[i].join();
        }
    }
    std::cout << "success!";
    return 0;
}