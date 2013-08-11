#ifndef SF1R_B5MMANAGER_B5MTHREADPOOL_H_
#define SF1R_B5MMANAGER_B5MTHREADPOOL_H_
#include <vector>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>

namespace sf1r {


    template <class E>
    class B5mThreadPool
    {
    public:
        typedef boost::function<void(E&)> Processor;
        //struct TProcessor
        //{
            //Processor p;
            //TProcessor(){}
            //TProcessor(const Processor& pp):p(pp)
            //{
            //}
            //TProcessor(const TProcessor& tp):p(tp.p)
            //{
            //}
        //};
        B5mThreadPool(std::size_t count, const Processor& p): quit_(false), p_(p)
        {
            for(std::size_t i=0;i<count;i++)
            {
                threads_.push_back(new boost::thread( boost::bind(&B5mThreadPool::run_, this)));
            }
        }

        ~B5mThreadPool()
        {
            wait();
            for(std::size_t i=0;i<threads_.size();i++)
            {
                delete threads_[i];
            }
            threads_.clear();
        }

        void schedule(E* e)
        {
            while(true)
            {
                if(queue_.empty()) 
                {
                    queue_.push(e);
                    break;
                }
                sched_yield();
            }
        }

        //void schedule(const Processor& p)
        //{
            //if(queue_.empty()) queue_.push(TProcessor(p));
            ////i_++;
            ////i_ = i_%count_;
            ////boost::mutex& mutex = mutexes_[i_];
            ////while(true)
            ////{
                ////i = i%mutexes_.size();
                ////{
                    ////boost::unique_lock<boost::mutex> lock(mutex, boost::try_to_lock);
                    ////if(!lock.is_lock())
                    ////{
                        ////if(i==mutexes_.size()-1)
                        ////{
                        ////}
                        ////++i;
                        ////continue;
                    ////}
                    ////p();
                    ////break;
                ////}
            ////}
        //}

        void wait()
        {
            if(!quit_)
            {
                quit_ = true;
                for(std::size_t i=0;i<threads_.size();i++)
                {
                    threads_[i]->join();
                }
            }
        }

    private:
        void run_()
        {
            E* e;
            while(!quit_ || !queue_.empty())
            {
                if(queue_.pop(e))
                {
                    p_(*e);
                    delete e;
                    //(tp.p)();
                }
                //else
                //{
                    //sched_yield();
                //}
            }
        }

    private:
        std::vector<boost::thread*> threads_;
        bool quit_;
        boost::lockfree::queue<E*> queue_;
        Processor p_;
        //std::vector<boost::mutex> mutexes_;
        //std::size_t i_;
    };

}

#endif

