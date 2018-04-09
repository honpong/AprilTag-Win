/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: BrainStem-util.h                                          //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: BrainStem API's and support.                       //
//                                                                 //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// Copyright 1994-2015. Acroname Inc.                              //
//                                                                 //
// This software is the property of Acroname Inc.  Any             //
// distribution, sale, transmission, or re-use of this code is     //
// strictly forbidden except with permission from Acroname Inc.    //
//                                                                 //
// To the full extent allowed by law, Acroname Inc. also excludes  //
// for itself and its suppliers any liability, whether based in    //
// contract or tort (including negligence), for direct,            //
// incidental, consequential, indirect, special, or punitive       //
// damages of any kind, or for loss of revenue or profits, loss of //
// business, loss of information or data, or other financial loss  //
// arising out of or in connection with this software, even if     //
// Acroname Inc. has been advised of the possibility of such       //
// damages.                                                        //
//                                                                 //
// Acroname Inc.                                                   //
// www.acroname.com                                                //
// 720-564-0373                                                    //
//                                                                 //
/////////////////////////////////////////////////////////////////////

#ifndef __BrainStem_util_H__
#define __BrainStem_util_H__

#include "aDefs.h"
#include "aError.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef in_addr_t
#define in_addr_t uint32_t
#endif
#else // Not MSWindows
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // end platform ifdef.

namespace Acroname {
    
// MARK: - aSettings Class
    /////////////////////////////////////////////////////////////////////
    /// The Settings class provides a convenient way to access command line
    /// arguments and configuration variables. If Provided with a valid filename
    /// the Settings object will parse it for key = value pairs, and produce an
    /// associative array based on those configuration items it finds. Key value
    /// pairs can be added to the object via the API, and command line arguments
    /// can optionally be parsed to add to or override configuration file values.
    /// values in the array can be accessed by primitive type.
    ///
    /// The configuration file has a very simple structure. Comment lines are
    /// prepended with '//' and key value pairs are defined with an '='. For Example;
    /// 'key = value'
        
    class aLIBEXPORT Settings
    {
    public:
        /// Constructor.
        ///
        /// \param argc The number of arguments.
        /// \param argv An array of string arguments.
        /// \param filename The filename containing the settings. (Optional)
        Settings(const int argc, const char* argv[], const char* filename = NULL);
        
        ~Settings(void);
        
        /// Initializes the Settings object with a series of arguments in the argv
        /// style. For Example; -ip_address 127.0.0.1
        /// \param argc The number of arguments.
        /// \param argv An array of string arguments.
        /// \param filename The filename containing the settings.
        aErr init(const int argc, const char* argv[], const char* filename);
        
        /// Retrieve a boolean value associated with the key parameter.
        /// \param key - The name of the setting.
        /// \param defaultValue - The default value to return if there is
        /// no setting with that name.
        /// \return The value.
        bool getBool(const char* key,
                     const bool defaultValue);
        
        /// Retrieve a 32 bit signed integer value associated with the key parameter.
        /// \param key - The name of the setting.
        /// \param defaultValue - The default value to return if there is
        /// no setting with that name.
        /// \return The value.
        int getInt(const char* key,
                   const int defaultValue);
        
        /// Retrieve a string value associated with the key parameter.
        /// \param key - The name of the setting.
        /// \param defaultValue - The default value to return if there is
        /// no setting with that name.
        /// \return The value.
        const char* getString(const char* key,
                              const char* defaultValue);
        
        /// Retrieve a float value associated with the key parameter.
        /// \param key - The name of the setting.
        /// \param defaultValue - The default value to return if there is
        /// no setting with that name.
        /// \return The value.
        float getFloat(const char* key,
                       const float defaultValue);
        
        /// Retrieve an IPv4 (as an unsigned 32 bit integer) value associated with
        /// the key parameter.
        /// \param key - The name of the setting.
        /// \param defaultValue - The default value to return if there is
        /// no setting with that name.
        /// \return The value.
        in_addr_t getIP4Address(const char* key,
                                const in_addr_t defaultValue);
        
    private:
        class impl; impl* zit;
        
    };
    
    // MARK: - Model Class
    /////////////////////////////////////////////////////////////////////
    /// The Model class provides a convenient way to convert model ids to
    /// human readable strings.
    
    class aLIBEXPORT Model
    {
    public:
        /// Name.
        /// Converts an id to a human readable string.
        static const char* Name(const int id);
    };
} // namespace Acroname

#endif //__BrainStem_util_H__
