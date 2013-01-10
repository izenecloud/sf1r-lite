#ifndef IMG_DUP_DETECTOR_H_
#define IMG_DUP_DETECTOR_H_

#include "img_dup_helper.h"
#include "img_dup_fujimap.h"
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


namespace sf1r {

    class ImgDupDetector
    {
    public:
        typedef idmlib::dd::PSM<64, 0, 24, uint32_t, std::string, PsmAttach> PsmType;

        ImgDupDetector();
        ImgDupDetector(std::string sp,
                std::string op,
                std::string sn,
                bool li,
                bool im,
                int con,
                int icl
                );
        ~ImgDupDetector();

        void SetPsmK(uint32_t k) {psmk_ = k;}

        static ImgDupDetector* get()
        {
            return izenelib::util::Singleton<ImgDupDetector>::get();
        }

        bool SetParams(std::string sp,
                std::string op,
                std::string sn,
                bool li,
                bool im,
                int con,
                int icl
                );

        bool SetController();
        bool ClearHistory();
        bool ClearHistoryCon();
        bool ClearHistoryUrl();
        bool SetPath();
        bool InitFujiMap();
        bool DupDetectorMain();
        bool BeginToDupDetect(const std::string& filename);
        bool BuildUrlIndex(const std::string& scd_file, const std::string& psm_path);
        bool BuildConIndex(const std::string& scd_file, const std::string& psm_path);
        bool DetectUrl(const std::string& scd_file, const std::string& psm_path, const std::string& res_file, const std::string& output_path);
        bool DetectCon(const std::string& scd_file, const std::string& psm_path, const std::string& res_file, const std::string& output_path);
        bool WriteFile(const std::string& filename);

    public:

        std::string scd_path_;
        std::string scd_temp_path_;
        std::string output_path_;
        std::string source_name_;
        bool log_info_;
        bool incremental_mode_;

        int controller_;
        uint32_t image_content_length_;

        std::string cma_path_;
        uint32_t psmk_;

        std::string psm_path_;
        std::string psm_path_incr_;
        std::string psm_path_noin_;
        std::string psm_path_incr_url_;
        std::string psm_path_noin_url_;
        std::string psm_path_incr_con_;
        std::string psm_path_noin_con_;

        bool dup_by_url_;
        bool dup_by_con_;

//        std::map<uint32_t, uint32_t> con_docid_key_;
//        std::map<uint32_t, uint32_t> con_key_docid_;
        ImgDupFujiMap* con_docid_key_;
        ImgDupFujiMap* con_key_docid_;
        uint32_t con_key_;

//        std::map<uint32_t, uint32_t> url_docid_key_;
//        std::map<uint32_t, uint32_t> url_key_docid_;
        ImgDupFujiMap* url_docid_key_;
        ImgDupFujiMap* url_key_docid_;
        uint32_t url_key_;

        ImgDupFujiMap* docid_docid_;
        std::map<uint32_t, std::vector<uint32_t> > gid_docids_;

        std::map<uint32_t, UString> key_con_map_;
        std::map<uint32_t, UString> key_url_map_;
    };
}

#endif // IMG_DUP_DETECTOR_H_
