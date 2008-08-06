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

#ifndef _STATEVAL_HPP
#define _STATEVAL_HPP


//! Dependencies
#include <WNS/PyConfigViewCreator.hpp>

template <typename T>
T getMaxError();

template <>
inline
double getMaxError<double>()
{
	return 0.000000001;
}

class StatEvalInterface
{
public:
	virtual
	~StatEvalInterface(){}

	virtual void
	put(double) = 0;

	virtual void
	reset() = 0;

	virtual const std::string&
	getName() const = 0;

	virtual void
	print(std::ostream&) const = 0;

        /**
         * @todo dbn: This was introduced to handle LogEval in some way. LogEvals flush
         * their contents completely to disk and thus cannot be const. We should think
         * of a generic way to handle both possibilities behind a common interface.
         */
        virtual void
        printLog(std::ostream&) = 0;
};

/*
 * The class StatEval
 */
/*! \brief Class \bStatEval: Base class for statistical evaluation */
class StatEval :
	public StatEvalInterface
{
  public:
    //! Phase of evaluation
    enum phaseType
    {
	initialize, iterate, finish
    };

    //! Type of evaluation
    enum functionType
    {
	df, cdf, pf
    };


    //! How to format the output of numbers
    enum formatType
    {
	fixed, scientific
    };


    //#  Constructors and destructor

    // Constructors
        // Default constructor
    StatEval(bool anActiveFlag,
             phaseType   aStartingPhase,
			 formatType  aFormat,
			 std::string aName,
			 std::string aDesc);

	StatEval(const wns::pyconfig::View& config);

        //! Default copy constructor is correct


    // Destructor
    virtual ~StatEval();

    // Print output
    virtual void print(std::ostream& aStreamRef) const;

    virtual void printLog(std::ostream& aStreamRef);

    //#  Public members

    //! Default assign operator is correct

    //! Type of statistical evaluation
    enum statEvalType
    {
	all = 0, lref, lreg, plref, plreg, moments, pmoments, batchMns,
	pbatchMns, histogrm, phistogrm, dlref, dlreg, dlrep,
	pdlref, pdlreg, pdlrep, dlref_nonequi, dlreg_nonequi, dlrep_nonequi,
	pdlref_nonequi, pdlreg_nonequi, pdlrep_nonequi, pdf, ppdf, logeval,
	plogeval, probetext, pprobetext, unknown
    };

    //! put a value to evaluation
    virtual void       put(double xI)               ;

    // Return mean value
    virtual double   mean()                   const  ;

    // Return variance
    virtual double   variance()               const  ;

    // Return relative variance
    virtual double   relativeVariance()       const  ;

    // Return coefficient of variation
    virtual double   coeffOfVariation()       const  ;

    // Return moment m2
    virtual double   M2()                     const  ;

    // Return moment m3
    virtual double   M3()                     const  ;

    // Return z3
    virtual double   Z3()                     const  ;

    // Return skewness
    virtual double   skewness()               const  ;

    // Return standard deviation
    virtual double   deviation()              const  ;

    // Return relative standard deviation
    virtual double   relativeDeviation()      const  ;

    // Return number of trials
    virtual uint32_t     trials()                 const  ;

    // minimal value of trials
    virtual double   min()                    const  ;

    // maximal value of trials
    virtual double   max()                    const  ;

    // Reset evaluation
    virtual void       reset()                         ;

    // end of evaluation reached ?
    virtual bool       end()                    const  ;

    // Return state of evaluation
    virtual phaseType  status()                 const  ;

    // Get name of
	virtual const std::string& getName()               const  ;

    // Get description
    virtual const std::string& getDesc()               const  ;

    /* This probe is an active one, i.e. PDataBase::end() should take
       the result of this->end() into account? */
    //# Set/get active flag
    void               setActive(bool aFlag)           ;
    bool               getActive()              const  ;

    // Set the output format: fixed or scientific.
    virtual void       setFormat(formatType aFormat)   ;
    // Get the output format: fixed or scientific.
    virtual formatType getFormat()              const  ;

    // map string to statEvalType
    static statEvalType mapToStatEvalType(
        std::string aType);


    // map string to statEvalType
    static std::string       mapEvalTypeToString(
        statEvalType);

  protected:
    // print the banner containing common statistics
    void   pd_printBanner(std::ostream& aStreamRef,
                          std::string aProbeTypeDesc,
                          std::string anErrorString) const ;

    /*! flag showing that this object is active, which means that the
      end() method influences the end of the evaluation.*/
    bool                 pd_active;

    //! minimum value of all collected trials
    double             pd_minValue;
    //! maximum value of all collected trials
    double             pd_maxValue;
    //! number of collected trials
    uint32_t               pd_numTrials;
    //! sum of collected values
    double             pd_sum;
    //! square sum of collected values
    double             pd_squareSum;
    //! cube sum of collected values
    double             pd_cubeSum;
    /*! Current status of evaluation algorithm: initialize,
      iterate, or finish. */
    StatEval::phaseType  pd_phase;
    //! format of numbers (fixed, scientific)
    StatEval::formatType pd_format;
    //! probe name
    std::string          pd_name;
    //! probe description
    std::string          pd_desc;
	//! comment prefix to be used in output files
	std::string          pd_prefix;
};


typedef wns::PyConfigViewCreator<StatEvalInterface, StatEvalInterface> Creator;
typedef wns::StaticFactory<Creator> Factory;


#endif  // _STATEVAL_HPP


/*
Local Variables:
mode: c++
folded-file: t
End:
*/

