#ifndef SF1R_B5MMANAGER_MAPPERMODEL_H_
#define SF1R_B5MMANAGER_MAPPERMODEL_H_

#include <idmlib/util/svm_model.h>
#include <am/sequence_file/ssfr.h>
#include <boost/unordered_map.hpp>


namespace sf1r {
class MapperModel
{
public:
    typedef idmlib::util::SvmModel::IdManager IdManager;
    struct Feature
    {
        Feature():weight(0.0), id(0)
        {
        }
        std::string text;
        double weight;
        int id;
    };
    struct Instance
    {
        std::vector<Feature> features;
        double label;
    };
    typedef std::vector<std::pair<int, double> > Vector;
    MapperModel(IdManager* id_manager=NULL):id_manager_(id_manager)
    {
    }
    ~MapperModel()
    {
    }
    void SetIdManager(IdManager* idm)
    {
        id_manager_ = idm;
    }
    bool Open(const std::string& path)
    {
        path_ = path;
        std::string fpath = path+"/name";
        izenelib::am::ssf::Util<>::Load(fpath, name_);
        std::string file = path+"/positive";
        izenelib::am::ssf::Util<>::Load(file, positive_vec_);
        file = path+"/negative";
        izenelib::am::ssf::Util<>::Load(file, negative_vec_);
        return true;
    }

    bool TrainStart(const std::string& path, const std::string& name="")
    {
        path_ = path;
        name_ = name;
        return true;
    }
    void Add(const Instance& ins)
    {
        Vector dvec;
        GetVector_(ins, dvec);
        if(dvec.empty()) return;
        if(ins.label>0.0)
        {
            positive_list_.push_back(dvec);
        }
        else
        {
            negative_list_.push_back(dvec);
        }
    }
    const std::string& Name() const
    {
        return name_;
    }
    bool TrainEnd()
    {
        Central_(positive_list_, positive_vec_);
        Central_(negative_list_, negative_vec_);

        boost::filesystem::create_directories(path_);
        std::string file = path_+"/positive";
        izenelib::am::ssf::Util<>::Save(file, positive_vec_);
        file = path_+"/negative";
        izenelib::am::ssf::Util<>::Save(file, negative_vec_);
        std::string path = path_+"/name";
        izenelib::am::ssf::Util<>::Save(path, name_);
        return true;
    }

    double Test(const Instance& ins, std::vector<double>& probs)
    {
        Vector v;
        GetVector_(ins, v);
        if(v.empty()) return 0.0;
        double p = Cosine_(v, positive_vec_);
        double n = Cosine_(v, negative_vec_);
        probs.push_back(p);
        probs.push_back(n);
        if(p>n) return 1.0;
        else return -1.0;
    }

private:
    void GetVector_(const Instance& ins, Vector& dvec)
    {
        Vector vec;
        for(uint32_t i=0;i<ins.features.size();i++)
        {
            const Feature& f = ins.features[i];
            int id = f.id;
            if(id==0)
            {
                id = id_manager_->GetIdByText(f.text);
            }
            vec.push_back(std::make_pair(id, f.weight));
        }
        if(vec.empty()) return;
        std::sort(vec.begin(), vec.end());
        dvec.push_back(vec.front());
        for(uint32_t i=1;i<vec.size();i++)
        {
            if(vec[i].first==dvec.back().first)
            {
                dvec.back().second+=vec[i].second;
            }
            else
            {
                dvec.push_back(vec[i]);
            }
        }
        Normalize_(dvec);
    }
    static double Cosine_(const Vector& v1, const Vector& v2)
    {
        double value = 0.0;
        uint32_t i=0;
        uint32_t j=0;
        while(i<v1.size()&&j<v2.size())
        {
            if(v1[i].first<v2[j].first)
            {
                ++i;
            }
            else if(v1[i].first>v2[j].first)
            {
                ++j;
            }
            else
            {
                value += v1[i].second*v2[j].second;
                ++i;
                ++j;
            }
        }
        return value;
    }

    void Central_(const std::vector<Vector>& vector_list, Vector& centre)
    {
        std::vector<double> full(id_manager_->max+1, 0.0);
        for(uint32_t i=0;i<vector_list.size();i++)
        {
            const Vector& v = vector_list[i];
            for(uint32_t j=0;j<v.size();j++)
            {
                full[v[j].first]+=v[j].second;
            }
        }
        for(uint32_t i=0;i<full.size();i++)
        {
            if(full[i]>0.0)
            {
                centre.push_back(std::make_pair(i, full[i]));
            }
        }
        for(uint32_t i=0;i<centre.size();i++)
        {
            centre[i].second/=vector_list.size();
        }
        Normalize_(centre);
    }

    static void Normalize_(Vector& dvec)
    {
        double ssum = 0.0;
        for(uint32_t f=0;f<dvec.size();f++)
        {
            ssum += dvec[f].second;
        }
        ssum = std::sqrt(ssum);
        for(uint32_t f=0;f<dvec.size();f++)
        {
            dvec[f].second/=ssum;
        }
    }

    static void Merge_(Vector& v, const Vector& o)
    {
        Vector newv;
        uint32_t i=0;
        uint32_t j=0;
        while(i<v.size()&&j<o.size())
        {
            if(v[i].first<o[j].first)
            {
                newv.push_back(v[i++]);
            }
            else if(v[i].first>o[j].first)
            {
                newv.push_back(o[j++]);
            }
            else
            {
                newv.push_back(std::make_pair(v[i].first, v[i].second+o[j].second));
                ++i;
                ++j;
            }
        }
        while(i<v.size())
        {
            newv.push_back(v[i++]);
        }
        while(j<o.size())
        {
            newv.push_back(o[j++]);
        }
        v.swap(newv);
    }

private:

    std::string path_;
    std::string name_;
    Vector positive_vec_;
    Vector negative_vec_;
    std::vector<Vector> positive_list_;
    std::vector<Vector> negative_list_;
    IdManager* id_manager_;


};
}


#endif

