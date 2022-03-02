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

#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


//全局坐标buffer
double x=0;
double y=0;
double z=0;
double xo=0;
double yo=0;
double zo=0;
double xt=0;
double yt=0;
double zt=0;
int input=0;

mavros_msgs::State current_state;
void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}
geometry_msgs::PoseStamped pose;

void poseCallback(const geometry_msgs::PoseStamped::ConstPtr &msg){
   if (input==1){
   pose=*msg;
   xo=pose.pose.position.x;
   yo=pose.pose.position.y;
   zo=pose.pose.position.z;
   ROS_INFO("origin:xo=%lf,yo=%lf,zo=%lf",xo,yo,zo);
   input=0;
  }
}


//子线程监听键盘输入
void *listener(void *dummy)
{ 
	while(ros::ok())
	{    
	    if(current_state.armed){
		xo=0;yo=0;zo=0;
		printf("please input target position:\n");
		scanf("%lf %lf %lf",&xt,&yt,&zt);
		printf("get position [%lf,%lf,%lf] successfully!\n",xt,yt,zt);
		input =1;
	    }
	}

	return NULL;
}

int main(int argc, char **argv)
{

    ros::init(argc, argv, "fly_to_point");
    ros::NodeHandle nh;
    ros::Subscriber sub=nh.subscribe<geometry_msgs::PoseStamped>("mavros/local_position/pose",1,poseCallback);
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

    // wait for FCU connection
    while(ros::ok() && !current_state.connected){
        ros::spinOnce();
        rate.sleep();
    }


    geometry_msgs::PoseStamped pose;
    pose.pose.position.x = 0;
    pose.pose.position.y = 0;
    pose.pose.position.z = 0;

    //send a few setpoints before starting
    for(int i = 100; ros::ok() && i > 0; --i){
        local_pos_pub.publish(pose);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    ros::Time last_request = ros::Time::now();

    pthread_t th; 
    pthread_create(&th,NULL,listener,0);

    while(ros::ok()){
        if( current_state.mode != "OFFBOARD" &&
            (ros::Time::now() - last_request > ros::Duration(5.0))){
            if( set_mode_client.call(offb_set_mode) &&
                offb_set_mode.response.mode_sent){
                ROS_INFO("Offboard enabled");
            }
            last_request = ros::Time::now();
        } else {
            if( !current_state.armed &&
                (ros::Time::now() - last_request > ros::Duration(5.0))){
                if( arming_client.call(arm_cmd) &&
                    arm_cmd.response.success){
                    ROS_INFO("Vehicle armed");
                }
                last_request = ros::Time::now();
            }
        }
	
        pose.pose.position.x = xo+xt;
        pose.pose.position.y = yo+yt;
        pose.pose.position.z = zo+zt;
        local_pos_pub.publish(pose);

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}
