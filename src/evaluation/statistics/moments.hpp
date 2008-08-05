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


#ifndef _MOMENTS_HPP
#define _MOMENTS_HPP

#include <WNS/evaluation/statistics/stateval.hpp>

/*
 * The class Moments
 */
/*! \brief Class \bMoments: Simple stat. evaluation: mean, variance,
  skewness etc. */
class Moments : public StatEval
{
  public:

    //#  Constructors and Destructors

    // Default constructor
    Moments();

    Moments(std::string aName,
			std::string aDescription,
			formatType  aFormat);

	Moments(const wns::pyconfig::View& config);

    // Destructor
    virtual ~Moments();

    // Print output
    virtual void print(std::ostream& aStreamRef = std::cout) const;

    //#  Public members

    // Input of value xI, weighted with wI
    virtual void put(double xI, double wI)             ;

    // Input of value xI
    virtual void put(double xI)                          ;

    // Return mean value
    virtual double   mean()                        const ;

    // Return variance
    virtual double   variance()                    const ;

    // Return size of 95%-confidence interval of mean
    virtual double   getConfidenceInterval95Mean() const ;

    // Return size of 99%-confidence interval of mean
    virtual double   getConfidenceInterval99Mean() const ;

    // Return moment m2
    virtual double   M2()                          const ;

    // Return moment m3
    virtual double   M3()                          const ;

    // Reset evaluation
    virtual void       reset()                             ;

  protected:

    //! sum of weights
    double            pd_wSum;

  private:

    double
    getConfidenceIntervalMean(double x) const;

};


#endif  // _MOMENTS_HPP


/*
Local Variables:
mode: c++
folded-file: t
End:
*/




