#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>

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

int main()
{
    typedef idmlib::dd::PSM<64, 3, std::string, izenelib::util::UString, PriceAttach> PSMType;
    PSMType psm("./psm_test");
    psm.Open();
    std::vector<std::pair<izenelib::util::UString, double> > doc_vector;
    std::string key = "001";
    PriceAttach attach;
    attach.price = 1.0;
    psm.Insert(key, doc_vector, attach);
    psm.Build();
    psm.Search(doc_vector, attach, key);
}

