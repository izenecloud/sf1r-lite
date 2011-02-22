/** 
 * @file TokenizerConfigUnit.h
 * @brief header file of TokenizerConfigUnit
 * @author MyungHyun (Kent)
 * @date 2008-07-07
 */


#if !defined(_REGULATION_CONFIG_UNIT_H_)
#define _REGULATION_CONFIG_UNIT_H_


#include <util/ustring/UString.h>

#include <boost/serialization/access.hpp>

#include <sstream>
#include <string>

namespace sf1r
{

    /**
     * @brief A TokenizerConfigUnit represents a single tokenizer setting ("<Tokenizer>") from the configuration file.
     */
    class TokenizerConfigUnit 
    {

        public:
            //------------------------  PUBLIC MEMBER FUNCTIONS  ------------------------

            TokenizerConfigUnit(){}

            /**
             * @brief Initializes the member variables
             *
             * @param id        The id(name) of the unit
             * @param method    The tokenizer method of the unit
             * @param value     The method of the tokenizer unit
             * @param code      The code of the tokenizer unit
             */
            TokenizerConfigUnit(
                    const std::string& id, 
                    const std::string& method, 
                    const std::string& value, 
                    const std::string& code)
                : id_( id ), method_( method ), value_( value ), code_( code )
            {}

            /**
            */
            ~TokenizerConfigUnit() {}

            /**
             * @breif Sets the ID(name) of the unit
             * @param id        The id(name) of the unit
             */
            //void setID( const std::string & id ) { id_ = id; }

            /**
             * @breif Gets the ID(name) of the unit
             */
            std::string getId() const { return id_; }


            /**
             * @breif Sets the method of the unit
             * @param method    The tokenizer method of the unit
             */
            //void setMethod( const std::string & method) { method_ = method; }

            /**
             * @breif Gets the method of the unit
             */
            std::string getMethod() const { return method_; }


            /**
             * @breif Sets the value of the unit
             * @param value     The method of the tokenizer unit
             */
            void setValue( const std::string & value ) { value_ = value; }

            /**
             * @breif Gets the value of the unit
             */
            //std::string getValue() const { return value_; }


            /**
             * @breif Sets the code of the unit
             * @param code      The code of the tokenizer unit
             */
            void setCode( const std::string & code ) { code_ = code; }

            /**
             * @breif Gets the code of the unit
             */
            //std::string getCode() const { return code_; }


            /**
             * @brief   Combines the chracters in "value" and "code" into a single UString
             */
            izenelib::util::UString getChars() const;



            /**
             * @brief   Puts the configuration into string
             */
            std::string toString()
            {
                std::stringstream ss;
                ss << *this;
                return ss.str();
            }



            /**
             * @brief
             */
            friend std::ostream & operator<<( std::ostream & out, TokenizerConfigUnit & unit )
            {
                out << "Tokenizer configuration: id=" << unit.id_ << " method=" << unit.method_ 
                    << " @(value=" << unit.value_ << " code=" << unit.code_ << ")";
                return out;
            }



        private:
            //------------------------  PRIVATE MEMBER FUNCTIONS  ------------------------

            friend class boost::serialization::access;

            template <class Archive>
                void serialize( Archive & ar, const unsigned int version ) 
                {
                    ar & id_;
                    ar & method_;
                    ar & value_;
                    ar & code_;
                }


        public:
            //------------------------  PRIVATE MEMBER VARIABLES  ------------------------

            /// @brief The identifier of this unit. Used to identify a unit in property setting of documents
            std::string id_;

            /// @brief The tokenizer method that this unit uses.
            std::string method_;

            /// @brief The value setting of a regulator. (optional) (See Regulator documentation) 
            std::string value_;

            /// @brief The code setting of a regulator. (optional) (Soo Regulator documentation)
            std::string code_;
    };

} // namespace

#endif  //_REGULATION_CONFIG_UNIT_H_

// eof
