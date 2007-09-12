#ifndef __ARC_REGEX_H__
#define __ARC_REGEX_H__

#include <list>
#include <string>
#include <regex.h>

namespace Arc {

  //! A regular expression class.
  /*! This class is a wrapper around the functions provided in
    regex.h.
  */
  class RegularExpression {
  public:

    //! Creates a reges from a pattern string.
    RegularExpression(std::string pattern);
    
    //! Copy constructor.
    RegularExpression(const RegularExpression& regex);

    //! Destructor
    ~RegularExpression();

    //! Assignment operator.
    const RegularExpression& operator=(const RegularExpression& regex);

    //! Returns true if the pattern of this regex is ok.
    bool isOk();

    //! Returns true if this regex has the pattern provided.
    bool hasPattern(std::string str);

    //! Returns true if this regex matches whole string provided.
    bool match(const std::string& str) const;
    
    //! Returns true if this regex matches the string provided.
    //! Unmatched parts of the string are stored in 'unmatched'.
    bool match(const std::string& str,std::list<std::string>& unmatched) const;
    
    //! Returns patter 
    std::string getPattern();

  private:
    std::string pattern;
    regex_t preg;
    int status;
  };
}

#endif
