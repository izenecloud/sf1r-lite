#include "ProcessOptions.h"

#include <boost/lexical_cast.hpp>

#include <iostream>

using namespace std;
using namespace boost;

namespace po = boost::program_options;

void validate(boost::any& v,
              const std::vector<std::string>& values,
              ProcessOptions::Port*, int)
{
    static const unsigned kPortLowerLimit = 1024;
    static const std::string kPortLowerLimitStr = "1024";

    using namespace boost::program_options;
    validators::check_first_occurrence(v);
    const string& value = validators::get_single_string(values);

    unsigned port = 0;
    try
    {
        port = boost::lexical_cast<unsigned>(value);
    }
    catch (boost::bad_lexical_cast&)
    {
        throw invalid_option_value("Port requires an unsigned integer.");
    }

    if (port <= kPortLowerLimit)
    {
        throw invalid_option_value("Port must higher than " + kPortLowerLimitStr);
    }

    v = boost::any(ProcessOptions::Port(port));
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              ProcessOptions::String*, int)
{
    using namespace boost::program_options;
    validators::check_first_occurrence(v);
    const string& value = validators::get_single_string(values);

    if (value.empty() || value[0] == '-')
    {
        throw invalid_option_value(
            "Option value cannot start with hyphen: " + value
        );
    }

    v = boost::any(ProcessOptions::String(value));
}

ProcessOptions::ProcessOptions()
        :   cobraProcessDescription_("Cobra Process Options"),
        isVerboseOn_(false),
        logPrefix_("")
{
    additional_.add("additional_", 0);
    po::options_description base;
    po::options_description configFile;
    po::options_description scdParsing;
    po::options_description verbose;
    po::options_description logPrefix;
    po::options_description pidFile;

    logPrefix.add_options()
    ("log-prefix,l", po::value<String>(),
     "Prefix including log Path or one of pre-defined values.\n\n"
     "Pre-defined values:\n"
     "  stdout: \tDisplay log message into stdout\n"
     "  NULL: \tNo logging\n\n"
     "Example: -l ./log/BA\n"
     "  path: \tlog\n"
     "  prefix: \tBA\n"
     "  log file: \t./BA.info.xxx ./BA.error.xxx ...\n"
    );

    pidFile.add_options()
    ("pid-file", po::value<String>(),
     "Specify the file to store pid");

    verbose.add_options()
    ("verbose,v","verbosely display log information");

    base.add_options()
    ("help", "Display help message");

    configFile.add_options()
    ("config-file,F", po::value<String>(), "Path to the configuration file");

    cobraProcessDescription_.add(base).add(verbose).add(logPrefix).add( configFile ).add(pidFile);
}


void ProcessOptions::setProcessOptions()
{
    if ( variableMap_.count("config-file") ) //-F in BA
    {
        configFileName_ = variableMap_["config-file"].as<String>().str;
    }

    if ( variableMap_.count("log-prefix")  )
    {
        isVerboseOn_ = true;
    }
    if ( variableMap_.count("log-prefix")  )
    {
        logPrefix_ = variableMap_["log-prefix"].as<String>().str;
    }

    pidFile_.clear();
    if (variableMap_.count("pid-file"))
    {
        pidFile_ = variableMap_["pid-file"].as<String>().str;
    }
}

bool ProcessOptions::setCobraProcessArgs(const std::vector<std::string>& args)
{
    std::string cobraProcessSample = "Example: ./CobraProcess -F ./config/default_sf1config.xml";
    try
    {
        po::store(po::command_line_parser(args).options(cobraProcessDescription_).positional(additional_).run(), variableMap_);
        po::notify(variableMap_);

        bool flag=variableMap_.count("verbose");
        unsigned int size=variableMap_.size();
        if ( variableMap_.count("help") || variableMap_.empty())
        {
            cout << "Usage:  CobraProcess <settings (-F)[, -V]>" << endl;
            cout << cobraProcessSample<<endl;
            cout << cobraProcessDescription_;
            return false;
        }
        if ( (flag&&(size < 2) )||(!flag&&(size<1)) )
        {
            cout << "Warning: Missing Mandatory Parameter"<<endl;
            cout << "Usage:  CobraProcess <settings (-F)[, -V]>" << endl;
            cout << cobraProcessSample<<endl;
            cout << cobraProcessDescription_;
            return false;
        }
        setProcessOptions();
    }
    catch (std::exception &e)
    {
        cerr<<"Warning: "<<e.what()<<std::endl;
        cout << "Usage:  CobraProcess <settings (-F )[, -V]>" << endl;
        cout<<cobraProcessSample<<endl;
        cout << cobraProcessDescription_;
        return false;
    }
    return true;
}

