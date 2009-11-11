#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <string>
#include <iostream>
#include <fstream>
#include <glibmm/fileutils.h>
#include <algorithm>
#include <unistd.h>

Arc::NS ns;

struct to_upper {
  int operator() (int ch)
  {
    return std::toupper(ch);
  }
};

struct to_lower {
  int operator() (int ch)
  {
    return std::tolower(ch);
  }
};

static void std_h_header(std::string &name, std::ofstream &h)
{
    std::string uname = name;
    std::transform(name.begin(), name.end(), uname.begin(), to_upper());
    h << "// Generated by wsdl2hed " << std::endl;
    h << "#ifndef __ARC_" << uname << "_H__" << std::endl;
    h << "#define __ARC_" << uname << "_H__" << std::endl;
    h << std::endl;
    h << "#include <arc/message/Service.h>" << std::endl;
    h << "#include <arc/delegation/DelegationInterface.h>" << std::endl;
    h << "#include <arc/infosys/InformationInterface.h>" << std::endl;
    h << std::endl;
    h << "namespace " << name << " {" << std::endl;
    h << std::endl;
    h << "class " << name << "Service: public Arc::Service" << std::endl;
    h << "{" << std::endl;
    h << std::endl;
}

static void h_public_part(std::string &name, std::ofstream &h)
{
    h << "    public:" << std::endl;
    h << "        " << name << "Service(Arc::Config *cfg);" << std::endl;
    h << "        virtual ~" << name << "Service(void);" << std::endl;
    h << "        virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);" << std::endl;
}

static void h_private_part(std::string &/*name*/, std::ofstream &h, Arc::XMLNode &xml)
{
    h << "    private:" << std::endl;
    h << "        Arc::NS ns;" << std::endl;
    h << "        Arc::Logger logger;" << std::endl;
    h << "        Arc::DelegationContainerSOAP delegation;" << std::endl;
    h << "        Arc::InformationContainer infodoc;" << std::endl;
    h << "        Arc::MCC_Status make_soap_fault(Arc::Message &outmsg);" << std::endl;
    h << "        // Operations from WSDL" << std::endl;
    Arc::XMLNode op;
    for (int i = 0; (op = xml["wsdl:portType"]["wsdl:operation"][i]) == true; i++) {
        std::string n = (std::string) op.Attribute("name");
        if (!n.empty()) {
            h << "        Arc::MCC_Status " << n << "(Arc::XMLNode &in, Arc::XMLNode &out);" << std::endl;
        }
    }
}

static void std_cpp_header(std::string &name, std::ofstream &cpp)
{   
    std::string lname = name;
    std::transform(name.begin(), name.end(), lname.begin(), to_lower());
    cpp << "// Generated by wsdl2hed" << std::endl;
    cpp << "#ifdef HAVE_CONFIG_H" << std::endl;
    cpp << "#include <config.h>" << std::endl;
    cpp << "#endif" << std::endl;
    cpp << std::endl;
    cpp << "#include <arc/loader/Loader.h>" << std::endl;
    cpp << "#include <arc/loader/ServiceLoader.h>" << std::endl;
    cpp << "#include <arc/message/PayloadSOAP.h>" << std::endl;
    cpp << "#include <arc/ws-addressing/WSA.h>" << std::endl;
    cpp << std::endl;
    cpp << "#include \"" << lname << ".h\"" << std::endl;
    cpp << std::endl;
    cpp << "namespace " << name << " {" << std::endl;
    cpp << std::endl;
    cpp << "static Arc::Service *get_service(Arc::Config *cfg, Arc::ChainContext *) { " << std::endl;
    cpp << "    return new " << name << "Service(cfg);" << std::endl;
    cpp << "}" << std::endl; 
}

