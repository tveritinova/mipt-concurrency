#include <mutex>
#include <vector>
#include <forward_list>
#include <deque>
#include <mutex>

template <typename T, class H = std::hash<T>>
class striped_hash_set
{
public:
    striped_hash_set(int _num_stripes = 1, int _growth_factor = 2, double _load_factor = 100.)
    : num_stripes(_num_stripes), growth_factor(_growth_factor), load_factor(_load_factor), num_elements(0)
    {
        locks.resize(_num_stripes);
        table.resize(_num_stripes * _growth_factor);
    }
    
    void add(const T& e)
    {
        // сначала просто добавляем
        size_t hash_value = H()(e);
        int stripe_ind = get_stripe_index(hash_value);
        std::unique_lock<std::mutex> lock(locks[stripe_ind]);
        int bucket_ind = get_bucket_index(hash_value);
        table[bucket_ind].push_front(e);
        num_elements.fetch_add(1);
        
        if (((float) num_elements.load()) / ((float) table.size()) > load_factor)
        {
            // расширение
            int last_size = (int) table.size();
            lock.unlock();
            // захватываем первый мьютекс и сравниваем размеры
            
            std::vector<std::unique_lock<std::mutex>> v;
            v.emplace_back(locks[0]);
            
            if (table.size() != (size_t) last_size)
                return;
            
            for (size_t i = 1; i < locks.size(); i++)
            {
                v.emplace_back(locks[i]);
            }
            
            int new_size = last_size * growth_factor;
            std::vector<std::forward_list<T>> new_table(new_size);
            
            for (int i = 0; i < last_size; i++)
            {
                for (auto it = table[i].begin(); it != table[i].end(); it++)
                {
                    size_t hash_value = H()(*it);
                    new_table[hash_value % new_size].push_front(*it);
                }
            }
            
            table = new_table;
        }
    }
    
    void remove(const T& e)
    {
        size_t hash_value = H()(e);
        int stripe_ind = get_stripe_index(hash_value);
        std::unique_lock<std::mutex> lock(locks[stripe_ind]);
        int bucket_ind = get_bucket_index(hash_value);
        table[bucket_ind].remove(e);
        num_elements.fetch_sub(1);
    }
    
    bool contains(const T& e)
    {
        size_t hash_value = H()(e);
        int stripe_ind = get_stripe_index(hash_value);
        std::unique_lock<std::mutex> lock(locks[stripe_ind]);
        int bucket_ind = get_bucket_index(hash_value);
        for (T elem : table[bucket_ind])
        {
            if (elem == e)
                return true;
        }
        
        return false;
    }
    
private:
    int num_stripes;
    int growth_factor;
    double load_factor;
    std::atomic<int> num_elements;
    std::deque<std::mutex> locks;
    std::vector<std::forward_list<T>> table;
    
    int get_bucket_index (size_t hash_value)
    {
        return hash_value % ((int) table.size());
    }
    
    int get_stripe_index (size_t hash_value)
    {
        return hash_value % num_stripes;
    }
};

