/**
 * @file   sr_test_runner.hpp
 * @author Ugo Cupcic <ugo@shadowrobot.com>
 * @date   Feb 4, 2013
 *
 * Copyright 2011 Shadow Robot Company Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * @brief This class inherits from the default ROS Test Runner. It adds a set of useful
 *        functionalities for testing topics and services more easily.
 *
 *
 */

#ifndef SR_TEST_RUNNER_HPP_
#define SR_TEST_RUNNER_HPP_

#include <self_test/self_test.h>
#include "sr_self_test/gnuplot-iostream.h"

namespace shadow_robot
{
class SrTestRunner : public self_test::TestRunner
{
public:
  SrTestRunner();

  virtual ~SrTestRunner();

  using DiagnosticTaskVector::add;
  using TestRunner::setID;

  void addTopicTest(std::string topic_name, double frequency);
  void addServicesTest(std::vector<std::string> services_to_test);

  void plot(std::map<std::string, std::vector<double> > joints);

private:
  static const double SERVICE_TIMEOUT_CONST_;

  std::vector<std::string> services_to_test_;
  void service_test_cb_(diagnostic_updater::DiagnosticStatusWrapper& status);
  size_t index_service_to_test_;

  boost::shared_ptr<Gnuplot> gnuplot_;
};

}

/* For the emacs weenies in the crowd.
   Local Variables:
   c-basic-offset: 2
   End:
*/

#endif /* SR_TEST_RUNNER_HPP_ */
