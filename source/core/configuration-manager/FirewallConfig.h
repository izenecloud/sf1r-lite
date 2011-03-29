/**
 * @file    FirewallConfig.h
 * @brief   Defines the firewall-config class
 * @author  Dohyun Yun
 * @date    2009-11-23
 */

#ifndef _FIREWALLCONFIG_H_
#define _FIREWALLCONFIG_H_

#include <vector>

namespace sf1r
{

///
/// @brief The class containing the configuration data for firewall.
///
class FirewallConfig
{
public:
    FirewallConfig() {}
    ~FirewallConfig() {}

    ///
    /// @brief string list which stores allow IP list.
    ///
    std::vector<std::string>    allowIPList_;

    ///
    /// @brief string list which stores deny IP list.
    ///
    std::vector<std::string>    denyIPList_;

}; // end - class FirewallConfig

} // end - namespace sf1r

#endif // _FIREWALLCONFIG_H_
