/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
 * phone: ++49-241-80-27910,
 * fax: ++49-241-80-22242
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <WNS/evaluation/statistics/dlre.hpp>

#include <cmath>
#include <algorithm>

using namespace std;
using namespace wns::evaluation::statistics;

//! Constructor for non-equi-distant x-values
DLRE::DLRE(vector<double> xValuesArr,
           int level,
           double error,
           double preFirst,
           string name,
           string description,
           bool forceRminusAOK,
           int maxNrv,
           int skipInterval,
           formatType format)
    : StatEval(format, name, description),
      results_(NULL),
      line_(),
      relErrMax_(error),
      maxNrv_(maxNrv),
      wastedLeft_(0),
      wastedRight_(0),
      h_(0),
      xOffset_(0),
      xMin_(0.0),
      xMax_(0.0),
      indexMin_(0),
      indexMax_(level),
      equiDist_(false),
      intSize_(0.0),
      curIndex_(0),
      preRv_(preFirst),
      preIndex_(0),
      base_(1.0),
      reason_(ok),
      curLevelIndex_(0),
      skipInterval_(skipInterval),
      forceRminusAOK_(forceRminusAOK),
      phase_(initialize)
{
    this->initNonEqui(level, xValuesArr, preFirst);
}

void
DLRE::initNonEqui(int level,
                  vector<double> xValuesArr,
                  double preFirst)
{
    indexMax_ = level;

    assure(level > 1, "Number of levels must be larger than 1");

    // one element more because of getIndex_
    results_ = new Result [level + 1];

    if (xValuesArr.empty())
    {
        // fill results_[].x with default-values
        // 0 ... level-1
        for (int i = 0; i <= level; i++)
        {
            results_[i].x_ = i;
            results_[i].h_ = 0;
            results_[i].sumh_ = 0;
            results_[i].c_ = 0;
        }

        xMin_        = 0.0;
        xMax_        = level - 1;
        intSize_     = 1.0;
        equiDist_    = true;
    }
    else
    {
        unsigned int i = 0;
        while((i < xValuesArr.size()) and (i < level))
        {
            results_[i].x_ = xValuesArr[i];
            results_[i].h_ = 0;
            results_[i].c_ = 0;
            ++i;
        }

        results_[level].h_ = 0;
        results_[level].c_ = 0;
        xMin_       = results_[0].x_;
        xMax_       = results_[level - 1].x_;
        intSize_    = 0.0;
        equiDist_   = false;
    }

    if (preFirst < xMin_)
    {
        preIndex_ = indexMin_;
    }
    else if (preFirst > xMax_)
    {
        preIndex_ = indexMax_ - 1;
    }
    else
    {
        preIndex_ = 0;
        while (preIndex_ < level)
        {
            if (fabs(results_[preIndex_].x_ - preFirst) < getMaxError<double>())
            {
                break;
            }
            else
            {
                preIndex_++;
            }
        }
    }

    // state of beginning
    curLevelIndex_ = indexMin_;
}


//! Constructor for equi-distant x-values
DLRE::DLRE(double xMin,
           double xMax,
           double intSize,
           double error,
           double preFirst,
           string name,
           string description,
           bool forceRminusAOK,
           int maxNrv,
           int skipInterval,
           formatType format)
    : StatEval(format, name, description),
      results_(NULL),
      line_(),
      relErrMax_(error),
      maxNrv_(maxNrv),
      wastedLeft_(0),
      wastedRight_(0),
      h_(0),
      xOffset_(0),
      xMin_(xMin),
      xMax_(xMax),
      indexMin_(0),
      indexMax_(0),
      equiDist_(true),
      intSize_(intSize),
      curIndex_(0),
      preRv_(preFirst),
      preIndex_(0),
      base_(1.0),
      reason_(ok),
      curLevelIndex_(0),
      skipInterval_(skipInterval),
      forceRminusAOK_(forceRminusAOK),
      phase_(initialize)
{
    this->initEqui(xMin_, xMax_, intSize_, preFirst);
}

