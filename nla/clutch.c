#define USE_OCTAVE_IO   1
#include "nonsmooth_clutch.h"
#include <stdio.h>



int main(){
 
  double step = 1.0 / 20.0;
  nonsmooth_clutch_params p ={
    {5.0,5.0, 5.0, 5000},           // masses
    539,			       // first spring constant: Scania data
    4.3927e4,		       // second spring constant: Scania data
    200,                        // pressure on plates
    1.0,                        // friction coefficient
    100,                        // torque on input shaft
    0,                           // torque on output shaft
    step,                // time step
    2.0 * step,                         // damping
    { -0.052359877559829883, -0.087266462599716474}, // lower limits: Scania data
    {0.09599310885968812 , 0.17453292519943295 },    // upper limits: Scania data
    0.0,                       // compliance 1
    0.0,                       // compliance 2
    0.0,                       // plate slip
    {0.0,0.0,0.0,0.0},                   // initial position
    {8.0,0.0,0.0,0.0}                    // initial velocity
  };

  void * sim = nonsmooth_clutch_init( p );

  for ( size_t i = 0; i < 100; ++i ){ 
    nonsmooth_clutch_step( sim, 1 );
    nonsmooth_clutch_params * q = ( nonsmooth_clutch_params * ) sim;
#if 0
    fprintf(stderr, "%g  ",  i * p.step);
    for ( size_t j = 0; j < sizeof(p.x) / sizeof( p.x[ 0 ] ) ; ++j){
      fprintf(stderr, "%g ", q->x[ j ] );
    }
#endif
    fprintf(stderr, "%g  ",  i * p.step);
    for ( size_t j = 0; j < sizeof(p.x) / sizeof( p.x[ 0 ] ) ; ++j){
      fprintf(stderr, "%g ", q->v[ j ] );
    }
    fputs("\n", stderr);
#if 0
    for ( size_t j = 0; j < sizeof(p.constraint_state) / sizeof( p.constraint_state[ 0 ] ) ; ++j){
      fprintf(stderr, "%g ", q->constraint_state[ j ] );
    }

    for ( size_t j = 0; j < sizeof(p.lambda) / sizeof( p.lambda[ 0 ] ) ; ++j){
      fprintf(stderr, "%g ", q->lambda[ j ] );
    }

    for ( size_t j = 0; j < sizeof(p.lambda) / sizeof( p.lambda[ 0 ] ) ; ++j){
      fprintf(stderr, "%g ", q->constraints[ j ] );
    }

    fprintf(stderr, "%d ", q->has_impact );

    fprintf(stderr, "%g \n", q->drive_torque );
#endif
  }

  nonsmooth_clutch_free( sim );

  return 0;
}
