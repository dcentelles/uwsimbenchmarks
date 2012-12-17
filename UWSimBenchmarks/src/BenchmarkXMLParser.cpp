#include "BenchmarkXMLParser.h"


  void BenchmarkXMLParser::esPi(string in, double * param){
    in.erase(0, in.find_first_not_of("\t "));
    in.erase(in.find_last_not_of("\t ")+1,-1);  

    if(in=="M_PI")
      *param=M_PI;
    else if (in=="M_PI_2")
      *param=M_PI_2;
    else if (in=="M_PI_4")
      *param=M_PI_4;
    else if(in=="-M_PI")
      *param=-M_PI;
    else if (in=="-M_PI_2")
      *param=-M_PI_2;
    else if (in=="-M_PI_4")
      *param=-M_PI_4;
    else
      *param= atof(in.c_str());	
  }

  void BenchmarkXMLParser::extractFloatChar(const xmlpp::Node* node,double * param){
    xmlpp::Node::NodeList list = node->get_children();
  
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(*iter);
      if(nodeText)
	esPi(nodeText->get_content(),param);
        //*param=atof(nodeText->get_content().c_str());
    }
  }

  void BenchmarkXMLParser::extractIntChar(const xmlpp::Node* node,int * param){
    xmlpp::Node::NodeList list = node->get_children();
  
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(*iter);
      if(nodeText)
        *param=atoi(nodeText->get_content().c_str());
    }
  }

  void BenchmarkXMLParser::extractUIntChar(const xmlpp::Node* node, unsigned int * param){
    xmlpp::Node::NodeList list = node->get_children();
  
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(*iter);
      if(nodeText)
        *param=(unsigned int)atoi(nodeText->get_content().c_str());
    }
  }

  void BenchmarkXMLParser::extractStringChar(const xmlpp::Node* node,string * param){
    xmlpp::Node::NodeList list = node->get_children();
  
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(*iter);
      if(nodeText)
        *param=nodeText->get_content();
        param->erase(0, param->find_first_not_of("\t "));
	param->erase(param->find_last_not_of("\t ")+1,-1);
    }
  }

  void BenchmarkXMLParser::extractPositionOrColor(const xmlpp::Node* node,double * param){
    xmlpp::Node::NodeList list = node->get_children();
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);

      if(child->get_name()=="x" || child->get_name()=="r")
	extractFloatChar(child,&param[0]);
      else if(child->get_name()=="y" || child->get_name()=="g")
	extractFloatChar(child,&param[1]);
      else if(child->get_name()=="z" || child->get_name()=="b")
	extractFloatChar(child,&param[2]);
    }
  }

void BenchmarkXMLParser::processTrigger(const xmlpp::Node* node,TriggerInfo * trigger){

  xmlpp::Node::NodeList list = node->get_children();
  for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
    xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);

    if(child->get_name()=="type"){
      string aux;
      extractStringChar(child,&aux);
      if(aux=="topic")
	trigger->type=TriggerInfo::Topic;
      else if(aux=="onInit")
	trigger->type=TriggerInfo::AlwaysOn;
      else if(aux=="never")
	trigger->type=TriggerInfo::AlwaysOff;
      else if(aux=="service")
	trigger->type=TriggerInfo::Service;
      else if(aux=="onMove")
	trigger->type=TriggerInfo::OnMove;
      else if(aux=="onNoMove")
	trigger->type=TriggerInfo::OnNoMove;
      else if(aux=="position")
	trigger->type=TriggerInfo::Position;
    }
    else if(child->get_name()=="target")
      extractStringChar(child,&trigger->target);
    else if(child->get_name()=="position")
      extractPositionOrColor(child,trigger->position);

  }

}

void BenchmarkXMLParser::processVector(const xmlpp::Node* node, std::vector<double> &groundTruth){
    xmlpp::Node::NodeList list = node->get_children();
    groundTruth.resize((list.size()-1)/2);
    int pos=0;
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);
      if(child->get_name()=="value"){
        extractFloatChar(child,&groundTruth[pos++]);
      }
    }
  }


void BenchmarkXMLParser::processMeasure(const xmlpp::Node* node,MeasureInfo * measure){

    xmlpp::Node::NodeList list = node->get_children();
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);
      //cout<<child->get_name()<<endl;
      if(child->get_name()=="name")
	extractStringChar(child,&measure->name);
      else if(child->get_name()=="startOn")
        processTrigger(child,&measure->startOn);
      else if(child->get_name()=="stopOn")
        processTrigger(child,&measure->stopOn);
      else if(child->get_name()=="target")
        extractStringChar(child,&measure->target);
      else if(child->get_name()=="position")
        extractPositionOrColor(child,measure->position);
      else if(child->get_name()=="groundTruth"){
   	xmlpp::Attribute * atrib =  dynamic_cast<const xmlpp::Element*>(child)->get_attribute("type");
	if(atrib->get_value()=="constant"){
	  measure->subtype=MeasureInfo::Constant;
	  processVector(child,measure->groundTruth);
	}
	else if (atrib->get_value()=="cornersFromCamera"){
	  measure->subtype=MeasureInfo::CornersFromCam;
	  processGTFromCam(child,measure);
	}
	else if (atrib->get_value()=="centroidFromCamera"){
	  measure->subtype=MeasureInfo::CentroidFromCam;
	  processGTFromCam(child,measure);
	}
      }
    }
}

