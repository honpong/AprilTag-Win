//
//  fast_9.h
//
//  Created by Eagle Jones on 6/9/17.
//  Based on FAST - https://www.edwardrosten.com/work/fast.html
//  This is mechanically generated code
//

/*
 Copyright (c) 2006, 2008, 2009, 2010 Edward Rosten
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 
	*Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
	*Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 
	*Neither the name of the University of Cambridge nor the names of
 its contributors may be used to endorse or promote products derived
 from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef fast_9_h
#define fast_9_h

#ifdef _MSC_VER
__forceinline
#else
inline __attribute__((always_inline))
#endif
bool fast_9_kernel(const unsigned char *p, const int pixel[], unsigned char val, int b)
{
    int cb = val + b;
    int c_b= val - b;

    if( p[pixel[0]] > cb)
         if( p[pixel[1]] > cb)
          if( p[pixel[2]] > cb)
           if( p[pixel[3]] > cb)
            if( p[pixel[4]] > cb)
             if( p[pixel[5]] > cb)
              if( p[pixel[6]] > cb)
               if( p[pixel[7]] > cb)
                if( p[pixel[8]] > cb)
                 return true;
                else
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  return false;
               else if( p[pixel[7]] < c_b)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  return false;
                else if( p[pixel[14]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[13]] < c_b)
                       if( p[pixel[15]] < c_b)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  return false;
                else
                 return false;
              else if( p[pixel[6]] < c_b)
               if( p[pixel[15]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  return true;
                 else
                  return false;
                else if( p[pixel[13]] < c_b)
                 if( p[pixel[7]] < c_b)
                  if( p[pixel[8]] < c_b)
                   if( p[pixel[9]] < c_b)
                    if( p[pixel[10]] < c_b)
                     if( p[pixel[11]] < c_b)
                      if( p[pixel[12]] < c_b)
                       if( p[pixel[14]] < c_b)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[13]] < c_b)
                       if( p[pixel[14]] < c_b)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  return false;
                else
                 return false;
               else if( p[pixel[13]] < c_b)
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[14]] < c_b)
                       if( p[pixel[15]] < c_b)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else if( p[pixel[5]] < c_b)
              if( p[pixel[14]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      if( p[pixel[10]] > cb)
                       if( p[pixel[11]] > cb)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 return false;
               else if( p[pixel[12]] < c_b)
                if( p[pixel[6]] < c_b)
                 if( p[pixel[7]] < c_b)
                  if( p[pixel[8]] < c_b)
                   if( p[pixel[9]] < c_b)
                    if( p[pixel[10]] < c_b)
                     if( p[pixel[11]] < c_b)
                      if( p[pixel[13]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else if( p[pixel[14]] < c_b)
               if( p[pixel[7]] < c_b)
                if( p[pixel[8]] < c_b)
                 if( p[pixel[9]] < c_b)
                  if( p[pixel[10]] < c_b)
                   if( p[pixel[11]] < c_b)
                    if( p[pixel[12]] < c_b)
                     if( p[pixel[13]] < c_b)
                      if( p[pixel[6]] < c_b)
                       return true;
                      else
                       if( p[pixel[15]] < c_b)
                        return true;
                       else
                        return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               if( p[pixel[6]] < c_b)
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[13]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      if( p[pixel[10]] > cb)
                       if( p[pixel[11]] > cb)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 return false;
               else
                return false;
              else if( p[pixel[12]] < c_b)
               if( p[pixel[7]] < c_b)
                if( p[pixel[8]] < c_b)
                 if( p[pixel[9]] < c_b)
                  if( p[pixel[10]] < c_b)
                   if( p[pixel[11]] < c_b)
                    if( p[pixel[13]] < c_b)
                     if( p[pixel[14]] < c_b)
                      if( p[pixel[6]] < c_b)
                       return true;
                      else
                       if( p[pixel[15]] < c_b)
                        return true;
                       else
                        return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else if( p[pixel[4]] < c_b)
             if( p[pixel[13]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      if( p[pixel[10]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      if( p[pixel[10]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                return false;
              else if( p[pixel[11]] < c_b)
               if( p[pixel[5]] < c_b)
                if( p[pixel[6]] < c_b)
                 if( p[pixel[7]] < c_b)
                  if( p[pixel[8]] < c_b)
                   if( p[pixel[9]] < c_b)
                    if( p[pixel[10]] < c_b)
                     if( p[pixel[12]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else if( p[pixel[13]] < c_b)
              if( p[pixel[7]] < c_b)
               if( p[pixel[8]] < c_b)
                if( p[pixel[9]] < c_b)
                 if( p[pixel[10]] < c_b)
                  if( p[pixel[11]] < c_b)
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[5]] < c_b)
                      return true;
                     else
                      if( p[pixel[14]] < c_b)
                       return true;
                      else
                       return false;
                    else
                     if( p[pixel[14]] < c_b)
                      if( p[pixel[15]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              if( p[pixel[5]] < c_b)
               if( p[pixel[6]] < c_b)
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             if( p[pixel[11]] > cb)
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      if( p[pixel[10]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      if( p[pixel[10]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                return false;
              else
               return false;
             else if( p[pixel[11]] < c_b)
              if( p[pixel[7]] < c_b)
               if( p[pixel[8]] < c_b)
                if( p[pixel[9]] < c_b)
                 if( p[pixel[10]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[5]] < c_b)
                      return true;
                     else
                      if( p[pixel[14]] < c_b)
                       return true;
                      else
                       return false;
                    else
                     if( p[pixel[14]] < c_b)
                      if( p[pixel[15]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
           else if( p[pixel[3]] < c_b)
            if( p[pixel[10]] > cb)
             if( p[pixel[11]] > cb)
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               return false;
             else
              return false;
            else if( p[pixel[10]] < c_b)
             if( p[pixel[7]] < c_b)
              if( p[pixel[8]] < c_b)
               if( p[pixel[9]] < c_b)
                if( p[pixel[11]] < c_b)
                 if( p[pixel[6]] < c_b)
                  if( p[pixel[5]] < c_b)
                   if( p[pixel[4]] < c_b)
                    return true;
                   else
                    if( p[pixel[12]] < c_b)
                     if( p[pixel[13]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                  else
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[13]] < c_b)
                     if( p[pixel[14]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                 else
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     if( p[pixel[15]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
            else
             return false;
           else
            if( p[pixel[10]] > cb)
             if( p[pixel[11]] > cb)
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     if( p[pixel[9]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               return false;
             else
              return false;
            else if( p[pixel[10]] < c_b)
             if( p[pixel[7]] < c_b)
              if( p[pixel[8]] < c_b)
               if( p[pixel[9]] < c_b)
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[4]] < c_b)
                     return true;
                    else
                     if( p[pixel[13]] < c_b)
                      return true;
                     else
                      return false;
                   else
                    if( p[pixel[13]] < c_b)
                     if( p[pixel[14]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                  else
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     if( p[pixel[15]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
            else
             return false;
          else if( p[pixel[2]] < c_b)
           if( p[pixel[9]] > cb)
            if( p[pixel[10]] > cb)
             if( p[pixel[11]] > cb)
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              return false;
            else
             return false;
           else if( p[pixel[9]] < c_b)
            if( p[pixel[7]] < c_b)
             if( p[pixel[8]] < c_b)
              if( p[pixel[10]] < c_b)
               if( p[pixel[6]] < c_b)
                if( p[pixel[5]] < c_b)
                 if( p[pixel[4]] < c_b)
                  if( p[pixel[3]] < c_b)
                   return true;
                  else
                   if( p[pixel[11]] < c_b)
                    if( p[pixel[12]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                 else
                  if( p[pixel[11]] < c_b)
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[13]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   if( p[pixel[14]] < c_b)
                    if( p[pixel[15]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               return false;
             else
              return false;
            else
             return false;
           else
            return false;
          else
           if( p[pixel[9]] > cb)
            if( p[pixel[10]] > cb)
             if( p[pixel[11]] > cb)
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    if( p[pixel[8]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              return false;
            else
             return false;
           else if( p[pixel[9]] < c_b)
            if( p[pixel[7]] < c_b)
             if( p[pixel[8]] < c_b)
              if( p[pixel[10]] < c_b)
               if( p[pixel[11]] < c_b)
                if( p[pixel[6]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[3]] < c_b)
                    return true;
                   else
                    if( p[pixel[12]] < c_b)
                     return true;
                    else
                     return false;
                  else
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[13]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                 else
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   if( p[pixel[14]] < c_b)
                    if( p[pixel[15]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                return false;
              else
               return false;
             else
              return false;
            else
             return false;
           else
            return false;
         else if( p[pixel[1]] < c_b)
          if( p[pixel[8]] > cb)
           if( p[pixel[9]] > cb)
            if( p[pixel[10]] > cb)
             if( p[pixel[11]] > cb)
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[2]] > cb)
               if( p[pixel[3]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             return false;
           else
            return false;
          else if( p[pixel[8]] < c_b)
           if( p[pixel[7]] < c_b)
            if( p[pixel[9]] < c_b)
             if( p[pixel[6]] < c_b)
              if( p[pixel[5]] < c_b)
               if( p[pixel[4]] < c_b)
                if( p[pixel[3]] < c_b)
                 if( p[pixel[2]] < c_b)
                  return true;
                 else
                  if( p[pixel[10]] < c_b)
                   if( p[pixel[11]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[10]] < c_b)
                  if( p[pixel[11]] < c_b)
                   if( p[pixel[12]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[10]] < c_b)
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[10]] < c_b)
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   if( p[pixel[14]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[10]] < c_b)
               if( p[pixel[11]] < c_b)
                if( p[pixel[12]] < c_b)
                 if( p[pixel[13]] < c_b)
                  if( p[pixel[14]] < c_b)
                   if( p[pixel[15]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             return false;
           else
            return false;
          else
           return false;
         else
          if( p[pixel[8]] > cb)
           if( p[pixel[9]] > cb)
            if( p[pixel[10]] > cb)
             if( p[pixel[11]] > cb)
              if( p[pixel[12]] > cb)
               if( p[pixel[13]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[15]] > cb)
                  return true;
                 else
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[2]] > cb)
               if( p[pixel[3]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[7]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             return false;
           else
            return false;
          else if( p[pixel[8]] < c_b)
           if( p[pixel[7]] < c_b)
            if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[6]] < c_b)
               if( p[pixel[5]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[2]] < c_b)
                   return true;
                  else
                   if( p[pixel[11]] < c_b)
                    return true;
                   else
                    return false;
                 else
                  if( p[pixel[11]] < c_b)
                   if( p[pixel[12]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   if( p[pixel[14]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[11]] < c_b)
                if( p[pixel[12]] < c_b)
                 if( p[pixel[13]] < c_b)
                  if( p[pixel[14]] < c_b)
                   if( p[pixel[15]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              return false;
            else
             return false;
           else
            return false;
          else
           return false;
        else if( p[pixel[0]] < c_b)
         if( p[pixel[1]] > cb)
          if( p[pixel[8]] > cb)
           if( p[pixel[7]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[6]] > cb)
              if( p[pixel[5]] > cb)
               if( p[pixel[4]] > cb)
                if( p[pixel[3]] > cb)
                 if( p[pixel[2]] > cb)
                  return true;
                 else
                  if( p[pixel[10]] > cb)
                   if( p[pixel[11]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[10]] > cb)
                  if( p[pixel[11]] > cb)
                   if( p[pixel[12]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[10]] > cb)
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[10]] > cb)
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   if( p[pixel[14]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[10]] > cb)
               if( p[pixel[11]] > cb)
                if( p[pixel[12]] > cb)
                 if( p[pixel[13]] > cb)
                  if( p[pixel[14]] > cb)
                   if( p[pixel[15]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             return false;
           else
            return false;
          else if( p[pixel[8]] < c_b)
           if( p[pixel[9]] < c_b)
            if( p[pixel[10]] < c_b)
             if( p[pixel[11]] < c_b)
              if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[2]] < c_b)
               if( p[pixel[3]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             return false;
           else
            return false;
          else
           return false;
         else if( p[pixel[1]] < c_b)
          if( p[pixel[2]] > cb)
           if( p[pixel[9]] > cb)
            if( p[pixel[7]] > cb)
             if( p[pixel[8]] > cb)
              if( p[pixel[10]] > cb)
               if( p[pixel[6]] > cb)
                if( p[pixel[5]] > cb)
                 if( p[pixel[4]] > cb)
                  if( p[pixel[3]] > cb)
                   return true;
                  else
                   if( p[pixel[11]] > cb)
                    if( p[pixel[12]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                 else
                  if( p[pixel[11]] > cb)
                   if( p[pixel[12]] > cb)
                    if( p[pixel[13]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   if( p[pixel[14]] > cb)
                    if( p[pixel[15]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               return false;
             else
              return false;
            else
             return false;
           else if( p[pixel[9]] < c_b)
            if( p[pixel[10]] < c_b)
             if( p[pixel[11]] < c_b)
              if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              return false;
            else
             return false;
           else
            return false;
          else if( p[pixel[2]] < c_b)
           if( p[pixel[3]] > cb)
            if( p[pixel[10]] > cb)
             if( p[pixel[7]] > cb)
              if( p[pixel[8]] > cb)
               if( p[pixel[9]] > cb)
                if( p[pixel[11]] > cb)
                 if( p[pixel[6]] > cb)
                  if( p[pixel[5]] > cb)
                   if( p[pixel[4]] > cb)
                    return true;
                   else
                    if( p[pixel[12]] > cb)
                     if( p[pixel[13]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                  else
                   if( p[pixel[12]] > cb)
                    if( p[pixel[13]] > cb)
                     if( p[pixel[14]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                 else
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     if( p[pixel[15]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
            else if( p[pixel[10]] < c_b)
             if( p[pixel[11]] < c_b)
              if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               return false;
             else
              return false;
            else
             return false;
           else if( p[pixel[3]] < c_b)
            if( p[pixel[4]] > cb)
             if( p[pixel[13]] > cb)
              if( p[pixel[7]] > cb)
               if( p[pixel[8]] > cb)
                if( p[pixel[9]] > cb)
                 if( p[pixel[10]] > cb)
                  if( p[pixel[11]] > cb)
                   if( p[pixel[12]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[5]] > cb)
                      return true;
                     else
                      if( p[pixel[14]] > cb)
                       return true;
                      else
                       return false;
                    else
                     if( p[pixel[14]] > cb)
                      if( p[pixel[15]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else if( p[pixel[13]] < c_b)
              if( p[pixel[11]] > cb)
               if( p[pixel[5]] > cb)
                if( p[pixel[6]] > cb)
                 if( p[pixel[7]] > cb)
                  if( p[pixel[8]] > cb)
                   if( p[pixel[9]] > cb)
                    if( p[pixel[10]] > cb)
                     if( p[pixel[12]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      if( p[pixel[10]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      if( p[pixel[10]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                return false;
              else
               return false;
             else
              if( p[pixel[5]] > cb)
               if( p[pixel[6]] > cb)
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else if( p[pixel[4]] < c_b)
             if( p[pixel[5]] > cb)
              if( p[pixel[14]] > cb)
               if( p[pixel[7]] > cb)
                if( p[pixel[8]] > cb)
                 if( p[pixel[9]] > cb)
                  if( p[pixel[10]] > cb)
                   if( p[pixel[11]] > cb)
                    if( p[pixel[12]] > cb)
                     if( p[pixel[13]] > cb)
                      if( p[pixel[6]] > cb)
                       return true;
                      else
                       if( p[pixel[15]] > cb)
                        return true;
                       else
                        return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else if( p[pixel[14]] < c_b)
               if( p[pixel[12]] > cb)
                if( p[pixel[6]] > cb)
                 if( p[pixel[7]] > cb)
                  if( p[pixel[8]] > cb)
                   if( p[pixel[9]] > cb)
                    if( p[pixel[10]] > cb)
                     if( p[pixel[11]] > cb)
                      if( p[pixel[13]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      if( p[pixel[10]] < c_b)
                       if( p[pixel[11]] < c_b)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 return false;
               else
                return false;
              else
               if( p[pixel[6]] > cb)
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[13]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else if( p[pixel[5]] < c_b)
              if( p[pixel[6]] > cb)
               if( p[pixel[15]] < c_b)
                if( p[pixel[13]] > cb)
                 if( p[pixel[7]] > cb)
                  if( p[pixel[8]] > cb)
                   if( p[pixel[9]] > cb)
                    if( p[pixel[10]] > cb)
                     if( p[pixel[11]] > cb)
                      if( p[pixel[12]] > cb)
                       if( p[pixel[14]] > cb)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  return true;
                 else
                  return false;
                else
                 return false;
               else
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[13]] > cb)
                       if( p[pixel[14]] > cb)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else if( p[pixel[6]] < c_b)
               if( p[pixel[7]] > cb)
                if( p[pixel[14]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[13]] > cb)
                       if( p[pixel[15]] > cb)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  return false;
                else
                 return false;
               else if( p[pixel[7]] < c_b)
                if( p[pixel[8]] < c_b)
                 return true;
                else
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  return false;
               else
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[13]] > cb)
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[14]] > cb)
                       if( p[pixel[15]] > cb)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[12]] > cb)
               if( p[pixel[7]] > cb)
                if( p[pixel[8]] > cb)
                 if( p[pixel[9]] > cb)
                  if( p[pixel[10]] > cb)
                   if( p[pixel[11]] > cb)
                    if( p[pixel[13]] > cb)
                     if( p[pixel[14]] > cb)
                      if( p[pixel[6]] > cb)
                       return true;
                      else
                       if( p[pixel[15]] > cb)
                        return true;
                       else
                        return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      if( p[pixel[10]] < c_b)
                       if( p[pixel[11]] < c_b)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             if( p[pixel[11]] > cb)
              if( p[pixel[7]] > cb)
               if( p[pixel[8]] > cb)
                if( p[pixel[9]] > cb)
                 if( p[pixel[10]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[5]] > cb)
                      return true;
                     else
                      if( p[pixel[14]] > cb)
                       return true;
                      else
                       return false;
                    else
                     if( p[pixel[14]] > cb)
                      if( p[pixel[15]] > cb)
                       return true;
                      else
                       return false;
                     else
                      return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else if( p[pixel[11]] < c_b)
              if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      if( p[pixel[10]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      if( p[pixel[10]] < c_b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                return false;
              else
               return false;
             else
              return false;
           else
            if( p[pixel[10]] > cb)
             if( p[pixel[7]] > cb)
              if( p[pixel[8]] > cb)
               if( p[pixel[9]] > cb)
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[6]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[4]] > cb)
                     return true;
                    else
                     if( p[pixel[13]] > cb)
                      return true;
                     else
                      return false;
                   else
                    if( p[pixel[13]] > cb)
                     if( p[pixel[14]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                  else
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     if( p[pixel[15]] > cb)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
            else if( p[pixel[10]] < c_b)
             if( p[pixel[11]] < c_b)
              if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     if( p[pixel[9]] < c_b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               return false;
             else
              return false;
            else
             return false;
          else
           if( p[pixel[9]] > cb)
            if( p[pixel[7]] > cb)
             if( p[pixel[8]] > cb)
              if( p[pixel[10]] > cb)
               if( p[pixel[11]] > cb)
                if( p[pixel[6]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[3]] > cb)
                    return true;
                   else
                    if( p[pixel[12]] > cb)
                     return true;
                    else
                     return false;
                  else
                   if( p[pixel[12]] > cb)
                    if( p[pixel[13]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                 else
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   if( p[pixel[14]] > cb)
                    if( p[pixel[15]] > cb)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                return false;
              else
               return false;
             else
              return false;
            else
             return false;
           else if( p[pixel[9]] < c_b)
            if( p[pixel[10]] < c_b)
             if( p[pixel[11]] < c_b)
              if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    if( p[pixel[8]] < c_b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              return false;
            else
             return false;
           else
            return false;
         else
          if( p[pixel[8]] > cb)
           if( p[pixel[7]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[6]] > cb)
               if( p[pixel[5]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[3]] > cb)
                  if( p[pixel[2]] > cb)
                   return true;
                  else
                   if( p[pixel[11]] > cb)
                    return true;
                   else
                    return false;
                 else
                  if( p[pixel[11]] > cb)
                   if( p[pixel[12]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   if( p[pixel[14]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[11]] > cb)
                if( p[pixel[12]] > cb)
                 if( p[pixel[13]] > cb)
                  if( p[pixel[14]] > cb)
                   if( p[pixel[15]] > cb)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              return false;
            else
             return false;
           else
            return false;
          else if( p[pixel[8]] < c_b)
           if( p[pixel[9]] < c_b)
            if( p[pixel[10]] < c_b)
             if( p[pixel[11]] < c_b)
              if( p[pixel[12]] < c_b)
               if( p[pixel[13]] < c_b)
                if( p[pixel[14]] < c_b)
                 if( p[pixel[15]] < c_b)
                  return true;
                 else
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[3]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[2]] < c_b)
               if( p[pixel[3]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[6]] < c_b)
                   if( p[pixel[7]] < c_b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             return false;
           else
            return false;
          else
           return false;
        else
         if( p[pixel[7]] > cb)
          if( p[pixel[8]] > cb)
           if( p[pixel[9]] > cb)
            if( p[pixel[6]] > cb)
             if( p[pixel[5]] > cb)
              if( p[pixel[4]] > cb)
               if( p[pixel[3]] > cb)
                if( p[pixel[2]] > cb)
                 if( p[pixel[1]] > cb)
                  return true;
                 else
                  if( p[pixel[10]] > cb)
                   return true;
                  else
                   return false;
                else
                 if( p[pixel[10]] > cb)
                  if( p[pixel[11]] > cb)
                   return true;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[10]] > cb)
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[10]] > cb)
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[10]] > cb)
               if( p[pixel[11]] > cb)
                if( p[pixel[12]] > cb)
                 if( p[pixel[13]] > cb)
                  if( p[pixel[14]] > cb)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
           else
            return false;
          else
           return false;
         else if( p[pixel[7]] < c_b)
          if( p[pixel[8]] < c_b)
           if( p[pixel[9]] < c_b)
            if( p[pixel[6]] < c_b)
             if( p[pixel[5]] < c_b)
              if( p[pixel[4]] < c_b)
               if( p[pixel[3]] < c_b)
                if( p[pixel[2]] < c_b)
                 if( p[pixel[1]] < c_b)
                  return true;
                 else
                  if( p[pixel[10]] < c_b)
                   return true;
                  else
                   return false;
                else
                 if( p[pixel[10]] < c_b)
                  if( p[pixel[11]] < c_b)
                   return true;
                  else
                   return false;
                 else
                  return false;
               else
                if( p[pixel[10]] < c_b)
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if( p[pixel[10]] < c_b)
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if( p[pixel[10]] < c_b)
               if( p[pixel[11]] < c_b)
                if( p[pixel[12]] < c_b)
                 if( p[pixel[13]] < c_b)
                  if( p[pixel[14]] < c_b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
           else
            return false;
          else
           return false;
         else
          return false;
}

#endif /* fast_9_h */
