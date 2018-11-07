// simple ariac intro node: 
#include <ros/ros.h> 
#include <std_msgs/String.h> 
#include <std_msgs/Float64.h> 
#include <std_srvs/Trigger.h>
#include <osrf_gear/ConveyorBeltControl.h>
#include <osrf_gear/DroneControl.h>
#include <osrf_gear/LogicalCameraImage.h>


bool boxSeenByCamera = false;

void logCameraCallback(const osrf_gear::LogicalCameraImage& message) 
{ 
 
  if(message.models.size() > 0){
    //there is a box
    if(message.models[0].pose.position.z <= 0.05 && message.models[0].pose.position.z >= -0.05){
        boxSeenByCamera = true;
    }
  }
  
} 

int main(int argc, char **argv) {
    ros::init(argc, argv, "simple_ariac_intro");
    
    ros::NodeHandle n; // node handle 

    //setup a service client to start the competition
    ros::ServiceClient startClient = n.serviceClient<std_srvs::Trigger>("/ariac/start_competition");
    std_srvs::Trigger startSrv;
    while(!startSrv.response.success){
        if (startClient.call(startSrv)) {
            ROS_INFO("Started Competition");
        }
        else{
            ROS_INFO("Failed to Start Competition");
        }
    }
    

    //setup a service client to control the conveyor
    ros::ServiceClient conveyorClient = n.serviceClient<osrf_gear::ConveyorBeltControl>("/ariac/conveyor/control");
    osrf_gear::ConveyorBeltControl conveyorSrv;
    conveyorSrv.request.power = 100.0;
    if (conveyorClient.call(conveyorSrv)) {
        ROS_INFO("Started Conveyor");
    }
    else{
        ROS_INFO("Failed to Start Conveyor");
    }

    //monitor logical-camera 2 until a shipping_box has z-value of ~0.
    ros::Subscriber logCameraSubscriber= n.subscribe("/ariac/logical_camera_2",1,logCameraCallback);
    while(!boxSeenByCamera){
        ros::spinOnce();
    }

    //stop the conveyor
    conveyorSrv.request.power = 0.0;
    if (conveyorClient.call(conveyorSrv)) {
        ROS_INFO("Stopped Conveyor");
    }
    else{
        ROS_INFO("Failed to Stop Conveyor");
    }

    //pause for 5 seconds
    ros::Duration(5).sleep();

    //restart the conveyor, run until a box slides into the loading dock
    conveyorSrv.request.power = 100.0;
    if (conveyorClient.call(conveyorSrv)) {
        ROS_INFO("Restarted Conveyor");
    }
    else{
        ROS_INFO("Failed to Restart Conveyor");
    }

    //call the drone
    ros::ServiceClient droneClient = n.serviceClient<osrf_gear::DroneControl>("/ariac/drone");
    osrf_gear::DroneControl droneSrv;
    droneSrv.request.shipment_type = "order_0_shipment_0";

    //keep calling the drone until it works
    bool droneSuccess = false;
    while(!droneSuccess){
        if (droneClient.call(droneSrv)) {
            if(droneSrv.response.success == true){
                ROS_INFO("Called the Drone");
                droneSuccess = true;
            }
            else{
                ROS_INFO("Nothing for drone to collect");
            }
            
        }
        else{
            ROS_INFO("Failed to Call Drone");
        }
        ros::Duration(1).sleep();
    }
    
    //stop conveyor
    conveyorSrv.request.power = 0.0;
    if (conveyorClient.call(conveyorSrv)) {
        ROS_INFO("Stopped Conveyor");
    }
    else{
        ROS_INFO("Failed to Stop Conveyor");
    }


    return 0;
} 
