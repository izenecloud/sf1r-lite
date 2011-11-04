#include "NoSqlConnection.h"
#include "NoSqlConnectionBase.h"
#include "CassandraConnection.h"

#include <iostream>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace sf1r {

NoSqlConnection::NoSqlConnection()
:impl_(NULL)
{
}

NoSqlConnection::~NoSqlConnection()
{
    if(impl_)
    {
        delete impl_;
    }
}

void NoSqlConnection::close()
{
    if(impl_)
    {
        impl_->close();
    }
}

bool NoSqlConnection::init(const std::string& str)
{
    std::string cassandra_prefix = "cassandra://";
    if (boost::algorithm::starts_with(str, cassandra_prefix))
    {
        impl_ = new CassandraConnection();
        std::string path = str.substr(cassandra_prefix.length());
        return impl_->init(path);
    }
    else
    {
        std::cerr << "[DbConnection::init] " << str << " unrecognized" << std::endl;
        return false;
    }

}

}
