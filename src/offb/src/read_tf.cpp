#include <iostream>
#include <ros/ros.h>
#include <math.h>
#include <tf/tfMessage.h>
#include <geometry_msgs/PoseStamped.h>
#include <Eigen/Dense>

using namespace Eigen;
using namespace std;

geometry_msgs::PoseStamped pose;
tf::tfMessage listener;

void tf_cb(const tf::tfMessage::ConstPtr &msg){
    listener=*msg;
    pose.pose.position.x=listener.transforms[0].transform.translation.x;
    pose.pose.position.y=listener.transforms[0].transform.translation.y;
    pose.pose.position.z=listener.transforms[0].transform.translation.z;
    pose.pose.orientation.w=listener.transforms[0].transform.rotation.w;
    pose.pose.orientation.x=listener.transforms[0].transform.rotation.x;
    pose.pose.orientation.y=listener.transforms[0].transform.rotation.y;
    pose.pose.orientation.z=listener.transforms[0].transform.rotation.z;
}

int main(int argc, char **argv)
{
    Eigen::Matrix3d Mat1; //90 degree rotation matrix
    Mat1 << 0,-1,0,1,0,0,0,0,1;


    ros::init(argc, argv, "read_tf");
    ros::NodeHandle nh;
    ros::Subscriber tf_sub=nh.subscribe<tf::tfMessage>("/tf",1,tf_cb);
    ros::Rate rate(20.0);

    while(ros::ok()){

        Eigen::Vector3d P1(pose.pose.position.x,pose.pose.position.y,pose.pose.position.z);
        Eigen::Quaterniond Q1(pose.pose.orientation.w,pose.pose.orientation.x,pose.pose.orientation.y,pose.pose.orientation.z);
        Eigen::Vector3d P2=Mat1*P1;
        Eigen::Quaterniond QMat1(Mat1);
        Eigen::Quaterniond Q2=QMat1*Q1;

        printf("---\n");
        float yaw1;
        Eigen::Vector3d eulerAngle1 = Q1.matrix().eulerAngles(2,1,0);
        yaw1=eulerAngle1[0];
        printf("yaw1: %lf\n",yaw1);

        float yaw2;
        Eigen::Vector3d eulerAngle2 = Q2.matrix().eulerAngles(2,1,0);
        yaw2=eulerAngle2[0];
        printf("yaw2: %lf\n",yaw2);

        cout << "---"<<endl;
        cout << "   translation: "<< endl;
        cout << "       x: "<<P2.x() <<endl;
        cout << "       y: "<<P2.y() <<endl;
        cout << "       z: "<<P2.z() <<endl;
        cout << "   orientation: "<< endl;
        cout << "       x: "<<Q2.x() <<endl;
        cout << "       y: "<<Q2.y() <<endl;
        cout << "       z: "<<Q2.z() <<endl;
        cout << "       w: "<<Q2.w() <<endl;

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}