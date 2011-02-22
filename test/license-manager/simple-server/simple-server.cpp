#include <license-module/LicenseNetModule.h>
#include <license-module/LicenseServer.h>

#include <iostream>

void displayUsage()
{
    std::cout << "[Usage] LicenseServer PortNo [ThreadNo]" << std::endl;
    std::cout << "  PortNo\t\tPort Number of license Server service" << std::endl;
    std::cout << "  threadNo\t\tA number of concurrent queries[1~100]. Default is 3" << std::endl;
}


int main(int argc, char* argv[])
{
    if ( argc != 3 && argc != 2 )
    {
        displayUsage();
        return -1;
    }

    int threadNo(3);
    std::string portNo( argv[1] );

    if ( argc == 3 )
        threadNo = atoi( argv[2] );
    else
        threadNo = 3;

    // threadNo validation
    if ( threadNo < 1 || threadNo > 100 )
    {
        displayUsage();
        return -1;
    }

    license_module::LicenseNetServer<license_module::LicenseServer> licenseServer( portNo, threadNo );
    licenseServer.run();

    return 0;
} // end - main()


