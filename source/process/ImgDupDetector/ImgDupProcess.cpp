#include "ImgDupCfg.h"
#include "ImgDupProcess.h"
#include <b5m-manager/img_dup_detector.h>

#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>

#include <iostream>

using namespace sf1r;

#define RETURN_ON_FAILURE(condition)                                        \
if (! (condition))                                                          \
{                                                                           \
    return false;                                                           \
}

#define LOG_SERVER_ASSERT(condition)                                        \
if (! (condition))                                                          \
{                                                                           \
    std::cerr << "Assertion failed: " #condition                            \
              << std::endl;                                                 \
    return false;                                                           \
}


ImgDupProcess::ImgDupProcess()
{
}

ImgDupProcess::~ImgDupProcess()
{
}

bool ImgDupProcess::init(const std::string& cfgFile)
{
    // Parse config first
    RETURN_ON_FAILURE(ImgDupCfg::get()->parse(cfgFile));

    return true;
}

void ImgDupProcess::start()
{
    LOG(INFO) << "\tImage duplicate detection server start " << std::endl;
    std::string scd_path = ImgDupCfg::get()->scd_input_dir_;
    std::string output_path = ImgDupCfg::get()->scd_output_dir_;
    std::string source_name = ImgDupCfg::get()->source_name_;
    std::string incremental_mode = ImgDupCfg::get()->incremental_mode_;
    std::string byurl = ImgDupCfg::get()->detect_by_imgurl_;
    std::string bycontent = ImgDupCfg::get()->detect_by_imgcontent_;
    int controller = 0;
    if(byurl.compare("y") == 0 || byurl.compare("Y") == 0 )
        controller++;
    if(bycontent.compare("y") == 0 || bycontent.compare("Y") == 0)
        controller+=2;
    if(incremental_mode.compare("n") == 0 || incremental_mode.compare("N") == 0)
        ImgDupDetector::get()->DupDetectByImgUrlNotIn(scd_path, output_path, source_name, controller);
    else ImgDupDetector::get()->DupDetectByImgUrlIncre(scd_path, output_path, source_name, controller);
}

void ImgDupProcess::join()
{
}

void ImgDupProcess::stop()
{
}
