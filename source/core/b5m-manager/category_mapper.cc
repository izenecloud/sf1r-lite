#include "category_mapper.h"
#include <stack>

using namespace sf1r;

CategoryMapper::CategoryMapper(): root_classifier_(NULL)
{
    idmlib::util::IDMAnalyzerConfig config = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path_,"");
    //config.symbol = true;
    analyzer_ = new idmlib::util::IDMAnalyzer(config);
}
CategoryMapper::~CategoryMapper()
{
    delete analyzer_;
}
bool CategoryMapper::IsOpen() const
{
    return true;
}
bool CategoryMapper::Open(const std::string& path)
{
    root_classifier_ = new Classifier();
    svm_id_manager_ = new SvmIdManager();
    std::string id_path = path+"/id";
    izenelib::am::ssf::Util<>::Load(id_path, *svm_id_manager_);
    LOG(INFO)<<"id manager size "<<svm_id_manager_->map.size()<<std::endl;
    std::map<std::string, std::vector<UString> > category_sim_stdmap;
    std::string sim_path = path+"/category_sim";
    izenelib::am::ssf::Util<>::Load(sim_path, category_sim_stdmap);
    category_sim_map_.insert(category_sim_stdmap.begin(), category_sim_stdmap.end());
    namespace bfs = boost::filesystem;
    std::string model_dir = path+"/models";
    if(!bfs::is_directory(model_dir)) return false;
    bfs::path p(model_dir);
    bfs::directory_iterator end;
    std::vector<Classifier*> all_classifiers;
    LOG(INFO)<<"start to open models.."<<std::endl;
    for(bfs::directory_iterator it(p);it!=end;it++)
    {
        if(bfs::is_directory(it->path()))
        {
            Model* model = new Model(svm_id_manager_);
            std::string path = it->path().string();
            if(!model->Open(path))
            {
                LOG(ERROR)<<"model open "<<path<<" error"<<std::endl;
                continue;
            }
            Classifier* c = new Classifier();
            c->category = model->Name();
            c->model = model;
            all_classifiers.push_back(c);
        }
    }
    std::sort(all_classifiers.begin(), all_classifiers.end(), ClassifierCompare_);
    std::stack<Classifier*> stack;
    stack.push(root_classifier_);
    for(uint32_t i=0;i<all_classifiers.size();i++)
    {
        Classifier* c = all_classifiers[i];
        while(!stack.empty())
        {
            Classifier* p = stack.top();
            if(boost::algorithm::starts_with(c->category, p->category))
            {
                p->sub_classifiers.push_back(c);
                stack.push(c);
                //LOG(INFO)<<"sub append "<<p->category<<","<<c->category<<std::endl;
                break;
            }
            stack.pop();
        }
    }
    LOG(INFO)<<"models opened"<<std::endl;
    return true;
}
bool CategoryMapper::Index(const std::string& path, const std::string& scd_path)
{
    //typedef std::pair<std::string, std::string> CategoryPair;
    typedef std::vector<CategoryPair> CategoryPairList;
    CategoryPairList pair_list;
    std::string train_file = scd_path+"/map_train.txt";
    if(!boost::filesystem::exists(train_file))
    {
        std::string offer_scd = scd_path+"/OFFER.SCD";
        if(!boost::filesystem::exists(offer_scd))
        {
            LOG(ERROR)<<"OFFER.SCD not exists"<<std::endl;
            return false;
        }
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(offer_scd);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%100000==0)
            {
                LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
            }
            Document doc;
            UString ocategory;
            UString category;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                if(property_name=="OriginalCategory")
                {
                    ocategory = propstr_to_ustr(p->second);
                }
                else if(property_name=="Category")
                {
                    category = propstr_to_ustr(p->second);
                }
            }
            UString::algo::compact(ocategory);
            UString::algo::compact(category);
            if(category.length()==0 || ocategory.length()==0)
            {
                continue;
            }
            CategoryPair pair;
            category.convertString(pair.category, UString::UTF_8);
            ocategory.convertString(pair.ocategory, UString::UTF_8);
            pair_list.push_back(pair);
        }
        std::string category_path = scd_path+"/category";
        std::string line;
        std::ifstream ifs(category_path.c_str());
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of(","));
            if(vec.size()<2) continue;
            const std::string& scategory = vec[0];
            if(scategory.empty()) continue;
            std::set<std::string> akeywords;
            std::set<std::string> rkeywords;
            for(uint32_t i=2;i<vec.size();i++)
            {
                std::string keyword = vec[i];
                bool remove = false;
                if(keyword.empty()) continue;
                if(keyword[0]=='+')
                {
                    keyword = keyword.substr(1);
                }
                else if(keyword[0]=='-')
                {
                    keyword = keyword.substr(1);
                    remove = true;
                }
                if(keyword.empty()) continue;
                if(!remove)
                {
                    akeywords.insert(keyword);
                }
                else
                {
                    rkeywords.insert(keyword);
                }
            }
            std::vector<std::string> cs_list;
            boost::algorithm::split( cs_list, scategory, boost::algorithm::is_any_of(">") );
            if(cs_list.empty()) continue;
            std::vector<std::string> keywords_vec;
            boost::algorithm::split( keywords_vec, cs_list.back(), boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<keywords_vec.size();i++)
            {
                akeywords.insert(keywords_vec[i]);
            }
            for(std::set<std::string>::const_iterator it = rkeywords.begin();it!=rkeywords.end();it++)
            {
                akeywords.erase(*it);
            }
            for(std::set<std::string>::const_iterator it = akeywords.begin();it!=akeywords.end();it++)
            {
                CategoryPair pair(scategory, *it, 1.0);
                pair_list.push_back(pair);
            }
        }
        ifs.close();
        LOG(INFO)<<"pair before "<<pair_list.size()<<std::endl;
        std::sort(pair_list.begin(), pair_list.end());
        pair_list.erase(std::unique(pair_list.begin(), pair_list.end()), pair_list.end());
        LOG(INFO)<<"pair after "<<pair_list.size()<<std::endl;
        std::ofstream ofs(train_file.c_str());
        for(uint32_t i=0;i<pair_list.size();i++)
        {
            ofs<<pair_list[i].category<<","<<pair_list[i].ocategory<<","<<pair_list[i].score<<std::endl;
        }
        ofs.close();
    }
    else
    {
        std::ifstream ifs(train_file.c_str());
        std::string line;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of(","));
            if(vec.size()<3) continue;
            CategoryPair pair;
            pair.category = vec[0];
            pair.ocategory = vec[1];
            pair.score = boost::lexical_cast<double>(vec.back());
            pair_list.push_back(pair);
        }
        ifs.close();
    }
    svm_id_manager_ = new SvmIdManager();
    LOG(INFO)<<"Generating instance_list"<<std::endl;
    std::vector<Model::Instance> instance_list(pair_list.size());
    for(uint32_t i=0;i<pair_list.size();i++)
    {
        GetInstance_(pair_list[i].ocategory, instance_list[i]);
    }
    //LOG(INFO)<<"Calculating category sim"<<std::endl;
    //{
        //std::vector<std::string> all_categories;
        //std::vector<UString> uall_categories;
        //std::string category_path = scd_path+"/category";
        //std::string line;
        //std::ifstream ifs(category_path.c_str());
        //while(getline(ifs, line))
        //{
            //boost::algorithm::trim(line);
            //std::vector<std::string> vec;
            //boost::algorithm::split(vec, line, boost::algorithm::is_any_of(","));
            //if(vec.size()<2) continue;
            //const std::string& scategory = vec[0];
            //if(scategory.empty()) continue;
            //all_categories.push_back(scategory);
            //uall_categories.push_back(UString(scategory, UString::UTF_8));
        //}
        //ifs.close();
        //std::vector<std::pair<double, UString> > vec;
        //vec.reserve(all_categories.size());
        //for(uint32_t c=0;c<all_categories.size();c++)
        //{
            //const std::string& ci = all_categories[c];
            //LOG(INFO)<<"calculate for "<<ci<<std::endl;
            //const UString& uci = uall_categories[c];
            //vec.resize(0);
            //for(uint32_t j=0;j<all_categories.size();j++)
            //{
                //if(c==j) continue;
                //const UString& ucj = uall_categories[j];
                //const std::string& cj = all_categories[j];
                //double sim = string_similarity_.Sim(uci, ucj);
                //vec.push_back(std::make_pair(sim, ucj));
            //}
            //std::sort(vec.begin(), vec.end(), std::greater<std::pair<double, UString> >());
            //std::vector<UString> sim_list;
            //for(uint32_t j=0;j<vec.size();j++)
            //{
                //if(j>2) break;
                //if(vec[j].first<0.5) break;
                //sim_list.push_back(vec[j].second);
                //std::string ssim;
                //vec[j].second.convertString(ssim, UString::UTF_8);
                //LOG(INFO)<<"find "<<ssim<<std::endl;
            //}
            //category_sim_map_.insert(std::make_pair(ci, sim_list));
        //}
    //}
    //for(uint32_t i=0;i<pair_list.size();i++)
    //{
        //std::cout<<pair_list[i].first<<","<<pair_list[i].second<<std::endl;
    //}
    typedef std::pair<uint32_t, uint32_t> Range;
    typedef std::pair<Range, Range> PNRange; //positive and negative ranges
    typedef std::map<std::string, Range> CategoryRange;
    CategoryRange category_range;
    for(uint32_t i=0;i<pair_list.size();i++)
    {
        const CategoryPair& pair = pair_list[i];
        const std::string& key = pair.category;
        CategoryRange::iterator it = category_range.find(key);
        if(it==category_range.end())
        {
            category_range.insert(std::make_pair(key, std::make_pair(i, i+1)));
        }
        else
        {
            it->second.second = i+1;
        }
    }
    CategoryRange final_range;
    for(CategoryRange::const_iterator it=category_range.begin();it!=category_range.end();++it)
    {
        const std::string& category = it->first;
        const Range& range = it->second;
        std::vector<std::string> texts;
        SplitCategory_(category, texts);
        for(uint32_t i=0;i<texts.size();i++)
        {
            std::vector<std::string> sub_texts(texts.begin(), texts.begin()+i+1);
            std::string sub_category;
            CombineCategory_(sub_texts, sub_category);
            CategoryRange::const_iterator it2 = final_range.find(sub_category);
            if(it2!=final_range.end()) continue;
            Range new_range(range);
            uint32_t j = new_range.second;
            for(;j<pair_list.size();j++)
            {
                if(!boost::algorithm::starts_with(pair_list[j].category, sub_category))
                {
                    break;
                }
            }
            new_range.second = j;
            final_range.insert(std::make_pair(sub_category, new_range));
        }
    }
    //for(CategoryRange::const_iterator it=final_range.begin();it!=final_range.end();++it)
    //{
        //std::cout<<it->first<<","<<it->second.first<<","<<it->second.second<<std::endl;
    //}
    //generate positive and negative instances
    //also train SVM
    boost::filesystem::remove_all(path);
    std::string model_dir = path+"/models";
    boost::filesystem::create_directories(model_dir);
    std::string id_path = path+"/id";
    uint32_t model_id = 0;
    for(CategoryRange::const_iterator it=final_range.begin();it!=final_range.end();++it)
    {
        model_id++;
        const std::string& category = it->first;
        std::vector<std::string> texts;
        SplitCategory_(category, texts);
        texts.resize(texts.size()-1);
        std::string parent_category;
        CombineCategory_(texts, parent_category);
        const Range& prange = it->second;
        //ocategory list
        std::vector<uint32_t> positive; //index in pair_list and instance_list
        std::vector<uint32_t> negative;
        for(uint32_t i=prange.first;i<prange.second;i++)
        {
            //const CategoryPair& pair = pair_list[i];
            positive.push_back(i);
        }
        for(uint32_t i=0;i<prange.first;i++)
        {
            const CategoryPair& pair = pair_list[i];
            if(!IsNegative_(pair.category, category, parent_category)) continue;
            negative.push_back(i);
        }
        for(uint32_t i=prange.second;i<pair_list.size();i++)
        {
            const CategoryPair& pair = pair_list[i];
            if(!IsNegative_(pair.category, category, parent_category)) continue;
            negative.push_back(i);
        }
        LOG(INFO)<<"training category "<<category<<","<<model_id<<","<<positive.size()<<","<<negative.size()<<std::endl;
        //std::cout<<category<<","<<positive.size()<<","<<negative.size()<<std::endl;
        Model* model = new Model(svm_id_manager_);
        //svm_parameter& param = model->SvmParam();
        //param.kernel_type = LINEAR;
        //param.nr_weight = 2;
        //param.weight_label = (int *)malloc(sizeof(int)*param.nr_weight);
        //param.weight = (double *)malloc(sizeof(double)*param.nr_weight);
        //param.weight_label[0] = 1;
        //param.weight[0] = 5.0;
        //param.weight_label[1] = -1;
        //param.weight[1] = 1.0;
        model->TrainStart(model_dir+"/"+boost::lexical_cast<std::string>(model_id), category);
        for(uint32_t i=0;i<positive.size();i++)
        {
            uint32_t index = positive[i];
            Model::Instance& ins = instance_list[index];
            const CategoryPair& pair = pair_list[index];
            ins.label = 1.0;
            AddInstance_(model, ins, pair.score);
        }
        for(uint32_t i=0;i<negative.size();i++)
        {
            uint32_t index = negative[i];
            Model::Instance& ins = instance_list[index];
            const CategoryPair& pair = pair_list[index];
            ins.label = -1.0;
            AddInstance_(model, ins, pair.score);
        }
        model->TrainEnd();
        delete model;
    }
    izenelib::am::ssf::Util<>::Save(id_path, *svm_id_manager_);
    std::map<std::string, std::vector<UString> > category_sim_stdmap(category_sim_map_.begin(), category_sim_map_.end());
    std::string sim_path = path+"/category_sim";
    izenelib::am::ssf::Util<>::Save(sim_path, category_sim_stdmap);
    return true;
}
bool CategoryMapper::Map(const UString& ocategory, std::vector<ResultItem>& results)
{
    std::string socategory;
    ocategory.convertString(socategory, UString::UTF_8);
    Model::Instance instance;
    GetInstance_(socategory, instance);
    std::string scategory;
    std::vector<MatchCandidate> candidates;
    MatchCandidate m;
    m.classifier = root_classifier_;
    candidates.push_back(m);
    GetSubMatch_(socategory, instance, candidates);
    for(uint32_t i=0;i<candidates.size();i++)
    {
        const std::string& category = candidates[i].classifier->category;
        if(category.empty()) continue;
        UString uc(category, UString::UTF_8);
        double str_sim = string_similarity_.Sim(ocategory, uc);
        if(str_sim<0.3) continue;
        ResultItem result;
        result.category = uc;
        result.sim = category_sim_map_[category];
        //std::cout<<"[DETAIL]"<<category<<","<<candidates[i].score<<","<<str_sim<<std::endl;
        results.push_back(result);
    }
    //category = UString(scategory, UString::UTF_8);
    //for(uint32_t i=0;i<root_classifier_->sub_classifiers.size();i++)
    //{
        //Classifier* c = root_classifier_->sub_classifiers[i];
        //std::vector<double> probs;
        //double label = c->model->Test(instance, probs);
        //if(label<=0.0) continue;
        //std::cout<<"[LABEL]"<<c->category<<","<<label<<std::endl;
        //std::cout<<"[PROBS]"<<std::endl;
        //for(uint32_t p=0;p<probs.size();p++)
        //{
            //std::cout<<probs[p]<<std::endl;
        //}
    //}

    return true;
}
void CategoryMapper::GetSubMatch_(const std::string& text, const Model::Instance& ins, std::vector<MatchCandidate>& results)
{
    while(true)
    {
        std::vector<MatchCandidate> new_results;
        for(uint32_t i=0;i<results.size();i++)
        {
            MatchCandidate& m = results[i];
            if(m.finish) continue;
            Classifier* c = m.classifier;
            std::vector<MatchCandidate> match_subs;
            for(uint32_t i=0;i<c->sub_classifiers.size();i++)
            {
                Classifier* sub = c->sub_classifiers[i];
                if(IsSexFilter_(sub->category, text)) continue;
                std::vector<double> probs;
                double label = sub->model->Test(ins, probs);
                if(label<=0.0) continue;
                //std::cout<<"match "<<sub->category<<","<<probs[0]<<std::endl;
                MatchCandidate subm;
                subm.classifier = sub;
                subm.score = probs[0];
                match_subs.push_back(subm);
            }
            bool find_sub = false;
            if(!match_subs.empty())
            {
                std::sort(match_subs.begin(), match_subs.end());
                double min_sub_score = match_subs.front().score*0.6;
                for(uint32_t s=0;s<match_subs.size();s++)
                {
                    const MatchCandidate& subm = match_subs[s];
                    if(subm.score>=0.01&&subm.score>=m.score&&subm.score>=min_sub_score)
                    {
                        new_results.push_back(subm);
                        find_sub = true;
                    }
                }
            }
            if(!find_sub)
            {
                m.finish = true;
                new_results.push_back(m);
            }
        }
        if(!new_results.empty())
        {
            if(new_results.size()>3) new_results.resize(3);
            std::sort(new_results.begin(), new_results.end());
            results.swap(new_results);
            //GetSubMatch_(ins, new_results);
        }
        else
        {
            break;
        }
    }
}
bool CategoryMapper::DoMap(const std::string& scd_path)
{
    std::vector<std::string> scd_list;
    ScdParser::getScdList(scd_path, scd_list);
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        LOG(INFO)<<"processing "<<scd_file<<std::endl;
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%100000==0)
            {
                LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
            }
            Document doc;
            UString ocategory;
            UString category;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                if(property_name=="OriginalCategory")
                {
                    ocategory = propstr_to_ustr(p->second);
                }
                else if(property_name=="Category")
                {
                    category = propstr_to_ustr(p->second);
                }
            }
            UString::algo::compact(ocategory);
            UString::algo::compact(category);
            if(ocategory.length()==0)
            {
                continue;
            }
            //if(!category.length()==0) continue;
            category.clear();
            std::vector<ResultItem> results;
            Map(ocategory, results);
            std::string socategory;
            ocategory.convertString(socategory, UString::UTF_8);
            for(uint32_t i=0;i<results.size();i++)
            {
                std::string scategory;
                results[i].category.convertString(scategory, UString::UTF_8);
                std::cout<<"[MAP]"<<socategory<<","<<scategory<<" | ";
                for(uint32_t j=0;j<results[i].sim.size();j++)
                {
                    std::string ssim;
                    results[i].sim[j].convertString(ssim, UString::UTF_8);
                    std::cout<<ssim<<",";
                }
                std::cout<<std::endl;
            }
        }
    }
    return true;
}
void CategoryMapper::GetInstance_(const std::string& text, Model::Instance& instance)
{
    std::vector<std::string> ctexts;
    boost::algorithm::split(ctexts, text, boost::algorithm::is_any_of(">"));

    if(ctexts.empty()) return;
    for(uint32_t c=0;c<ctexts.size();c++)
    {
        std::vector<std::string> terms;
        Analyze_(ctexts[c], terms);
        for(uint32_t t=0;t<terms.size();t++)
        {
            Model::Feature f;
            f.text = terms[t];
            f.id = svm_id_manager_->GetIdByText(f.text);
            f.weight = 0.4;
            if(c==ctexts.size()-1) f.weight=1.0;
            instance.features.push_back(f);
        }
    }
}
void CategoryMapper::AddInstance_(Model* model, const Model::Instance& instance, double weight)
{
    //Model::Instance instance;
    //GetInstance_(text, instance);
    //instance.label = label;
    if(instance.features.empty()) return;
    int w = (int)weight;
    for(int i=0;i<w;i++)
    {
        model->Add(instance);
    }
}
void CategoryMapper::SplitCategory_(const std::string& category, std::vector<std::string>& texts)
{
    boost::algorithm::split( texts, category, boost::algorithm::is_any_of(">") );
}
void CategoryMapper::CombineCategory_(const std::vector<std::string>& texts, std::string& category)
{
   for(uint32_t i=0;i<texts.size();i++)
   {
       if(!category.empty()) category+=">";
       category+=texts[i];
   }
}
void CategoryMapper::Analyze_(const std::string& text, std::vector<std::string>& terms)
{
    izenelib::util::UString utext(text, izenelib::util::UString::UTF_8);
    utext.toLowerString();
    std::vector<idmlib::util::IDMTerm> term_list;
    analyzer_->GetTermList(utext, term_list);
    terms.resize(term_list.size());
    for(uint32_t i=0;i<term_list.size();i++)
    {
        term_list[i].text.convertString(terms[i], UString::UTF_8);
    }
}
bool CategoryMapper::IsNegative_(const std::string& text, const std::string& category, const std::string& parent_category) const
{
    //if(!boost::algorithm::starts_with(text, category)&&boost::algorithm::starts_with(text, parent_category)) return true;
    //return false;
    if(boost::algorithm::starts_with(category, text)) return false;
    return true;
}

bool CategoryMapper::IsSexFilter_(const std::string& c1, const std::string& c2) const
{
    if(boost::algorithm::starts_with(c1, "男")&&boost::algorithm::starts_with(c2, "女")) return true;
    if(boost::algorithm::starts_with(c2, "男")&&boost::algorithm::starts_with(c1, "女")) return true;
    return false;
}
