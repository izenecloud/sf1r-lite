#include <iostream>
#include <license-module/LicenseNetModule.h>

int main(int argc, char* argv[])
{
    if ( argc != 3 )
    {
        std::cerr << "[Usage] simple-client ServerIP ServerPort" << std::endl;
        return -1;
    }

    std::string serverIP(argv[1]), serverPort(argv[2]);
    license_module::LicenseNetClient licenseClient(serverIP, serverPort);

    size_t inputSize, outputSize;
    LICENSE_DATA_T inputData, outputData;

    inputSize = 12;
    inputData.reset( new unsigned char[12] );
    inputData[0] = 0x01;
    for(size_t i = 1; i < 12; i ++)
        inputData[i] = inputData[i-1] + 0x02;

    licenseClient.request(inputSize, inputData, outputSize, outputData);

    std::cout << "Server Output size : " << outputSize << std::endl;
    std::cout << "Server Output data : ";
    for(size_t i = 0; i < outputSize; i ++)
        printf("%02X ", outputData[i]);
    std::cout << std::endl;

} // end - main()