void
DLRE::initEqui(double xMin,
               double xMax,
               double intSize,
               double preFirst)
{
    assure(intSize > 0, "Interval width must be > 0!");
    assure(xMin < xMax, "xMin must be smaller than xMax");

    double tmp = (xMax - xMin) / intSize;
    unsigned int level = (unsigned int)(tmp) + 1;

    assure(level > 1, "Settings for xMax, xMin, intSize results in less than 2 levels");

    indexMax_ = level;
    results_ = new Result [level + 1];

    for (int i = 0; i <= level; i++)
    {
        results_[i].x_ = (i * intSize) + xMin;
        results_[i].h_ = 0;
        results_[i].sumh_ = 0;
        results_[i].c_ = 0;
    }

    if (preFirst < xMin_)
    {
        preIndex_ = indexMin_;
    }
    else if (preFirst > xMax_)
    {
        preIndex_ = indexMax_;
    }
    else
    {
        preIndex_ = 0;
        while (preIndex_ < level)
        {
            if (results_[preIndex_].x_ == preFirst)
            {
                break;
            }
            else
            {
                preIndex_++;
            }
        }
    }
}

DLRE::DLRE(const wns::pyconfig::View& config) :
    StatEval(config),
    results_(NULL),
    line_(),
    relErrMax_(config.get<double>("maxError")),
    maxNrv_(UINT_MAX),
    wastedLeft_(0),
    wastedRight_(0),
    h_(0),
    xOffset_(0),
    xMin_(-1.0),
    xMax_(-1.0),
    indexMin_(0),
    indexMax_(0),
    equiDist_(true),
    intSize_(-1.0),
    curIndex_(0),
    preRv_(config.get<double>("initValue")),
    preIndex_(0),
    base_(1.0),
    reason_(ok),
    curLevelIndex_(0),
    skipInterval_(config.get<int>("skipInterval")),
    forceRminusAOK_(config.get<bool>("forceRminusAOK")),
    phase_(initialize)
{
    if (config.get<string>("maxNumTrials") != "infinity")
    {
        maxNrv_ = config.get<int>("maxNumTrials");
    }

    if (config.get<string>("distances") == "equi")
    {
        // equidistant x-values
        equiDist_ = true;
        xMin_ = config.get<double>("xMin");
        xMax_ = config.get<double>("xMax");
        intSize_ = config.get<double>("intervalWidth");

        this->initEqui(xMin_,
                       xMax_,
                       intSize_,
                       config.get<double>("initValue") );

    }
    else if (config.get<string>("distances") == "nonequi")
    {
        string errorString = "Read-in of non-equidistance values not implemented (yet)";
        throw(wns::Exception(errorString));
        // non-equidistant x-values
        /*equiDist_ = false;
		ArrayDouble64* values = NULL;
		setPtr(values, new ArrayDouble64);
		int numXValues = config.len("xValues");
		values->setSize(numXValues);
		for (int ii=0; ii<numXValues; ++ii)
			values->put(ii, config.get<double>("xValues",ii));

		this->initNonEqui(numXValues,
							values,
							config.get<double64>("initValue"));

                            setPtr(values, NULL);*/
    }
    else
    {
        string errorString = ("Unknown 'distances' setting '" + config.get<string>("distances") + "'\n");
        throw(wns::Exception(errorString));
    }
}


//! Destructor
DLRE::~DLRE()
{
    delete [] results_;
}





//! print
void DLRE::print(ostream& aStreamRef) const
{
    aStreamRef << "skipInterval_ : "       << skipInterval_       << endl;
    aStreamRef << "forceRminusAOK_ : "     << forceRminusAOK_     << endl;
    aStreamRef << "name_ : "               << name_               << endl;
    aStreamRef << "desc_ : "               << desc_               << endl;
}





//! reset collected data
void DLRE::reset()
{
    StatEval::reset();

    h_           = 0;
    wastedRight_ = 0;
    wastedLeft_  = 0;
    phase_       = initialize;

    for (int i = indexMin_; i < indexMax_; i++)
    {
        results_[i].h_ = 0;
        results_[i].c_ = 0;
        results_[i].sumh_ = 0;
    }
}


//! set base
void DLRE::setBase(double newBase)
{
    base_ = newBase;
}

//! change maximum relative error
void DLRE::changeError(double newError)
{
    relErrMax_ = newError;
}

//! return current x level
double DLRE::curXLev()
{
    return results_[curLevelIndex_].x_;
}


//! return percentage of values collected for interval xt
double DLRE::p(double xt)
{

    int index = getIndex(xt);

    if((numTrials_ > 0) and (index >= 0))
    {
        return (double)(results_[index].h_) / (double)numTrials_;
    }
    else
    {
        return(0.0);
    }
}

//! return the index of the minimum x value to be evaluated
int DLRE::minIndex() const
{
    return indexMin_;
}

