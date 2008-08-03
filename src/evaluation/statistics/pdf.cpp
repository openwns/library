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


#include "pdf.hpp"

#include <cmath>
#include <iomanip>
#include <climits>
#include <cfloat>

using namespace std;

STATIC_FACTORY_REGISTER_WITH_CREATOR(PDF,
									 StatEvalInterface,
									 "openwns.evaluation.statistics.PDF",
									 wns::PyConfigViewCreator);

//! Default constructor

PDF::PDF(double aMinXValue,
		 double aMaxXValue,
		 uint32_t aResolution,
		 scaleType aScaleType,
		 formatType aFormat,
		 const char* aName,
		 const char* aDescription) 
    : StatEval(false, initialize, aFormat, aName, aDescription),
      p_minXValue(aMinXValue),
      p_maxXValue(aMaxXValue),
      p_resolution(aResolution),
      p_scaleType(aScaleType),
      p_values(p_resolution+1, 0),
      p_underFlows(0),
      p_overFlows(0)
{
	assure(p_minXValue < p_maxXValue,
		   "Wrong min/max values. min=" << p_minXValue << ", max="<<p_maxXValue);
	assure(p_resolution > 0, "Resolution must be >0, but is " << p_resolution);
}

PDF::PDF(const wns::pyconfig::View& config)
    : StatEval(config),
      p_minXValue(config.get<double>("minXValue")),
      p_maxXValue(config.get<double>("maxXValue")),
      p_resolution(config.get<int>("resolution")),
      p_scaleType(config.get<std::string>("xScaleType") == "logarithmical" ? logarithmical : linear),
      p_values(p_resolution+1, 0),
      p_underFlows(0),
      p_overFlows(0)
{
	assure(p_minXValue < p_maxXValue,
		   "Wrong min/max values. min=" << p_minXValue << ", max="<<p_maxXValue);
	assure(p_resolution > 0, "Resolution must be >0, but is " << p_resolution);
}


//! Default copy constructor is correct

PDF::PDF(const PDF& aPDFRef) 
    : StatEval(aPDFRef),
      p_minXValue(0.0),
      p_maxXValue(1.0),
      p_resolution(100),
      p_scaleType(linear),
      p_values(),
      p_underFlows(0),
      p_overFlows(0)
{
    // Copy all internal data
    p_minXValue                   = aPDFRef.p_minXValue;
    p_maxXValue                   = aPDFRef.p_maxXValue;
    p_resolution                  = aPDFRef.p_resolution;
    p_scaleType                   = aPDFRef.p_scaleType;
    p_values                      = aPDFRef.p_values;
    p_name                        = aPDFRef.p_name;
    p_desc                        = aPDFRef.p_desc;
}



//! Destructor
PDF::~PDF()
{
}


