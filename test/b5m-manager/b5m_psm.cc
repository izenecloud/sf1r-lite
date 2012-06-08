#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>

struct PriceAttach
{
    double price;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & price;
    }

    bool dd(const PriceAttach& other) const
    {
        static const double ratio = 0.7;
        double max = std::max(price, other.price);
        double min = std::min(price, other.price);
        double r = min/max;
        if(r>=ratio) return true;
        return false;
    }
};

int main(int ac, char** av)
{
    //std::string p1(av[1]);
    //double w1 = boost::lexical_cast<double>(p1);
    typedef idmlib::dd::PSM<64, 3, 24, std::string, std::string, PriceAttach> PSMType;
    std::string tmp_path = "./psm_test";
    boost::filesystem::remove_all(tmp_path);
    boost::filesystem::create_directories(tmp_path);
    PSMType psm(tmp_path);
    psm.Open();
    std::vector<std::pair<std::string, double> > doc_vector;
    doc_vector.push_back(std::make_pair("abc", 0.2));
    doc_vector.push_back(std::make_pair("123", 0.5));
    doc_vector.push_back(std::make_pair("zxc", 0.3));
    doc_vector.push_back(std::make_pair("ertreter", 0.3));
    doc_vector.push_back(std::make_pair("qweqwasd", 0.3));
    doc_vector.push_back(std::make_pair("qweasdawe", 0.3));
    doc_vector.push_back(std::make_pair("1htertert", 0.8));
    std::string key = "001";
    PriceAttach attach;
    attach.price = 1.0;
    psm.Insert(key, doc_vector, attach);
    if(!psm.Build())
    {
        std::cerr<<"psm build error"<<std::endl;
        return EXIT_FAILURE;
    }
    doc_vector[6].second = 0.7;
    if(!psm.Search(doc_vector, attach, key))
    {
        std::cout<<"search not found";
    }
    else
    {
        std::cout<<"find key : "<<key<<std::endl;
    }
    return EXIT_SUCCESS;
}

