#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>
#include <util/ustring/UString.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>
#include <b5m-manager/comment_db.h>
#include <b5m-manager/product_db.h>
#include <util/ustring/UString.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
using namespace sf1r;
using namespace std;
using namespace boost;

//BOOST_AUTO_TEST_SUITE(cdbffd)


//BOOST_AUTO_TEST_CASE(cdbInsert)
int main()
{
    ProductProperty pp;
    Document doc;
    string spid="7bc999f5d10830d0c59487bd48a73cae",soid="46c999f5d10830d0c59487bd48adce8a",source="SourceA",date="20130301",price="1000~3213",attribute="产地:中国,质量:优";//spid和soid反了
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    izenelib::util::UString pid(spid,izenelib::util::UString::UTF_8);
    doc.property("DOCID") = pid;
    izenelib::util::UString usource(source, izenelib::util::UString::UTF_8);
    izenelib::util::UString oid(soid,izenelib::util::UString::UTF_8);
    doc.property("uuid") = oid;
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);;
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);;

    bool independent=false;




    {
        pp.Parse(doc);
    }
  
    ProductProperty another;
    spid="12345f5d10830d0c59487bd48a73cae",soid="46c999f5d10830d0c59487bd48adce8a",source="SourceB",date="20130229",price="100~1213",attribute="产地:美国,质量:优,品牌:阿迪王";
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(spid,izenelib::util::UString::UTF_8);
    doc.property("uuid") =     izenelib::util::UString(soid,izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);
    another.Parse(doc);
    std::cout<<pp.GetSourceString()<<endl;

   izenelib::util::UString AttributeUString=pp.GetAttributeUString();
   
    string Attribute;
    AttributeUString.convertString(Attribute, izenelib::util::UString::UTF_8);

    cout<<Attribute<<endl;
    std::cout<<"AddbeforToString"<<pp.ToString()<<endl;
    pp+=another;


    std::cout<<"ToString"<<pp.ToString()<<endl;
    izenelib::util::UString ua=pp.GetAttributeUString();
    ua.convertString(attribute, izenelib::util::UString::UTF_8);
    std::cout<<"GetAttributeUString"<<attribute<<endl;
    ProductProperty third;
    spid="12345f5d54433d0c59487bd48a73cae",soid="46c999f5d10830d0c59487bd48adce8a",source="SourceC",date="20130529",price="5000~1000213",attribute="产地:美国,质量:差,品牌:阿迪王";
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(spid,izenelib::util::UString::UTF_8);
    doc.property("uuid") =     izenelib::util::UString(soid,izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);
    third.Parse(doc);
    third+=another;



    std::cout<<"ToString"<<third.ToString()<<endl;
    izenelib::util::UString ua1=third.GetAttributeUString();
    ua1.convertString(attribute, izenelib::util::UString::UTF_8);
    std::cout<<"GetAttributeUString"<<attribute<<endl;

    pp+=third;
    std::cout<<"ToString"<<pp.ToString()<<endl;
    izenelib::util::UString ua2=pp.GetAttributeUString();
    ua2.convertString(attribute, izenelib::util::UString::UTF_8);
    std::cout<<"GetAttributeUString"<<attribute<<endl;
    //pp.Set(value.doc);

}


//BOOST_AUTO_TEST_SUITE_END()

