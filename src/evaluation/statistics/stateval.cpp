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

#include <WNS/pyconfig/View.hpp>
#include <WNS/simulator/ISimulator.hpp>
#include <WNS/Exception.hpp>

#include "stateval.hpp"
#include <iomanip>
#include <climits>
#include <cfloat>
#include <cmath>

using namespace std;



STATIC_FACTORY_REGISTER_WITH_CREATOR(StatEval,
									 StatEvalInterface,
									 "wns.probe.stateval.StatEval",
									 wns::PyConfigViewCreator);


//! Default constructor

StatEval::StatEval(bool anActiveFlag,
				   phaseType aStartingPhase,
				   formatType aFormat,
				   std::string aName,
				   std::string aDesc)
	: pd_active(anActiveFlag),
	  pd_minValue(DBL_MAX),
	  pd_maxValue(-DBL_MAX),
	  pd_numTrials(0),
	  pd_sum(0.0),
	  pd_squareSum(0.0),
	  pd_cubeSum(0.0),
	  pd_phase(aStartingPhase),
	  pd_format(aFormat),
	  pd_name(aName),
	  pd_desc(aDesc),
	  pd_prefix("#")
{
}


StatEval::StatEval(const wns::pyconfig::View& config) :
	pd_active(false),
	pd_minValue(DBL_MAX),
	pd_maxValue(-DBL_MAX),
	pd_numTrials(0),
	pd_sum(0.0),
	pd_squareSum(0.0),
	pd_cubeSum(0.0),
	pd_phase(StatEval::initialize),
	pd_format((config.get<std::string>("format")=="scientific") ? StatEval::scientific : StatEval::fixed),
	pd_name(config.get<std::string>("name")),
	pd_desc(config.get<std::string>("description")),
	pd_prefix(config.get<std::string>("prefix"))
{}




//! Destructor
StatEval::~StatEval()
{}


void StatEval::print(std::ostream& aStreamRef) const
{
//    String error_string("I/O Error: Can't dump StatEval results");
    this->pd_printBanner(aStreamRef,
						 std::string(pd_prefix + " Evaluation: StatEval"),
						 std::string("I/O Error: Can't dump StatEval results"));
}

void StatEval::printLog(std::ostream& /*aStreamRef*/)
{
        // Default implementation does nothing
}

void StatEval::put(double xI)  {
	if(xI > pd_maxValue)
                pd_maxValue = xI;
	if(xI < pd_minValue)
                pd_minValue = xI;

	double tmp = xI;
	pd_sum += tmp;
	tmp *= xI;
	pd_squareSum += tmp;
	tmp *= xI;
	pd_cubeSum += tmp;

	++pd_numTrials;
}



//		  __ n				1	__ n
// E{x} = \		   x * p  = _ * \		 x
//		  /_ i = 1		i	n	/_ i = 1

//! Return mean value of all collected values
double StatEval::mean() const 
{
	return pd_numTrials ? pd_sum / (double)pd_numTrials : 0.0;
}


//	2	   2	 2		1	__ n	  2	   2
// c  = E{x } - E {x} = - * \		 x	- E {x}
//						n	/_ i = 1

//! Return variance of all collected values
double StatEval::variance() const 
{
	double the_mean;

	if (pd_numTrials > 1)
	{
		the_mean = mean();
		assert((sqrt(DBL_MAX) > fabs(the_mean)));

		double square_mean = the_mean * the_mean;

		assert(DBL_MAX / square_mean > (double)pd_numTrials);
		double sum_square = (double)pd_numTrials * square_mean;

		return ::max((pd_squareSum - sum_square) /
					 ((double)pd_numTrials - 1.0) , 0.0);

	}
	else
	{
		return 0.0;
	}
}


//! return relative variance
double StatEval::relativeVariance() const 
{
	double the_mean = mean();

	if (fabs(the_mean) > getMaxError<double>())
	{
		assert((sqrt(DBL_MAX) > fabs(the_mean)));
		return (variance() / (the_mean * the_mean));
	}
	else
	{
		return 0.0;
	}
}


//! return coefficient of variation
double StatEval::coeffOfVariation() const 
{
	double rel_var = relativeVariance();

	return rel_var > 0.0 ? sqrt(rel_var): 0.0;
}


