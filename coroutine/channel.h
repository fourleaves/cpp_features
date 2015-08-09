#pragma once
#include <memory>
#include "block_object.h"
#include "co_mutex.h"
#include <queue>

template <typename T>
class Channel
{
    class ChannelImpl;
    mutable std::shared_ptr<ChannelImpl> impl_;

public:
    explicit Channel(std::size_t capacity = 0)
    {
        impl_.reset(new ChannelImpl(capacity));
    }

    template <typename U>
    Channel const& operator<<(U && t) const
    {
        (*impl_) << std::forward<U>(t);
        return *this;
    }

    template <typename U>
    Channel const& operator>>(U & t) const
    {
        (*impl_) >> t;
        return *this;
    }

    Channel const& operator>>(nullptr_t ignore) const
    {
        (*impl_) >> ignore;
        return *this;
    }

    template <typename U>
    bool TryPush(U && t) const
    {
        return impl_->TryPush(std::forward<U>(t));
    }

    template <typename U>
    bool TryPop(U & t) const
    {
        return impl_->TryPop(t);
    }

    bool TryPop(nullptr_t ignore) const
    {
        return impl_->TryPop(ignore);
    }

private:
    class ChannelImpl
    {
        BlockObject write_block_;
        BlockObject read_block_;
        std::queue<T> queue_;
        CoMutex queue_lock_;

    public:
        explicit ChannelImpl(std::size_t capacity)
            : write_block_(capacity)
        {}

        // write
        template <typename U>
        void operator<<(U && t)
        {
            write_block_.CoBlockWait();

            {
                std::unique_lock<CoMutex> lock(queue_lock_);
                queue_.push(std::forward<U>(t));
            }

            read_block_.Wakeup();
        }

        // read
        template <typename U>
        void operator>>(U & t)
        {
            write_block_.Wakeup();
            read_block_.CoBlockWait();

            {
                std::unique_lock<CoMutex> lock(queue_lock_);
                t = std::move(queue_.front());
                queue_.pop();
            }
        }

        // read and ignore
        void operator>>(nullptr_t ignore)
        {
            write_block_.Wakeup();
            read_block_.CoBlockWait();

            {
                std::unique_lock<CoMutex> lock(queue_lock_);
                queue_.pop();
            }
        }

        // try write
        template <typename U>
        bool TryPush(U && t)
        {
            if (!write_block_.TryBlockWait())
                return false;

            {
                std::unique_lock<CoMutex> lock(queue_lock_);
                queue_.push(std::forward<U>(t));
            }

            read_block_.Wakeup();
            return true;
        }

        // try read
        template <typename U>
        bool TryPop(U & t)
        {
            write_block_.Wakeup();
            while (!read_block_.TryBlockWait())
                if (write_block_.TryBlockWait())
                    return false;
                else
                    g_Scheduler.Yield();

            {
                std::unique_lock<CoMutex> lock(queue_lock_);
                t = std::move(queue_.front());
                queue_.pop();
            }
            return true;
        }

        // try read and ignore
        bool TryPop(nullptr_t ignore)
        {
            write_block_.Wakeup();
            while (!read_block_.TryBlockWait())
                if (write_block_.TryBlockWait())
                    return false;
                else
                    g_Scheduler.Yield();

            {
                std::unique_lock<CoMutex> lock(queue_lock_);
                queue_.pop();
            }
            return true;
        }
    };
};

