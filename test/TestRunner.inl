#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/unit_test.hpp>
#include <boost/test/output/xml_log_formatter.hpp>

#include "TestHelper.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

class MyXmlLogFormatter : public boost::unit_test::output::xml_log_formatter
{
    typedef boost::unit_test::output::xml_log_formatter super;

public:
    MyXmlLogFormatter()
    : outBackup_(0), errBackup_(0)
    {
        outBackup_ = std::cout.rdbuf(out_.rdbuf());
        errBackup_ = std::cerr.rdbuf(err_.rdbuf());
    }

    ~MyXmlLogFormatter()
    {
        if (outBackup_)
        {
            std::cout.rdbuf(outBackup_);
        }
        if (errBackup_)
        {
            std::cerr.rdbuf(errBackup_);
        }
    }

    void test_unit_start(std::ostream& ostr, boost::unit_test::test_unit const& tu)
    {
        out_.str("");
        err_.str("");
        super::test_unit_start(ostr, tu);
    }

    void test_unit_finish(std::ostream& ostr, boost::unit_test::test_unit const& tu, unsigned long elapsed)
    {
        if (tu.p_type == boost::unit_test::tut_case)
        {
            ostr << "<SystemOut><![CDATA[" << out_.str() << "]]></SystemOut>";
            ostr << "<SystemErr><![CDATA[" << err_.str() << "]]></SystemErr>";
        }

        super::test_unit_finish(ostr, tu, elapsed);
    }

private:
    std::ostringstream out_;
    std::ostringstream err_;
    std::streambuf* outBackup_;
    std::streambuf* errBackup_;
};

static std::ofstream* gOutStream = 0;

bool my_init_unit_test()
{
#ifdef BOOST_TEST_MODULE
    {
        using namespace ::boost::unit_test;
        assign_op( framework::master_test_suite().p_name.value, BOOST_TEST_STRINGIZE( BOOST_TEST_MODULE ).trim( "\"" ), 0 );
    }
#endif

    const char* const gEnvName = "BOOST_TEST_LOG_FILE";
    const char* const gEnvValue = std::getenv(gEnvName);

    if (gEnvValue)
    {
        gOutStream = new std::ofstream(gEnvValue);
        boost::unit_test::unit_test_log.set_stream(*gOutStream);

        boost::unit_test::unit_test_log.set_formatter(new MyXmlLogFormatter());
    }

    return true;
}

int BOOST_TEST_CALL_DECL
main(int argc, char* argv[])
{
    int status = boost::unit_test::unit_test_main(&my_init_unit_test, argc, argv);

    // delete global stream to flush
    delete gOutStream;

    return status;
}
