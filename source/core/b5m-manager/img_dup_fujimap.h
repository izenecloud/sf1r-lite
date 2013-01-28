/*
 *    @file img_dup_fujimap.h
 *    @author Kuilong Liu
 *    @date 2013.01.08
 */
#ifndef SF1R_B5MMANAGER_IMG_DUP_FUJIMAP_H_
#define SF1R_B5MMANAGER_IMG_DUP_FUJIMAP_H_

#include <string>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>

namespace sf1r{
    /*
     *    KeyType:     uint32_t
     *    ValueType:   uint32_t
     */
    class ImgDupFujiMap
    {
        typedef uint32_t KeyType;
        typedef uint32_t ValueType;
        typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, ValueType> ImgDupFujiType;
        ImgDupFujiType* imgDupFujiMap;
        /*
         * A name pf working file which stores temporary data
         */
        std::string path_;

        /*
         * fplen: A negative of false positive rate power
         *        prob. of false positive rate is 2^{-fplen}
         */
        uint64_t fplen;
        /*
         * A succinct and static map will be constructed after every tmpN_ key/values added
         * tmpN: A size of temporary map
         */
        uint64_t tmpN;
    public:
        ImgDupFujiMap(const std::string& path)
            :path_(path)
            ,fplen(32)
            ,tmpN(30000000)
        {
        }
        ~ImgDupFujiMap()
        {
            if(imgDupFujiMap != NULL)
            {
                delete imgDupFujiMap;
            }
        }

        bool open()
        {
            imgDupFujiMap = new ImgDupFujiType(path_.c_str());
            imgDupFujiMap->initFP(fplen);
            imgDupFujiMap->initTmpN(tmpN);
            return true;
        }

        bool get(const KeyType& key, ValueType& value) const
        {
            value = imgDupFujiMap->getInteger(key);
            if(value == (ValueType)izenelib::am::succinct::fujimap::NOTFOUND)
            {
                return false;
            }
            return true;
        }
        bool insert(const KeyType& key, const ValueType& value)
        {
            ValueType evalue = imgDupFujiMap->getInteger(key);
            if(evalue==value)
            {
                return false;
            }
            imgDupFujiMap->setInteger(key, value);
            return true;
        }
        bool update(const KeyType& key, const ValueType& value)
        {
            imgDupFujiMap->setInteger(key, value);
            return true;
        }
        bool close()
        {
            if(imgDupFujiMap != NULL)
            {
                delete imgDupFujiMap;
            }
            return true;
        }
        int build()
        {
            return imgDupFujiMap->build();
        }
        bool empty()
        {
            return imgDupFujiMap->empty();
        }
        void clear()
        {
            imgDupFujiMap->clear();
        }
        std::string what()
        {
            return imgDupFujiMap->what();
        }
        int save(const std::string& index)
        {
            return imgDupFujiMap->save(index.c_str());
        }
        int load(const std::string& index)
        {
            return imgDupFujiMap->load(index.c_str());
        }
    };
}

#endif //SF1R_B5MMANAGER_IMG_DUP_FUJIMAP_H_
