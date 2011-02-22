///
/// @brief  header file of Language Analysis Extention Information structure.
/// @author Dohyun Yun
/// @date   2010.06.24 (First Created)
/// 

#ifndef _LAEXINFO_H_
#define _LAEXINFO_H_

#include <la-manager/AnalysisInformation.h>

namespace sf1r {

    ///
    /// @brief a structure which is used to extend keyword query tree.
    ///
    struct LAEXInfo
    {
        LAEXInfo(bool unigramFlag, bool synonymExtension, const AnalysisInfo& analysisInfo):
            unigramFlag_(unigramFlag), synonymExtension_(synonymExtension), analysisInfo_(analysisInfo) {}

        ///
        /// @brief a flag whether unigram analyzed wildcard is used.
        ///
        bool unigramFlag_;

        ///
        /// @brief a flag whether synonym extension is used while analyzing.
        ///
        bool synonymExtension_;

        ///
        /// @brief an object which contains analyzing information.
        ///
        AnalysisInfo analysisInfo_;

    }; // end - struct LAEXInfo

} // end - namespace sf1r

#endif // _LAEXINFO_H_
