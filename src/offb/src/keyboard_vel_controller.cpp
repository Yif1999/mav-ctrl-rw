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
#include <geometry_msgs/TwistStamped.h>

#include <tf/tfMessage.h>


geometry_msgs::Twist vector;
geometry_msgs::TwistStamped  speed; //zzx
mavros_msgs::State current_state;
geometry_msgs::PoseStamped pose;
geometry_msgs::PoseStamped hover_pose;
pthread_t th; 
int ctrlMode=0;
int hover=0;
int controlMode = 1;
float xt = 0;
float yt = 0;
float zt = 0;
float vx_temp =0;
float vy_temp =0;
float vz_temp =0;

void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

void poseCallback(const geometry_msgs::PoseStamped::ConstPtr &msg){
   pose=*msg;
}
//tf get position
geometry_msgs::PoseStamped pos;
tf::tfMessage listener;

void tf_cb(const tf::tfMessage::ConstPtr &msg){
    listener=*msg;
    pos.pose.position.x=listener.transforms[0].transform.translation.x;
    pos.pose.position.y=listener.transforms[0].transform.translation.y;
    pos.pose.position.z=listener.transforms[0].transform.translation.z;
    pos.pose.orientation.w=listener.transforms[0].transform.rotation.w;
    pos.pose.orientation.x=listener.transforms[0].transform.rotation.x;
    pos.pose.orientation.y=listener.transforms[0].transform.rotation.y;
    pos.pose.orientation.z=listener.transforms[0].transform.rotation.z;
}

//



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
        //printf("%d\r\n",key);
	    if(current_state.armed){
            validInput=1;
            //if control mode is hover(2), then set to default velocity control; if it's position control (0), then as is 
            if (controlMode==2)
                controlMode = 1;

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
                    vector.linear.y=vector.linear.y+scale;
                    //vector.linear.x=vector.linear.x+scale;
                    break;
                case 106:   //j->106
                    vector.linear.x=vector.linear.x-scale;
                    //vector.linear.y=vector.linear.y+scale;
                    break;
                case 107:   //k->107
                    vector.linear.y=vector.linear.y-scale;
                    //vector.linear.x=vector.linear.x-scale;
                    break;
                case 108:   //l->108
                    vector.linear.x=vector.linear.x+scale;
                    //vector.linear.y=vector.linear.y-scale;
                    break;
                case 32:    //Space->32 (Press to brake)
                    vector.linear.x=0;vector.linear.y=0;vector.linear.z=0;vector.angular.z=0;
                    controlMode =2;   //hover
                    break;
                case 109:    //m->109 (Press to select control mode)
                    if(controlMode ==1)
                        printf("Current control mode: \"Veolcity\"\r\n");
                    else
                        printf("Current control mode: \"Position\"\r\n");
                    
                    printf("Please select offboard control mode: press v for velocity control; press p for position control: \n");
                    key=sh_getch(); 
                    switch (key)
                    {
                        case 118:  //v->118
                            controlMode = 1;
                            break;
                        case 112:  //p->112
                            controlMode = 0;
                            printf("please input target position in \"x  y  z\" :\n");
                            scanf("%f %f %f",&xt,&yt,&zt);
                            printf("get position [%f, %f, %f] successfully!\n",xt,yt,zt);
                            break;
                        default:
                            printf("[Warning!] Invalid selection, please press m and slelect again!: \n");
                    }                     
                    break;
                default:
/*                     if (controlMode ==2)
                    {
                        printf("[Warning!] Please first press m and slelect a flight mode,v for velocity and p for position!: \n");
                    }
                    else{
                        printf("No this command! (Press Esc to exit, your drone will brake and land)\n");
                    } */                   
                    validInput=0; 
            }       
            if (validInput) {
                printf("vx=%f, vy=%f, vz=%f, wx=%f, wy=%f, wz=%f\r\n",vector.linear.x,vector.linear.y,vector.linear.z,vector.angular.x,vector.angular.y,vector.angular.z);
                if (abs(vector.linear.x)+abs(vector.linear.y)+abs(vector.linear.z)+abs(vector.angular.z)==0  && controlMode ==2){
                    hover=1;
                    hover_pose=pose;
                }
                else{   
                    hover=0;
                }
            }
            printf("positionx=%f, positiony=%f, positionz=%f, wx=%f, wy=%f, wz=%f\r\n", pose.pose.position.x,pose.pose.position.y,pose.pose.position.z,vector.linear.x,vector.linear.y,vector.linear.z);
            
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
//位置PID控制器
float Position_PID (float curPos ,float TarPos)
{ 	
	 float Position_KP=0.3,  Position_KI=0.03, Position_KD=0.1;
	 float Bias =0,  Velocity=0,  Integral_bias=0,  Last_Bias=0;
	 Bias = TarPos -  curPos;                 //计算偏差
	 Integral_bias+=Bias;	                                 //求出偏差的积分
	 Velocity = Position_KP*Bias+Position_KI*Integral_bias+Position_KD*(Bias-Last_Bias);       //位置式PID控制器
	 Last_Bias=Bias;                                       //保存上一次偏差 

	 return Velocity;                                           //增量输出
}

