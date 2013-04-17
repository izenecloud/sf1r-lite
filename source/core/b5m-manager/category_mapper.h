#ifndef SF1R_B5MMANAGER_CATEGORYMAPPER_H_
#define SF1R_B5MMANAGER_CATEGORYMAPPER_H_
#include "b5m_helper.h"
#include "b5m_types.h"
#include "mapper_model.h"
#include <idmlib/util/idm_analyzer.h>
#include <idmlib/util/svm_model.h>
#include <idmlib/similarity/string_similarity.h>
#include <document-manager/Document.h>

namespace sf1r {

    using izenelib::util::UString;
    using idmlib::util::SvmModel;

    class CategoryMapper {
        typedef idmlib::util::SvmModel::IdManager SvmIdManager;
        //typedef SvmModel Model;
        typedef MapperModel Model;
    public:
        struct Classifier
        {
            Classifier():model(NULL)
            {
            }
            std::string category;
            Model* model;
            std::vector<Classifier*> sub_classifiers;
        };
        typedef Classifier* ClassifierPtr;
        struct CategoryPair
        {
            CategoryPair():score(1.0)
            {
            }
            CategoryPair(const std::string& c, const std::string& o, double s=1.0)
            : category(c), ocategory(o), score(s)
            {
            }
            std::string category;
            std::string ocategory;
            double score;
            bool operator<(const CategoryPair& p) const
            {
                if(category!=p.category) return category<p.category;
                else if(ocategory!=p.ocategory) return ocategory<p.ocategory;
                else
                {
                    return score>p.score;
                }
            }
            bool operator==(const CategoryPair& p) const
            {
                return category==p.category&&ocategory==p.ocategory;
            }

            bool operator!=(const CategoryPair& p)const
            {
                return !(operator==(p));
            }
        };
        struct MatchCandidate
        {
            MatchCandidate():classifier(0), score(0.0), finish(false)
            {
            }
            ClassifierPtr classifier;
            double score;
            bool finish;

            bool operator<(const MatchCandidate& m) const
            {
                return score>m.score;
            }
        };
        struct ResultItem
        {
            UString category;
            std::vector<UString> sim;
        };
        typedef boost::unordered_map<std::string, std::vector<UString> > CategorySimMap;
        CategoryMapper();
        ~CategoryMapper();
        void SetCmaPath(const std::string& path)
        {
            cma_path_ = path;
        }
        bool IsOpen() const;
        bool Open(const std::string& path);
        bool Index(const std::string& path, const std::string& scd_path);
        bool Map(const UString& ocategory, std::vector<ResultItem>& results);
        bool DoMap(const std::string& scd_path);
    private:
        static bool ClassifierCompare_(Classifier* c1, Classifier* c2)
        {
            return c1->category<c2->category;
        }
        void GetSubMatch_(const std::string& text, const Model::Instance& ins, std::vector<MatchCandidate>& results);
        bool IsNegative_(const std::string& text, const std::string& category, const std::string& parent_category) const;
        void GetInstance_(const std::string& text, Model::Instance& instance);
        void AddInstance_(Model* model, const Model::Instance& ins, double weight);
        void Analyze_(const std::string& text, std::vector<std::string>& terms);
        static void SplitCategory_(const std::string& category, std::vector<std::string>& texts);
        static void CombineCategory_(const std::vector<std::string>& texts, std::string& category);
        bool IsSexFilter_(const std::string& c1, const std::string& c2) const;
    private:
        std::string cma_path_;
        idmlib::util::IDMAnalyzer* analyzer_;
        SvmIdManager* svm_id_manager_;
        Classifier* root_classifier_;
        CategorySimMap category_sim_map_;
        idmlib::sim::StringSimilarity string_similarity_;

    };

}

#endif