//! return the index of the maximum x value to be evaluated
int DLRE::maxIndex() const
{
    return indexMax_ - 1;
}



//! Constructor
DLRE::ResultLine::ResultLine()
    : vf_(0.0),
      x_(0.0),
      relErr_(0.0),
      rho_(0.0),
      sigRho_(0.0),
      nx_(0)
{}


/*! Unequal '!=' comparison operator. This operator only works if
  both objects compared are of the same type and if method compare of
  this type has been overloaded. */
bool DLRE::ResultLine::operator != (const ResultLine& aLineRef)
    const
{
    const ResultLine* ptr = &aLineRef;

    return not
        ((fabs(x_ - ptr->x_) < getMaxError<double>()) and
         (fabs(vf_ - ptr->vf_) < getMaxError<double>()) and
         (fabs(relErr_ - ptr->relErr_) < getMaxError<double>()) and
         (fabs(rho_ - ptr->rho_) < getMaxError<double>()) and
         (fabs(sigRho_ - ptr->sigRho_) < getMaxError<double>()) and
         (fabs(double(nx_ - ptr->nx_)) < getMaxError<double>()));
}

const int DLRE::largeSampleNumTrials_       = 1000;
const int DLRE::largeSampleNumSortedValues_ = 100;
const int DLRE::largeSampleNumTransitions_  = 10;

//! return index of x value
int DLRE::getIndex(double curRv)
{
    if (curRv < xMin_)
    {
        return lower;
    }
    else if (curRv > xMax_)
    {
        return greater;
    }
    else if (equiDist_)
    {
        int index = std::min(int(indexMin_ + (curRv - xMin_) / intSize_),
                             int((indexMax_ - 1)));

        return (fabs(curRv - results_[index].x_) < getMaxError<double>()) ?
            index : int(noIndex);
    }
    else
    {
        // search the correct index (you know a better algorithm ? - Send it !)
        Result* start = results_;
        Result* end   = results_ + indexMax_;
        Result* lauf;
        register int step;

        while ((step = (end - start)) >= 1)
        {
            // `>>1' equals `/2' but is faster
            lauf = start + (step >> 1);
            if (fabs(lauf->x_ - curRv) < getMaxError<double>())
            {
                return (step >> 1) + (start - results_);
            }
            else if (lauf->x_ > curRv)
            {
                if (end - lauf > 0)
                    end = lauf;
                else
                    return noIndex;
            }
            else if (lauf->x_ < curRv)
            {
                if (lauf - start > 0)
                    start = lauf;
                else
                    return noIndex;
            }
        }

        return noIndex;
    }
}



