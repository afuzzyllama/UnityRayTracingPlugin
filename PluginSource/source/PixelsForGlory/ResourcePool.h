#pragma once

#include <iterator>  
#include <vector>

namespace PixelsForGlory
{
    template<typename T>
    struct resourcePool
    {
    public:

        uint32_t add(const T& object)
        {
            uint32_t index;
            if (available_index_.size() > 0)
            {
                index = available_index_.back();
                available_index_.pop_back();
                in_use_index_.push_back(index);
                pool_[index] = object;
            }
            else
            {
                index = static_cast<uint32_t>(pool_.size());
                in_use_index_.push_back(index);
                pool_.push_back(object);
            }

            return index;
        }

        uint32_t add(T&& object)
        {
            uint32_t index;
            if (available_index_.size() > 0)
            {
                index = available_index_.back();
                available_index_.pop_back();
                in_use_index_.push_back(index);
                pool_[index] = std::move(object);
            }
            else
            {
                index = static_cast<uint32_t>(pool_.size());
                in_use_index_.push_back(index);
                pool_.push_back(std::move(object));
            }

            return index;
        }

        uint32_t get_next_index()
        {
            uint32_t index;
            if (available_index_.size() > 0)
            {
                index = available_index_.back();
                available_index_.pop_back();
                in_use_index_.push_back(index);
            }
            else
            {
                index = static_cast<uint32_t>(pool_.size());
                in_use_index_.push_back(index);
                pool_.resize(static_cast<size_t>(index) + 1);
            }

            return index;
        }

        void remove(uint32_t index)
        {
            pool_[index] = T();

            // Remove the index from the in_use vector
            in_use_index_.erase(std::remove(in_use_index_.begin(), in_use_index_.end(), index), in_use_index_.end());

            // Add it to the available vector
            available_index_.push_back(index);
        }

        T& operator[](int index)
        {
            return pool_[index];
        }

        typename std::vector<T>::iterator pool_begin()
        {
            return pool_.begin();
        }

        typename std::vector<T>::iterator pool_end()
        {
            return pool_.end();
        }

        typename std::vector<uint32_t>::iterator in_use_begin()
        {
            return in_use_index_.begin();
        }

        typename std::vector<uint32_t>::iterator in_use_end()
        {
            return in_use_index_.end();
        }

        size_t pool_size()
        {
            return pool_.size();
        }

        size_t in_use_size()
        {
            return in_use_index_.size();
        }

        T* data()
        {
            return pool_.data();
        }

        void clear()
        {
            pool_.clear();
        }

    private:
        std::vector<T> pool_;
        std::vector<uint32_t> in_use_index_;
        std::vector<uint32_t> available_index_;
    };
}

