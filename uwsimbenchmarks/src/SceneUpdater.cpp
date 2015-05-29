#include "SceneUpdater.h"
#include <std_srvs/Empty.h>


SceneUpdater::SceneUpdater(double interval){
  this->interval=interval;
  started=0;
  child=NULL;
}

void SceneUpdater::start(){
  init = ros::WallTime::now();
  started=1;
  if(child)
    child->start();
}

int SceneUpdater::needsUpdate(){
  //Check lowest level needs update
  if(child)
    return child->needsUpdate();
  else if(started){
    update();
    ros::WallDuration t_diff = ros::WallTime::now() - init;
    return t_diff.toSec() > interval;
  }
  return 0;
}

void SceneUpdater::restartTimer(){
  init = ros::WallTime::now();
  started=0;
}

void SceneUpdater::tick(){
  if(child)
    child->tick();
}

void SceneUpdater::getReferences(std::vector<double> &refs){
  for(SceneUpdater * iterator=this;iterator!=NULL;iterator=iterator->child)
    refs.push_back(iterator->getReference());
}

void SceneUpdater::getNames(std::vector<std::string> &names){
  for(SceneUpdater * iterator=this;iterator!=NULL;iterator=iterator->child)
    names.push_back(iterator->getName());
}
/*NULL SCENE UPDATER*/

int NullSceneUpdater::needsUpdate(){
  return 0;
}

int NullSceneUpdater::updateScene(){
  finish=1;
  return 1;
}

int NullSceneUpdater::finished(){
  return finish;
}

double NullSceneUpdater::getReference(){
  return 1;
}

std::string NullSceneUpdater::getName(){
  return "None";
}

/*SCENE FOG UPDATER*/

int SceneFogUpdater::updateScene(){
  int updateLevel;

  if(child)
    updateLevel= child->updateScene() +1;
  if(!child || child->finished()){
    restartTimer();
    fog+=step;

    for(unsigned int i=0;i<cameras.size();i++){
      ((osg::Fog *)cameras[i]->getOrCreateStateSet()->getAttribute(osg::StateAttribute::FOG))->setDensity(fog);
      cameras[i]->getOrCreateStateSet()->getUniform("osgOcean_UnderwaterFogDensity")->set(-fog*fog*1.442695f);
    }
    scene->getOceanScene()->setUnderwaterFog(fog, osg::Vec4f(0,0.05,0.3,1) );
    if(child)
      child->restart();
    return 1;
  }
  return updateLevel;
} 

int SceneFogUpdater::finished(){
  return fog>finalFog;
}

SceneFogUpdater::SceneFogUpdater(double initialFog, double finalFog, double step, double interval, std::vector<osg::ref_ptr<osg::Camera> > cameras, osg::ref_ptr<osgOceanScene> scene): SceneUpdater(interval){
  this->initialFog=initialFog;
  this->fog=initialFog;
  this->finalFog=finalFog;
  this->step=step;
  this->cameras=cameras;
  this->scene=scene;

  for( unsigned int i=0;i<cameras.size();i++){
    ((osg::Fog *)cameras[i]->getOrCreateStateSet()->getAttribute(osg::StateAttribute::FOG))->setDensity(initialFog);
    cameras[i]->getOrCreateStateSet()->getUniform("osgOcean_UnderwaterFogDensity")->set(-this->initialFog*this->initialFog*1.442695f);
  }
  scene->getOceanScene()->setUnderwaterFog(initialFog, osg::Vec4f(0,0.05,0.3,1) );
}

double SceneFogUpdater::getReference(){
  return fog;
}

std::string SceneFogUpdater::getName(){
  return "Fog";
}

void SceneFogUpdater::restart(){
  fog=initialFog;

  for(unsigned int i=0;i<cameras.size();i++){
    ((osg::Fog *)cameras[i]->getOrCreateStateSet()->getAttribute(osg::StateAttribute::FOG))->setDensity(fog);
    cameras[i]->getOrCreateStateSet()->getUniform("osgOcean_UnderwaterFogDensity")->set(-fog*fog*1.442695f);
  }
  scene->getOceanScene()->setUnderwaterFog(fog, osg::Vec4f(0,0.05,0.3,1) );
}

/*Current Force Updater*/

int CurrentForceUpdater::updateScene(){
  int updateLevel;

  if(child)
    updateLevel= child->updateScene() +1;
  if(!child || child->finished()){
    restartTimer();
    myCurrent+=step;
    vehicle->setVehiclePosition(m);
    current->changeCurrentForce(myCurrent,2);
    std_srvs::Empty::Request request, response;
    ros::service::call("dynamics/reset",request, response); //Call to dynamics reset
    if(child)
      child->restart();
    return 1;
  }
  return updateLevel;
}

int CurrentForceUpdater::finished(){
  return myCurrent>finalCurrent;
}

CurrentForceUpdater::CurrentForceUpdater(double initialCurrent, double finalCurrent, double step, double interval, SimulatedIAUV *  vehicle,CurrentInfo currentInfo,int publishAs): SceneUpdater(interval){
  this->initialCurrent=initialCurrent;
  this->myCurrent=initialCurrent;
  this->finalCurrent=finalCurrent;
  this->step=step;
  this->vehicle=vehicle;
  this->current=(boost::shared_ptr<Current>) new Current(initialCurrent, currentInfo.dir,currentInfo.forceVar,currentInfo.forcePer,currentInfo.dirVar,currentInfo.dirPer,currentInfo.random);
  m=vehicle->baseTransform->getMatrix();
  pub=NULL;
  if(publishAs)
    pub= new CurrentToROSWrenchStamped("currentForce", 100, current, vehicle);

}

