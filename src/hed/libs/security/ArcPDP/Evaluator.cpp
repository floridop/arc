#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Evaluator.h"

#include "Request.h"
#include "ArcRequest.h"
#include "Response.h"
#include "EvaluationCtx.h"
#include <fstream>
#include <iostream>

using namespace Arc;
/*
static Arc::Evaluator* get_evaluator(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::Evaluator(cfg);
}
*/

void Evaluator::parsecfg(Arc::XMLNode& cfg){
  std::string policystore, policylocation, functionfactory, attributefactory, combingalgfactory;
  XMLNode nd;

  Arc::NS nsList;
  std::list<XMLNode> res;
  nsList.insert(std::pair<std::string, std::string>("config","http://www.nordugrid.org/ws/schemas/policycfg-arc"));

  res = cfg.XPathLookup("//config:PolicyStore", nsList);
  //presently, there can be only one PolicyStore
  if(!(res.empty())){
    nd = *(res.begin());
    policystore = (std::string)(nd.Attribute("name"));
    policylocation =  (std::string)(nd.Attribute("location"));
  }

  res = cfg.XPathLookup("//config:FunctionFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    functionfactory = (std::string)(nd.Attribute("name"));
  }           

  res = cfg.XPathLookup("//config:AttributeFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    attributefactory = (std::string)(nd.Attribute("name"));
  }  

  res = cfg.XPathLookup("//config:CombingAlgorithmFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    combingalgfactory = (std::string)(nd.Attribute("name"));
  }

  //TODO: load the class by using the configuration information. 

  //temporary solution
  std::list<std::string> filelist;
  filelist.push_back("Policy_Example.xml");
  std::string alg("Permit-Overrides");
  plstore = new Arc::PolicyStore(filelist, alg);
  fnfactory = new Arc::ArcFnFactory() ;
  attrfactory = new Arc::ArcAttributeFactory();
}

Evaluator::Evaluator (Arc::XMLNode& cfg){
  parsecfg(cfg);
}

Evaluator::Evaluator(const char * cfgfile){
  std::string str;
  std::string xml_str = "";
  std::ifstream f(cfgfile);

  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();

  Arc::XMLNode node(xml_str);
  parsecfg(node); 
}

Arc::Response* Evaluator::evaluate(Arc::Request* request){
  Arc::EvaluationCtx * evalctx = new Arc::EvaluationCtx(request);
  return (evaluate(evalctx));   
}

Arc::Response* Evaluator::evaluate(const std::string& reqfile){
  Arc::Request* request = new Arc::ArcRequest(reqfile);
  Arc::EvaluationCtx * evalctx = new Arc::EvaluationCtx(request);
 
  //evaluate the request based on policy
  return(evaluate(evalctx));
  
}

Arc::Response* Evaluator::evaluate(Arc::EvaluationCtx* ctx){
  //Split request into <subject, action, object, environment> tuples
  ctx->split();
  
  std::list<Arc::Policy*> policies;
  std::list<Arc::Policy*>::iterator policyit;
  std::list<Arc::RequestTuple> reqtuples = ctx->getRequestTuples();
  std::list<Arc::RequestTuple>::iterator it;
  
  Arc::Response* resp = new Arc::Response();
  for(it = reqtuples.begin(); it != reqtuples.end(); it++){
    //set the present RequestTuple for evaluation
    ctx->setEvalTuple(*it);

    //get the policies which match the present context (RequestTuple), which means 
    //the <subject, action, object, environment> of a policy
    //match the present RequestTuple
    policies = plstore->findPolicy(ctx);
    
    std::list<Arc::Policy*> permitset;
    bool atleast_onepermit = false;
    //Each matched policy evaluates the present RequestTuple, using default combiningalg: DENY-OVERRIDES
    for(policyit = policies.begin(); policyit != policies.end(); policyit++){
      Arc::Result res = (*policyit)->eval(ctx);

      std::cout<<res<<std::endl;

      if(res == DECISION_DENY || res == DECISION_INDETERMINATE){
        while(!permitset.empty()) permitset.pop_back();
        break;
      }
      if(res == DECISION_PERMIT){
        permitset.push_back(*policyit);
        atleast_onepermit = true;
      }
    }
    //For "Deny" RequestTuple, do we need to give some information?? 
    //TODO
    if(atleast_onepermit){
      ResponseItem* item = new ResponseItem;
      RequestTuple reqtuple = (*it);
      item->reqtp = reqtuple; 
      item->pls = permitset;
      resp->addResponseItem(item);
    }
  }
  
  return resp;

/*
  Arc::Response response = new Arc::Response();
  for(Arc::MatchedItem::iterator it = matcheditem.begin(); it!=matcheditem.end(); it++){
    Arc::RequestItem* reqitem = (*it).first;
    Arc::Policy* policies = (*it).second;
    ctx->setRequestItem(reqitem);
    Arc::Response* resp = policies->eval(ctx);
    response->merge(resp);
  }   
  
  return response;
*/
  
/*  Request* req = ctx->getRequest();
  ReqItemList reqitems = req->getRequestItems();
*/
  
}

Evaluator::~Evaluator(){
}

