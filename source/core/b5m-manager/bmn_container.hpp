#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>

class BmnContainer
{
public:
    struct Value
    {
        Value():gid(0)
        {
        }
        std::string brand;
        std::string model;
        Document doc;
        std::size_t gid;//group_id
        std::vector<std::string> common;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & brand & model & doc & gid & common;
        }
    };
    typedef std::vector<Value> ValueArray;
    BmnContainer(const std::string& path)
    : path_(path), file_(path+"/file"), file1_(path+"/file1"), file2_(path+"/file2")
    {
        if(!boost::filesystem::exists(path))
        {
            boost::filesystem::create_directories(path);
        }
        writer_.reset(new Writer(file1_));
    }
    void Append(const std::string& brand, const std::string& model, const Document& doc)
    {
        std::string skey = brand+"|"+model;
        std::size_t key = izenelib::util::HashFunction<std::string>::generateHash64(skey);
        FValue value(brand, model, doc);
        writer_->Append(key, value);
    }
    void Finish(const boost::function<ValueArray&>& func)
    {
        std::size_t write_count = writer_->Count();
        writer_->Close();
        if(write_count==0) return;
        namespace bfs = boost::filesystem;
        if(bfs::exists(file2_))
        {
            bfs::remove_all(file2_);
        }
        Writer writer(file2_);
        writer.Open();
        {
            Reader reader(writer_->GetPath());
            reader.Open();
            std::size_t key=0;
            Value value;
            while(reader.Next(key, value))
            {
                writer.Append(key, value);
            }
            reader.Close();
            bfs::remove_all(writer_->GetPath());
        }
        if(bfs::exists(file_))
        {
            Reader reader(file_);
            reader.Open();
            std::size_t key=0;
            Value fvalue;
            while(reader.Next(key, value))
            {
                writer.Append(key, value);
            }
            reader.Close();
            bfs::remove_all(file_);
        }
        writer.Close();
        Sorter sorter;
        sorter.Sort(file2_);
        Writer fwriter(file_);
        fwriter.Open();
        Reader reader(file2_);
        reader.Open();
        std::size_t key=0;
        Value value;
        std::size_t lkey=0;
        ValueArray va;
        while(reader.Next(key, value))
        {
            if(key!=lkey&&!va.empty())
            {
                func(va);
                for(std::size_t i=0;i<va.size();i++)
                {
                    fwriter.Append(lkey, va[i]);
                }
                va.resize(0);
            }
            lkey = key;
            va.push_back(value);
        }
        if(!va.empty()) 
        {
            func(va);
            for(std::size_t i=0;i<va.size();i++)
            {
                fwriter.Append(lkey, va[i]);
            }
        }
        reader.Close();
        fwriter.Close();
        bfs::remove_all(file2_);
    }
};

