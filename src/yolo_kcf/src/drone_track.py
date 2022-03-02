#!/usr/bin/env python3

import sys
import cv2 as cv
import rospy
import argparse
import sys
import numpy as np
import time
from copy import deepcopy
import imutils
from object_detection import object_detector
from std_msgs.msg import Int16MultiArray
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError


def imgCallback(data):
    global bridge, stream,wait
    wait += 1
    if wait ==1:
        wait = 0
        stream = bridge.imgmsg_to_cv2(data, "bgr8")
    else:
        pass 

def drawPred(frame, objects_detected):

    objects_list = list(objects_detected.keys())

    for object_, info in objects_detected.items():
        box = info[0]
        confidence = info[1]
        label = '%s: %.2f' % (object_, confidence)
        p1 = (int(box[0]), int(box[1]))
        p2 = (int(box[0] + box[2]), int(box[1] + box[3]))
        cv.rectangle(frame, p1, p2, (0, 255, 0))
        left = int(box[0])
        top = int(box[1])
        labelSize, baseLine = cv.getTextSize(label, cv.FONT_HERSHEY_SIMPLEX, 0.5, 1)
        top = max(top, labelSize[1])
        cv.rectangle(frame, (left, top - labelSize[1]), (left + labelSize[0], top + baseLine), (255, 255, 255), cv.FILLED)
        cv.putText(frame, label, (left, top), cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0))  


def postprocess(frame, out, threshold, classes, framework):

    frameHeight = frame.shape[0]
    frameWidth = frame.shape[1]
    objects_detected = dict()

    # Network produces output blob with a shape NxC where N is a number of
    # detected objects and C is a number of classes + 4 where the first 4
    # numbers are [center_x, center_y, width, height]
    for detection in out:
        confidences = detection[5:]
        classId = np.argmax(confidences)
        confidence = confidences[classId]
        if confidence > threshold:
            center_x = int(detection[0] * frameWidth)
            center_y = int(detection[1] * frameHeight)
            width = int(detection[2] * frameWidth)
            height = int(detection[3] * frameHeight)
            left = center_x - (width / 2)
            top = center_y - (height / 2)
            
            i = 0
            label = classes[classId]
            label_with_num = str(label) + '_' + str(i)
            while(True):
                if label_with_num not in objects_detected.keys():
                    break
                label_with_num = str(label) + '_' + str(i)
                i = i+1
            objects_detected[label_with_num] = [(int(left),int(top),int(width),int(height)),confidence]
            #print(label_with_num + ' at co-ordinates '+ str(objects_detected[label_with_num]))

    return objects_detected

def intermediate_detections(stream, predictor, threshold, classes):
    
    frame = stream
    predictions = predictor.predict(frame)
    objects_detected = postprocess(frame, predictions, threshold, classes, predictor.framework)
        
    objects_list = list(objects_detected.keys())
    # print('Tracking the following objects', objects_list)

    trackers_dict = dict()    
    #multi_tracker = cv.MultiTracker_create()

    if len(objects_list) > 0:
        
        trackers_dict = {key : cv.TrackerKCF_create() for key in objects_list}
        for item in objects_list:
            trackers_dict[item].init(frame, objects_detected[item][0])
            
    return stream, objects_detected, objects_list, trackers_dict

