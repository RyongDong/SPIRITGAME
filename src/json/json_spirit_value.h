#ifndef JSON_SPIRIT_VALUE
#define JSON_SPIRIT_VALUE

//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.03

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <boost/config.hpp> 
#include <boost/cstdint.hpp> 
#include <boost/shared_ptr.hpp> 
#include <boost/variant.hpp> 

namespace json_spirit
{
    enum Value_type{ obj_type, array_type, str_type, bool_type, int_type, real_type, null_type };
    static const char* Value_type_name[]={"obj", "array", "str", "bool", "int", "real", "null"};

    template< class Config >    // Config determines whether the value uses std::string or std::wstring and
                                // whether JSON Objects are represented as vectors or maps
    class Value_impl
    {
    public:

        typedef Config Config_type;
        typedef typename Config::String_type String_type;
        typedef typename Config::Object_type Object;
        typedef typename Config::Array_type Array;
        typedef typename String_type::const_pointer Const_str_ptr;  // eg const char*

        Value_impl();  // creates null value
        Value_impl( Const_str_ptr      value ); 
        Value_impl( const String_type& value );
        Value_impl( const Object&      value );
        Value_impl( const Array&       value );
        Value_impl( bool               value );
        Value_impl( int                value );
        Value_impl( boost::int64_t     value );
        Value_impl( boost::uint64_t    value );
        Value_impl( double             value );

        Value_impl( const Value_impl& other );

        bool operator==( const Value_impl& lhs ) const;

        Value_impl& operator=( const Value_impl& lhs );

        Value_type type() const;

        bool is_uint64() const;
        bool is_null() const;

        const String_type& get_str()    const;
        const Object&      get_obj()    const;
        const Array&       get_array()  const;
        bool               get_bool()   const;
        int                get_int()    const;
        boost::int64_t     get_int64()  const;
        boost::uint64_t    get_uint64() const;
        double             get_real()   const;

        Object& get_obj();
        Array&  get_array();

        template< typename T > T get_value() const;  // example usage: int    i = value.get_value< int >();
                                                     // or             double d = value.get_value< double >();

        static const Value_impl null;

    private:

        void check_type( const Value_type vtype ) const;

        typedef boost::variant< String_type, 
                                boost::recursive_wrapper< Object >, boost::recursive_wrapper< Array >, 
                                bool, boost::int64_t, double > Variant;

        Value_type type_;
        Variant v_;
        bool is_uint64_;
    };

    // vector objects

    template< class Config >
    struct Pair_impl
    {
        typedef typename Config::String_type String_type;
        typedef typename Config::Value_type Value_type;

        Pair_impl( const String_type& name, const Value_type& value );

        bool operator==( const Pair_impl& lhs ) const;

        String_type name_;
        Value_type value_;
    };

    template< class String >
    struct Config_vector
    {
        typedef String String_type;
        typedef Value_impl< Config_vector > Value_type;
        typedef Pair_impl < Config_vector > Pair_type;
        typedef std::vector< Value_type > Array_type;
        typedef std::vector< Pair_type > Object_type;

        static Value_type& add( Object_type& obj, const String_type& name, const Value_type& value )
        {
            obj.push_back( Pair_type( name , value ) );

            return obj.back().value_;
        }
                
        static String_type get_name( const Pair_type& pair )
        {
            return pair.name_;
        }
                
        static Value_type get_value( const Pair_type& pair )
        {
            return pair.value_;
        }
    };

    // typedefs for ASCII

    typedef Config_vector< std::string > Config;

    typedef Config::Value_type  Value;
    typedef Config::Pair_type   Pair;
    typedef Config::Object_type Object;
    typedef Config::Array_type  Array;

    // typedefs for Unicode

#ifndef BOOST_NO_STD_WSTRING

    typedef Config_vector< std::wstring > wConfig;

    typedef wConfig::Value_type  wValue;
    typedef wConfig::Pair_type   wPair;
    typedef wConfig::Object_type wObject;
    typedef wConfig::Array_type  wArray;
#endif

    // map objects

    template< class String >