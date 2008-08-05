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

#ifndef _PDF_HPP
#define _PDF_HPP


#include "stateval.hpp"

#include <vector>


/*
 * The class PDF
 */
/*!\brief Class \bPDF: Probability Distribution Function. This class
  can be used to output the distribution function, the complementary
  distribution function, and the probability function of a given
  random x-sequence. The user only has to define the left and right
  border of the x-axis, the given x-values are expected to lie in, and
  the resolution of this range on the x-axis, i.e. the number of
  intervals. This algorithm does not consider correlation at all and,
  thus, cannot be used for controlling the length of a simulation
  run. This algorithm is especially useful, when you have to examine a
  fixed set of values or when you want to gain a first notion of the
  behaviour of a correlated sequence of values. */
class PDF : public StatEval
{
  public:

    //! scale type of the x-axis
    enum scaleType
    {
	logarithmical, linear
    };

    //# Constructors and destructor
	// Default constructor
    PDF(double    aMinXValue,
		double    aMaxXValue,
		uint32_t      aResolution,
		scaleType   aScaleType,
	    formatType  aFormat,
		const char* aName,
		const char* aDescription) ;

	PDF(const wns::pyconfig::View& config);

	// Copy constructor
    PDF(const PDF& aPDFRef)                                   ;

	// Destructor
    virtual ~PDF();

    // Normal output
    virtual void     print(std::ostream& aStreamRef = std::cout) const ;

    // Input a value to the statistical evaluation
    virtual void      put(double aValue) ;

    // Reset evaluation algorithm to its initial state
    virtual void      reset()              ;

private:

    //! Return statistical information of the given interval
    void getResult(uint32_t anIndex, double& anAbscissa,
                   double& anF, double& aG, double& aP) const ;

	//! Calculate the index in the array for the given Value
	uint32_t       p_getIndex(double aValue) const ;

	//! Calculate the abscissa value for the given index
	double     p_getAbscissa(uint32_t anIndex) const ;

	//! Exception classes for percentile calculation
	class PercentileError : public std::exception
	{};

	class PercentileUnderFlow : public PercentileError
	{};

	class PercentileOverFlow : public PercentileError
	{};

	//! Get the (approximated) p-th percentile, throws a PercentileError if not possible
	double p_getPercentile(int p) const;

	//! Print String representation of p-th percentile into stream
	void p_printPercentile(int p, std::ostream& aStreamRef = std::cout) const ;

    //! Left border of the x-axis
    double                p_minXValue;
    //! Right border of the x-axis
    double                p_maxXValue;
    //! Resolution of the x-axis
    uint32_t                  p_resolution;

	//! scale of the x-axis
	scaleType               p_scaleType;

    //! Array containing occurrences of all values
    std::vector<int>        p_values;

	int p_underFlows;
	int p_overFlows;

    //! probe name
    std::string                  p_name;

    //! probe description
    std::string                  p_desc;

};

#endif  // _PDF_HPP

/*
Local Variables:
mode: c++
folded-file: t
End:
*/
