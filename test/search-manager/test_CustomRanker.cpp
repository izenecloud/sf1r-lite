/**
 * @file test_CustomRanker.cpp
 * @author Zhongxia Li
 * @date Apr 13, 2011
 * @brief test custom ranker
 */

#include <iostream>

#include <search-manager/CustomRanker.h>

using namespace std;
using namespace sf1r;

int main(int argc, char* argv[])
{
    cout << "\n=== Parser of Expression for Custom Ranking ===\n" << endl;
    cout << "[example] param1 + param2 * price + log(x)" << endl;
    cout << "parameter name: include alphabets, numbers  or '_', start with an alphabet or '_'." << endl;
    cout << "operator: +, -, *, /, log(x), sqrt(x), pow(x,y) " << endl;

    std::string exp;

    cout << "\ninput expression('x' to exit): " ;
    while (getline(cin, exp)) {

        CustomRanker customRanker(exp);

        if (!customRanker.parse()) {
            cout << customRanker.getErrorInfo() << endl;
        }
        else {
            customRanker.printESTree();
        }

        // exit
        if (exp == "x") {
            break;
        }
        cout << "\ninput expression('x' to exit): " ;
    }
    return 0;
}
