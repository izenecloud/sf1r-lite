///
/// @file MiningException.hpp
/// @brief several exceptions used by MIA.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-02-05
/// @date Updated 2010-02-05
///  --- Log


#ifndef MININGEXCEPTION_H_
#define MININGEXCEPTION_H_


namespace sf1r
{
    
    
class MiningException : public std::exception
{
public:
    MiningException(const std::string& msg, const std::string& parameter, const std::string& thrower="#")
    : msg_(msg), parameter_(parameter), thrower_(thrower)
    {
    }
    
    ~MiningException() throw ()
    {
    }
    
    virtual const char* what() const throw()
    {
        return toString();
    }

    
    std::string getThrower()
    {
        return thrower_;
    }

    std::string getParameter()
    {
        return parameter_;
    }
    
    const char* toString() const
    {
        return (msg_+" : "+parameter_+" ("+thrower_+")").c_str();
    }
    
private:
    std::string msg_;
    std::string parameter_;
    std::string thrower_;
};

class NotEnoughMemoryException : public std::bad_alloc
{
public:
    NotEnoughMemoryException(const std::string& thrower="#")
    :std::bad_alloc(), thrower_(thrower)
    {
    }
    
    ~NotEnoughMemoryException() throw ()
    {
    }
    
    virtual const char* what() const throw()
    {
        return "Not enough memory";
    }

    
    std::string getThrower()
    {
        return thrower_;
    }
    
    const char* toString()
    {
        return ("Not enough memory ("+thrower_+")").c_str();
    }
    
private:

    std::string thrower_;

};


class MiningConfigException : public MiningException
{
public:
    MiningConfigException(const std::string& parameter, const std::string& thrower="#")
    : MiningException("Mining Config Error", parameter, thrower)
    {
    }


};

class FileOperationException : public MiningException
{
public:
    FileOperationException(const std::string& parameter, const std::string& thrower="#")
    : MiningException("File operation error", parameter, thrower)
    {
    }


};

class ResourceNotFoundException : public MiningException
{
public:
    ResourceNotFoundException(const std::string& parameter, const std::string& thrower="#")
    : MiningException("Resource not found", parameter,thrower)
    {
    }


};

class FileCorruptedException : public MiningException
{
public:
    FileCorruptedException(const std::string& parameter, const std::string& thrower="#")
    : MiningException("File corrupted", parameter,thrower)
    {
    }


};


    
}

#endif
