#pragma once

#include <string>
#include <vector>

#include "../deps/mongoose/mongoose.h"

namespace vaccine {

  inline std::vector<std::string> split(const char *str, const char c = ' ')
  {
    std::vector<std::string> result;

    if ( str ) 
    {
      do
      {
        const char *begin = str;

        while(*str != c && *str)
          str++;

        if ( begin != str )
          result.push_back(std::string(begin, str));

      } while (0 != *str++);
    }

    return result;
  }

  inline std::vector<std::string> split(const mg_str & str, const char c = ' ')
  {
    std::string tmp(str.p, str.len);

    return split( tmp.c_str(), c);
  }

  inline std::string get_until_char(std::string const& s, char c, size_t p = 0)
  {
    std::string::size_type pos = s.find(c,p);
    if (pos != std::string::npos)
    {
      return s.substr(0, pos);
    }
    else
    {
      return s;
    }
  }

  // TODO: not covered
  inline bool is_mg_str_equal(const mg_str & str1, const char * str2)
  {
    return (strncmp(str1.p, str2, str1.len) == 0);
  }

  // TODO: not covered
  inline bool is_request_method(http_message * hc, const char * method)
  {
    return is_mg_str_equal(hc->method, method);
  }


  // copy paste from stack overflow: should work lolo
  inline std::string urldecode2(const char *src)
  {
    char a, b;
    std::string ret;
    char *dst = strdup(src), *cret = dst;

    while (*src) {
      if ((*src == '%') &&
          ((a = src[1]) && (b = src[2])) &&
          (isxdigit(a) && isxdigit(b))) {
        if (a >= 'a')
          a -= 'a'-'A';
        if (a >= 'A')
          a -= ('A' - 10);
        else
          a -= '0';
        if (b >= 'a')
          b -= 'a'-'A';
        if (b >= 'A')
          b -= ('A' - 10);
        else
          b -= '0';
        *dst++ = 16*a+b;
        src+=3;
      } else if (*src == '+') {
        *dst++ = ' ';
        src++;
      } else {
        *dst++ = *src++;
      }
    }
    *dst++ = '\0';
    ret = cret;

    return ret;
  }


  inline std::string urldecode2(const std::string src) {
    return urldecode2(src.c_str());
  }


};
