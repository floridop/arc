#ifdef SWIGPYTHON
%module delegation

%include "Arc.i"

%import "../src/hed/libs/common/XMLNode.h"
%import "../src/hed/libs/message/SOAPEnvelope.h"
%import "../src/hed/libs/message/MCC.h"
%import "../src/hed/libs/message/Message.h"
%import "../src/hed/libs/message/MessageAttributes.h"
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/delegation/DelegationInterface.h
%{
#include <arc/delegation/DelegationInterface.h>
%}
%ignore Arc::DelegationConsumer::operator!;
%ignore Arc::DelegationProvider::operator!;
#ifdef SWIGPYTHON
%ignore Arc::DelegationConsumer::Acquire(std::string&, std::string&);
%ignore Arc::DelegationConsumerSOAP::UpdateCredentials(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationConsumerSOAP::DelegatedToken(std::string&, std::string&, XMLNode);
%ignore Arc::DelegationContainerSOAP::DelegatedToken(std::string&, std::string&, XMLNode);
%ignore Arc::DelegationContainerSOAP::DelegatedToken(std::string&, std::string&, XMLNode, std::string const&);
%ignore Arc::DelegationContainerSOAP::Process(std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationContainerSOAP::Process(std::string&, const SOAPEnvelope&, SOAPEnvelope&, const std::string&);
%apply std::string& TUPLEOUTPUTSTRING { std::string& credentials }; /* Applies to:
 * bool DelegationConsumerSOAP::UpdateCredentials(std::string& credentials, const SOAPEnvelope& in, SOAPEnvelope& out);
 * bool DelegationConsumerSOAP::DelegatedToken(std::string& credentials, XMLNode token);
 * bool DelegationContainerSOAP::DelegatedToken(std::string& credentials, XMLNode token, const std::string& client = "");
 * bool DelegationContainerSOAP::Process(std::string& credentials, const SOAPEnvelope& in, SOAPEnvelope& out, const std::string& client = "");
 */
%apply std::string& TUPLEOUTPUTSTRING { std::string& content }; /* Applies to:
 * bool DelegationConsumer::Backup(std::string& content);
 * bool DelegationConsumer::Request(std::string& content);
 * bool DelegationConsumer::Acquire(std::string& content);
 */
#endif
%include "../src/hed/libs/delegation/DelegationInterface.h"
#ifdef SWIGPYTHON
%clear std::string& credentials;
%clear std::string& content;
#endif
