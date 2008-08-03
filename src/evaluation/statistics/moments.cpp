/******************************************************************************
 * SPEETCL (SDL Performace Evaluation Tool Class Library)                     *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2004-2007                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: speetcl@comnets.rwth-aachen.de                                      *
 * www: http://speetcl.comnets.rwth-aachen.de                                 *
 ******************************************************************************/


#include "moments.hpp"

#include <cmath>
#include <climits>
#include <cfloat>

using namespace std;

STATIC_FACTORY_REGISTER_WITH_CREATOR(Moments,
									 StatEvalInterface,
									 "openwns.evaluation.statistics.Moments",
									 wns::PyConfigViewCreator);


//! Default constructor
Moments::Moments():
	StatEval(false, initialize, StatEval::fixed, "", ""),
	pd_wSum(0.0)
{}

Moments::Moments(std::string aName,
				 std::string aDescription,
				 formatType aFormat)   
    : StatEval(false, initialize, aFormat, aName, aDescription),
      pd_wSum(0.0)
{}

Moments::Moments(const wns::pyconfig::View& config) :
	StatEval(config),
	pd_wSum(0.0)
{}


//! Destructor
Moments::~Moments()

{}


//! Normal output
void Moments::print(ostream& aStreamRef) const
{
    std::string error_string;
    error_string = "I/O Error: Can't dump Moments results";

    pd_printBanner(aStreamRef,
                   (std::string) (pd_prefix + " Evaluation: Moments"),
		   error_string);
    if (!aStreamRef)
    {
        throw(wns::Exception(error_string));
    }
}


void Moments::put(double xI, double wI) 
{
    // only positive weights
    assert(wI > 0.0);
    // this causes the square to be positive
    double square  = wI * xI * xI;
    double cube    = xI * square;
    pd_sum          += wI * xI;

    // if all weights are 1, this is equal to pd_numTrials
    // pd_wSum is positive
    pd_wSum         += wI;

    pd_squareSum    += square;
    pd_cubeSum      += cube;

    if (xI > pd_maxValue)
    {
	pd_maxValue = xI;
    }
    if (xI < pd_minValue)
    {
	pd_minValue = xI;
    }
    pd_numTrials++;
}



//! put an unweighted value to probe
void Moments::put(double xI) 
{
    put(xI, 1.0);
}



//	      __ n              1   __ n
// E{x} = \        x * p  = _ * \        x
//        /_ i = 1      i   n   /_ i = 1

//! Return mean value of all collected values
double Moments::mean() const 
{
    return pd_numTrials ? pd_sum / pd_wSum : 0.0;
}


//  2      2     2      1   __ n      2    2
// c  = E{x } - E {x} = - * \        x  - E {x}
//                      n   /_ i = 1

//! Return variance of all collected values
double Moments::variance() const 
{
    double the_mean;

    if (pd_numTrials > 1)
    {
	the_mean = mean();
	assert((sqrt(DBL_MAX) > fabs(the_mean)));

	double square_mean = the_mean * the_mean;

	// pd_wSum is always positive
	assert(DBL_MAX / square_mean > pd_wSum);
	double sum_square = pd_wSum * square_mean;

	return ::max((pd_squareSum - sum_square) /
		     (pd_wSum - 1.0) , 0.0);
    }
    else
    {
	return 0.0;
    }
}

//! Returns the size of the 95%-confidence interval of the mean
double Moments::getConfidenceInterval95Mean() const 
{
	// ci = 0.95
	// --> p = 1-(1-cl)/2 = 0.975
	// --> norminv of p with mean 0 and variance 1 = 1.95996398454005
	return this->getConfidenceIntervalMean(1.95996398454005);
}

//! Returns the size of the 99%-confidence interval of the mean
double Moments::getConfidenceInterval99Mean() const 
{
	// see above
	return this->getConfidenceIntervalMean(2.57582930354890);
}

double
Moments::getConfidenceIntervalMean(double x) const
{
	if (this->pd_numTrials < 100)
	{
		// need at least 100 trials to approximate the student-t distribution
		// with the Gaussian distribution (--> Error < 5%)
		return (DBL_MAX);
	}

	double std = sqrt(this->variance());
	uint32_t n = this->trials();

	double cimu = (2*x*std)/(sqrt(n));
	return cimu;
}

//! return 2nd moment m2
double Moments::M2() const 
{
    return (pd_wSum > getMaxError<double>()) ?
	(pd_squareSum / pd_wSum) : 0.0 ;
}


//! return third moment m3
double Moments::M3() const 
{
    return (pd_wSum > getMaxError<double>()) ?
	(pd_cubeSum / pd_wSum) : 0.0 ;
}



//! reset measurement results
void Moments::reset() 
{
    StatEval::reset();
    pd_wSum      = 0;
}



/*
Local Variables:
mode: c++
folded-file: t
End:
*/

// }}