//! output function-specific evaluation results
void DLRE::print(ostream& aStreamRef,
                 functionType aFunctionType,
                 const double yMin) const
{
    string prefix(prefix_ + " ");
    string separator = prefix + "---------------------------------------------------------------------------";
    string long_separator = separator + "---------\n";
    separator += "\n";

    string errorString;
    string eval_type_string;
    string function_string;
    int i;

    switch (aFunctionType)
    {
    case df:
        eval_type_string = prefix + "Evaluation: DLREF";
        errorString = "I/O Error: Can't dump DLREF results";
        function_string = "F";
        break;
    case cdf:
        eval_type_string = prefix + "Evaluation: DLREG";
        errorString = "I/O Error: Can't dump DLREG results";
        function_string = "G";
        break;
    case pf:
        eval_type_string = prefix + "Evaluation: DLREP";
        errorString = "I/O Error: Can't dump DLREP results";
        function_string = "P";
        break;
    default:
        throw(wns::Exception("DLRE: Unknown function type"));
    }

    printBanner(aStreamRef, eval_type_string, errorString);

    if (not aStreamRef)
    {
        throw(wns::Exception(errorString));
    }

    aStreamRef << separator << prefix;
    aStreamRef << " DLRE" << function_string << " statistics" << endl
               << prefix << endl;

    aStreamRef << prefix << setprecision(4);
    aStreamRef.setf(ios::right);
    aStreamRef << " lower border: " << xMin_ << endl
               << prefix << " upper border: " << xMax_ << endl
               << prefix << " number of intervals: " << indexMax_ << endl;

    if (equiDist_)
    {
        aStreamRef << prefix << " interval size: " << intSize_ << endl;
    }
    else
    {
        aStreamRef << prefix << endl
                   << prefix << "non-equidistant x values, provided x values: " << endl
                   << prefix;
        for (int i = 0; i < indexMax_; i++)
        {
            if (i % 10 == 0)
            {
                aStreamRef << endl << prefix;
            }

            aStreamRef << results_[i].x_ << "  ";
        }
        aStreamRef << endl << prefix << endl;
    }

    aStreamRef << prefix << " maximum number of samples: ";

    if (maxNrv_ == UINT_MAX)
    {
        aStreamRef << "infinity";
    }
    else
    {
        aStreamRef << maxNrv_;
    }
    aStreamRef << endl
               << prefix << " maximum relative error [%]: " << resetiosflags(ios::scientific) << setiosflags(ios::fixed) << relErrMax_ * 100.0 << endl
               << prefix << resetiosflags(ios::fixed) << resetiosflags(ios::scientific) << (format_ == fixed ? setiosflags(ios::fixed) : setiosflags(ios::scientific));

    aStreamRef << endl << prefix << " large sample condition r - a >= 10 ";

    if (not forceRminusAOK_)
    {
        aStreamRef << "not ";
    }
    aStreamRef << "enforced." << endl;

    aStreamRef << separator << prefix << " DLRE" << function_string << " Data" << endl
               << prefix << endl;

    aStreamRef << prefix << " evaluated levels: ";
    if (aFunctionType == cdf)
    {
        aStreamRef << curLevelIndex_ << endl;
    }
    else
    {
        aStreamRef << indexMax_ - curLevelIndex_ << endl;
    }

    aStreamRef.setf(ios::fixed);
    aStreamRef << setprecision(2) << prefix << " Underflows: " << wastedLeft_ << endl;
    aStreamRef << prefix << " Overflows: " << wastedRight_ << endl << prefix <<  endl;

    aStreamRef << prefix;
    if (phase_ == finish)
    {
        switch (reason_)
        {
        case ok:
            aStreamRef << "All levels completed." << endl;
            break;
        case minimum:
            aStreamRef << "Evaluated till minimum y value = " << yMin << "." << endl;
            break;
        case last:
            aStreamRef << "Last level cannot be calculated." << endl;
            break;
        default:
            break;
        }
    }
    else
    {
        aStreamRef << "Not finished, still collecting." << endl;
    }

    // an evaluation without values does not make sense
    if (not numTrials_)
    {
        return;
    }


    aStreamRef << setprecision(7) << long_separator
               << prefix + "                                              mean local "
               << "     deviation       number of    number of"
               << endl
               << prefix + "ordinate      abscissa        relative        correlation"
               << "     from mean       trials per   transitions    relative error"
               << endl
               << prefix + "                              error           coefficient"
               << "     local c.c.      interval     per interval   within limit"
               << endl
               << prefix + ""
               << endl
               << prefix + ""
               << function_string
               << "(x)          x               d(x)            rho(x)     "
               << "     sigma(x)        "
               << "n(x)         "
               << "t(x)"
               << endl
               << prefix + "                               "
               << function_string
               << "                 "
               << function_string
               << "                 rho"
               << endl
               << prefix + ""
               << endl
               << resetiosflags(ios::fixed) << resetiosflags(ios::scientific)
               << (format_ == fixed ? setiosflags(ios::fixed) :
                   setiosflags(ios::scientific));

    if (not aStreamRef)
    {
        throw(wns::Exception(errorString));
    }



    {
        double f = 0.0, x = 0.0;
        string infinity;

        if (aFunctionType == cdf)
        {
            f = 1.0;
            x = minValue_;
        }
        else if (aFunctionType == df)
        {
            f = 1.0;
            x = minValue_;
        }
        else if (aFunctionType == pf)
        {
            f = 0.0;
            x = maxValue_;
        }

        aStreamRef << resetiosflags(ios::right)
                   << setiosflags(ios::left)
                   << setw(15)
                   << f * base_;
        if (not aStreamRef)
        {
            throw(wns::Exception(errorString));
        }
        if (x == DBL_MAX or x == -DBL_MAX)
        {
            if (x == DBL_MAX)
            {
                infinity = "+infinity     ";
            }
            else
            {
                infinity = "-infinity     ";
            }
            aStreamRef << infinity;
        }
        else
        {
            aStreamRef << resetiosflags(ios::left)
                       << setiosflags(ios::right)
                       << setw(14)
                       << x;
        }
        if (not aStreamRef)
        {
            throw(wns::Exception(errorString));
        }
        aStreamRef << "   not_available   not_available   not_available   "
                   << resetiosflags(ios::right)
                   << setiosflags(ios::left)
                   << setw(10)
                   << results_[0].h_
                   << "   not_available    y"
                   << endl;
        if (not aStreamRef)
        {
            throw(wns::Exception(errorString));
        }
    }

    for (i = indexMin_; i < indexMax_; i++)
    {
        // Print current level
        p_printLevel(aStreamRef, i, errorString, false, aFunctionType);
    }

    if (not (aFunctionType == pf and not numTrials_))
    {
        double f, x;

        if (aFunctionType == cdf)
        {
            f = 0.0;
            x = maxValue_;
        }
        else
        {
            if(aFunctionType == pf)
            {
                f = (double(results_[curLevelIndex_ + 1].h_) / double(numTrials_));
            }
            else
            {
                f = 0.0;
            }
            x = maxValue_;
        }

        aStreamRef << resetiosflags(ios::right)
                   << setiosflags(ios::left)
                   << setw(15)
                   << f * base_;

        if (not aStreamRef)
        {
            throw(wns::Exception(errorString));
        }

        if (x == DBL_MAX or x == -DBL_MAX)
        {
            string infinity;

            if (x == DBL_MAX)
            {
                infinity = "+infinity     ";
            }
            else
            {
                infinity = "-infinity     ";
            }
            aStreamRef << infinity;
        }
        else
        {
            aStreamRef << resetiosflags(ios::left)
                       << setiosflags(ios::right)
                       << setw(14)
                       << x;
        }
        if (not aStreamRef)
        {
            throw(wns::Exception(errorString));
        }
        aStreamRef << "   not_available   not_available   not_available   "
                   << resetiosflags(ios::right)
                   << setiosflags(ios::left)
                   << setw(10)
                   << results_[curLevelIndex_ + 1].h_
                   << "   "
                   << setw(10)
                   << results_[curLevelIndex_ + 1].c_
                   << "       y"
                   << endl;
        if (not aStreamRef)
        {
            throw(wns::Exception(errorString));
        }
    }


    aStreamRef << long_separator;
    if (not aStreamRef)
    {
        throw(wns::Exception(errorString));
    }
}


