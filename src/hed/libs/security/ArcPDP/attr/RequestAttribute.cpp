#include "RequestAttribute.h"

using namespace Arc;

RequestAttribute::RequestAttribute(XMLNode& node, AttributeFactory* attrfactory){
  Arc::XMLNode nd;
  if(!(nd=node.Attribute("AttributeID"))){
    id = (std::string)nd;
  }
  if(!(nd=node.Attribute("Type"))){
    type = (std::string)nd;
  }
  if(!(nd=node.Attribute("Issuer"))){
    issuer = (std::string)nd;
  }

  attrval = attrfactory->createValue(node, type);

/*
  if(!(node.Size())){
    avlist.push_back(attrfactory->createValue(node, type));
  }
  else{
    for(int i=0; i<node.Size(); i++)
      avlist.push_back(attrfactory->createValue(node.Child(i), type));
  }
*/
}

std::string RequestAttribute::getAttributeId () const{
 
}

void RequestAttribute::setAttributeId (const std::string attributeId){
}

std::string RequestAttribute::getDataType () const{
}

void RequestAttribute::setDataType (const std::string dataType){
}

std::string RequestAttribute::getIssuer () const{
}

void RequestAttribute::setIssuer (const std::string issuer){
}

/*AttrValList RequestAttribute::getAttributeValueList () const{
}

void RequestAttribute::setAttributeValueList (const AttrValList& attributeValueList){
}
*/
AttributeValue* RequestAttribute::getAttributeValue() const{
  return attrval;
}


RequestAttribute::~RequestAttribute(){
  delete attrval;
  /*while(!avlist.empty()){
    delete (*(avlist.back()));
    avlist.pop_back();
  }
  */
}