//! return 2nd moment m2
double StatEval::M2() const 
{
	return (pd_numTrials) ?
		(pd_squareSum / (double)pd_numTrials) : 0.0 ;
}


//! return third moment m3
double StatEval::M3() const 
{
	return (pd_numTrials) ?
		(pd_cubeSum / (double)pd_numTrials) : 0.0 ;
}


//! return Z3
double StatEval::Z3() const 
{
	double m1 = mean();
	double m2 = M2();
	double m3 = M3();

	assert(DBL_MAX / 2.0 / fabs(m1) / fabs(m1) > fabs(m1));

	double two_m1_cube = 2.0 * m1 * m1 * m1;

	assert(DBL_MAX / 3.0 / fabs(m1) > 3.0 * fabs(m2));

	double three_m1_m2 = 3.0 * m1 * m2;

	if (m3 > 0.0)
	{
		// m3 - three_m1_2 > -DBL_MAX, as m3 > 0
		// m3 - three_m1_2 < DBL_MAX
		assert(three_m1_m2 > - ( DBL_MAX - m3));
	}
	else
	{
		// m3 - three_m1_m2 < DBL_MAX, as m3 < 0
		// m3 - three_m1_m2 > -DBL_MAX
		// - three_m1_m2 > -DBL_MAX - m3
		assert(- three_m1_m2 > -DBL_MAX - m3);
	}
	if (two_m1_cube > 0.0)
	{
		assert(m3 - three_m1_m2 < DBL_MAX - two_m1_cube);
	}
	else
	{
		assert(m3 - three_m1_m2 > -DBL_MAX - two_m1_cube);
	}

	return m3 - three_m1_m2 + two_m1_cube;
}


//! return skewness of distribution
double StatEval::skewness() const 
{
	double var = variance();
	assert(sqrt(var) < pow(DBL_MAX, 1.0/3.0));

	return (var > getMaxError<double>()) ? Z3() / (var * sqrt(var)) : 0.0;
}


//! return standard deviation
double StatEval::deviation() const 
{
	double var = variance();
	return (var > getMaxError<double>()) ? sqrt(var) : 0.0;
}


//! return relative standard deviation
double StatEval::relativeDeviation() const 
{
	double m1 = mean();
	double var = variance();

	return ((fabs(m1) > getMaxError<double>()) &&
			(var > getMaxError<double>())) ? sqrt(var) / m1 : 0.0;
}



//! Return number of evaluated values
uint32_t StatEval::trials() const 
{
	return pd_numTrials;
}



//! Return smallest encountered value of the examined random variable
double StatEval::min() const 
{
	return pd_minValue;
}


//! Return smallest encountered value of the examaxed random variable
double StatEval::max() const 
{
	return pd_maxValue;
}



//! reset measurement results
void StatEval::reset() 
{
	pd_numTrials = 0;
	pd_sum		 = 0;
	pd_squareSum = 0;
	pd_cubeSum	 = 0;
	pd_minValue	 =	DBL_MAX;
	pd_maxValue	 = -DBL_MAX;
}


/*! Have enough values been entered in order to guarantee a
  statistically certified result? */
bool StatEval::end() const 
{
	return ((pd_active == false) || (pd_phase == finish));
}


/*! Return current status of evaluation algorithm: initialize,
  iterate, or finish. */
StatEval::phaseType StatEval::status() const 
{
	return pd_phase;
}



//! Name of evaluation
const std::string& StatEval::getName() const 
{
	return pd_name;
}


//! Description of evaluation
const std::string& StatEval::getDesc() const 
{
	return pd_desc;
}



/*! Set active flag. This probe is an active one, i.e. PDataBase::end() should
  take the result of this->end() into account. */
void StatEval::setActive(bool aFlag) 
{
	pd_active = aFlag;
}


//! Get active flag
bool StatEval::getActive() const 
{
	return pd_active;
}



//! Set the output format: fixed or scientific.
void StatEval::setFormat(formatType aFormat) 
{
	pd_format = aFormat;
}


//! Get the output format: fixed or scientific.
StatEval::formatType StatEval::getFormat() const 
{
	return pd_format;
}



//! map string to statEvalType
StatEval::statEvalType StatEval::mapToStatEvalType(std::string aType)
	