void CurrentForceUpdater::tick(){
  if(child)
    child->tick();
  if(!pub)
    current->applyCurrent(vehicle);
}

double CurrentForceUpdater::getReference(){
  return myCurrent;
}

std::string CurrentForceUpdater::getName(){
  return "Current";
}

void CurrentForceUpdater::restart(){
  myCurrent=initialCurrent;
}

/*Arm Move Updater*/

int ArmMoveUpdater::updateScene(){
  int updateLevel;

  if(child)
    updateLevel= child->updateScene() +1;
  if(!child || child->finished()){
    restartTimer();

    vehicle->urdf->setJointPosition(armPositions.front());
    armPositions.pop_front();
    if(child)
      child->restart();
    return 1;
  }
  return updateLevel;
}

int ArmMoveUpdater::finished(){
  return !armPositions.size();
}

ArmMoveUpdater::ArmMoveUpdater(std::list<std::vector <double> > armPositions, double steps, double interval,SimulatedIAUV *  vehicle): SceneUpdater(interval){

  this->armPositions.push_back(armPositions.front());
  armPositions.pop_front();
  while(armPositions.size()>0){
    std::vector<double> actual=this->armPositions.back();
    for(unsigned int i=1;i<=steps;i++){ //0 steps = just move from one position to the next one
      std::vector<double> nextStep;
      for(unsigned int j=0;j<armPositions.front().size();j++){
	nextStep.push_back(actual[j]+ (armPositions.front()[j]-actual[j])/(steps+1)*i);
      }
      this->armPositions.push_back(nextStep);
    }
    this->armPositions.push_back(armPositions.front());
    armPositions.pop_front();
  }
  this->steps=1;
  this->vehicle=vehicle;
  this->armPositions.push_back(std::vector<double> ()); //Needed to check if update is over

  vehicle->urdf->setJointPosition(this->armPositions.front());
  this->armPositions.pop_front();

}

double ArmMoveUpdater::getReference(){
  return steps;
}

std::string ArmMoveUpdater::getName(){
  return "ArmPosition";
}

void ArmMoveUpdater::restart(){
  std::cout<<"WARNING: ARMMOVEUPDATER RESTART NOT IMPLEMENTED"<<std::endl;
}

/*Repeat Updater*/

int RepeatUpdater::updateScene(){
  int updateLevel;

  if(child)
    updateLevel= child->updateScene() +1;
  if(!child || child->finished()){
    restartTimer();
    current++;
    if(child)
      child->restart();
    return 1;
  }
  return updateLevel;
}

int RepeatUpdater::finished(){
  return current>=iterations;
}

RepeatUpdater::RepeatUpdater(int iterations, double interval): SceneUpdater(interval){

  this->iterations=iterations;
  current=0;
}

double RepeatUpdater::getReference(){
  return current;
}

std::string RepeatUpdater::getName(){
  return "Iteration";
}

void RepeatUpdater::restart(){
  current=0;
}

/*SCENE LIGHT UPDATER*/

int SceneLightUpdater::updateScene(){
  int updateLevel;

  if(child)
    updateLevel= child->updateScene() +1;
  if(!child || child->finished()){
    restartTimer();
    light+=step;

    //Update light
    osg::Uniform* lightUniform = new osg::Uniform("light",(float)light);
    for(int i=0;i<scene->iauvFile.size();i++){
      for (unsigned int j=0; j<scene->iauvFile[i]->getNumCams(); j++) {
        scene->iauvFile[i]->camview[j].textureCamera->getOrCreateStateSet()->addUniform(lightUniform);
      }
    }
    if(child)
      child->restart();
    return 1;
  }
  return updateLevel;
} 

int SceneLightUpdater::finished(){
  return light>finalLight;
}

SceneLightUpdater::SceneLightUpdater(double initialLight, double finalLight, double step, double interval,SceneBuilder * builder): SceneUpdater(interval){
  this->initialLight=initialLight;
  this->light=initialLight;
  this->finalLight=finalLight;
  this->step=step;
  this->scene=builder;

  //set Initial light
  osg::Uniform* lightUniform = new osg::Uniform("light",(float)initialLight);
  for(int i=0;i<scene->iauvFile.size();i++){
    for (unsigned int j=0; j<scene->iauvFile[i]->getNumCams(); j++) {
      builder->iauvFile[i]->camview[j].textureCamera->getOrCreateStateSet()->addUniform(lightUniform);
    }
  }

}

double SceneLightUpdater::getReference(){
  return light;
}

std::string SceneLightUpdater::getName(){
  return "Light";
}

void SceneLightUpdater::restart(){
  light=initialLight;


  //set Light
  osg::Uniform* lightUniform = new osg::Uniform("light",(float)light);
  for(int i=0;i<scene->iauvFile.size();i++){
    for (unsigned int j=0; j<scene->iauvFile[i]->getNumCams(); j++) {
      scene->iauvFile[i]->camview[j].textureCamera->getOrCreateStateSet()->addUniform(lightUniform);
    }
  }
}