static void cpp_public_part(std::string &name, std::ofstream &cpp, Arc::XMLNode &xml)
{   
    cpp << std::endl;
    cpp << name << "Service::" << name << "Service(Arc::Config *cfg):Service(cfg),logger(Arc::Logger::rootLogger, \"" << name << "\")" << std::endl;
    cpp << "{" << std::endl;
    cpp << "    // Define supported namespaces" << std::endl;
    Arc::NS n = xml.Namespaces();
    Arc::NS::iterator it;
    for (it = n.begin(); it != n.end(); it++) {
        // Ignore some default namespace
        if (it->first != "soap" &&
            it->first != "SOAP-ENV" &&
            it->first != "SOAP-ENC" &&
            it->first != "wsdl" &&
            it->first != "xsd") {
                cpp << "    ns[\"" << it->first << "\"]=\"" << it->second << "\";" << std::endl;
        }
    }
    cpp << "}" << std::endl;
    cpp << std::endl;
    cpp << name << "Service::~" << name << "Service(void)" << std::endl;
    cpp << "{" << std::endl;
    cpp << "}" << std::endl;
    cpp << std::endl;
    cpp << "Arc::MCC_Status " << name << "Service::process(Arc::Message &inmsg, Arc::Message &outmsg)" << std::endl;
    cpp << "{\n\
    // Both input and output are supposed to be SOAP\n\
    // Extracting payload\n\
    Arc::PayloadSOAP* inpayload = NULL;\n\
    try {\n\
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());\n\
    } catch(std::exception& e) { };\n\
    if(!inpayload) {\n\
      logger.msg(Arc::ERROR, \"input is not SOAP\");\n\
      return make_soap_fault(outmsg);\n\
    };\n\
    // Analyzing request\n\
    Arc::XMLNode op = inpayload->Child(0);\n\
    if(!op) {\n\
      logger.msg(Arc::ERROR, \"input does not define operation\");\n\
      return make_soap_fault(outmsg);\n\
    }; \n\
    logger.msg(Arc::VERBOSE,\"process: operation: %s\", op.Name());\n\
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns);\n\
    Arc::PayloadSOAP& res = *outpayload;\n\
    Arc::MCC_Status ret = Arc::STATUS_OK;" << std::endl;
    
    cpp << "    "; // just becuase good indent of following if section
    Arc::XMLNode op;
    for (int i = 0; (op = xml["wsdl:portType"]["wsdl:operation"][i]) == true; i++) {
        std::string n = (std::string) op.Attribute("name");
        std::string msg = (std::string) op["output"].Attribute("message");
        cpp << "if(MatchXMLName(op, \"" << n << "\")) {" << std::endl;
        cpp << "        Arc::XMLNode r = res.NewChild(\"" << msg << "\");" << std::endl;
        cpp << "        ret = " << n << "(op, r);" << std::endl;
        cpp << "    } else ";
    }
    cpp << "if(MatchXMLName(op, \"DelegateCredentialsInit\")) {\n\
        if(!delegation.DelegateCredentialsInit(*inpayload,*outpayload)) {\n\
          delete inpayload;\n\
          return make_soap_fault(outmsg);\n\
        }\n\
    // WS-Property\n\
    } else if(MatchXMLNamespace(op,\"http://docs.oasis-open.org/wsrf/rp-2\")) {\n\
        Arc::SOAPEnvelope* out_ = infodoc.Process(*inpayload);\n\
        if(out_) {\n\
          *outpayload=*out_;\n\
          delete out_;\n\
        } else {\n\
          delete inpayload; delete outpayload;\n\
          return make_soap_fault(outmsg);\n\
        };\n\
    } else {\n\
        logger.msg(Arc::ERROR,\"SOAP operation is not supported: %s\", op.Name());\n\
        return make_soap_fault(outmsg);\n\
    };\n\
    // Set output\n\
    outmsg.Payload(outpayload);\n\
    return Arc::MCC_Status(ret);\n\
}" << std::endl;

    cpp << std::endl;
}

