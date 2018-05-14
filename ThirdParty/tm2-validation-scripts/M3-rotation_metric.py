#!/usr/bin/env python
'''
//Computes rotation error of a rotation test 
//Rotation test scenario:
//Robot performs a few phases of rotation moves, in one of the axes yaw, pitch or roll.
//expected arguments to be sent to service:
//- Tracking Module poses during test movement
//- Intervals array, which indicates indices of TM poses corresponding to robot poses
//- Robot poses at begin and end of every rotated motion
//- Axis in which the robot moved (valid values are YAW, PITCH or ROLL)
'''
import sys
import numpy as np
import common_utils as cu
from collections import defaultdict
import os

def intervals_to_array(interval_filename):
    intervals = cu.read_intervals(interval_filename)
    return np.reshape(intervals,(-1,2))

def save_result(results_filename,metrics):
    result_file = open(results_filename, 'w')
    result_file.write(',TM Total Rotation,'+
           'Robot Total Rotation,'+
           'Rotation Error,'+
           'Scale Factor,'+
           'Scale Error' +'\n')
    result_file.write('Rotation_pitch (Avg),'+
                      str(np.mean(np.array(metrics["tm_rotation_magintude"]))) +','+
                      str(np.mean(np.array(metrics["robot_translation_magintude"]))) +','+
                      str(np.mean(np.array(metrics["rotation_error"]))) +','+
                      str(np.mean(np.array(metrics["rotation_scale_factor"]))) +','+
                      str(np.mean(abs(1-np.array(metrics["rotation_scale_factor"])))) +'\n')
    for i in range (0,2):
       result_file.write('Rotation_pitch (intrval ' + str(i) + '),' + 
                         str(np.array(metrics["tm_rotation_magintude"][i])) +','+
                         str(np.array(metrics["robot_translation_magintude"][i])) +','+
                         str(np.array(metrics["rotation_error"][i])) +','+
                         str(np.array(metrics["rotation_scale_factor"][i])) +','+
                         str(abs(1- np.array(metrics["rotation_scale_factor"][i]))) +'\n') 
       
    result_file.close()
    print "\n Relust saved to ", results_filename
   
def print_result(metrics):
    print (',TM Total Rotation,'+
           'Robot Total Rotation,'+
           'Rotation Error,'+
           'Scale Factor,'+
           'Scale Error' +'\n')
    print ('Rotation_pitch (Avg),'+
          str(np.mean(np.array(metrics["tm_rotation_magintude"]))) +','+
          str(np.mean(np.array(metrics["robot_translation_magintude"]))) +','+
          str(np.mean(np.array(metrics["rotation_error"]))) +','+
          str(np.mean(np.array(metrics["rotation_scale_factor"]))) +','+
          str(1- np.mean(abs(np.array(metrics["rotation_scale_factor"])))) +'\n')
    for i in range (0,2):
        print ('Rotation_pitch (intrval ' + str(i) + '),' + 
                str(np.array(metrics["tm_rotation_magintude"][i])) +','+
                str(np.array(metrics["robot_translation_magintude"][i])) +','+
                str(np.array(metrics["rotation_error"][i])) +','+
                str(np.array(metrics["rotation_scale_factor"][i])) +','+
                str(abs(1- np.array(metrics["rotation_scale_factor"][i]))) +'\n')      
    

def rotation_metric(tm2_static_intervals,tm2_poses,robot_poses):
    POSE_TESTED_RANGE =100
    metrics = defaultdict(list)
    for i, interval in enumerate(tm2_static_intervals):
        tm_si = tm2_static_intervals[i]       
        #//calculate average mat of poses in interval range
        tm_poses_avg1 = cu.average_transformation(np.array(tm2_poses[tm_si[0]:tm_si[0]+POSE_TESTED_RANGE]))
        tm_poses_avg2 = cu.average_transformation(np.array(tm2_poses[tm_si[1]:tm_si[1]+POSE_TESTED_RANGE]))
        tm_poses_avg1_inv = np.linalg.inv(tm_poses_avg1)
        tm_translation = np.dot(tm_poses_avg1_inv,tm_poses_avg2)
        tm_rotation_magintude = cu.rotation_magnitude(tm_translation)

        #robot_poses_1 = cu.vector_to_transform_matrix(cu.vector_degree_to_rad(robot_poses[2*i]))
        #robot_poses_2 = cu.vector_to_transform_matrix(cu.vector_degree_to_rad(robot_poses[2*i +1]))
        robot_poses_1 = cu.euler_to_transform_matrix(cu.vector_degree_to_rad(robot_poses[2*i]))
        robot_poses_2 = cu.euler_to_transform_matrix(cu.vector_degree_to_rad(robot_poses[2*i +1]))
        
        robot_poses_1_inv = np.linalg.inv(robot_poses_1)
        robot_translation = np.dot(robot_poses_1_inv,robot_poses_2)
        robot_translation_magintude = cu.rotation_magnitude(robot_translation)
        print tm_rotation_magintude , robot_translation_magintude
        
        metrics["tm_rotation_magintude"].append(tm_rotation_magintude)
        metrics["robot_translation_magintude"].append(robot_translation_magintude)
        metrics["rotation_error"].append(abs(tm_rotation_magintude - robot_translation_magintude))
        metrics["rotation_scale_factor"].append( tm_rotation_magintude/robot_translation_magintude)
    return metrics
        
def main(tm2_pose_filename, tm2_interval_filename,robot_pose_filename, results_filename):
    tm2_poses = cu.tum_to_pose_array(tm2_pose_filename)
    tm2_static_intervals = intervals_to_array(tm2_interval_filename)
    robot_poses = cu.robot_pose_to_array(robot_pose_filename) #only pn cornners

    if cu.check_input(tm2_poses,tm2_static_intervals) == 0:
        sys.exit(1)

    metrics= rotation_metric(tm2_static_intervals,tm2_poses,robot_poses)
    print_result(metrics)
    save_result(results_filename,metrics)
    # No KPI's for now, just create empty .kpi file
    open(results_filename + '.kpi', 'wt').close()
    
if __name__ == '__main__':
    if len(sys.argv) != 5 and len(sys.argv) != 1:
        print 'usage: %s <poses file> <intervals file> <robot_pose_file> <results filename>' % sys.argv[0]
        sys.exit(1)
    elif len(sys.argv) == 1:
        print 'No input filenames, using test data'
        dirpath = os.getcwd()
        tm2_pose_filename     = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Rotation_PITCH', 'tmPoses', 'HMD_TM_Rotation_pitch_20180123_151032.txt')
        tm2_interval_filename = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Rotation_PITCH', 'intervals', 'HMD_Intervals_Rotation_pitch_20180123_151032.txt')
        robot_pose_filename   = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Rotation_PITCH', 'robotPoses', 'HMD_Robot_Rotation_pitch_20180123_151032.txt')
        results_filename      = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Rotation_PITCH', 'Rotation_PITCH_results.csv')
                
    else:
        tm2_pose_filename = sys.argv[1]
        tm2_interval_filename = sys.argv[2]
        robot_pose_filename = sys.argv[3]
        results_filename = sys.argv[4]
    main(tm2_pose_filename, tm2_interval_filename,robot_pose_filename, results_filename)
 