// MAIN PROGRAM
int main(int argc,char** argv){
    //t265相机 旋转矩阵
    Eigen::Matrix3d Mat1; //90 degree rotation matrix
    Mat1 << 0,-1,0,1,0,0,0,0,1;
    
    ros::init(argc,argv,"keyboard_vel_controller");
    ros::NodeHandle nh;
    //t265 message
     ros::Subscriber tf_sub=nh.subscribe<tf::tfMessage>("/tf",1,tf_cb);
    //ros::Publisher velocity_pub = nh.advertise<geometry_msgs::TwistStamped>("mavros/setpoint_velocity/cmd_vel", 10); //zzx
    //ros::Publisher vel_pub=nh.advertise<geometry_msgs::Twist>("/mavros/setpoint_velocity/cmd_vel_stamped",1);

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
        pos_pub.publish(hover_pose);
        //vel_pub.publish(vector);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    mavros_msgs::SetMavFrame body_frame;
    body_frame.request.mav_frame=1;
    
    if( set_mode_client.call(offb_set_mode) && offb_set_mode.response.mode_sent)
    {
        printf("[INFO] Offboard enabled\n");
        if( arming_client.call(arm_cmd) && arm_cmd.response.success)
        {
            printf("[INFO] Vehicle armed\n");
            if( set_frame_client.call(body_frame) &&body_frame.response.success)       {
                printf("[INFO] Set BODY_FRAME of velocity successfully\n");
                pthread_create(&th,NULL,keyboard_listener,0);
            }
            else     {
                printf("[INFO] Failed to set BODY_FRAME, please retry!\n");
            }
        }
        else      {
            printf("[INFO] Failed to arm, please retry! \n");
        }
    }
    else  {
        printf("[INFO] Failed to set Offboard, please retry! \n");
    }

    sleep(1);
    if (ctrlMode)
        printf("[Ready] Controller Mode On !\n");
    else
        printf("[INFO] Failed to start Controller Mode, please retry!\n");
        
    
    //printf("\npositionx=%f \npositiony=%f \npositionz=%f\nwx=%f \nwy=%f \nwz=%f\n\n---\n",pose.pose.position.x,pose.pose.position.y,pose.pose.position.z,vector.angular.x,vector.angular.y,vector.angular.z);
    while(ros::ok()&& ctrlMode){
        //P1 original coordinate, P2: new coordinate
        Eigen::Vector3d P1(pos.pose.position.x,pos.pose.position.y,pos.pose.position.z);
        Eigen::Quaterniond Q1(pos.pose.orientation.w,pos.pose.orientation.x,pos.pose.orientation.y,pos.pose.orientation.z);
        Eigen::Vector3d P2=Mat1*P1;
        Eigen::Quaterniond QMat1(Mat1);
        Eigen::Quaterniond Q2=QMat1*Q1;

    // float yaw1;
	// Eigen::Vector3d eulerAngle1 = Q1.matrix().eulerAngles(2,1,0);
	// yaw1=eulerAngle1[0];
	// printf("%lf\n",yaw1);

    // float yaw2;
	// Eigen::Vector3d eulerAngle2 = Q2.matrix().eulerAngles(2,1,0);
	// yaw2=eulerAngle2[0];
	// printf("%lf\n",yaw2);

        // cout << "---"<<endl;
        // cout << "   translation: "<< endl;
        // cout << "       x: "<<P2.x() <<endl;
        // cout << "       y: "<<P2.y() <<endl;
        // cout << "       z: "<<P2.z() <<endl;
        // cout << "   orientation: "<< endl;
        // cout << "       x: "<<Q2.x() <<endl;
        // cout << "       y: "<<Q2.y() <<endl;
        // cout << "       z: "<<Q2.z() <<endl;
        // cout << "       w: "<<Q2.w() <<endl;
        //printf("vx=%#f,  vy=%#f,  vz=%#f,  pose.x=%#f,  pose.y=%#f,  pose.z=%#f\r\n", vector.linear.x ,   vector.linear.y,   vector.linear.z, pose.pose.position.x,  pose.pose.position.y,  pose.pose.position.z );
        
        if (hover){
            //hover control @ flight controller
            // printf("hoverMode:%d\nx:%f\ny:%f\nz:%f\n",hover,hover_pose.pose.position.x,hover_pose.pose.position.y,hover_pose.pose.position.z);
            pos_pub.publish(hover_pose);
        }
        else{
            if(controlMode ==1)
            {//veolcity control
                vel_pub.publish(vector);
            }
            else if (controlMode==0)            
            {//position control @ upboard

                vx_temp = Position_PID(pose.pose.position.x, xt);
                vy_temp = Position_PID(pose.pose.position.y, yt);
                vz_temp = Position_PID(pose.pose.position.z, zt);
                vector.linear.x = vx_temp;               
                vector.linear.y= vy_temp; 
                vector.linear.z = vz_temp;        

               vel_pub.publish(vector);
            }
        }

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}