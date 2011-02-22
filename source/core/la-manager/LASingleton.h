#if !defined(_LASINGLETON_)
#define _LASINGLETON_

namespace sf1r
{

    template <class T>
    class LASingleton
    {
    public:
        static T* getInstance(void);

        static void destroy(void);

    private:
        static T* sInstance_;
    };

    template <class T>
    T* LASingleton<T>::sInstance_ = 0;

    template <class T>
    T* LASingleton<T>::getInstance(void)
    {
        static izenelib::util::ReadWriteLock sgllock_;
        ScopedWriteLock<ReadWriteLock> swl( sgllock_ );
        if (!sInstance_)
        {
            sInstance_ = new T;
        }
        return sInstance_;
    }

    template <class T>
    void LASingleton<T>::destroy(void)
    {
        static izenelib::util::ReadWriteLock sgllock_;
        ScopedWriteLock<ReadWriteLock> swl( sgllock_ );
        if (sInstance_)
        {
            delete sInstance_;
        }
        sInstance_ = 0;
    }

} // namespace sf1r

#endif // _LASINGLETON_
