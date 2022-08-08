//
// Created by airwalk on 03-Jul-22.
//
#pragma once
#include "net_common.h"
#include <condition_variable>

namespace olc
{
    namespace net
    {

        template<typename T>
        class tsqueue {
        public:
            tsqueue() = default;
            tsqueue(const tsqueue<T>&) = delete;
            virtual ~tsqueue() {clear();}

            const T& front()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.front();
            }

            const T& back()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.back();
            }

            void push_back(const T& msg) {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_back(std::move(msg));

                std::unique_lock<std::mutex> ul(muxWait);
                cv.notify_one();
            }


            void push_front(const T& msg) {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_front(std::move(msg));

                std::unique_lock<std::mutex> ul(muxWait);
                cv.notify_one();
            }

            bool empty() {
                std::scoped_lock lock(muxQueue);
                 return deqQueue.empty();
            }

            void clear() {
                std::scoped_lock lock(muxQueue);
                deqQueue.clear();
            }

            size_t count() {
                std::scoped_lock lock(muxQueue);
                return deqQueue.size();
            }

            T pop_front() {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.front());
                deqQueue.pop_front();
                return t;
            }
            T pop_back() {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.back());
                deqQueue.pop_back();
                return t;
            }

            void wait(){
                std::unique_lock<std::mutex> ul(muxWait);
                while (empty()){
                    cv.wait(ul);
                }
//                cv.wait(ul, [&](){return empty();});
            }



        protected:
                    std::mutex muxQueue;
                    std::deque<T> deqQueue;
                    std::mutex muxWait;
                    std::condition_variable cv;
                };
    }
}