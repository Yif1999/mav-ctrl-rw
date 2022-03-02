/**
 * @file offb_node.cpp
 * @brief Offboard control example node, written with MAVROS version 0.19.x, PX4 Pro Flight
 * Stack and tested in Gazebo SITL
 */

#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <offb/target.h>
#include <offb/target.h>
#include <eigen3/Eigen/Dense>

mavros_msgs::State current_state;
geometry_msgs::PoseStamped pose;
float x=0,y=0,z=-1000;
float rx,ry,rz,rw;

void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

void targetCallback(const offb::target::ConstPtr&target){
    x=target->x;
    y=target->y;
    z=target->z;
    rx=target->rx;
    ry=target->ry;
    rz=target->rz;
    rw=target->rw;
    // ROS_INFO("get the target!");
}

void poseCallback(const geometry_msgs::PoseStamped::ConstPtr &msg){
   pose=*msg;
   x=pose.pose.position.x;
   y=pose.pose.position.y;
   z=pose.pose.position.z;
   rx= pose.pose.orientation.x;
   ry= pose.pose.orientation.y;
   rz= pose.pose.orientation.z;
   rw= pose.pose.orientation.w;
  //  ROS_INFO("actually:x=%f,y=%f",x,y);
}

int main(int argc, char **argv)
{

    ros::init(argc, argv, "listener");
    ros::NodeHandle nh;
    
    ros::Subscriber sub=nh.subscribe("target_info",1,targetCallback);
    ros::Subscriber local_pos_sub=nh.subscribe<geometry_msgs::PoseStamped>("mavros/local_position/pose",1,poseCallback);
    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>
            ("mavros/state", 10, state_cb);
    ros::Publisher local_pos_pub = nh.advertise<geometry_msgs::PoseStamped>
            ("mavros/setpoint_position/local", 10);
    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>
            ("mavros/cmd/arming");
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>
            ("mavros/set_mode");

    //the setpoint publishing rate MUST be faster than 2Hz
    ros::Rate rate(20.0);

    // wait for FCU connection and Data get
    while(ros::ok() &&( !current_state.connected || z==-1000)){
        ros::spinOnce();
        rate.sleep();
    }

    pose.pose.position.x = x;
    pose.pose.position.y = y;
    pose.pose.position.z = z;
    pose.pose.orientation.x=rx;
    pose.pose.orientation.y=ry;
    pose.pose.orientation.z=rz;
    pose.pose.orientation.w=rw;
     //Eigen::AngleAxisd rotation_vector(90,Eigen::Vector3d(0,0,1));
     //Eigen::Quaterniond q_mocap(rotation_vector);
     //rx=q_mocap.x();
     //ry=q_mocap.y();
     //rz=q_mocap.z();
     //rw=q_mocap.w();

    //send a few setpoints before starting
    for(int i = 20; ros::ok() && i > 0; --i){
        local_pos_pub.publish(pose);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    //ros::Time last_request = ros::Time::now();

	if( set_mode_client.call(offb_set_mode) &&
                offb_set_mode.response.mode_sent){
                ROS_INFO("Offboard enabled");
            }
	if( arming_client.call(arm_cmd) &&
                    arm_cmd.response.success){
                    ROS_INFO("Vehicle armed");
            }

    while(ros::ok()){
        //if( current_state.mode != "OFFBOARD" &&
        //    (ros::Time::now() - last_request > ros::Duration(5.0))){
        //    if( set_mode_client.call(offb_set_mode) &&
        //        offb_set_mode.response.mode_sent){
        //        ROS_INFO("Offboard enabled");
        //    }
        //    last_request = ros::Time::now();
        //} else {
        //    if( !current_state.armed &&
        //        (ros::Time::now() - last_request > ros::Duration(5.0))){
        //        if( arming_client.call(arm_cmd) &&
        //            arm_cmd.response.success){
        //            ROS_INFO("Vehicle armed");
        //        }
        //        last_request = ros::Time::now();
        //    }
        //}
	
        pose.pose.position.x = x;
        pose.pose.position.y = y;
        pose.pose.position.z = z;
        pose.pose.orientation.x=rx;
        pose.pose.orientation.y=ry;
        pose.pose.orientation.z=rz;
        pose.pose.orientation.w=rw;


        local_pos_pub.publish(pose);

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}
