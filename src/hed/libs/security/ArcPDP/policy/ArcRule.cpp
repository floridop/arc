#include <fstream>
#include <iostream>
#include "../attr/AttributeValue.h"
#include "ArcRule.h"
#include <list>

#include "../attr/ArcAttributeFactory.h"
#include "../fn/ArcFnFactory.h"

static Arc::LogStream logcerr(std::cerr);

using namespace Arc;

void ArcRule::getItemlist(XMLNode& nd, OrList& items, const std::string& itemtype, const std::string& type_attr, const std::string& function_attr){
  
  ArcAttributeFactory* attrfactory = new ArcAttributeFactory();
  ArcFnFactory* fnfactory = new ArcFnFactory();


  XMLNode tnd;

  for(int i=0; i<nd.Size(); i++){
    std::string type = type_attr;
    std::string funcname = function_attr;

    for(int j=0;;j++){
      tnd = nd[itemtype][j];
      if(!tnd) break;
      if(!((std::string)(tnd.Attribute("Type"))).empty())
        type = (std::string)(tnd.Attribute("Type"));
      if(!((std::string)(tnd.Attribute("Function"))).empty())
        funcname = (std::string)(tnd.Attribute("Function"));

      if(!(type.empty())&&(tnd.Size()==0)){
        AndList item;
        if(funcname.empty()) funcname = type + "equal";
        item.push_back(Match(attrfactory->createValue(tnd, type), fnfactory->createFn(funcname)));
        items.push_back(item);
        //items.push_back(item);
      }
      else if((type.empty())&&(tnd.Size()>0)){
        AndList item;
        int size = tnd.Size();
        for(int k=0; k<size; k++){
          XMLNode snd = tnd.Child(k);
          type = (std::string)(snd.Attribute("Type"));
          // The priority of function definition: Subelement.Attribute("Function") > Element.Attribute("Function") > Subelement.Attribute("Type") + "equal"
          if(!((std::string)(snd.Attribute("Function"))).empty())
            funcname = (std::string)(snd.Attribute("Function"));
          
          if(funcname.empty()) funcname = type + "equal";
          item.push_back(Match(attrfactory->createValue(snd, type), fnfactory->createFn(funcname)));
        }
        items.push_back(item);
      }
      else if((!type.empty())&&(tnd.Size()>0)){
        AndList item;
        int size = tnd.Size();
        for(int k=0; k<size; k++){
          XMLNode snd = tnd.Child(k);
          if(!((std::string)(snd.Attribute("Function"))).empty())
            funcname = (std::string)(snd.Attribute("Function"));

          if(funcname.empty()) funcname = type + "equal";
          item.push_back(Match(attrfactory->createValue(snd, type), fnfactory->createFn(funcname)));
        }
        items.push_back(item);
      }
      else{
        logger.msg(Arc::ERROR, "Error definition in policy"); 
        return;
      }
    }

    for(int l=0;;l++){
      tnd = nd["GroupIdRef"][l];
      if(!tnd) break;
      std::string location = (std::string)(tnd.Attribute("Location"));

      //Open the reference file and parse it to get external item information
      std::string xml_str = "";
      std::string str;
      std::ifstream f(location.c_str());
      while (f >> str) {
        xml_str.append(str);
        xml_str.append(" ");
      }
      f.close();

      XMLNode subref = XMLNode(xml_str);
      XMLNode snd;
      std::string itemgrouptype = itemtype + "Group";
      for(int k=0;;k++){
        snd = subref[itemgrouptype][k];
        //If the reference ID in the policy file matches the ID in the external file, try to get the subject information from the external file
        if((std::string)(snd.Attribute("GroupID")) == (std::string)tnd){
          getItemlist(snd, items, itemtype, type_attr, function_attr);
        }
      }
    }
  }
  return;
}

