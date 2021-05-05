#include <stdio.h>
#include <stdlib.h>

// Helper Functions 

int simulateLoss(double packetLoss);
int simulateACKLos(double loss);

int simulateLoss(double packetLoss){
  double random = (double)rand() / (double)RAND_MAX;
  if(random < packetLoss){
    return 1;
  }
  return 0;
}

int simulateACKLoss(double loss){
  double ratioLoss = ((double)rand() / (double)RAND_MAX);
  if(ratioLoss < loss){
    return 1;
  }
  return 0;
}