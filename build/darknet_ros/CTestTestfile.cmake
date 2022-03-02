# CMake generated Testfile for 
# Source directory: /home/yif/catkin_ws/src/darknet_ros/darknet_ros
# Build directory: /home/yif/catkin_ws/build/darknet_ros
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("_ctest_darknet_ros_rostest_test_object_detection.test" "/home/yif/catkin_ws/build/darknet_ros/catkin_generated/env_cached.sh" "/usr/bin/python2" "/opt/ros/melodic/share/catkin/cmake/test/run_tests.py" "/home/yif/catkin_ws/build/darknet_ros/test_results/darknet_ros/rostest-test_object_detection.xml" "--return-code" "/usr/bin/python2 /opt/ros/melodic/share/rostest/cmake/../../../bin/rostest --pkgdir=/home/yif/catkin_ws/src/darknet_ros/darknet_ros --package=darknet_ros --results-filename test_object_detection.xml --results-base-dir \"/home/yif/catkin_ws/build/darknet_ros/test_results\" /home/yif/catkin_ws/src/darknet_ros/darknet_ros/test/object_detection.test ")
set_tests_properties("_ctest_darknet_ros_rostest_test_object_detection.test" PROPERTIES  _BACKTRACE_TRIPLES "/opt/ros/melodic/share/catkin/cmake/test/tests.cmake;160;add_test;/opt/ros/melodic/share/rostest/cmake/rostest-extras.cmake;52;catkin_run_tests_target;/opt/ros/melodic/share/rostest/cmake/rostest-extras.cmake;80;add_rostest;/opt/ros/melodic/share/rostest/cmake/rostest-extras.cmake;100;_add_rostest_google_test;/home/yif/catkin_ws/src/darknet_ros/darknet_ros/CMakeLists.txt;242;add_rostest_gtest;/home/yif/catkin_ws/src/darknet_ros/darknet_ros/CMakeLists.txt;0;")
subdirs("gtest")
