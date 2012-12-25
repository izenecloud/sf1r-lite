#ifndef IMG_DUP_DETECTOR_H_
#define IMG_DUP_DETECTOR_H_

#include "img_dup_helper.h"
#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <idmlib/duplicate-detection/psm.h>
#include <common/ScdWriter.h>
//#include <idmlib/ise/psm.hpp>
//#include <idmlib/ise/extractor.hpp>


namespace sf1r {

    class ImgDupDetector
    {
        struct CacheType
        {
            CacheType(){}
            CacheType(const std::string& p1, const std::vector<std::pair<std::string, double> >& p2, const PsmAttach& p3)
            :key(p1), doc_vector(p2), attach(p3)
            {
            }
            std::string key;
            std::vector<std::pair<std::string, double> > doc_vector;
            PsmAttach attach;
        };
    public:
        typedef idmlib::dd::DupDetector<uint32_t, uint32_t, PsmAttach> DDType;
        typedef DDType::GroupTableType GroupTableType;
        typedef idmlib::dd::PSM<64, 0, 24, uint32_t, std::string, PsmAttach> PsmType;
        ImgDupDetector();
        void SetPsmK(uint32_t k) {psmk_ = k;}

        static ImgDupDetector* get()
        {
            return izenelib::util::Singleton<ImgDupDetector>::get();
        }

        bool DupDetectByImgUrlIncre(const std::string& scd_path, const std::string& output_path, std::string& toDelete);
        bool DupDetectByImgUrlNotIn(const std::string& scd_path, const std::string& output_path, std::string& toDelete);
//        bool DupDetectByImgSift(const std::string& scd_path,  const std::string& output_path, bool test);

        std::string cma_path_;
        uint32_t psmk_;
    };

}

#endif // IMG_DUP_DETECTOR_H_
