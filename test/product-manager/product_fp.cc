#include <product-manager/product_clustering.h>

using namespace sf1r;
using namespace idmlib::dd;


int main(int argc, char** argv)
{
    typedef FpItem<std::string, ProductClusteringAttach> FpItemType;
    std::string path(argv[1]);
    izenelib::am::ssf::Reader<> reader(path);
    reader.Open();
    FpItemType fp;
    while (reader.Next(fp))
    {
        std::cout << "[" << fp.docid << "]{";
        std::ostringstream oss;
        oss << hex << setfill('0');
        for (uint32_t j = fp.fp.size() - 1; j >= 0; j--)
            oss << setw(16) << fp.fp[j];
        std::cout << oss.str() << "," << fp.length << "}" << std::endl;
    }
    reader.Close();
}