//! check large sample conditions
bool DLRE::pd_checkLargeSample(unsigned int anIndex) const throw(BadTypeId, RangeErr)
{
    return
        (phase_ == initialize and numTrials_ >= largeSampleNumTrials_) or
        ((phase_ == iterate or phase_ == finish) and
         ((numTrials_ >= largeSampleNumTrials_) and
          ((unsigned int)results_[anIndex].sumh_ >= largeSampleNumSortedValues_) and
          ((numTrials_ - (unsigned int)results_[anIndex].sumh_) >= largeSampleNumSortedValues_) and
          ((unsigned int)results_[anIndex].c_ >= largeSampleNumTransitions_)  and
          // ai == ci +- 1, so we do not need to check ai
          (not forceRminusAOK_ or
           (((((unsigned int)results_[anIndex].sumh_ - (unsigned int)results_[anIndex].c_)  >= largeSampleNumTransitions_) or
             (anIndex > (unsigned int)indexMax_ - 3) ) and
            // again, we assume ai == ci
            (((numTrials_ - (unsigned int)results_[anIndex].sumh_ - (unsigned int)results_[anIndex].c_) >= largeSampleNumTransitions_) or
             (anIndex < (unsigned int)indexMin_ + 3)
                )
               )
              )
             )
            );

}

//! constructor
DLRE::pd_result::pd_result() throw()
    : x(0.0),
      h(0),
      sumh(0),
      c(0)
{}


/*! Unequal '!=' comparison operator. This operator only works if
  both objects compared are of the same type and if method compare of
  this type has been overloaded. */
bool DLRE::pd_result::operator != (const pd_result& aResultRef)
    const throw(GenErr)
{
    const pd_result* ptr = &aResultRef;

    return not ((fabs(x - ptr->x) < getMaxError<double>()) and
                (h == ptr->h) and
                (sumh == ptr->sumh) and
                (c == ptr->c));
}





