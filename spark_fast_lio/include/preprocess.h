#pragma once
#include <string>
#include <vector>

#include <pcl_conversions/pcl_conversions.h>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/time.hpp"

#if defined(LIVOX_ROS_DRIVER_FOUND) && LIVOX_ROS_DRIVER_FOUND
#include <livox_ros_driver/CustomMsg.h>
#endif

#define IS_VALID(a) ((abs(a) > 1e8) ? true : false)

typedef pcl::PointXYZINormal PointType;
typedef pcl::PointCloud<PointType> PointCloudXYZI;

enum LID_TYPE { AVIA = 1, VELO16 = 2, OUST64 = 3, KMOUST64 = 4 };  // {1, 2, 3, 4}
enum TIME_UNIT { SEC = 0, MS = 1, US = 2, NS = 3 };
enum Feature { Nor, Poss_Plane, Real_Plane, Edge_Jump, Edge_Plane, Wire, ZeroPoint };
enum Surround { Prev, Next };
enum E_jump { Nr_nor, Nr_zero, Nr_180, Nr_inf, Nr_blind };

struct orgtype {
  double range;
  double dista;
  double angle[2];
  double intersect;
  E_jump edj[2];
  Feature ftype;
  orgtype() {
    range     = 0;
    edj[Prev] = Nr_nor;
    edj[Next] = Nr_nor;
    ftype     = Nor;
    intersect = 2;
  }
};

namespace velodyne_ros {
struct EIGEN_ALIGN16 Point {
  PCL_ADD_POINT4D;
  float intensity;
  float time;
  uint16_t ring;
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
}  // namespace velodyne_ros
POINT_CLOUD_REGISTER_POINT_STRUCT(velodyne_ros::Point,
                                  (float, x, x)(float, y, y)(float, z, z)(
                                      float,
                                      intensity,
                                      intensity)(float, time, time)(std::uint16_t, ring, ring))

namespace ouster_ros {
struct EIGEN_ALIGN16 Point {
  PCL_ADD_POINT4D;
  float intensity;
  uint32_t t;
  uint16_t reflectivity;
  uint8_t ring;
  uint16_t ambient;
  uint32_t range;
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
}  // namespace ouster_ros

// clang-format off
POINT_CLOUD_REGISTER_POINT_STRUCT(ouster_ros::Point,
                                  (float, x, x)
                                    (float, y, y)
                                    (float, z, z)
                                    (float, intensity, intensity)
                                    // use std::uint32_t to avoid conflicting with pcl::uint32_t
                                    (std::uint32_t, t, t)
                                    (std::uint16_t, reflectivity, reflectivity)
                                    (std::uint8_t, ring, ring)
                                    (std::uint16_t, ambient, ambient)
                                    (std::uint32_t, range, range)
)

class Preprocess {
 public:
  Preprocess();
  ~Preprocess();

#if defined(LIVOX_ROS_DRIVER_FOUND) && LIVOX_ROS_DRIVER_FOUND
  void process(const livox_ros_driver::CustomMsg &msg, PointCloudXYZI::Ptr &pcl_out);
#endif
  void process(const sensor_msgs::msg::PointCloud2 &msg, PointCloudXYZI::Ptr &pcl_out);
  void set(bool feat_en, int lid_type, double bld, int pfilt_num);

  // sensor_msgs::PointCloud2::ConstPtr pointcloud;
  PointCloudXYZI  pl_full, pl_corn, pl_surf, pl_from_pilots;
  PointCloudXYZI  pl_buff[128];   // maximum 128 line lidar
  std::vector<orgtype> typess[128];    // maximum 128 line lidar
  float           time_unit_scale;
  int             lidar_type, point_filter_num, N_SCANS, SCAN_RATE, time_unit;
  double          blind, blind_for_human_pilots;
  bool            feature_enabled, given_offset_time;

 private:
  bool is_from_pilot_zone(const float &pt_x, const float &pt_y, const float &pt_z, const std::string mode = "velodyne");
#if defined(LIVOX_ROS_DRIVER_FOUND) && LIVOX_ROS_DRIVER_FOUND
  void avia_handler(const livox_ros_driver::CustomMsg::ConstPtr &msg);
#endif
  void oust64_handler(const sensor_msgs::msg::PointCloud2 &msg);
  void kmoust64_handler(const sensor_msgs::msg::PointCloud2 &msg);
  void velodyne_handler(const sensor_msgs::msg::PointCloud2 &msg);
  void give_feature(PointCloudXYZI &pl, std::vector<orgtype> &types);
  void pub_func(PointCloudXYZI &pl, const rclcpp::Time &ct);
  int plane_judge(const PointCloudXYZI &pl, std::vector<orgtype> &types, uint i, uint &i_nex, Eigen::Vector3d &curr_direct);
  bool small_plane(const PointCloudXYZI &pl,
                   std::vector<orgtype> &types,
                   uint i_cur,
                   uint &i_nex,
                   Eigen::Vector3d &curr_direct);
  bool edge_jump_judge(const PointCloudXYZI &pl, std::vector<orgtype> &types, uint i, Surround nor_dir);

  int    group_size;
  double disA, disB, inf_bound;
  double limit_maxmid, limit_midmin, limit_maxmin;
  double p2l_ratio;
  double jump_up_limit, jump_down_limit;
  double cos160;
  double edgea, edgeb;
  double smallp_intersect, smallp_ratio;
  double vx, vy, vz;
};