def process(args):

    objects_detected = dict()

    """
    #tracker_types = ['BOOSTING', 'MIL','KCF', 'TLD', 'MEDIANFLOW', 'GOTURN']
    #tracker_type = tracker_types[2]
    #tracker = None

    
    if tracker_type == 'BOOSTING':
        tracker = cv.TrackerBoosting_create()
    if tracker_type == 'MIL':
        tracker = cv.TrackerMIL_create()
    if tracker_type == 'KCF':
        tracker = cv.TrackerKCF_create()
    if tracker_type == 'TLD':
        tracker = cv.TrackerTLD_create()
    if tracker_type == 'MEDIANFLOW':
        tracker = cv.TrackerMedianFlow_create()
    if tracker_type == 'GOTURN':
        tracker = cv.TrackerGOTURN_create()
    """

    global stream

    predictor = object_detector(args.model, args.config)
    if (dev_mode):
        window_name = "Tracking in progress"
        cv.namedWindow(window_name, cv.WINDOW_NORMAL)
        cv.setWindowProperty(window_name, cv.WND_PROP_AUTOSIZE, cv.WINDOW_AUTOSIZE)        
        cv.moveWindow(window_name,10,10)
    time.sleep(1)

    if args.output:
        test_frame = stream
        height = test_frame.shape[0]
        width = test_frame.shape[1]
        fourcc = cv.VideoWriter_fourcc(*'XVID')
        #out = cv.VideoWriter(args.output,fourcc, 20.0, (640,480))
        out = cv.VideoWriter(args.output,fourcc, 20.0, (width,height))
        failTolerance = 0

    if args.classes:
        with open(args.classes, 'rt') as f:
            classes = f.read().rstrip('\n').split('\n')
    else:
        classes = list(np.arange(0,100))

    stream, objects_detected, objects_list, trackers_dict = intermediate_detections(stream, predictor, args.thr, classes)    

    while not rospy.is_shutdown():
    
        frame = stream

        timer = cv.getTickCount()

        """
        #Did not use OpenCV's multitracker because of the restrivtive nature of its Python counterpart.
        #If one tracker in the multitracker fails, there's no way to find out which tracker failed.
        #There's no easy way to delete individual trackers in the multitracker object.
        #Even when multitracker fails,  bboxes will have old values, but 'ok' will be false
        
        #if len(objects_list) > 0:
            #ok, bboxes = multi_tracker.update(frame)
        #bboxes = multi_tracker.getObjects()
        #ok = multi_tracker.empty()
        """
        
        print('Tracking - ',objects_list)

        if len(objects_detected) > 0:
            del_items = []
            for obj,tracker in trackers_dict.items():
                ok, bbox = tracker.update(frame)
                if ok:
                    objects_detected[obj][0] = bbox
                else:
                    print('Failed to track ', obj)
                    del_items.append(obj) 
        
            for item in del_items:            
                trackers_dict.pop(item)
                objects_detected.pop(item)
                
        fps = cv.getTickFrequency() / (cv.getTickCount() - timer)

        if len(objects_detected) > 0:
            if (dev_mode):
                # Display FPS on frame
                cv.putText(frame, "FPS : " + str(int(fps)), (100,50), cv.FONT_HERSHEY_SIMPLEX, 0.75, (50,170,50), 2)
                drawPred(frame, objects_detected)
            #ROS topic publisher
            info=list(objects_detected.values())
            box=info[0][0]
            coordinate=[int((box[0]+box[0] + box[2])/2),int((box[1]+box[1] + box[3])/2)]
            target_msg.data=coordinate
            track_pub.publish(target_msg)
        else:
            stream, objects_detected, objects_list, trackers_dict = intermediate_detections(stream, predictor, args.thr, classes)   
            #ROS topic publisher
            box=[-1,-1]
            target_msg.data=box
            track_pub.publish(target_msg)
            if (dev_mode):
                cv.putText(frame, 'Tracking Failure. Trying to detect more objects', (50,80), cv.FONT_HERSHEY_SIMPLEX, 0.75,(0,0,255),2)
        
        if (dev_mode):
            # Display result
            # If resolution is too big, resize the video
            if frame.shape[1] > 1240:
                cv.imshow(window_name, cv.resize(frame, (1240, 960)))
            else:
                cv.imshow(window_name, frame)
        
        #Write to output file
        if args.output:
            out.write(frame)
        k = cv.waitKey(1) & 0xff

        #Force detect new objects if 'q' is pressed
        if k == ord('q'):
            print('Refreshing. Detecting New objects')
            stream, objects_detected, objects_list, trackers_dict = intermediate_detections(stream, predictor, args.thr, classes)  
            if (dev_mode):
                cv.putText(frame, 'Refreshing. Detecting New objects', (100,80), cv.FONT_HERSHEY_SIMPLEX, 0.75,(0,0,255),2)
            
        # Exit if ESC pressed    
        if k == 27 : break 


    if args.output:
        out.release()
    # cv.destroyAllWindows()

def main():

    global target_msg, track_pub,bridge,stream,wait,dev_mode
    wait=0
    dev_mode=0
    target_msg=Int16MultiArray()
    bridge=CvBridge()
    rospy.init_node('tracker_pub', anonymous=True)
    track_pub = rospy.Publisher('/yolo_kcf/std_msgs', Int16MultiArray, queue_size=5)
    rospy.Subscriber("/usb_cam/image_raw", Image, imgCallback)

    parser = argparse.ArgumentParser(description='Object Detection and Tracking on Video Streams')
    parser.add_argument('--output', help='Path to save output as video file. If nothing is given, the output will not be saved.')

    # 测试视频必须使用绝对路径！
    # parser.add_argument('--input', default='/home/yif/视频/2.mp4')
    parser.add_argument('--model', default='/home/yif/catkin_ws/src/yolo_kcf/model_data/yolo-drone.weights')
    parser.add_argument('--config',default='/home/yif/catkin_ws/src/yolo_kcf/model_data/yolo-drone.cfg') 
    parser.add_argument('--classes', default='/home/yif/catkin_ws/src/yolo_kcf/model_data/coco_classes.txt')  
    parser.add_argument('--thr', type=float, default=0.2, help='Confidence threshold for detection')
    
    args = parser.parse_args()
    process(args)


if __name__ == '__main__':
    main()
