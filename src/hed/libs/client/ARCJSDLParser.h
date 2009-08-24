// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCJSDLPARSER_H__
#define __ARC_ARCJSDLPARSER_H__

#include <string>

#include "JobDescriptionParser.h"

/** ARCJSDLParser
 * The ARCJSDLParser class, derived from the JobDescriptionParser class, is
 * primarily a job description parser for the consolidated job description
 * language (ARCJSDL), derived from JSDL, described in the following document
 * <http://svn.nordugrid.org/trac/nordugrid/browser/arc1/trunk/doc/tech_doc/client/job_description.odt>.
 * However it is also capable of parsing regular JSDL (GFD 136), the POSIX-JSDL
 * extension (GFD 136) and the JSDL HPC Profile Application Extension (GFD 111
 * and GFD 114).
 * When parsing ARCJSDL takes precedence over other non-ARCJSDL, so if a
 * non-ARCJSDL element specifies the same attribute as ARCJSDL, the ARCJSDL
 * element will be saved.
 * The output generated by the ARCJSDLParser::UnParse method will follow that of
 * the ARCJSDL document, see reference above.
 */

namespace Arc {

  class ARCJSDLParser
    : public JobDescriptionParser {
  public:
    ARCJSDLParser();
    ~ARCJSDLParser();
    JobDescription Parse(const std::string& source) const;
    std::string UnParse(const JobDescription& job) const;
  private:
    bool parseSoftware(const XMLNode& xmlSoftware, SoftwareRequirement& sr) const;
    void outputSoftware(const SoftwareRequirement& sr, XMLNode& xmlSoftware) const;

    template<typename T>
    void parseRange(const XMLNode& xmlRange, Range<T>& range, const T& undefValue) const;
    template<typename T>
    Range<T> parseRange(const XMLNode& xmlRange, const T& undefValue) const;
    template<typename T>
    void outputARCJSDLRange(const Range<T>& range, XMLNode& arcJSDL, const T& undefValue) const;
    template<typename T>
    void outputJSDLRange(const Range<T>& range, XMLNode& jsdl, const T& undefValue) const;

    
    void parseBenchmark(const XMLNode& xmlBenchmark, std::pair<std::string, int>& benchmark) const;
    void outputBenchmark(const std::pair<std::string, int>& benchmark, XMLNode& xmlBenchmark) const;
  };

} // namespace Arc

#endif // __ARC_POSIXJSDLPARSER_H__