static void cpp_private_part(std::string &name, std::ostream &cpp, Arc::XMLNode &xml)
{
    cpp << "Arc::MCC_Status "<< name << "Service::make_soap_fault(Arc::Message& outmsg)\n\
{\n\
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns,true);\n\
    Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;\n\
    if(fault) {\n\
        fault->Code(Arc::SOAPFault::Sender);\n\
        fault->Reason(\"Failed processing request\");\n\
    };\n\
    outmsg.Payload(outpayload);\n\
    return Arc::MCC_Status(Arc::STATUS_OK);\n\
}" << std::endl << std::endl;
    Arc::XMLNode op;
    for (int i = 0; (op = xml["wsdl:portType"]["wsdl:operation"][i]) == true; i++) {
        std::string n = (std::string) op.Attribute("name");
        if (!n.empty()) {
            cpp << "Arc::MCC_Status " << name << "Service::" << n << "(Arc::XMLNode &in, Arc::XMLNode &out)" << std::endl;
            cpp << "{" << std::endl;
            cpp << "    return Arc::MCC_Status();" << std::endl;
            cpp << "}" << std::endl;
            cpp << std::endl;
        }
    }
}

static void std_h_footer(std::string &name, std::ofstream &h)
{
    std::string uname = name;
    std::transform(name.begin(), name.end(), uname.begin(), to_upper());
    h << std::endl;
    h << "}; // class " << name << std::endl;
    h << "}; // namespace " << name << std::endl;
    h << "#endif // __ARC_" << uname << "_H__" << std::endl;
}


static void std_cpp_footer(std::string &name, std::ofstream &cpp)
{
    std::string lname = name;
    std::transform(name.begin(), name.end(), lname.begin(), to_lower());
    cpp << "}; // namespace " << name << std::endl;
    cpp << std::endl;
    cpp << "service_descriptors ARC_SERVICE_LOADER = {" << std::endl;
    cpp << "    { \"" << lname << "\", 0, &" << name << "::get_service }," << std::endl;
    cpp << "    { NULL, 0, NULL }" << std::endl;
    cpp << "};" << std::endl;
}

static void gen_makefile_am(std::string &name)
{
    std::string lname = name;
    std::transform(name.begin(), name.end(), lname.begin(), to_lower());
    std::ofstream m("Makefile.am");
    m << "pkglib_LTLIBRARIES = lib" << lname << ".la" << std::endl;
    m << "lib" << lname << "_la_SOURCES = " << lname << ".cpp " << lname << ".h" << std::endl;
    m << "lib" << lname << "_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include" << std::endl;
    m << "lib" << lname << "_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la" << std::endl;
    m << "lib" << lname << "_la_LDFLAGS = -no-undefined -avoid-version -module" << std::endl;
    m.close();
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "Invalid arguments" << std::endl;        
        return -1;
    }
    ns["wsdl"] = "http://schemas.xmlsoap.org/wsdl/";
    std::string xml_str = Glib::file_get_contents(argv[1]);
    Arc::XMLNode xml(xml_str);
    if (xml == false) {
        std::cerr << "Failed parse XML! " << std::endl;
        return -1;
    }
    /* {
        std::string str;
        xml.GetXML(str);
        std::cout << str << std::endl;
    }; */

    // xml.Namespaces(ns);
    std::string name = argv[2];
    std::string lname = name;
    std::transform(name.begin(), name.end(), lname.begin(), to_lower());
    std::string header_path = lname; 
    header_path += ".h";
    std::string cpp_path = lname; 
    cpp_path += ".cpp";
    
    std::ofstream h(header_path.c_str());
    if (!h) {
        std::cerr << "Cannot create: " << header_path << std::endl;
        exit(1);
    }
    std::ofstream cpp(cpp_path.c_str());
    if (!cpp) {
        unlink (header_path.c_str());
        std::cerr << "Cannot create: " << cpp_path << std::endl;
    }
    
    std_h_header(name, h);
    h_public_part(name, h);
    h_private_part(name, h, xml);
    std_h_footer(name, h);
    std_cpp_header(name, cpp);
    cpp_public_part(name, cpp, xml);
    cpp_private_part(name, cpp, xml);
    std_cpp_footer(name, cpp);
    h.close();
    cpp.close();
    gen_makefile_am(name);
   
    return 0;
}
