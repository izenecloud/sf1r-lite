#include <iostream>
#include <string>

#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <log-manager/CassandraConnection.h>
#include <log-manager/PriceHistory.h>

#include <libcassandra/util_functions.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

using izenelib::util::UString;
using namespace sf1r;
using namespace boost;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace po = boost::program_options;

struct ToolOptions
{
    ToolOptions()
        : toolDescription_("Cassandra Tools Options")
    {
        po::options_description scdfile;
        scdfile.add_options()
        ("scd-file,F", po::value<std::string>(), "Path to the scd files contained doc ids");
        po::options_description connection;
        connection.add_options()
        ("connection,C", po::value<std::string>(), "Cassandra connection description");
        po::options_description keyspace;
        keyspace.add_options()
        ("keyspace,K", po::value<std::string>(), "Cassandra connection keyspace name");
        po::options_description start;
        start.add_options()
        ("start,B", po::value<std::string>(), "Start date");
        po::options_description finish;
        finish.add_options()
        ("finish,E", po::value<std::string>(), "Finish date");

        toolDescription_.add(scdfile).add(connection).add(keyspace).add(start).add(finish);
    }

    bool SetArgs(const std::vector<std::string>& args)
    {
        std::string cassandraToolSample = "Example: ./CassandraTool -C connection -F SCD";
        try
        {
            po::store(po::command_line_parser(args).options(toolDescription_).positional(additional_).run(), variableMap_);
            po::notify(variableMap_);

            if (variableMap_.empty())
            {
                std::cout << "Usage:  CassandraTool " << std::endl;
                std::cout << cassandraToolSample<<std::endl;
                std::cout << toolDescription_;
                return false;
            }
            if (variableMap_.count("scd-file"))
            {
                scd_file_ = variableMap_["scd-file"].as<std::string>();
            }
            else
            {
                std::cerr << "Warning: Missing Mandatory Parameter" << std::endl;
                std::cout << "Usage:  CassandraTool " << std::endl;
                std::cout << cassandraToolSample<<std::endl;
                std::cout << toolDescription_;
                return false;
            }
            if (variableMap_.count("connection"))
            {
                connection_str_ = variableMap_["connection"].as<std::string>();
            }
            else
            {
                std::cerr << "Warning: Missing Mandatory Parameter" << std::endl;
                std::cout << "Usage:  CassandraTool " << std::endl;
                std::cout << cassandraToolSample<<std::endl;
                std::cout << toolDescription_;
                return false;
            }

            if (variableMap_.count("keyspace"))
                keyspace_name_ = variableMap_["keyspace"].as<std::string>();
            if (variableMap_.count("start"))
                start_date_ = variableMap_["start"].as<std::string>();
            if (variableMap_.count("finish"))
                finish_date_ = variableMap_["finish"].as<std::string>();
        }
        catch (std::exception &e)
        {
            std::cerr << "Warning: " << e.what() << std::endl;
            std::cout << "Usage:  CassandraTool " << std::endl;
            std::cout << cassandraToolSample << std::endl;
            std::cout << toolDescription_;
            return false;
        }
        return true;
    }

    /// @brief  Stores the option values
    boost::program_options::variables_map variableMap_;
    boost::program_options::options_description toolDescription_;
    boost::program_options::positional_options_description additional_;

    std::string scd_file_;
    std::string connection_str_;
    std::string keyspace_name_;
    std::string start_date_;
    std::string finish_date_;
};


int main(int argc, char *argv[])
{
    ToolOptions po;
    std::string filePath, cassandraConnection;
    std::vector<std::string> args(argv + 1, argv + argc);
    if (po.SetArgs(args))
    {
        filePath = po.scd_file_;
        cassandraConnection = po.connection_str_;
    }
    else return -1;
    ScdParser parser(UString::UTF_8);
    if (!parser.load(filePath))
    {
        std::cerr << "Invalid SCD file "<<filePath << std::endl;
        return -1;
    }

    if(!CassandraConnection::instance().init(cassandraConnection))
    {
        std::cerr << "Can not initialize Cassandra connection :"<<cassandraConnection<<std::endl;
        return -1;
    }


    PriceHistory price_history(po.keyspace_name_.empty() ? "B5MO" : po.keyspace_name_);
    price_history.createColumnFamily();

    std::string start = po.start_date_.empty() ? "" : serializeLong(Utilities::createTimeStamp(po.start_date_));
    std::string finish = po.finish_date_.empty() ? "" : serializeLong(Utilities::createTimeStamp(po.finish_date_));

    const size_t buffer_size = 2000;
    std::vector<uint128_t> docid_list;
    docid_list.reserve(buffer_size);

    const std::string DOCID("DOCID");
    const std::string DATE("DATE");
    for (ScdParser::iterator doc_iter = parser.begin();
            doc_iter != parser.end(); ++doc_iter)
    {
        if (*doc_iter == NULL)
        {
            std::cerr << "SCD File not valid." << std::endl;
            return -1;
        }

        SCDDocPtr doc = (*doc_iter);
        for (SCDDoc::iterator p = doc->begin(); p != doc->end(); p++)
        {
            const std::string& fieldStr = p->first;
            if (boost::iequals(fieldStr, DOCID))
            {
                docid_list.push_back(Utilities::md5ToUint128(p->second));
                if (docid_list.size() == buffer_size)
                {
                    price_history.deleteMultiSlice(docid_list, start, finish);
                    docid_list.clear();
                }
            }
//          else if (boost::iequals(fieldStr, DATE))
//          {
//          }
        }
    }
    if (!docid_list.empty())
    {
        price_history.deleteMultiSlice(docid_list, start, finish);
        docid_list.clear();
    }

    return 0;
}
