#include<ros/ros.h>
#include<offb/target.h>
#include<math.h>
#include <geometry_msgs/PoseStamped.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

float x=0,y=0,z=2;
geometry_msgs::PoseStamped pose;
//float xt[200],yt[200],xa[200],ya[200];
int i=0;

void poseCallback(const geometry_msgs::PoseStamped::ConstPtr &msg){
   pose=*msg;
   x=pose.pose.position.x;
   y=pose.pose.position.y;
   z=pose.pose.position.z;
   //ROS_INFO("actually:x=%f,y=%f",x,y);
   //xa[i]=x;ya[i]=y;
}

int main(int argc,char** argv){
  ros::init(argc,argv,"talker");
  ros::NodeHandle nh;

  ros::Publisher pub=nh.advertise<offb::target>("target_info",1);
  ros::Subscriber sub=nh.subscribe<geometry_msgs::PoseStamped>("mavros/local_position/pose",1,poseCallback);
  ros::Rate rate(20);

  offb::target msg;
  msg.x=0;
  msg.y=0;
  msg.z=30;
  msg.state="ok";
  pub.publish(msg);
  
  float time=0;

 // while(ros::ok() && i<200){
  while(ros::ok()){
	ros::spinOnce();
	rate.sleep();
	time+=0.001;
	msg.x+=0.5;
	msg.y=100*sin(time);
	//ROS_INFO("target:x=%f,y=%f",msg.x,msg.y);
	//xt[i]=msg.x;yt[i]=msg.y;
	pub.publish(msg);
	i++;
  }
/*
	FILE *aWrite=fopen("a.txt","w+");
	if(aWrite==NULL){
	  printf("failed write file\n");
	}else{
	for(int i=0;i<200;i++){
	  fprintf(aWrite,"%f	%f\n",xa[i],ya[i]);
	}
	  fclose(aWrite);
	}
	
	FILE *tWrite=fopen("t.txt","w+");
	if(tWrite==NULL){
	  printf("failed write file\n");
	}else{
	for(int i=0;i<200;i++){
	  fprintf(tWrite,"%f	%f\n",xt[i],yt[i]);
	}
	  fclose(tWrite);
	}


	msg.x=0;
  	msg.y=0;
 	msg.z=2;
  	msg.state="ok";
  	pub.publish(msg);

*/
  return 0;
}

