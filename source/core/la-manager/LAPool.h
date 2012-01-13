/// @file LAPool.h
/// @brief header file of LAPool
/// @author JunHui
/// @date 2008-08-19
/// @details
/// - Log
///     - 2009.07.10 Replace extractor to new LA object.

#if !defined(_LA_Pool_)
#define _LA_Pool_

//#include <la-manager/NGramExtractor.h>
//#include <la-manager/LikeExtractor.h>
//#include <la-manager/StemExtractor.h>
//#include <la-manager/KoreanLanguageExtractor.h>
#include "AnalysisInformation.h"
#include <configuration-manager/LAManagerConfig.h>

#include <la/LA.h>          // la-manager library

#include <util/ThreadModel.h>
#include <util/singleton.h>

#include <boost/unordered_map.hpp>

#include <deque>
#include <map>

//#define LA_THREAD_NUM 3

//using namespace std;

namespace ilplib
{
namespace langid
{
class Analyzer;
class Knowledge;
class Factory;
}
}

namespace sf1r
{

#ifdef USE_WISEKMA
    typedef la::KoreanAnalyzer NKoreanAnalyzer;
#endif

#ifdef USE_IZENECMA
    typedef la::ChineseAnalyzer NChineseAnalyzer;
#endif

#ifdef USE_IZENEJMA
    typedef la::JapaneseAnalyzer NJapaneseAnalyzer;
#endif
    ///
    /// @brief
    /// This class looks up the configuration of LA and creates Extractor classes.
    ///
    class LAPool
    {

        private:
            static const unsigned int LA_THREAD_NUM = 10;

        public:

            // used when creating LAs.
            enum LA_MODE { LA_MODE_INDEXING, LA_MODE_SEARCHING };


            ///
            /// constructor.
            ///
            LAPool();

            ///
            /// destructor.
            ///
            virtual ~LAPool();

            /// for singleton
            static LAPool* getInstance()
            {
                return ::izenelib::util::Singleton<LAPool>::get();
            }

            void setLangId();

            ilplib::langid::Analyzer* getLangId()
            {
                return langIdAnalyzer_;
            }

            void setLangIdDbPath(const std::string& langIdDbPath)
            {
                langIdDbPath_ = langIdDbPath;
            }

            std::string& getLangIdDbPath()
            {
                return langIdDbPath_;
            }

            void initLangAnalyzer();
            ///
            /// initialize dictionary.
            ///
            bool init(const sf1r::LAManagerConfig& laManagerConfig);

            ///
            /// create LA classes according to the config.
            /// @param laConfig configuration class of LA
            /// @return

            la::LA* createDefaultLA();

            /// @param mode true is indexing, false is searching
            la::LA* createLA( const AnalysisInfo & analysisInfo, bool outputLog, bool mode );

            la::LA* getIndexLA(const AnalysisInfo& analysisInfo );

            la::LA * popSearchLA(const AnalysisInfo& analysisInfo );

            void pushSearchLA(const AnalysisInfo & analysisInfo, la::LA * laThread );

            ///
            /// @brief Don't remove the LA pointer in the Pool, Ensure the entire running environment
            /// is single-thread before use this function
            ///
            la::LA * topSearchLA(const AnalysisInfo& analysisInfo );

            bool getLAConfigUnit( const string & laConfigId, LAConfigUnit & laConfigUnit ) const;

            void set_kma_path(const std::string& path)
            {
              kma_path_ = path;
            }

            void get_kma_path(std::string& path)
            {
              path=  kma_path_;
            }

            void set_cma_path(const std::string& path)
            {
                cma_path_ = path;
            }

            void get_cma_path(std::string& path)
            {
                path=  cma_path_;
            }

            void set_jma_path(const std::string& path)
            {
                jma_path_ = path;
            }

            void get_jma_path(std::string& path)
            {
                path=  jma_path_;
            }

        private:

            ///
            /// @brief Holds the map of LAs for indexing, which is not multi-threaded
            ///
            boost::unordered_map<AnalysisInfo, la::LA*> laIndexMap_;

            ///
            /// @brief Holds the map of LAs for searching, has a pool(deque) for multithread
            ///
            boost::unordered_map<AnalysisInfo, std::deque<la::LA*> > laSearchPool_;

            /// @brief  LA unit ID mapped to its configuration
            std::map<std::string, LAConfigUnit> laConfigUnitMap_;

            /// @brief  Tokenzier unit ID mapped to its configuration
            std::map<std::string, TokenizerConfigUnit> tokenizerConfigUnitMap_;

            izenelib::util::ReadWriteLock lock_;

            std::map<std::string, AnalysisInfo> innerAnalysisInfos_;

            /// @brief resource path for KMA
            std::string kma_path_;

            std::string cma_path_;

            std::string jma_path_;

            ilplib::langid::Analyzer* langIdAnalyzer_;

            ilplib::langid::Knowledge* langIdKnowledge_;

            ilplib::langid::Factory* langIdFactory_;

            std::string langIdDbPath_;
    };

} // namespace sf1r

#endif // _LA_Pool_