//! Normal output
void PDF::print(ostream& aStreamRef) const 
{
    std::string error_string;
    error_string = "I/O Error: Can't dump PDF results";

    pd_printBanner(aStreamRef,
		   (std::string) (pd_prefix + " Evaluation: PDF"),
		   error_string);
    if (!aStreamRef)
    {
  	throw(wns::Exception(error_string));
    }
    std::string prefix(pd_prefix + " ");

    std::string separator;
    uint32_t i = 0, num_trials = 0;
    double x = 0.0, f = 0.0, g = 0.0, p = 0.0;

    separator = prefix + "-------------------------------------";
    separator += "----------------------";



    aStreamRef << resetiosflags(ios::fixed)
	       << resetiosflags(ios::scientific)
	       << resetiosflags(ios::right)
	       << setiosflags(ios::left)
	       << setiosflags(ios::dec)
	       << setprecision(7)
	       << setw(6)

	       << separator << endl << prefix
	       << "PDF statistics " << endl << prefix
	       << " Left border of x-axis: "
	       << p_minXValue
	       << endl << prefix
	       << " Right border of x-axis: "
	       << p_maxXValue
	       << endl << prefix
	       << " Resolution of x-axis: "
	       << p_resolution
	       << endl
	       << separator
	       << endl;

    aStreamRef << prefix
	       << "PDF data " << endl << prefix
	       << " Underflows: "
               << p_underFlows << endl;

    aStreamRef << prefix
	       << " Underflows in percent: "
	       << ((pd_numTrials > 0) ? (double)(p_underFlows) /
		   (double)pd_numTrials : 0.0)
	       << endl;

    aStreamRef << prefix
	       << " Overflows: "
               << (p_overFlows) << endl;

    aStreamRef << prefix
	       << " Overflows in percent: "
	       << ((pd_numTrials > 0) ?
		   (double)(p_overFlows) /
		   (double)pd_numTrials : 0.0) << endl;

    aStreamRef << separator
			   << endl << prefix
			   << "Percentiles" << endl;

	aStreamRef << prefix; this->p_printPercentile( 1, aStreamRef);
	aStreamRef << prefix; this->p_printPercentile( 5, aStreamRef);
	aStreamRef << prefix; this->p_printPercentile(50, aStreamRef);
	aStreamRef << prefix; this->p_printPercentile(95, aStreamRef);
	aStreamRef << prefix; this->p_printPercentile(99, aStreamRef);

    aStreamRef << separator
	       << endl << prefix << endl << prefix
		   << "x_n                  F(x_n)           G(x_n)     P(x_n-1 < X    n"
		   << endl << prefix
		   << "                   =P(X<=x_n)       =P(X>x_n)   AND X <= x_n)"
	       << endl << prefix << endl
	       << (pd_format == fixed ? setiosflags(ios::fixed) :
		   setiosflags(ios::scientific));


    if (!aStreamRef)
    {
	throw(wns::Exception(error_string));
    }

	// Handle the normal intervals
    for (i = 0, num_trials = p_underFlows;
		 i <= p_resolution; ++i)
    {
		// Normal interval
	    x = p_getAbscissa(i);
	    num_trials += p_values.at(i);

	    if (pd_numTrials != 0)
	    {
			f = double(num_trials) / double(pd_numTrials);
			assert(0.0 <= f && f <= 1.0);
			g = 1.0 - f;
			p = double(p_values.at(i)) / double(pd_numTrials);
	    }
	    else
	    {
			f = g = p = 0.0;
	    }

		aStreamRef << resetiosflags(ios::right)
				   << setiosflags(ios::left)
				   << setw(15)
				   << x
				   << resetiosflags(ios::left)
				   << setiosflags(ios::right)
				   << setw(14)
				   << f
				   << "  "
				   << setw(14)
				   << g
				   << "  "
				   << setw(14)
				   << p
				   << "  "
				   << setw(4)
				   << i
				   << endl;
		if (!aStreamRef)
		{
			throw(wns::Exception(error_string));
		}
    }

    aStreamRef << separator;

    if (!aStreamRef)
    {
	throw(wns::Exception(error_string));
    }
}


//! Input a value to the statistical evaluation
void PDF::put(double aValue) 
{
    StatEval::put(aValue);

    // Underflow?
    if (aValue < p_minXValue)
    {
		++p_underFlows;
    }
    // Overflow?
    else if (aValue > p_maxXValue)
    {
		++p_overFlows;
    }
	else
	{
		p_values.at(this->p_getIndex(aValue))++;
	}
}


uint32_t
PDF::p_getIndex(double aValue) const 
{
    assert((aValue >= p_minXValue && aValue <= p_maxXValue) && "Huge, Fat Error!");

	if (this->p_scaleType == PDF::linear)
	{
		return uint32_t(
			ceil((aValue - p_minXValue) * double(p_resolution) /
								(p_maxXValue - p_minXValue)));
	}
	else if (this->p_scaleType == PDF::logarithmical)
	{
		double logXMin = log10(p_minXValue);
		double logXMax = log10(p_maxXValue);
		double logValue = log10(aValue);
		double logXStep = (logXMax - logXMin)/double(p_resolution);

		return  uint32_t( ceil ((logValue - logXMin)/logXStep) );
	}

	throw wns::Exception("Unknown scaleType in PDF!");
	return 0;
}