{
	StatEval::statEvalType stat_type;


	if (aType == std::string("LREF"))
	{
		stat_type = StatEval::lref;
	}
	else if (aType == std::string("LREG"))
	{
		stat_type = StatEval::lreg;
	}
	else if (aType == std::string("PLREF"))
	{
		stat_type = StatEval::plref;
	}
	else if (aType == std::string("PLREG"))
	{
		stat_type = StatEval::plreg;
	}
	else if (aType == std::string("Moments"))
	{
		stat_type = StatEval::moments;
	}
	else if (aType == std::string("PMoments"))
	{
		stat_type = StatEval::pmoments;
	}
	else if (aType == std::string("BatchMns"))
	{
		stat_type = StatEval::batchMns;
	}
	else if (aType == std::string("PBatchMns"))
	{
		stat_type = StatEval::pbatchMns;
	}
	else if (aType == std::string("Histogrm"))
	{
		stat_type = StatEval::histogrm;
	}
	else if (aType == std::string("PHistogrm"))
	{
		stat_type = StatEval::phistogrm;
	}
	else if (aType == std::string("DLREF"))
	{
		stat_type = StatEval::dlref;
	}
	else if (aType == std::string("DLREG"))
	{
		stat_type = StatEval::dlreg;
	}
	else if (aType == std::string("DLREP"))
	{
		stat_type = StatEval::dlrep;
	}
	else if (aType == std::string("PDLREF"))
	{
		stat_type = StatEval::pdlref;
	}
	else if (aType == std::string("PDLREG"))
	{
		stat_type = StatEval::pdlreg;
	}
	else if (aType == std::string("PDLREP"))
	{
		stat_type = StatEval::pdlrep;
	}
	else if (aType == std::string("DLREF_NONEQUI"))
	{
		stat_type = StatEval::dlref_nonequi;
	}
	else if (aType == std::string("DLREG_NONEQUI"))
	{
		stat_type = StatEval::dlreg_nonequi;
	}
	else if (aType == std::string("DLREP_NONEQUI"))
	{
		stat_type = StatEval::dlrep_nonequi;
	}
	else if (aType == std::string("PDLREF_NONEQUI"))
	{
		stat_type = StatEval::pdlref_nonequi;
	}
	else if (aType == std::string("PDLREG_NONEQUI"))
	{
		stat_type = StatEval::pdlreg_nonequi;
	}
	else if (aType == std::string("PDLREP_NONEQUI"))
	{
		stat_type = StatEval::pdlrep_nonequi;
	}
	else if (aType == std::string("PDF"))
	{
		stat_type = StatEval::pdf;
	}
	else if (aType == std::string("PPDF"))
	{
		stat_type = StatEval::ppdf;
	}
	else if (aType == std::string("Log"))
	{
		stat_type = StatEval::logeval;
	}
	else if (aType == std::string("PLog"))
	{
		stat_type = StatEval::plogeval;
	}
	else if (aType == std::string("ProbeText"))
	{
		stat_type = StatEval::probetext;
	}
	else if (aType == std::string("PProbeText"))
	{
		stat_type = StatEval::pprobetext;
	}
	else
	{
		throw wns::Exception("StatEval: Unknown type '" + aType + "'!");
	}

	return stat_type;
}


//! map statEvalType to string, reduce to basic types
std::string StatEval::mapEvalTypeToString(statEvalType aType)   
{


	switch(aType)
	{
	  case StatEval::lref:
	  case StatEval::plref:

		return std::string("LREF");
		break;
	  case StatEval::lreg:
	  case StatEval::plreg:

		return std::string("LREG");
		break;

	  case StatEval::moments:
	  case StatEval::pmoments:

		return std::string("Mom");
		break;

	  case StatEval::batchMns:
	  case StatEval::pbatchMns:

		return std::string("BaM");
		break;

	  case StatEval::histogrm:
	  case StatEval::phistogrm:

		return std::string("His");
		break;

	  case StatEval::dlref:
	  case StatEval::pdlref:
	  case StatEval::dlref_nonequi:
	  case StatEval::pdlref_nonequi:

		return std::string("DLREF");
		break;

	  case StatEval::dlreg:
	  case StatEval::pdlreg:
	  case StatEval::dlreg_nonequi:
	  case StatEval::pdlreg_nonequi:

		return std::string("DLREG");
		break;

	  case StatEval::dlrep:
	  case StatEval::pdlrep:
	  case StatEval::dlrep_nonequi:
	  case StatEval::pdlrep_nonequi:

		return std::string("DLREP");
		break;

	  case StatEval::pdf:
	  case StatEval::ppdf:

		return std::string("PDF");
		break;

	  case StatEval::logeval:
	  case StatEval::plogeval:

		return std::string("Log");
		break;

	  case StatEval::probetext:
	  case StatEval::pprobetext:

		return std::string("Text");
		break;

	  default:

          throw wns::Exception("StatEval: Unknown type");
          break;
	}

}



