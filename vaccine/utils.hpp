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
};