//! Reset evaluation algorithm to its initial state
void PDF::reset() 
{
    StatEval::reset();
    p_values = std::vector<int>(p_resolution + 1);
}

double PDF::p_getAbscissa(uint32_t anIndex) const 
{
	if (this->p_scaleType == PDF::linear)
	{
		return p_minXValue
			+ (p_maxXValue - p_minXValue) * double(anIndex) / double(p_resolution);
	}
	else if (this->p_scaleType == PDF::logarithmical)
	{
		double logXMin = log10(p_minXValue);
		double logXMax = log10(p_maxXValue);
		double logXStep = (logXMax - logXMin)/double(p_resolution);
		return double(pow(10, logXMin + double(anIndex) * logXStep));
	}
	else
	{
		throw wns::Exception("Unknown scaleType in PDF!");
	}
}

double
PDF::p_getPercentile(int p) const
{
	assert(p>0 && "Percentile has to be greater than 0!");

	if (pd_numTrials == 0)
		return 0.0;

	double P = double(p)/100.0;

	if ( P<= double(p_underFlows)/double(pd_numTrials) )
		throw PercentileUnderFlow();

	if ( P > 1.0 - double(p_overFlows)/double(pd_numTrials) )
		throw PercentileOverFlow();

	double x = 0.0;
	double F = 0.0;
	double dummy1 = 0.0;
	double dummy2 = 0.0;
	double tmpF = 0.0;
	double tmpX = 0.0;

	this->getResult(0, x, F, dummy1, dummy2);

	if (F >= P)
		throw PercentileUnderFlow();

    for (unsigned int ii = 1; ii <= p_resolution; ++ii)
    {
		this->getResult(ii, tmpX, tmpF, dummy1, dummy2);

		if (F<P && tmpF >= P)
		{
			if (tmpF == 1.0)
				tmpX = pd_maxValue;

			return x + (tmpX-x) / (tmpF-F) * (P-F);
		}

		F = tmpF;
		x = tmpX;
    }

	throw wns::Exception("This point should never be reached");
}

void
PDF::p_printPercentile(int p, std::ostream& aStreamRef) const 
{
	stringstream ss;

	ss << " P"
	   << setiosflags(ios::right) << setfill('0') << setw(2)
	   << p << ": ";

	try
	{
		ss << setiosflags(ios::dec) << setprecision(7) << setw(6)
		   << setiosflags(ios::fixed) << this->p_getPercentile(p);
	}
	catch (PercentileUnderFlow)
	{
		ss << "NaN";
	}
	catch (PercentileOverFlow)
	{
		ss << "NaN";
	}
	aStreamRef << ss.str() << endl;
}


//! Return statistical information of the given interval
void PDF::getResult(uint32_t anIndex, double& anAbscissa,
		    double& anF, double& aG,
		    double& aP) const 
{
    uint32_t i;
    uint32_t num_trials = p_underFlows;

    assert(p_resolution);

    if (pd_numTrials == 0)
    {
		anAbscissa = 0.0;
		anF = 0.0;
		aG = 0.0;
		aP = 0.0;
		return;
    }

    for (i = 0; i <= anIndex; i++)
    {
		num_trials += p_values.at(i);
    }
	anAbscissa = p_getAbscissa(anIndex);
    anF = double(num_trials) / double(pd_numTrials);
    assert(0.0 <= anF && anF <= 1.0);
    aG = 1.0 - anF;
    aP = double(p_values.at(anIndex)) / double(pd_numTrials);
}



/*
Local Variables:
mode: c++
folded-file: t
End:
*/

