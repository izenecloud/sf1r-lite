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
    while( reader.Next(fp) )
    {
        std::cout<<"["<<fp.docid<<"]{";
        fp.fp.display(std::cout);
        std::cout<<","<<fp.length<<"}"<<std::endl;
    }
    reader.Close();
}