ArcRule::ArcRule(XMLNode& node) : effect(0), id(NULL), version(NULL), description(NULL){
  XMLNode nd, tnd;
  Arc::Logger logger(Arc::Logger::rootLogger, "Policy");
  logger.addDestination(logcerr);
  
  /* "And" relationship means the request should satisfy all of the items 
  <Subject>
   <SubFraction Type="X500DN">/O=Grid/OU=KnowARC/CN=XYZ</SubFraction>
   <SubFraction Type="ShibName">urn:mace:shibboleth:examples</SubFraction>
  </Subject>
  */
  /* "Or" relationship meand the request should satisfy any of the items
  <Subjects>
    <Subject Type="X500DN">/O=Grid/OU=KnowARC/CN=ABC</Subject>
    <Subject Type="VOMSAttribute">/vo.knowarc/usergroupA</Subject>
    <Subject>
      <SubFraction Type="X500DN">/O=Grid/OU=KnowARC/CN=XYZ</SubFraction>
      <SubFraction Type="ShibName">urn:mace:shibboleth:examples</SubFraction>
    </Subject>
    <GroupIdRef Location="./subjectgroup.xml">subgrpexample1</GroupIdRef>
  </Subjects>
  */


  id = (std::string)(node.Attribute("RuleID"));
  description = (std::string)(node["Description"]);
  if((std::string)(node.Attribute("Effect"))=="Permit")
    effect="Permit";
  else if((std::string)(node.Attribute("Effect"))=="Deny")
    effect="Deny";
  else 
    logger.msg(Arc::ERROR, "Invalid Effect");
 
  std::string type, funcname;
  //Parse the "Subjects" part of a Rule
  nd = node["Subjects"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, subjects, "Subject", type, funcname);  

  //Parse the "Resources" part of a Rule. The "Resources" does not include "Sub" item, so it is not such complicated, but we can handle it the same as "Subjects" 
  nd = node["Resources"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, resources, "Resource", type, funcname);

  //Parse the "Actions" part of a Rule
  nd = node["Actions"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, actions, "Action", type, funcname);

  //Parse the "Condition" part of a Rule
  nd = node["Conditions"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, conditions, "Condition", type, funcname);
 
}


static Arc::MatchResult itemMatch(Arc::OrList items, std::list<Arc::RequestAttribute*> req){

  Arc::OrList::iterator orit;
  Arc::AndList::iterator andit;
  std::list<Arc::RequestAttribute*>::iterator reqit;

  //For example, go through each <Subject> element in one rule, once one <Subject> is satisfied, skip put.
  for( orit = items.begin(); orit != items.end(); orit++ ){

    int all_fraction_matched = 0;
    //For example, go through each <SubFraction> element in one <Subject>, all of the <SubFraction> elements should be satisfied.
    for(andit = (*orit).begin(); andit != (*orit).end(); andit++){
      bool one_req_matched = false;

      //go through each <Attribute> element in one <Subject> in Request.xml, all of the <Attribute> should be satisfied.
      for(reqit = req.begin(); reqit != req.end(); reqit++){
        //evaluate two "AttributeValue*" based on "Function" definition in "Rule"
        if(((*andit).second)->evaluate((*andit).first, (*reqit)->getAttributeValue()))
          one_req_matched = true;
      }
      // if one of the Attribute in one Request's Subject does not match any of the Rule.Subjects.SubjectA.SubFractions,
      // then skip to the next: Rule.Subjects.SubjectB
      if(!one_req_matched) break;
      else all_fraction_matched +=1;
    }
    //One Rule.Subjects.Subject is satisfied (all of the SubFraction are satisfied) by the RequestTuple.Subject
    if(all_fraction_matched == int((*orit).size())){
      return MATCH;
    }
  }

  return NO_MATCH;

}

MatchResult ArcRule::match(EvaluationCtx* ctx){
  Arc::RequestTuple evaltuple = ctx->getEvalTuple();  

  if(itemMatch(subjects, evaltuple.sub)==MATCH &&
    itemMatch(resources, evaltuple.res)==MATCH &&
    itemMatch(actions, evaltuple.act)==MATCH)
    //&&itemMatch(environments, evaltuple.sub)==MATCH)
    return MATCH;
  else return NO_MATCH;

}

Result ArcRule::eval(EvaluationCtx* ctx){
  Result result;
  //TODO
  // need to evaluate the "Condition"
  if (effect == "Permit") result = DECISION_PERMIT;
  else if (effect == "Deny") result = DECISION_DENY;
  return result;
}

std::string ArcRule::getEffect(){
  return effect;
}
