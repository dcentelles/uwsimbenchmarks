#ifndef CURRENT_H_
#define CURRENT_H_
#include "SimulatedIAUV.h"

class Current {
private:
  double waitFor;
  double module, moduleVariation, modulePeriod, randomNoise;
  double direction[2],directionVariation[2], directionPeriod[2];
  ros::WallTime last,init;
public:

  Current(double module, double direction[], double moduleVariation, double modulePeriod, double directionVariation[], double directionPeriod[], double randomNoise);
  void applyCurrent(std::vector<boost::shared_ptr<SimulatedIAUV> > iauvFile);
  void changeCurrentForce(double modul,double wait=0);
};

#endif
