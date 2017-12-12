/*******************************************************************************
  
 Copyright 2015-2018 Intel Corporation.
 
 This software and the related documents are Intel copyrighted materials,
 and your use of them is governed by the express license under which
 they were provided to you. Unless the License provides otherwise,
 you may not use, modify, copy, publish, distribute, disclose or transmit
 this software or the related documents without Intel's prior written permission.
 
 This software and the related documents are provided as is, with no express or
 implied warranties, other than those that are expressly stated in the License.
 
 *******************************************************************************/

#pragma once
#ifndef rs_box_api_h
#define rs_box_api_h

#include <exception>
#include <type_traits>
#define LOG_WARNING(...) do { CLOG(WARNING ,"boxsdk") << __VA_ARGS__; } while(false)

#include "librealsense2/rs.hpp"
#include "librealsense2/rsutil.h"

struct rs2_error
{
    std::string message;
    const char* function;
    std::string args;
    rs2_exception_type exception_type;
};

namespace boxsdk
{
    // Facilities for streaming function arguments

    // First, we define a generic parameter streaming
    // Assumes T is streamable, reasonable for C API parameters
    template<class T, bool S>
    struct arg_streamer
    {
        void stream_arg(std::ostream & out, const T& val, bool last)
        {
            out << ':' << val << (last ? "" : ", ");
        }
    };

    // Next we define type trait for testing if *t for some T* is streamable
    template<typename T>
    class is_streamable
    {
        template <typename S>
        static auto test(const S* t) -> decltype(std::cout << **t);
        static auto test(...)->std::false_type;
    public:
        enum { value = !std::is_same<decltype(test((T*)0)), std::false_type>::value };
    };

    // Using above trait, we can now specialize our streamer
    // for streamable pointers:
    template<class T>
    struct arg_streamer<T*, true>
    {
        void stream_arg(std::ostream & out, T* val, bool last)
        {
            out << ':'; // If pointer not null, stream its content
            if (val) out << *val;
            else out << "nullptr";
            out << (last ? "" : ", ");
        }
    };

    // .. and for not streamable pointers
    template<class T>
    struct arg_streamer<T*, false>
    {
        void stream_arg(std::ostream & out, T* val, bool last)
        {
            out << ':'; // If pointer is not null, stream the pointer
            if (val) out << (int*)val; // Go through (int*) to avoid dumping the content of char*
            else out << "nullptr";
            out << (last ? "" : ", ");
        }
    };

    // This facility allows for translation of exceptions to rs2_error structs at the API boundary
    template<class T> void stream_args(std::ostream & out, const char * names, const T & last)
    {
        out << names;
        arg_streamer<T, is_streamable<T>::value> s;
        s.stream_arg(out, last, true);
    }
    template<class T, class... U> void stream_args(std::ostream & out, const char * names, const T & first, const U &... rest)
    {
        while (*names && *names != ',') out << *names++;
        arg_streamer<T, is_streamable<T>::value> s;
        s.stream_arg(out, first, false);
        while (*names && (*names == ',' || isspace(*names))) ++names;
        stream_args(out, names, rest...);
    }

    static void translate_exception(const char * name, std::string args, rs2_error ** error)
    {
        try { throw; }
        //catch (const boxsdk_exception& e) { if (error) *error = new rs2_error{ e.what(), name, move(args), e.get_exception_type() }; }
        catch (const std::exception& e) { if (error) *error = new rs2_error{ e.what(), name, move(args) }; }
        catch (...) { if (error) *error = new rs2_error{ "unknown error", name, move(args) }; }
    }

    struct invalid_value_exception : public std::exception
    {
        invalid_value_exception(const std::string& msg) noexcept :
        _msg(msg) {}
        virtual const char* what() const noexcept override { return _msg.c_str(); }
        const std::string _msg;
    };

#define BEGIN_API_CALL try
#define NOEXCEPT_RETURN(R, ...) catch(...) { std::ostringstream ss; boxsdk::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rs2_error* e; boxsdk::translate_exception(__FUNCTION__, ss.str(), &e); LOG_WARNING(rs2_get_error_message(e)); rs2_free_error(e); return R; }
#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { std::ostringstream ss; boxsdk::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); boxsdk::translate_exception(__FUNCTION__, ss.str(), error); return R; }
#define NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { boxsdk::translate_exception(__FUNCTION__, "", error); return R; }

#define VALIDATE_NOT_NULL(ARG) if(!(ARG)) throw std::runtime_error("null pointer passed for argument \"" #ARG "\"");
#define VALIDATE_ENUM(ARG) if(!boxsdk::is_valid(ARG)) { std::ostringstream ss; ss << "invalid enum value for argument \"" #ARG "\""; throw boxsdk::invalid_value_exception(ss.str()); }
#define VALIDATE_RANGE(ARG, MIN, MAX) if((ARG) < (MIN) || (ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw boxsdk::invalid_value_exception(ss.str()); }
#define VALIDATE_LE(ARG, MAX) if((ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
#define VALIDATE_INTERFACE_NO_THROW(X, T)                                                   \
    ([&]() -> T* {                                                                              \
        T* p = dynamic_cast<T*>(&(*X));                                                         \
        if (p == nullptr)                                                                       \
        {                                                                                       \
            auto ext = dynamic_cast<boxsdk::extendable_interface*>(&(*X));                \
            if (ext == nullptr) return nullptr;                                                 \
            else                                                                                \
            {                                                                                   \
                if(!ext->extend_to(TypeToExtension<T>::value, (void**)&p))                      \
                    return nullptr;                                                             \
                return p;                                                                       \
            }                                                                                   \
        }                                                                                       \
        return p;                                                                               \
    })()

#define VALIDATE_INTERFACE(X,T)                                                             \
        ([&]() -> T* {                                                                          \
            T* p = VALIDATE_INTERFACE_NO_THROW(X,T);                                            \
            if(p == nullptr)                                                                    \
                throw std::runtime_error("Object does not support \"" #T "\" interface! " );    \
            return p;                                                                           \
        })()
}

#endif
