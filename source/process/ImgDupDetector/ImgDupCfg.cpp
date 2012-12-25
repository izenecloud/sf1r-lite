#include "ImgDupCfg.h"

#include <util/string/StringUtils.h>

#include <iostream>
#include <fstream>

#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

ImgDupCfg::ImgDupCfg()
{
}

ImgDupCfg::~ImgDupCfg()
{
}

bool ImgDupCfg::parse(const std::string& cfgFile)
{
    cfgFile_ = cfgFile;
    return parseCfgFile_(cfgFile);
}

bool ImgDupCfg::parseCfgFile_(const std::string& cfgFile)
{
    try
    {
        if (!bfs::exists(cfgFile) || !bfs::is_regular_file(cfgFile))
        {
            std::cerr <<"\""<<cfgFile<< "\" is not existed or not a regular file." << std::endl;
            return false;
        }

        // load configuration file
        std::ifstream cfgInput(cfgFile.c_str());
        std::string cfgString;
        std::string line;

        if (!cfgInput.is_open())
        {
            std::cerr << "Could not open file: " << cfgFile << std::endl;
            return false;
        }

        while (getline(cfgInput, line))
        {
            izenelib::util::Trim(line);
            if (line.empty() || line[0] == '#')
            {
                // ignore empty line and comment line
                continue;
            }

            if (!cfgString.empty())
            {
                cfgString.push_back('\n');
            }
            cfgString.append(line);
        }

        // check configuration properties
        properties props('\n', '=');
        props.loadKvString(cfgString);

        parseCfg(props);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

void ImgDupCfg::parseCfg(properties& props)
{
    parseServerCfg(props);
}

void ImgDupCfg::parseServerCfg(properties& props)
{
    if (!props.getValue("host", host_))
    {
        host_ = "localhost";
    }

    if (!props.getValue("imgdupdetector.inputpath", scd_input_dir_))
    {
        throw std::runtime_error("Img Dup Configuration missing property: imgdupdetector.inputpath");
    }

    if (!props.getValue("imgdupdetector.outputpath", scd_output_dir_))
    {
        throw std::runtime_error("Img Dup Configuration missing property: imgdupdetector.outputpath");
    }

    if (!props.getValue("imgdupdetector.sourcename", source_name_))
    {
        throw std::runtime_error("Img Dup Configuration missing property: imgdupdetector.sourcename");
    }

    if(!props.getValue("imgdupdetector.mode.incremental", incremental_mode_))
    {
        throw std::runtime_error("Img Dup Configuration missing property: imgdupdetector.mode.incremental");
    }
}
