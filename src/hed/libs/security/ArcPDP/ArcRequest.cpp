#include <fstream>
#include "ArcRequest.h"
#include "ArcRequestItem.h"

using namespace Arc;

ReqItemList ArcRequest::getRequestItems () const {
  return rlist;
}

void ArcRequest::setRequestItems (ReqItemList sl){
  rlist = sl;
}


void ArcRequest::make_request(XMLNode& node){
  Arc::NS nsList;
  nsList.insert(std::pair<std::string, std::string>("request","http://www.nordugrid.org/ws/schemas/request-arc"));
  std::list<XMLNode> reqlist = node.XPathLookup("//request:Request", nsList);
 
  std::list<XMLNode>::iterator itemit;
  
  if(!(reqlist.empty())){
    std::list<XMLNode> itemlist = node.XPathLookup("//request:RequestItem", nsList);
    for ( itemit=itemlist.begin() ; itemit != itemlist.end(); itemit++ ){
      XMLNode itemnd=*itemit;
      rlist.push_back(new ArcRequestItem(itemnd));
    }
  }
}

ArcRequest::ArcRequest(const std::string& filename) : Request(filename){
  std::string str;
  std::string xml_str = "";
  std::ifstream f(filename.c_str());

  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();

  Arc::XMLNode node(xml_str);
  make_request(node);
}

ArcRequest::ArcRequest (XMLNode& node) : Request(node) {
  make_request(node);
}

ArcRequest::~ArcRequest(){
  while(!rlist.empty()){
    delete rlist.back();
    rlist.pop_back();
  }
}
