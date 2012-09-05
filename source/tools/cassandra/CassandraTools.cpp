#include <iostream>
#include <string>

#include <common/ScdParser.h>
#include <log-manager/CassandraConnection.h>

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

        toolDescription_.add(scdfile).add(connection);
    }

    bool SetArgs(const std::vector<std::string>& args)
    {
        std::string cassandraToolSample = "Example: ./CassandraTool -C connection -F SCD";
        try
        {
            po::store(po::command_line_parser(args).options(toolDescription_).positional(additional_).run(), variableMap_);
            po::notify(variableMap_);

            unsigned int size = variableMap_.size();
            if (variableMap_.empty())
            {
                std::cout << "Usage:  CassandraTool " << std::endl;
                std::cout << cassandraToolSample<<std::endl;
                std::cout << toolDescription_;
                return false;
            }
            if (size < 2)
            {
                std::cerr << "Warning: Missing Mandatory Parameter" << std::endl;
                std::cout << "Usage:  CassandraTool " << std::endl;
                std::cout << cassandraToolSample<<std::endl;
                std::cout << toolDescription_;
                return false;
            }
            if (variableMap_.count("scd-file"))
                scd_file_ = variableMap_["scd-file"].as<std::string>();
            if (variableMap_.count("connection"))
                connection_str_ = variableMap_["connection"].as<std::string>();

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

    std::string connection_str_;

    std::string scd_file_;
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
        SCDDoc::iterator p = doc->begin();
        for (; p != doc->end(); p++)
        {
            const std::string& fieldStr = p->first;
            const izenelib::util::UString & propertyValueU = p->second;
            if (boost::iequals(fieldStr, DOCID))
            {
                std::string docIdStr;
                propertyValueU.convertString(docIdStr, UString::UTF_8);
                //std::cout<<docIdStr<<std::endl;
            }
            else if (boost::iequals(fieldStr, DATE))
            {
            }

        }
    }


    return 0;
}
