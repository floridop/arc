// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/data/DataPoint.h>

namespace Arc {

  Logger DataPoint::logger(Logger::rootLogger, "DataPoint");

  DataPoint::DataPoint(const URL& url)
    : url(url),
      size((unsigned long long int)(-1)),
      created(-1),
      valid(-1),
      triesleft(5),
      failure_code(DataStatus::UnknownError),
      cache(url.Option("cache") != "no") {}

  DataPoint::~DataPoint() {}

  const URL& DataPoint::GetURL() const {
    return url;
  }

  std::string DataPoint::str() const {
    return url.str();
  }

  DataPoint::operator bool() const {
    return (bool)url;
  }

  bool DataPoint::operator!() const {
    return !url;
  }

  DataStatus DataPoint::GetFailureReason() const {
    return failure_code;
  }
  
  bool DataPoint::Cache() const {
    return cache;
  }
  
  bool DataPoint::CheckSize() const {
    return (size != (unsigned long long int)(-1));
  }

  void DataPoint::SetSize(const unsigned long long int val) {
    size = val;
  }

  unsigned long long int DataPoint::GetSize() const {
    return size;
  }

  bool DataPoint::CheckCheckSum() const {
    return (!checksum.empty());
  }

  void DataPoint::SetCheckSum(const std::string& val) {
    checksum = val;
  }

  const std::string& DataPoint::GetCheckSum() const {
    return checksum;
  }

  bool DataPoint::CheckCreated() const {
    return (created != -1);
  }

  void DataPoint::SetCreated(const Time& val) {
    created = val;
  }

  const Time& DataPoint::GetCreated() const {
    return created;
  }

  bool DataPoint::CheckValid() const {
    return (valid != -1);
  }

  void DataPoint::SetValid(const Time& val) {
    valid = val;
  }

  const Time& DataPoint::GetValid() const {
    return valid;
  }

  int DataPoint::GetTries() const {
    return triesleft;
  }

  void DataPoint::SetTries(const int n) {
    triesleft = std::max(0, n);
  }

  bool DataPoint::NextTry() {
    if(triesleft) --triesleft;
  }

  void DataPoint::SetMeta(const DataPoint& p) {
    if (!CheckSize())
      SetSize(p.GetSize());
    if (!CheckCheckSum())
      SetCheckSum(p.GetCheckSum());
    if (!CheckCreated())
      SetCreated(p.GetCreated());
    if (!CheckValid())
      SetValid(p.GetValid());
  }

  bool DataPoint::CompareMeta(const DataPoint& p) const {
    if (CheckSize() && p.CheckSize())
      if (GetSize() != p.GetSize())
        return false;
    if (CheckCheckSum() && p.CheckCheckSum())
      // TODO: compare checksums properly
      if (strcasecmp(GetCheckSum().c_str(), p.GetCheckSum().c_str()))
        return false;
    if (CheckCreated() && p.CheckCreated())
      if (GetCreated() != p.GetCreated())
        return false;
    if (CheckValid() && p.CheckValid())
      if (GetValid() != p.GetValid())
        return false;
    return true;
  }

  void DataPoint::AssignCredentials(const std::string& proxyPath,
                                    const std::string& certificatePath,
                                    const std::string& keyPath,
                                    const std::string& caCertificatesDir) {
    this->proxyPath = proxyPath;
    this->certificatePath = certificatePath;
    this->keyPath = keyPath;
    this->caCertificatesDir = caCertificatesDir;
  }

  void DataPoint::AssignCredentials(const XMLNode& node) {
    proxyPath = (std::string)node["ProxyPath"];
    certificatePath = (std::string)node["CertificatePath"];
    keyPath = (std::string)node["KeyPath"];
    caCertificatesDir = (std::string)node["CACertificatesDir"];
  }

} // namespace Arc
