#pragma once

#include <iterator>  
#include <vector>

namespace PixelsForGlory
{
    template<typename K, typename T>
    struct resourcePool
    {
    public:

        uint32_t add(const K& key, const T& object)
        {
            uint32_t index;
            if (available_index_.size() > 0)
            {
                index = available_index_.back();
                available_index_.pop_back();

                pool_[index] = object;
            }
            else
            {
                index = static_cast<uint32_t>(pool_.size());
                pool_.push_back(object);
            }

            map_.insert(std::make_pair(key, index));

            return index;
        }

        uint32_t add(const K& key, T&& object)
        {
            uint32_t index;
            if (available_index_.size() > 0)
            {
                index = available_index_.back();
                available_index_.pop_back();

                pool_[index] = std::move(object);
            }
            else
            {
                index = static_cast<uint32_t>(pool_.size());
                pool_.push_back(std::move(object));
            }

            map_.insert(std::make_pair(key, index));

            return index;
        }

        typename std::map<K, int>::iterator find(const K& key)
        {
            return map_.find(key);
        }

        void remove(const K& key)
        {
            auto index = map_[key];
            map_.erase(key);
            pool_[index] = T();

            // Add it to the available vector
            available_index_.push_back(index);
        }

        T& operator[](K key)
        {
            return pool_[map_[key]];
        }

        typename std::map<K, int>::iterator in_use_begin()
        {
            return map_.begin();
        }

        typename std::map<K, int>::iterator in_use_end()
        {
            return map_.end();
        }

        size_t in_use_size()
        {
            return map_.size();
        }

        typename std::vector<T>::iterator pool_begin()
        {
            return pool_.begin();
        }

        typename std::vector<T>::iterator pool_end()
        {
            return pool_.end();
        }

        size_t pool_size()
        {
            return pool_.size();
        }

        T* data()
        {
            return pool_.data();
        }

        void clear()
        {
            map_.clear();
            pool_.clear();
        }

    private:
        std::map<K, int> map_;
        std::vector<T> pool_;
        std::vector<uint32_t> available_index_;
    };
}

