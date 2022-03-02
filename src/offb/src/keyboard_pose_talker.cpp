#include<ros/ros.h>
#include<offb/target.h>
#include <geometry_msgs/PoseStamped.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <mavros_msgs/State.h>
#include<termio.h>
#include <pthread.h>
#include <eigen3/Eigen/Dense>

float x,y,z=-1000;
float xt,yt,zt,yaw=0;
float rx,ry,rz,rw;
geometry_msgs::PoseStamped pose;
mavros_msgs::State current_state;
void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

int sh_getch(void) {
    int cr;
    struct termios nts, ots;

    if (tcgetattr(0, &ots) < 0) // 得到当前终端(0表示标准输入)的设置
        return EOF;

    nts = ots;
    cfmakeraw(&nts); // 设置终端为Raw原始模式，该模式下所有的输入数据以字节为单位被处理
    if (tcsetattr(0, TCSANOW, &nts) < 0) // 设置上更改之后的设置
        return EOF;

    cr = getchar();
    if (tcsetattr(0, TCSANOW, &ots) < 0) // 设置还原成老的模式
        return EOF;

    return cr;
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

//get a few setpoints before starting
//void init(){
//	ros::Rate rate(20.0);
//	for(int i = 20; ros::ok() && i > 0; --i){
//	    ros::spinOnce();
//	    rate.sleep();
//	}

//	Eigen::Quaterniond quaternion(rw,rx,ry,rz);
//	Eigen::Vector3d eulerAngle = quaternion.matrix().eulerAngles(2,1,0);
//	yaw=eulerAngle[0];
//	printf("%lf",yaw);
//}

//子线程监听键盘方向键
// w->119   a->97   s->115  d->100  i->105    j->106   k->107  l->108    Esc->27
void *listener(void *dummy)
{ 
	int key;
	while(ros::ok())
	{    
	    // if(current_state.armed){
				key=sh_getch(); 
                if (key==27)    return NULL;
                switch (key)
                {
                    case 119:
                        zt=z+(1.5*(z<0.5)+0.5*(z>=0.5));
                        break;
                    case 97:
                        yaw+=0.05;
                        break;
                    case 115:
                        zt=z-0.5;
                        break;
                    case 100:
                        yaw-=0.05;
                        break;
                    case 105:
                        xt=x+0.5;
                        break;
                    case 106:
                        yt=y+0.5;
                        break;
                    case 107:
                        xt=x-0.5;
                        break;
                    case 108:
                        yt=y-0.5;
                        break;
                    default:
                        printf("No this command! (Press Esc to exit)\n");
                }       
                Eigen::AngleAxisd rotation_vector(yaw,Eigen::Vector3d(0,0,1));
                Eigen::Quaterniond q_mocap(rotation_vector);
                rx=q_mocap.x();
                ry=q_mocap.y();
                rz=q_mocap.z();
                rw=q_mocap.w();   
	    // }
	}
	return NULL;
}



int main(int argc,char** argv){
  ros::init(argc,argv,"keyboard_pose_controller");
  ros::NodeHandle nh;

  ros::Publisher pub=nh.advertise<offb::target>("target_info",1);
  ros::Subscriber sub=nh.subscribe<geometry_msgs::PoseStamped>("mavros/local_position/pose",1,poseCallback);
  ros::Rate rate(20);

  pthread_t th; 
  pthread_create(&th,NULL,listener,0);

  ros::spinOnce();
  offb::target msg;
  msg.state="ok";

   while( ros::ok() && z==-1000){
        ros::spinOnce();
        rate.sleep();
        xt=x;yt=y;zt=z;
    }

	Eigen::Quaterniond quaternion(rw,rx,ry,rz);
	Eigen::Vector3d eulerAngle = quaternion.matrix().eulerAngles(2,1,0);
	yaw=eulerAngle[0];
	xt=x;yt=y;zt=z;
	printf("yaw:%lf\n",yaw);
        printf("--Controller Mode--\n");

  while(ros::ok()){
	ros::spinOnce();
	rate.sleep();
	msg.x=xt;
	msg.y=yt;
	msg.z=zt;
	msg.rx=rx;
	msg.ry=ry;
	msg.rz=rz;
	msg.rw=rw;   
	pub.publish(msg);
  }

  return 0;
}

