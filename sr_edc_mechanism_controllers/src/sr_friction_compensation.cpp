/**
 * @file   sr_friction_compensation.hpp
 * @author Ugo Cupcic <ugo@shadowrobot.com>
 * @date   Wed Aug 17 11:14:31 2011
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
 * @brief  Compensate the tendon friction by adding a given value depending on the sign of the force demand.
 *
 *
 */

#include "sr_edc_mechanism_controllers/sr_friction_compensation.hpp"

#include <sstream>
#include <math.h>
#include "sr_utilities/sr_math_utils.hpp"
#include "sr_utilities/sr_deadband.hpp"

namespace sr_friction_compensation
{

  SrFrictionCompensator::SrFrictionCompensator(std::string joint_name)
  {
    joint_name_ = joint_name;

    std::pair<std::vector<joint_calibration::Point>, std::vector<joint_calibration::Point> > both_maps = read_friction_map();
    friction_interpoler_forward = boost::shared_ptr<shadow_robot::JointCalibration>( new shadow_robot::JointCalibration( both_maps.first ) );
    friction_interpoler_backward = boost::shared_ptr<shadow_robot::JointCalibration>( new shadow_robot::JointCalibration( both_maps.second ) );
  }

  SrFrictionCompensator::~SrFrictionCompensator()
  {}

  double SrFrictionCompensator::friction_compensation( double position, int force_demand_sign, int deadband )
  {
    double compensation = 0.0;

    if( sr_deadband::simple_deadband<int>(force_demand_sign, deadband) )
    {
      if( force_demand_sign > 0 )
        compensation = friction_interpoler_forward->compute( position );
      else
        compensation = friction_interpoler_backward->compute( position );
    }

    return compensation;
  }

  std::pair<std::vector<joint_calibration::Point>, std::vector<joint_calibration::Point> > SrFrictionCompensator::read_friction_map()
  {
    std::vector<joint_calibration::Point> friction_map_forward, friction_map_backward;
    std::string param_name = "/sr_friction_map";

    bool joint_not_found_forward = true;
    bool joint_not_found_backward = true;

    XmlRpc::XmlRpcValue calib;
    if( node_.hasParam(param_name) )
    {
      node_.getParam(param_name, calib);

      ROS_DEBUG_STREAM("  Reading friction for: " <<  joint_name_);
      ROS_DEBUG_STREAM(" value: " << calib);

      ROS_ASSERT(calib.getType() == XmlRpc::XmlRpcValue::TypeArray);
      //iterate on all the joints
      for(int32_t index_cal = 0; index_cal < calib.size(); ++index_cal)
      {
        //check the calibration is well formatted:
        // first joint name, then calibration table
        ROS_ASSERT(calib[index_cal][0].getType() == XmlRpc::XmlRpcValue::TypeString);
        ROS_ASSERT(calib[index_cal][1].getType() == XmlRpc::XmlRpcValue::TypeInt);
        ROS_ASSERT(calib[index_cal][2].getType() == XmlRpc::XmlRpcValue::TypeArray);

        std::string joint_name_tmp = static_cast<std::string> (calib[index_cal][0]);

        ROS_DEBUG_STREAM("  Checking joint name: "<< joint_name_tmp << " / " << joint_name_);
        if(  joint_name_tmp.compare( joint_name_ ) != 0 )
          continue;

        //reading the forward map:
        if( static_cast<int>(calib[index_cal][1]) == 1 )
        {
          joint_not_found_forward = false;

          friction_map_forward = read_one_way_map(calib[index_cal][2]);
        }
        else
        {
          joint_not_found_backward = false;

          friction_map_backward = read_one_way_map(calib[index_cal][2]);
        }
      }
    }

    if( joint_not_found_forward )
    {
      ROS_INFO_STREAM("  No forward friction compensation map for: " << joint_name_ );

      friction_map_forward = generate_flat_map();
    }

    if( joint_not_found_backward )
    {
      ROS_INFO_STREAM("  No backward friction compensation map for: " << joint_name_ );

      friction_map_backward = generate_flat_map();
    }

    /*
      ROS_INFO_STREAM(" Friction map forward: [" << joint_name_ << "]");
      for( unsigned int i=0; i<friction_map_forward.size(); ++i )
      ROS_INFO_STREAM("    -> position=" << friction_map_forward[i].raw_value << " compensation: " << friction_map_forward[i].calibrated_value);
      ROS_INFO_STREAM(" Friction map backward: [" << joint_name_ << "]");
      for( unsigned int i=0; i<friction_map_backward.size(); ++i )
      ROS_INFO_STREAM("    -> position=" << friction_map_backward[i].raw_value << " compensation: " << friction_map_backward[i].calibrated_value);
    */

    std::pair<std::vector<joint_calibration::Point>, std::vector<joint_calibration::Point> > both_maps;
    both_maps.first = friction_map_forward;
    both_maps.second = friction_map_backward;

    return both_maps;
  } //end read_friction_map

  std::vector<joint_calibration::Point> SrFrictionCompensator::read_one_way_map(XmlRpc::XmlRpcValue raw_map)
  {
    std::vector<joint_calibration::Point> friction_map;

    //now iterates on the calibration table for the current joint
    for(int32_t index_table=0; index_table < raw_map.size(); ++index_table)
    {
      ROS_ASSERT(raw_map[index_table].getType() == XmlRpc::XmlRpcValue::TypeArray);
      //only 2 values per calibration point: raw and calibrated (doubles)
      ROS_ASSERT(raw_map[index_table].size() == 2);
      ROS_ASSERT(raw_map[index_table][0].getType() == XmlRpc::XmlRpcValue::TypeDouble);
      ROS_ASSERT(raw_map[index_table][1].getType() == XmlRpc::XmlRpcValue::TypeDouble);


      joint_calibration::Point point_tmp;
      point_tmp.raw_value = sr_math_utils::to_rad(static_cast<double> (raw_map[index_table][0]));
      point_tmp.calibrated_value = static_cast<double> (raw_map[index_table][1]);
      friction_map.push_back(point_tmp);
    }

    return friction_map;
  }

  std::vector<joint_calibration::Point> SrFrictionCompensator::generate_flat_map()
  {
    std::vector<joint_calibration::Point> friction_map;

    joint_calibration::Point point_tmp;
    point_tmp.raw_value = 0.0;
    point_tmp.calibrated_value = 0.0;
    friction_map.push_back(point_tmp);
    point_tmp.raw_value = 1.0;
    friction_map.push_back(point_tmp);

    return friction_map;
  }
}



/* For the emacs weenies in the crowd.
   Local Variables:
   c-basic-offset: 2
   End:
*/

