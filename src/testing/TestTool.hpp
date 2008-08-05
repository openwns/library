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

#ifndef WNS_TESTING_TESTTOOL_HPP
#define WNS_TESTING_TESTTOOL_HPP

#include <string>
#include <vector>

namespace wns { namespace testing {

	/**
	 * @brief Match a file against a regular expression.
	 *
	 */
	bool compareFile(const std::string filename, const std::string regex);

	/**
	 * @brief Check whether a file matches a regexp
	 */
	bool matchInFile(const std::string filename, const std::string regex);

	/**
	 * @brief Check whether a file matches a list of regexps
	 */
	bool matchInFile(const std::string filename, std::vector<std::string> regexps);

	/**
	 * @brief Check whether a string matches a regexp
	 */
	bool matchInString(const std::string content, const std::string regex);

	/**
	 * @brief Check whether a string matches a list of regexps
	 */
	bool matchInString(const std::string content, std::vector<std::string> regexps);

	/**
	 * @brief Match a string against a regular expression.
	 *
	 */
	bool compareString(const std::string content, const std::string regex, bool matchEntireContent = true);

}
}

#endif // NOT defined WNS_TESTING_TESTTOOL_HPP

/*
  Local Variables:
  mode: c++
  fill-column: 80
  c-basic-offset: 4
  c-comment-only-line-offset: 0
  c-tab-always-indent: t
  indent-tabs-mode: t
  tab-width: 4
  End:
*/
