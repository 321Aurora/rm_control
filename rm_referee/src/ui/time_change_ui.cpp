//
// Created by llljjjqqq on 22-11-4.
//

#include "rm_referee/ui/time_change_ui.h"

#include <sensor_msgs/Image.h>

namespace rm_referee
{
void TimeChangeUi::update()
{
  updateConfig();
  graph_->setOperation(rm_referee::GraphOperation::UPDATE);
  UiBase::display(ros::Time::now());
}

void TimeChangeGroupUi::update()
{
  updateConfig();
  for (auto graph : graph_vector_)
    graph.second->setOperation(rm_referee::GraphOperation::UPDATE);
  for (auto character : character_vector_)
    character.second->setOperation(rm_referee::GraphOperation::UPDATE);
  display(ros::Time::now());
}

void TimeChangeUi::updateForQueue()
{
  updateConfig();
  graph_->setOperation(rm_referee::GraphOperation::UPDATE);

  if (graph_queue_ && character_queue_ && ros::Time::now() - last_send_ > delay_)
  {
    UiBase::updateForQueue();
    last_send_ = ros::Time::now();
  }
}

void TimeChangeGroupUi::updateForQueue()
{
  updateConfig();
  if (graph_queue_ && character_queue_ && ros::Time::now() - last_send_ > delay_)
  {
    GroupUiBase::updateForQueue();
    last_send_ = ros::Time::now();
  }
}

void CapacitorTimeChangeUi::add()
{
  if (remain_charge_ != 0.)
    graph_->setOperation(rm_referee::GraphOperation::ADD);
  UiBase::display(false);
}

void CapacitorTimeChangeUi::updateConfig()
{
  if (remain_charge_ > 0.)
  {
    graph_->setStartX(610);
    graph_->setStartY(100);
    graph_->setEndX(610 + 600 * remain_charge_);
    graph_->setEndY(100);
    if (remain_charge_ > 0.7)
      graph_->setColor(rm_referee::GraphColor::GREEN);
    else if (remain_charge_ > 0.3)
      graph_->setColor(rm_referee::GraphColor::ORANGE);
    else
      graph_->setColor(rm_referee::GraphColor::PINK);
  }
  else
    return;
}

void CapacitorTimeChangeUi::updateRemainCharge(const double remain_charge, const ros::Time& time)
{
  remain_charge_ = remain_charge;
  updateForQueue();
}

void EffortTimeChangeUi::updateConfig()
{
  char data_str[30] = { ' ' };
  sprintf(data_str, "%s:%.2f N.m", joint_name_.c_str(), joint_effort_);
  graph_->setContent(data_str);
  if (joint_effort_ > 20.)
    graph_->setColor(rm_referee::GraphColor::ORANGE);
  else if (joint_effort_ < 10.)
    graph_->setColor(rm_referee::GraphColor::GREEN);
  else
    graph_->setColor(rm_referee::GraphColor::YELLOW);
}

void EffortTimeChangeUi::updateJointStateData(const sensor_msgs::JointState::ConstPtr data, const ros::Time& time)
{
  int max_index = 0;
  if (!data->name.empty())
  {
    for (int i = 0; i < static_cast<int>(data->effort.size()); ++i)
      if ((data->name[i] == "joint1" || data->name[i] == "joint2" || data->name[i] == "joint3" ||
           data->name[i] == "joint4" || data->name[i] == "joint5") &&
          data->effort[i] > data->effort[max_index])
        max_index = i;
    if (max_index != 0)
    {
      joint_effort_ = data->effort[max_index];
      joint_name_ = data->name[max_index];
      updateForQueue();
    }
  }
}

void ProgressTimeChangeUi::updateConfig()
{
  char data_str[30] = { ' ' };
  if (total_steps_ != 0)
    sprintf(data_str, " %.1f%%", finished_data_ / total_steps_ * 100.);
  else
    sprintf(data_str, " %.1f%%", finished_data_ / total_steps_ * 100.);
  graph_->setContent(data_str);
}

void ProgressTimeChangeUi::updateEngineerUiData(const rm_msgs::EngineerUi::ConstPtr data,
                                                const ros::Time& last_get_data_time)
{
  /*total_steps_ = data->total_steps;
  finished_data_ = data->finished_step;*/
  TimeChangeUi::updateForQueue();
}

void DartStatusTimeChangeUi::updateConfig()
{
  char data_str[30] = { ' ' };
  if (dart_launch_opening_status_ == 1)
  {
    sprintf(data_str, "Dart Status: Close");
    graph_->setColor(rm_referee::GraphColor::YELLOW);
  }
  else if (dart_launch_opening_status_ == 2)
  {
    sprintf(data_str, "Dart Status: Changing");
    graph_->setColor(rm_referee::GraphColor::ORANGE);
  }
  else if (dart_launch_opening_status_ == 0)
  {
    sprintf(data_str, "Dart Open!");
    graph_->setColor(rm_referee::GraphColor::GREEN);
  }
  graph_->setContent(data_str);
}

void DartStatusTimeChangeUi::updateDartClientCmd(const rm_msgs::DartClientCmd::ConstPtr data,
                                                 const ros::Time& last_get_data_time)
{
  dart_launch_opening_status_ = data->dart_launch_opening_status;
  TimeChangeUi::updateForQueue();
}

void RotationTimeChangeUi::updateConfig()
{
  if (chassis_mode_ != rm_msgs::ChassisCmd::RAW)
    graph_->setColor(rm_referee::GraphColor::PINK);
  else
    graph_->setColor(rm_referee::GraphColor::GREEN);
  if (!tf_buffer_.canTransform(gimbal_reference_frame_, chassis_reference_frame_, ros::Time(0)))
    return;
  try
  {
    int angle;
    double roll, pitch, yaw;
    quatToRPY(
        tf_buffer_.lookupTransform(chassis_reference_frame_, gimbal_reference_frame_, ros::Time(0)).transform.rotation,
        roll, pitch, yaw);
    angle = static_cast<int>(yaw * 180 / M_PI);

    graph_->setStartAngle((angle - arc_scale_ / 2) % 360 < 0 ? (angle - arc_scale_ / 2) % 360 + 360 :
                                                               (angle - arc_scale_ / 2) % 360);
    graph_->setEndAngle((angle + arc_scale_ / 2) % 360 < 0 ? (angle + arc_scale_ / 2) % 360 + 360 :
                                                             (angle + arc_scale_ / 2) % 360);
  }
  catch (tf2::TransformException& ex)
  {
    ROS_WARN("%s", ex.what());
  }
}

void RotationTimeChangeUi::updateChassisCmdData(const rm_msgs::ChassisCmd::ConstPtr data)
{
  chassis_mode_ = data->mode;
}

void LaneLineTimeChangeGroupUi::updateConfig()
{
  double spacing_x_a = robot_radius_ * screen_y_ / 2 * tan(M_PI / 2 - camera_range_ / 2) /
                       (cos(end_point_a_angle_ - pitch_angle_) * robot_height_ / sin(end_point_a_angle_)),
         spacing_x_b = robot_radius_ * screen_y_ / 2 * tan(M_PI / 2 - camera_range_ / 2) /
                       (cos(end_point_b_angle_ - pitch_angle_) * robot_height_ / sin(end_point_b_angle_)),
         spacing_y_a = screen_y_ / 2 * tan(M_PI / 2 - camera_range_ / 2) * tan(end_point_a_angle_ - pitch_angle_),
         spacing_y_b = screen_y_ / 2 * tan(M_PI / 2 - camera_range_ / 2) * tan(end_point_b_angle_ - pitch_angle_);
  if (spacing_x_a < 0)
    return;

  if (spacing_x_b < 0)
    return;

  for (auto it : graph_vector_)
  {
    if (it.first == "lane_line_left")
    {
      it.second->setStartX(screen_x_ / 2 - spacing_x_a);
      it.second->setStartY(screen_y_ / 2 - spacing_y_a);
      it.second->setEndX(screen_x_ / 2 - spacing_x_b * surface_coefficient_);
      it.second->setEndY(screen_y_ / 2 - spacing_y_b);
    }
    else if (it.first == "lane_line_right")
    {
      it.second->setStartX(screen_x_ / 2 + spacing_x_a);
      it.second->setStartY(screen_y_ / 2 - spacing_y_a);
      it.second->setEndX(screen_x_ / 2 + spacing_x_b * surface_coefficient_);
      it.second->setEndY(screen_y_ / 2 - spacing_y_b);
    }
  }
}

void LaneLineTimeChangeGroupUi::updateJointStateData(const sensor_msgs::JointState::ConstPtr data, const ros::Time& time)
{
  if (!tf_buffer_.canTransform("yaw", reference_frame_, ros::Time(0)))
    return;
  try
  {
    double roll, pitch, yaw;
    quatToRPY(tf_buffer_.lookupTransform("yaw", reference_frame_, ros::Time(0)).transform.rotation, roll, pitch, yaw);
    pitch_angle_ = pitch;
  }
  catch (tf2::TransformException& ex)
  {
    ROS_WARN_THROTTLE(3.0, "%s \n Replace joint state data", ex.what());

    for (unsigned int i = 0; i < data->name.size(); i++)
      if (data->name[i] == reference_frame_ + "_joint")
        pitch_angle_ = data->position[i];
  }

  end_point_a_angle_ = camera_range_ / 2 + pitch_angle_;
  end_point_b_angle_ = 0.6 * (0.25 + pitch_angle_);
  updateForQueue();
}

void BalancePitchTimeChangeGroupUi::updateConfig()
{
  for (auto it : graph_vector_)
  {
    if (it.first == "triangle_left_side")
    {
      it.second->setStartX(centre_point_[0]);
      it.second->setStartY(centre_point_[1]);
      it.second->setEndX(triangle_left_point_[0]);
      it.second->setEndY(triangle_left_point_[1]);
    }
    else if (it.first == "triangle_right_side")
    {
      it.second->setStartX(centre_point_[0]);
      it.second->setStartY(centre_point_[1]);
      it.second->setEndX(triangle_right_point_[0]);
      it.second->setEndY(triangle_right_point_[1]);
    }
  }
}

void BalancePitchTimeChangeGroupUi::calculatePointPosition(const rm_msgs::BalanceStateConstPtr& data,
                                                           const ros::Time& time)
{
  triangle_left_point_[0] = centre_point_[0] - length_ * sin(bottom_angle_ / 2 + data->theta);
  triangle_left_point_[1] = centre_point_[1] + length_ * cos(bottom_angle_ / 2 + data->theta);
  triangle_right_point_[0] = centre_point_[0] + length_ * sin(bottom_angle_ / 2 - data->theta);
  triangle_right_point_[1] = centre_point_[1] + length_ * cos(bottom_angle_ / 2 - data->theta);
  updateForQueue();
}

void DistanceBaseTimeChangeUi::updateDistanceBaseData(const std_msgs::Float32ConstPtr data, const ros::Time& time)
{
  distance_base_ = data->data;
  updateForQueue();
}

void DistanceBaseTimeChangeUi::updateConfig()
{
  std::string distance = std::to_string(distance_base_);
  graph_->setContent(distance);
  graph_->setColor(rm_referee::GraphColor::ORANGE);
}

void PitchAngleTimeChangeUi::updateJointStateData(const sensor_msgs::JointState::ConstPtr data, const ros::Time& time)
{
  for (unsigned int i = 0; i < data->name.size(); i++)
    if (data->name[i] == "pitch_joint")
      pitch_angle_ = data->position[i];
  updateForQueue();
}

void PitchAngleTimeChangeUi::updateConfig()
{
  std::string pitch = std::to_string(pitch_angle_);
  graph_->setContent(pitch);
  graph_->setColor(rm_referee::GraphColor::YELLOW);
}

void ImageTransmissionAngleTimeChangeUi::updateJointStateData(const sensor_msgs::JointState::ConstPtr data,
                                                              const ros::Time& time)
{
  for (unsigned int i = 0; i < data->name.size(); i++)
    if (data->name[i] == "image_transmission_joint")
      image_transmission_angle_ = data->position[i];
  updateForQueue();
}

void ImageTransmissionAngleTimeChangeUi::updateConfig()
{
  std::string image_transmission = std::to_string(image_transmission_angle_);
  graph_->setContent(image_transmission);
  graph_->setColor(rm_referee::GraphColor::PINK);
}

void JointPositionTimeChangeUi::updateConfig()
{
  double proportion = (current_val_ - min_val_) / (max_val_ - min_val_);
  graph_->setStartX(graph_->getConfig().start_x);
  graph_->setStartY(graph_->getConfig().start_y);
  if (direction_ == "horizontal")
  {
    graph_->setEndY(graph_->getConfig().start_y);
    graph_->setEndX(graph_->getConfig().start_x + length_ * proportion);
  }
  else if (direction_ == "vertical")
  {
    graph_->setEndY(graph_->getConfig().start_y + length_ * proportion);
    graph_->setEndX(graph_->getConfig().end_x);
  }
  else
  {
    graph_->setEndY(graph_->getConfig().start_y + length_ * proportion);
    graph_->setEndX(graph_->getConfig().start_x + length_ * proportion);
  }
  if (abs(proportion) > 0.96)
    graph_->setColor(rm_referee::GraphColor::BLACK);
  else if (abs(proportion) > 0.8)
    graph_->setColor(rm_referee::GraphColor::PINK);
  else if (abs(proportion) > 0.6)
    graph_->setColor(rm_referee::GraphColor::PURPLE);
  else if (abs(proportion) > 0.3)
    graph_->setColor(rm_referee::GraphColor::ORANGE);
  else
    graph_->setColor(rm_referee::GraphColor::GREEN);
}

void JointPositionTimeChangeUi::updateJointStateData(const sensor_msgs::JointState::ConstPtr data, const ros::Time& time)
{
  for (unsigned int i = 0; i < data->name.size(); i++)
    if (data->name[i] == name_)
      current_val_ = data->position[i];
  updateForQueue();
}

void BulletTimeChangeUi::updateBulletData(const rm_msgs::BulletAllowance& data, const ros::Time& time)
{
  if (data.bullet_allowance_num_17_mm >= 0 && data.bullet_allowance_num_17_mm < 1000)
  {
    if (bullet_allowance_num_17_mm_ > data.bullet_allowance_num_17_mm)
      bullet_num_17_mm_ += (bullet_allowance_num_17_mm_ - data.bullet_allowance_num_17_mm);
    bullet_allowance_num_17_mm_ = data.bullet_allowance_num_17_mm;
  }
  if (data.bullet_allowance_num_42_mm >= 0 && data.bullet_allowance_num_42_mm < 1000)
  {
    if (bullet_allowance_num_42_mm_ > data.bullet_allowance_num_42_mm)
      bullet_num_42_mm_ += (bullet_allowance_num_42_mm_ - data.bullet_allowance_num_42_mm);
    bullet_allowance_num_42_mm_ = data.bullet_allowance_num_42_mm;
  }
  updateForQueue();
}

void BulletTimeChangeUi::reset()
{
  bullet_num_17_mm_ = 0;
  bullet_num_42_mm_ = 0;
}

void BulletTimeChangeUi::updateConfig()
{
  std::string bullet_allowance_num;
  if (base_.robot_id_ == RED_HERO || base_.robot_id_ == BLUE_HERO)
  {
    graph_->setIntNum(bullet_num_42_mm_);
    if (bullet_allowance_num_42_mm_ > 5)
      graph_->setColor(rm_referee::GraphColor::GREEN);
    else if (bullet_allowance_num_42_mm_ < 3)
      graph_->setColor(rm_referee::GraphColor::PINK);
    else
      graph_->setColor(rm_referee::GraphColor::YELLOW);
  }
  else
  {
    graph_->setIntNum(bullet_num_17_mm_);
    if (bullet_allowance_num_17_mm_ > 50)
      graph_->setColor(rm_referee::GraphColor::GREEN);
    else if (bullet_allowance_num_17_mm_ < 10)
      graph_->setColor(rm_referee::GraphColor::PINK);
    else
      graph_->setColor(rm_referee::GraphColor::YELLOW);
  }
  graph_->setColor(rm_referee::GraphColor::YELLOW);
}

void TargetDistanceTimeChangeUi::updateTargetDistanceData(const rm_msgs::TrackData::ConstPtr& data)
{
  if (data->id == 0)
    return;
  geometry_msgs::PointStamped output;
  geometry_msgs::PointStamped input;
  input.point.x = data->position.x;
  input.point.y = data->position.y;
  input.point.z = data->position.z;
  //  tf_buffer_.transform(input, output, "base_link");
  try
  {
    geometry_msgs::TransformStamped transform_stamped = tf_buffer_.lookupTransform("base_link", "odom", ros::Time(0));
    tf2::doTransform(input, output, transform_stamped);
  }
  catch (tf2::TransformException& ex)
  {
    ROS_ERROR("Failed to transform point: %s", ex.what());
  }
  target_distance_ = std::sqrt((output.point.x) * (output.point.x) + (output.point.y) * (output.point.y) +
                               (output.point.z) * (output.point.z));
  updateForQueue();
}

void TargetDistanceTimeChangeUi::updateConfig()
{
  graph_->setFloatNum(target_distance_);
}

void DroneTowardsTimeChangeGroupUi::updateTowardsData(const geometry_msgs::PoseStampedConstPtr& data)
{
  angle_ = yawFromQuat(data->pose.orientation) - M_PI / 2;
  mid_line_x2_ = ori_x_ + 60 * cos(angle_ - M_PI / 2);
  mid_line_y2_ = ori_y_ + 60 * sin(angle_ - M_PI / 2);
  mid_line_x1_ = ori_x_ + 60 * cos(angle_ + M_PI / 2);
  mid_line_y1_ = ori_y_ + 60 * sin(angle_ + M_PI / 2);
  left_line_x2_ = ori_x_ + 40 * cos(angle_ + (5 * M_PI) / 6);
  left_line_y2_ = ori_y_ + 40 * sin(angle_ + (5 * M_PI) / 6);
  right_line_x2_ = ori_x_ + 40 * cos(angle_ + M_PI / 6);
  right_line_y2_ = ori_y_ + 40 * sin(angle_ + M_PI / 6);
  updateForQueue();
}

void DroneTowardsTimeChangeGroupUi::updateConfig()
{
  for (auto it : graph_vector_)
  {
    if (it.first == "drone_towards_mid")
    {
      it.second->setStartX(mid_line_x2_);
      it.second->setStartY(mid_line_y2_);
      it.second->setEndX(mid_line_x1_);
      it.second->setEndY(mid_line_y1_);
    }
    else if (it.first == "drone_towards_left")
    {
      it.second->setStartX(left_line_x2_);
      it.second->setStartY(left_line_y2_);
      it.second->setEndX(mid_line_x1_);
      it.second->setEndY(mid_line_y1_);
    }
    else if (it.first == "drone_towards_right")
    {
      it.second->setStartX(right_line_x2_);
      it.second->setStartY(right_line_y2_);
      it.second->setEndX(mid_line_x1_);
      it.second->setEndY(mid_line_y1_);
    }
  }
}

void FriendBulletsTimeChangeGroupUi::updateBulletsData(const rm_referee::BulletNumData& data)
{
  if (data.header_data.sender_id == rm_referee::RobotId::RED_HERO ||
      data.header_data.sender_id == rm_referee::RobotId::BLUE_HERO)
    hero_bullets_ = data.bullet_42_mm_num;
  else if (data.header_data.sender_id == rm_referee::RobotId::RED_STANDARD_3 ||
           data.header_data.sender_id == rm_referee::RobotId::BLUE_STANDARD_3)
    standard3_bullets_ = data.bullet_17_mm_num;
  else if (data.header_data.sender_id == rm_referee::RobotId::RED_STANDARD_4 ||
           data.header_data.sender_id == rm_referee::RobotId::BLUE_STANDARD_4)
    standard4_bullets_ = data.bullet_17_mm_num;
  else if (data.header_data.sender_id == rm_referee::RobotId::RED_STANDARD_5 ||
           data.header_data.sender_id == rm_referee::RobotId::BLUE_STANDARD_5)
    standard5_bullets_ = data.bullet_17_mm_num;
  updateForQueue();
}

void FriendBulletsTimeChangeGroupUi::updateConfig()
{
  for (auto it : graph_vector_)
  {
    if (it.first == "hero")
      it.second->setIntNum(hero_bullets_);
    else if (it.first == "standard3")
      it.second->setIntNum(standard3_bullets_);
    else if (it.first == "standard4")
      it.second->setIntNum(standard4_bullets_);
    else if (it.first == "standard5")
      it.second->setIntNum(standard5_bullets_);
  }
}

void TargetHpTimeChangeUi::setEnemyHp(const rm_msgs::GameRobotHp& data)
{
  bool is_enemy_red = base_.robot_id_ > 100;
  if (is_enemy_red)
  {
    enemy_robot_hp_[1] = data.red_1_robot_hp;
    enemy_robot_hp_[2] = data.red_2_robot_hp;
    enemy_robot_hp_[3] = data.red_3_robot_hp;
    enemy_robot_hp_[4] = data.red_4_robot_hp;
    enemy_robot_hp_[7] = data.red_7_robot_hp;
  }
  else
  {
    enemy_robot_hp_[1] = data.blue_1_robot_hp;
    enemy_robot_hp_[2] = data.blue_2_robot_hp;
    enemy_robot_hp_[3] = data.blue_3_robot_hp;
    enemy_robot_hp_[4] = data.blue_4_robot_hp;
    enemy_robot_hp_[7] = data.blue_7_robot_hp;
  }
}

void TargetHpTimeChangeUi::updateTrackID(int id)
{
  target_id_ = id;
  updateTargeHptData();
}

void TargetHpTimeChangeUi::updateTargeHptData()
{
  auto it = enemy_robot_hp_.find(target_id_);
  target_hp_ = it == enemy_robot_hp_.end() ? 0 : it->second;
  updateForQueue();
}

void TargetHpTimeChangeUi::updateConfig()
{
  graph_->setIntNum(target_hp_);
  if (target_hp_ > 50)
    graph_->setColor(rm_referee::GraphColor::GREEN);
  else
    graph_->setColor(rm_referee::GraphColor::ORANGE);
}

}  // namespace rm_referee