void BenchmarkXMLParser::processGTFromCam(const xmlpp::Node* node,MeasureInfo * measure){

    xmlpp::Node::NodeList list = node->get_children();
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);
      if(child->get_name()=="target")
	extractStringChar(child,&measure->object);
      else if(child->get_name()=="camera")
        extractStringChar(child,&measure->camera);
    }

}
void BenchmarkXMLParser::processMeasures(const xmlpp::Node* node){

  xmlpp::Node::NodeList list = node->get_children();
  for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
    xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);

    MeasureInfo measure;
    measure.type=MeasureInfo::Unknown;
    if(child->get_name()=="time"){
      measure.type=MeasureInfo::Time;
    }
    else if(child->get_name()=="collision"){
      measure.type=MeasureInfo::Collisions;
    }
    else if(child->get_name()=="positionError"){
      measure.type=MeasureInfo::PositionError;
    }
    else if(child->get_name()=="distance"){
      measure.type=MeasureInfo::Distance;
    }
    else if(child->get_name()=="euclideanNorm"){
      measure.type=MeasureInfo::EuclideanNorm;
    }

    if(measure.type!=MeasureInfo::Unknown){
      processMeasure(child,&measure);
      measures.push_back(measure);	
    }
  }
}

void BenchmarkXMLParser::processSceneUpdater(const xmlpp::Node* node,SceneUpdaterInfo * su){

  xmlpp::Node::NodeList list = node->get_children();
  for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
    xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);

    if(child->get_name()=="none")
      su->type=SceneUpdaterInfo::None;
    else if(child->get_name()=="sceneFogUpdater"){
      su->type=SceneUpdaterInfo::SceneFogUpdater;
      processSceneUpdaters(child,su);
    }
  }

}

void BenchmarkXMLParser::processSceneUpdaters(const xmlpp::Node* node,SceneUpdaterInfo * su){

  xmlpp::Node::NodeList list = node->get_children();
  for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
    xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);
    
    if(child->get_name()=="initialFog")
      extractFloatChar(child,&su->initialFog);
    else if(child->get_name()=="finalFog")
      extractFloatChar(child,&su->finalFog);
    else if(child->get_name()=="step")
      extractFloatChar(child,&su->step);
    else if(child->get_name()=="interval")
      extractFloatChar(child,&su->interval);
  }
}


void BenchmarkXMLParser::processXML(const xmlpp::Node* node){

  if(node->get_name()!="benchmark")
    cout<<"CFG: XML file is not Benchmark file."<<endl;
  else{
    xmlpp::Node::NodeList list = node->get_children();
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter){
      xmlpp::Node* child=dynamic_cast<const xmlpp::Node*>(*iter);
      //cout<<child->get_name()<<endl;
      if(child->get_name()=="measures")
        processMeasures(child);
      else if(child->get_name()=="startOn")
        processTrigger(child,&startOn);
      else if(child->get_name()=="stopOn")
        processTrigger(child,&stopOn);
      else if(child->get_name()=="function")
        extractStringChar(child,&function);
      else if(child->get_name()=="sceneUpdater")
	processSceneUpdater(child,&sceneUpdater);
    }
  }
}

void BenchmarkXMLParser::applyOffset(double position[3],double offsetp[3],double offsetr[3]){
  osg::Matrixd matrix;

  matrix.makeRotate(offsetr[0],1,0,0);
  matrix.preMultRotate(osg::Quat(offsetr[1],osg::Vec3d(0,1,0)));
  matrix.preMultRotate(osg::Quat(offsetr[2],osg::Vec3d(0,0,1)));
  matrix.setTrans(offsetp[0],offsetp[1],offsetp[2]);
  matrix.preMultTranslate(osg::Vec3d(position[0],position[1],position[2]));

  osg::Vec3d trans=matrix.getTrans();
  position[0]=trans.x();
  position[1]=trans.y();
  position[2]=trans.z();
}

//Applies offset to global position, used with uwsim world offset
void BenchmarkXMLParser::postProcessWorldOffset(double offsetp[3],double offsetr[3]){
  //Search for position triggers and positionerror measures
  for(list<MeasureInfo>::iterator i=measures.begin(); i != measures.end(); ++i) {
    if(i->type==MeasureInfo::PositionError)
      applyOffset(i->position,offsetp,offsetr);
    if(i->startOn.type==TriggerInfo::Position)
      applyOffset(i->startOn.position,offsetp,offsetr);
    if(i->stopOn.type==TriggerInfo::Position)
      applyOffset(i->stopOn.position,offsetp,offsetr);
  }
  if(startOn.type==TriggerInfo::Position)
    applyOffset(startOn.position,offsetp,offsetr);
  if(stopOn.type==TriggerInfo::Position)
    applyOffset(stopOn.position,offsetp,offsetr);   
}

BenchmarkXMLParser::BenchmarkXMLParser(const std::string &fName){

    try
    {
      xmlpp::DomParser parser;
      parser.set_validate();
      parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
      parser.parse_file(fName);
      if(parser)
      {
        //Walk the tree:
        const xmlpp::Node* pNode = parser.get_document()->get_root_node(); //deleted by DomParser.
        processXML(pNode);
      }
    }
    catch(const std::exception& ex)
    {
      std::cerr << "Exception caught: " << ex.what() << std::endl;
      std::cerr << "Please check your XML benchmark file" << std::endl;
      exit(0);
    }
}

BenchmarkXMLParser::BenchmarkXMLParser(){
}
