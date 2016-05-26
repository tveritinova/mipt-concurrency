//
//  lock_free_queue.h
//  lock_free_queue
//
//  Created by Евгения Тверитинова on 25.05.16.
//  Copyright © 2016 Евгения Тверитинова. All rights reserved.
//

#ifndef lock_free_queue_h
#define lock_free_queue_h

#include <memory>
#include <atomic>


template <typename T>
class lock_free_queue
{
public:
    void enqueue(T item);
    bool dequeue(T& item);
    
    lock_free_queue(): head(new node), tail(head.load()), free(head.load()) {}
    ~lock_free_queue();
    
private:
    
    struct node
    {
        std::shared_ptr<T> data;
        std::atomic<node*> next;
        node(): next(nullptr) {}
        node (T& item): data(new T(item)), next(nullptr) {}
    };
    
    std::atomic<node*> head;
    std::atomic<node*> tail;
    std::atomic<node*> free;
    std::atomic<std::size_t> thrd_cnt;
};

template<typename T>
void lock_free_queue<T>::enqueue (T item)
{
    node* new_node = new node(item);
    node* curr_tail;
    thrd_cnt.fetch_add(1);
    while (true)
    {
        node* curr_tail = tail.load();
        node* curr_tail_next = curr_tail->next;
        
        if (!curr_tail_next)
        {
            if (tail.load()->next.compare_exchange_weak(curr_tail_next, new_node)) // 1
                break;
        }
        else
        {
            tail.compare_exchange_weak(curr_tail, curr_tail_next); // helping
        }
    }
    tail.compare_exchange_weak(curr_tail, new_node); // 2
    thrd_cnt.fetch_sub(1);
}

template<typename T>
bool lock_free_queue<T>::dequeue (T& item)
{
    thrd_cnt.fetch_add(1);
    node* curr_head_next;
    while (true)
    {
        node* curr_head = head.load();
        node* curr_tail = tail.load();
        curr_head_next = curr_head->next;
        if (curr_head == curr_tail)
        {
            if (!curr_head_next)
            {
                return false;
            }
            else
                tail.compare_exchange_weak(curr_head, curr_head_next); // helping
        }
        else
        {
            if (head.compare_exchange_weak(curr_head, curr_head_next))
            {
                item = *curr_head_next->data;
                return true;
            }
        }
    }
    
    if (thrd_cnt.load() == 1)
    {
        node* curr_free = free.exchange(curr_head_next);
        while (curr_free != free.load())
        {
            node* tmp = curr_free->next.load();
            delete curr_free;
            curr_free = tmp;
        }
    }
    
    thrd_cnt.fetch_sub(1);
    return true;
}

template<typename T>
lock_free_queue<T>::~lock_free_queue()
{
    for (node* it = free.load(); it != nullptr;){
        node* next = it->next.load();
        delete it;
        it = next;
    }
}

#endif /* lock_free_queue_h */
