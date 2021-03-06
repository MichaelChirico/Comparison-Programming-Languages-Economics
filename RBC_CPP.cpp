//============================================================================
// Name        : RBC_CPP.cpp
// Description : Basic RBC model with full depreciation
// Date        : July 21, 2013
//============================================================================

// AUXILIARY TIMER FUNCTIONS

#include <iostream>
#include <math.h>       // power
#include <cmath>        // abs
using namespace std;

// The next few lines are just for counting time
//  Windows
#ifdef _WIN32
#include <Windows.h>
double get_cpu_time(){
    FILETIME a,b,c,d;
    if (GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d) != 0){
        //  Returns total user time.
        //  Can be tweaked to include kernel times as well.
        return
        (double)(d.dwLowDateTime |
                 ((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
    }else{
        //  Handle error
        return 0;
    }
}
//  Posix/Linux
#else
#include <ctime>        // time
double get_cpu_time(){
    return (double)clock() / CLOCKS_PER_SEC;
}
#endif

int main() {
    
   double cpu0  = get_cpu_time();
  
  ///////////////////////////////////////////////////////////////////////////////////////////
  // 1. Calibration
  ///////////////////////////////////////////////////////////////////////////////////////////

  const double aalpha = 0.33333333333;     // Elasticity of output w.r.t. capital
  const double bbeta  = 0.95;              // Discount factor;

  // Productivity values

  double vProductivity[5] ={0.9792, 0.9896, 1.0000, 1.0106, 1.0212};

  // Transition matrix
  double mTransition[5][5] = {
			{0.9727, 0.0273, 0.0000, 0.0000, 0.0000},
			{0.0041, 0.9806, 0.0153, 0.0000, 0.0000},
			{0.0000, 0.0082, 0.9837, 0.0082, 0.0000},
			{0.0000, 0.0000, 0.0153, 0.9806, 0.0041},
			{0.0000, 0.0000, 0.0000, 0.0273, 0.9727}
			};

  ///////////////////////////////////////////////////////////////////////////////////////////
  // 2. Steady State
  ///////////////////////////////////////////////////////////////////////////////////////////

  double capitalSteadyState = pow(aalpha*bbeta,1/(1-aalpha));
  double outputSteadyState  = pow(capitalSteadyState,aalpha);
  double consumptionSteadyState = outputSteadyState-capitalSteadyState;

  cout <<"Output = "<<outputSteadyState<<", Capital = "<<capitalSteadyState<<", Consumption = "<<consumptionSteadyState<<"\n";
  cout <<" ";

  // We generate the grid of capital
  int nCapital, nCapitalNextPeriod, gridCapitalNextPeriod, nProductivity, nProductivityNextPeriod;
  const int nGridCapital = 17820, nGridProductivity = 5;
  double vGridCapital[nGridCapital] = {0.0};

  for (nCapital = 0; nCapital < nGridCapital; ++nCapital){
    vGridCapital[nCapital] = 0.5*capitalSteadyState+0.00001*nCapital;
  }

  // 3. Required matrices and vectors

  double mOutput[nGridCapital][nGridProductivity] = {0.0};
  double mValueFunction[nGridCapital][nGridProductivity] = {0.0};
  double mValueFunctionNew[nGridCapital][nGridProductivity] = {0.0};
  double mPolicyFunction[nGridCapital][nGridProductivity]= {0.0};
  double expectedValueFunction[nGridCapital][nGridProductivity] = {0.0};

  // 4. We pre-build output for each point in the grid

  for (nProductivity = 0; nProductivity<nGridProductivity; ++nProductivity){
    for (nCapital = 0; nCapital < nGridCapital; ++nCapital){
      mOutput[nCapital][nProductivity] = vProductivity[nProductivity]*pow(vGridCapital[nCapital],aalpha);
    }
  }

  // 5. Main iteration

  double maxDifference = 10.0, diff, diffHighSoFar;
  double tolerance = 0.0000001;
  double valueHighSoFar, valueProvisional, consumption, capitalChoice;

  int iteration = 0;

  while (maxDifference>tolerance){

    for (nProductivity = 0;nProductivity<nGridProductivity;++nProductivity){
      for (nCapital = 0;nCapital<nGridCapital;++nCapital){
	expectedValueFunction[nCapital][nProductivity] = 0.0;
	for (nProductivityNextPeriod = 0;nProductivityNextPeriod<nGridProductivity;++nProductivityNextPeriod){
	  expectedValueFunction[nCapital][nProductivity] += mTransition[nProductivity][nProductivityNextPeriod]*mValueFunction[nCapital][nProductivityNextPeriod];
	}
      }
    }

    for (nProductivity = 0;nProductivity<nGridProductivity;++nProductivity){

      // We start from previous choice (monotonicity of policy function)
      gridCapitalNextPeriod = 0;

      for (nCapital = 0;nCapital<nGridCapital;++nCapital){

	valueHighSoFar = -100000.0;
	capitalChoice  = vGridCapital[0];

	for (nCapitalNextPeriod = gridCapitalNextPeriod;nCapitalNextPeriod<nGridCapital;++nCapitalNextPeriod){

	  consumption = mOutput[nCapital][nProductivity]-vGridCapital[nCapitalNextPeriod];
	  valueProvisional = (1-bbeta)*log(consumption)+bbeta*expectedValueFunction[nCapitalNextPeriod][nProductivity];

	  if (valueProvisional>valueHighSoFar){
	    valueHighSoFar = valueProvisional;
	    capitalChoice = vGridCapital[nCapitalNextPeriod];
	    gridCapitalNextPeriod = nCapitalNextPeriod;
	  }
	  else{
	    break; // We break when we have achieved the max
	  }

	  mValueFunctionNew[nCapital][nProductivity] = valueHighSoFar;
	  mPolicyFunction[nCapital][nProductivity] = capitalChoice;
	}

      }

    }

    diffHighSoFar = -100000.0;
    for (nProductivity = 0;nProductivity<nGridProductivity;++nProductivity){
      for (nCapital = 0;nCapital<nGridCapital;++nCapital){
	diff = std::abs(mValueFunction[nCapital][nProductivity]-mValueFunctionNew[nCapital][nProductivity]);
	if (diff>diffHighSoFar){
	  diffHighSoFar = diff;
	}
	mValueFunction[nCapital][nProductivity] = mValueFunctionNew [nCapital][nProductivity];
      }
    }
    maxDifference = diffHighSoFar;

    iteration = iteration+1;
    if (iteration % 10 == 0 || iteration ==1){
      cout <<"Iteration = "<<iteration<<", Sup Diff = "<<maxDifference<<"\n";
    }
  }

  cout <<"Iteration = "<<iteration<<", Sup Diff = "<<maxDifference<<"\n";
  cout <<" \n";
  cout <<"My check = "<< mPolicyFunction[999][2]<<"\n";
  cout <<" \n";
  
  double cpu1  = get_cpu_time();

  cout << "Elapsed time is   = " << cpu1  - cpu0  << endl;
    
  cout <<" \n";  

  return 0;

}