//! print one y level
void DLRE::p_printLevel(ostream& aStreamRef,
                        unsigned int aLevel,
                        const string& errorString,
                        bool aDiscretePointFlag,
                        functionType aFunctionType) const
{
    string prefix(prefix_ + " ");

    double64 f = 0.0, d = 0.0, rho = 0.0, sigma = 0.0,
	     v = 0.0, r = 0.0, n = 0.0, a = 0.0;

    bool evaluated = checkLargeSample_(aLevel);

    // Just to play safe
    if (aFunctionType == pf)
    {
	aDiscretePointFlag = false;
    }
    n = double64(numTrials_);
    r = double64(results_[aLevel].sumh_);
    if (evaluated)
    {

	// Calculate local correlation coefficient rho(x) resp. rho(x)
	//                                            F            G
	// n:   num trials
	// r:   num sorted values
	// a:   num transitions
	//
	//                a/r
	// rho(x) = 1 - -------
	//              1 - r/n
	//
	a = double64(results_[aLevel].c_);

	rho = 1.0 - a / r / (1.0 - r / n);



	//
	//  2     1 - r/n   1 + rho(x)                 a/r
	// d(x) = ------- * ----------; rho(x) = 1 - -------
	//  F        r      1 - rho(x)               1 - r/n

	d = ((1.0 - r / n) / r) * (1.0 + rho) / (1.0 - rho);

	if (d >= 0.0)
	{
	    d = sqrt(d);
	}


	// Calculate mean quadratic deviation from mean local
	// correlation coefficient
	//
	//      2          1 - a/r   1 - a/v
	// sigma(x) = a * (------- + -------); v = n - r
	//      rho         r * r     v * v
	//
	v = n - r;
	sigma = a * ((1.0 - a / r) / (r * r) + (1.0 - a / v) / (v * v));
	if (sigma > 0.0)
	{
	    sigma = sqrt(sigma);
	}

    }

    if ((not evaluated) or d > relErrMax_)
    {
	if (not aStreamRef)
	{
	    throw(wns::Exception(errorString));
	}
    }


    if (aDiscretePointFlag)
    {
	// Output F(x)/G(x), x, number of sorted values, and number of
	// transitions of previous interval
	aLevel--;
    }
    // Discrete point in evaluated interval
    if (evaluated and aDiscretePointFlag)
    {
	f = double64(results_[aLevel].sumh_ - results_[aLevel].h_) / n;
    }
    else
    {
	double64 occ = double64(results_[aLevel].h_);

	if (aFunctionType == pf)
    {
        f = occ / n;

        //
        // cdf: use left  insertion limit
        //  df: use right insertion limit
        //
        //	    f = function_ == cdf ?
        //		(r - double64(rmc_(aLevel + 1).numSortedValues)) / n :
        //		(double64(rmc_(aLevel - 1).numSortedValues) - r) / n;
    }
	// Discrete point in non evaluated interval
	else if (not evaluated and not aDiscretePointFlag and
		 occ > 1.0 and r - occ > 0.0)
	{
	    f = (r - occ) / n;
	}
	// Non discrete point in non evaluated interval or
	// evaluated interval
    else
    {
        f = r / n;
    }
    }
    assert(0.0 <= f and f <= 1.0);


    aStreamRef << resetiosflags(ios::right)
               << setiosflags(ios::left)
               << setw(15)
               << f * base_
               << resetiosflags(ios::left)
               << setiosflags(ios::right)
               << setw(14)
               << results_[aLevel].x_
               << "  ";
    if (not aStreamRef)
    {
        throw(wns::Exception(errorString));
    }


    if (evaluated and d >= 0.0)
    {
        aStreamRef << setw(14)
                   << d
                   << "  ";
    }
    else
    {
        aStreamRef << " not_available  ";
    }
    if (not aStreamRef)
    {
        throw(wns::Exception(errorString));
    }


    if (evaluated)
    {
        aStreamRef << setw(14)
                   << rho;
    }
    else
    {
        aStreamRef << " not_available";
    }
    if (not aStreamRef)
    {
        throw(wns::Exception(errorString));
    }


    if (not evaluated or sigma < 0.0)
    {
        aStreamRef << "   not_available";
    }
    else
    {
        aStreamRef << "  "
                   << setw(14)
                   << sigma;
    }
    if (not aStreamRef)
    {
        throw(wns::Exception(errorString));
    }


    aStreamRef << "   "
               << resetiosflags(ios::right)
               << setiosflags(ios::left)
               << setw(10)
               << results_[aLevel].h_
               << "   "
               << setw(10)
               << results_[aLevel].c_
               << "       ";

    if ((not evaluated) or d > relErrMax_)
    {
        aStreamRef << "n";
    }
    else
    {
        aStreamRef << "y";
    }
    aStreamRef << endl;
}

//virtual void
//put(double value);
