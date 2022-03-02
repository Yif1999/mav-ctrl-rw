#include <stdio.h>
#include <ros/ros.h>
#include <offb/target.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/PoseStamped.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <mavros_msgs/SetMavFrame.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <termio.h>
#include <pthread.h>
#include <eigen3/Eigen/Dense>

geometry_msgs::Twist vector;
mavros_msgs::State current_state;
geometry_msgs::PoseStamped pose;
geometry_msgs::PoseStamped hover_pose;
pthread_t th; 
int ctrlMode=0;
int hover=0;

void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

void poseCallback(const geometry_msgs::PoseStamped::ConstPtr &msg){
   pose=*msg;
}

//Keyboard Mode Setting
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

//子线程监听键盘
void *keyboard_listener(void *dummy)
{ 
    ctrlMode=1;
	int key;
    float scale=0.2;
    int validInput=1;
	while(ros::ok())
	{    
        key=sh_getch(); 
	    if(current_state.armed){
            validInput=1;
            if (key==27){   //Esc->27
                hover=1;
                hover_pose=pose;
                ctrlMode=0;
                printf("[INFO]Landing...\n");
                pthread_cancel(th);
                return NULL;
            }  
            switch (key)
            {
                case 119:   // w->119
                    vector.linear.z=vector.linear.z+scale;
                    break;
                case 97:     // a->97 
                    vector.angular.z=vector.angular.z+scale;
                    break;
                case 115:   //s->115 
                    vector.linear.z=vector.linear.z-scale;
                    break;
                case 100:   //d->100
                    vector.angular.z=vector.angular.z-scale;
                    break;
                case 105:   //i->105
                    vector.linear.x=vector.linear.x+scale;
                    break;
                case 106:   //j->106
                    vector.linear.y=vector.linear.y+scale;
                    break;
                case 107:   //k->107
                    vector.linear.x=vector.linear.x-scale;
                    break;
                case 108:   //l->108
                    vector.linear.y=vector.linear.y-scale;
                    break;
                case 32:    //Space->32 (Press to brake)
                    vector.linear.x=0;vector.linear.y=0;vector.linear.z=0;vector.angular.z=0;
                    break;
                default:
                    printf("No this command! (Press Esc to exit, your drone will brake and land)\n");
                    validInput=0;
            }       
            if (validInput) {
                printf("\nvx=%f \nvy=%f \nvz=%f\nwx=%f \nwy=%f \nwz=%f\n\n---\n",vector.linear.x,vector.linear.y,vector.linear.z,vector.angular.x,vector.angular.y,vector.angular.z);
                if (abs(vector.linear.x)+abs(vector.linear.y)+abs(vector.linear.z)+abs(vector.angular.z)==0){
                    hover=1;
                    hover_pose=pose;
                }
                else{
                    hover=0;
                }
            }
	    }
        else{
            printf("\n[INFO] Takeoff Error, please retry!\n");
            ctrlMode=0;
            pthread_cancel(th);
        }
        pthread_testcancel();
	}

	return NULL;
}

// MAIN PROGRAM
int main(int argc,char** argv){

    ros::init(argc,argv,"keyboard_vel_controller");
    ros::NodeHandle nh;
    ros::Publisher vel_pub=nh.advertise<geometry_msgs::Twist>("/mavros/setpoint_velocity/cmd_vel_unstamped",1);
    ros::Publisher pos_pub = nh.advertise<geometry_msgs::PoseStamped>("mavros/setpoint_position/local", 1);
    ros::Subscriber pos_sub=nh.subscribe<geometry_msgs::PoseStamped>("mavros/local_position/pose",1,poseCallback);
    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>("mavros/state", 1, state_cb);
    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>("mavros/cmd/arming");
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>("mavros/set_mode");
    ros::ServiceClient set_frame_client =nh.serviceClient<mavros_msgs::SetMavFrame>("/mavros/setpoint_velocity/mav_frame");
    ros::Rate rate(20);

    // wait for FCU connection
    while(ros::ok() && !current_state.connected){
        ros::spinOnce();
        rate.sleep();
    }

    vector.linear.x=0;
    vector.linear.y=0;
    vector.linear.z=0;
    vector.angular.x=0;
    vector.angular.y=0;
    vector.angular.z=0;
    hover_pose=pose;

    //send a few info before starting
    for(int i = 20; ros::ok() && i > 0; --i){
        vel_pub.publish(vector);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    mavros_msgs::SetMavFrame body_frame;
    body_frame.request.mav_frame=8;
    
    if( set_mode_client.call(offb_set_mode) &&
        offb_set_mode.response.mode_sent){
        printf("[INFO] Offboard enabled\n");
        if( arming_client.call(arm_cmd) &&
            arm_cmd.response.success){
            printf("[INFO] Vehicle armed\n");
            if( set_frame_client.call(body_frame) &&
            body_frame.response.success){
                printf("[INFO] Set BODY_FRAME of velocity successfully\n");
                pthread_create(&th,NULL,keyboard_listener,0);
            }else{
                printf("[INFO] Failed to set BODY_FRAME, please retry!\n");
            }
        }else{
            printf("[INFO] Failed to arm, please retry! \n");
        }
    }else{
        printf("[INFO] Failed to set Offboard, please retry! \n");
    }

    sleep(1);
    if (ctrlMode)
        printf("[Ready] Controller Mode On !\n");
    else
        printf("[INFO] Failed to start Controller Mode, please retry!\n");

    while(ros::ok()&& ctrlMode){
        if (hover){
            // printf("hoverMode:%d\nx:%f\ny:%f\nz:%f\n",hover,hover_pose.pose.position.x,hover_pose.pose.position.y,hover_pose.pose.position.z);
            pos_pub.publish(hover_pose);
        }
        else{
            vel_pub.publish(vector);
        }

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}