//! print the banner containing common statistics
void StatEval::pd_printBanner(ostream& aStreamRef,
                              std::string aProbeTypeDesc,
                              std::string anErrorString) const
{
	std::string prefix(pd_prefix + " ");

	std::string separator = prefix + "-------------------------------------";
	separator += "--------------------------------------\n";

	aStreamRef << prefix + " PROBE RESULTS (THIS IS A MAGIC LINE)"
			   << endl
			   << separator
			   << aProbeTypeDesc
			   << endl
			   << separator
			   << prefix+ " Name: "
			   << getName()
			   << endl
			   << prefix + " Description: "
			   << getDesc()
			   << endl
			   << separator;
	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}

	if (pd_active)
	{
		// stochastical evaluation terminated successfully?
		if (pd_phase == finish)
		{
			aStreamRef << prefix + " Evaluation terminated successfully!"
					   << endl
					   << separator;
			if (!aStreamRef)
			{
				throw(wns::Exception(anErrorString));
			}
		}
		else
		{
			aStreamRef << prefix + " Evaluation is still in progress!"
					   << endl
					   << separator;
			if (!aStreamRef)
			{
				throw(wns::Exception(anErrorString));
			}
		}
	}
	else
	{
		aStreamRef << prefix + " No active probe" << endl << separator;
		if (!aStreamRef)
		{
			throw(wns::Exception(anErrorString));
		}
	}

	aStreamRef << resetiosflags(ios::fixed)
			   << resetiosflags(ios::scientific)
			   << resetiosflags(ios::right)
			   << setiosflags(ios::left)
			   << setiosflags(ios::dec)
			   << setprecision(7)
			   << setw(6)
			   << (pd_format == fixed ? setiosflags(ios::fixed) :
				   setiosflags(ios::scientific))
			   << prefix + " Common Statistics " << endl



			   << prefix + " Minimum: ";
	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}
	if (pd_minValue == DBL_MAX)
	{
		aStreamRef << "+infinity";
	}
	else
	{
		aStreamRef << pd_minValue;
	}


	aStreamRef << endl
			   << prefix + " Maximum: ";
	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}
	if (pd_maxValue == -DBL_MAX)
	{
		aStreamRef << "-infinity";
	}
	else
	{
		aStreamRef << pd_maxValue;
	}
	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << endl
			   << prefix + " Trials: "
			   << pd_numTrials
			   << endl


			   << prefix + " Mean: ";
	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}
	if (mean() == DBL_MAX)
	{
		aStreamRef << "+infinity";
	}
	else
	{
		aStreamRef << mean();
	}
	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << endl << prefix << endl
			   << prefix + " Variance: "
			   << variance()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix + " Relative variance: "
			   << relativeVariance()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix + " Coefficient of variation: "
			   << coeffOfVariation()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}



	aStreamRef << prefix + " Standard deviation: "
			   << deviation()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix + " Relative standard deviation: "
			   << relativeDeviation()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix << endl
			   << prefix + " Skewness: "
			   << skewness()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix << endl
			   << prefix + " 2nd moment: "
			   << M2()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix + " 3rd moment: "
			   << M3()
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix << endl
			   << prefix + " Sum of all values: "
			   << pd_sum
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix + " (Sum of all values)^2: "
			   << pd_squareSum
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


	aStreamRef << prefix + " (Sum of all values)^3: "
			   << pd_cubeSum
			   << endl;

	if (!aStreamRef)
	{
		throw(wns::Exception(anErrorString));
	}


}




/*
Local Variables:
mode: c++
folded-file: t
End:
*/

