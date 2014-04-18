/*
 *  DNFParser.cpp
 */

#include "DNFParser.h"
#include <boost/algorithm/string.hpp>

namespace sf1r
{
bool DNFParser::parseDNF(const std::string& str, DNF& dnf)
{
    std::vector<std::string> strs;
    Conjunction conjunction;
    boost::split(strs, str, boost::is_any_of("^"));
    for(std::vector<std::string>::iterator it = strs.begin();
            it != strs.end();it++)
    {

        Assignment assignment;
        basic_string<char>::size_type index;
        index = it->find("!~");
        if(index != string::npos)
        {
            assignment.belongsTo = false;
            assignment.attribute = it->substr(0,index);
            index += 3;
            std::string ts = it->substr(index);
            ts = ts.substr(0, ts.size()-1);
            boost::split(assignment.values, ts, boost::is_any_of(","));
            conjunction.assignments.push_back(assignment);
        }
        else
        {
            index = it->find("~");
            if(index != string::npos)
            {
                assignment.belongsTo = true;
                assignment.attribute = it->substr(0, index);
                index += 2;
                std::string ts = it->substr(index);
                ts = ts.substr(0, ts.size()-1);
                boost::split(assignment.values, ts, boost::is_any_of(","));
                conjunction.assignments.push_back(assignment);
            }
            else
            {
                LOG(INFO)<<"Unknown DNF formart"<<endl;
                return false;
            }
        }
    }
    dnf.conjunctions.push_back(conjunction);
    return true;
}
}
