#include "bmn_clustering.cc"
bool Append(const Document& doc, const std::vector<std::string>& brands)
{
    std::string title;
    doc.getString("Title", title);
    std::string model;
    ExtractModel_(title, model);
    for(std:size_t i=0;i<brands.size();i++)
    {
        const std::string& brand = brands[i];
        Container* container = GetContainer_();
        container->Append(brand, model, doc);
    }
}
bool Finish(const Func& func)
{
    func_ = func;
    if(container_!=NULL) 
    {
        container_->Finish(boost::bind(&BmnClustering::Calculate_, _1));
        delete container_;
    }
}
void Calculate_(ValueArray& va)
{
    bool has_new = false;
    for(std::size_t i=0;i<va.size();i++)
    {
        Value& v = va[i];
        if(v.gid==0)
        {
            has_new = true;
        }
    }
    if(!has_new) return;
}

