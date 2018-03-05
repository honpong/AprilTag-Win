#!/usr/bin/env python
'''
//Computes translation error of a VR translation movement test
//VR Translation test scenario:
//Robot moves in a square-like motions, in one of the planes x, y or z. 
//expected arguments to be sent to service:
//- Tracking Module poses during test movement
//- Intervals array, which indicates indices of TM poses corresponding to robot poses
//- Robot poses at begin and end of every 'edge' of square
//- Axes list of squares edges in which the robot moved (valid values are X, Y or Z)
'''
import sys
import numpy as np
import common_utils as cu
from collections import defaultdict
import math
import os

def intervals_to_array(interval_filename):
    intervals = cu.read_intervals(interval_filename)
    return np.reshape(intervals,(-1,2))

def movement_axes_to_list(robot_movement_axes_filename):
    with open(robot_movement_axes_filename) as f:
        movement_axes = f.read().splitlines()[0]
    return movement_axes.replace(',','')

def sort_moves_by_axes(tm2_static_intervals,tm2_poses,robot_movement_axes,robot_poses):
    POSE_TESTED_RANGE =100
    metrics = defaultdict(list)
    for i, interval in enumerate(tm2_static_intervals):
        tm_si = tm2_static_intervals[i]       
        #//calculate average mat of poses in interval range
        tm_poses_avg1 = cu.average_transformation(np.array(tm2_poses[tm_si[0]:tm_si[0]+POSE_TESTED_RANGE]))
        tm_poses_avg2 = cu.average_transformation(np.array(tm2_poses[tm_si[1]:tm_si[1]+POSE_TESTED_RANGE]))
        tm_poses_avg1_inv = np.linalg.inv(tm_poses_avg1)
        tm_translation = np.dot(tm_poses_avg1_inv,tm_poses_avg2)
        tm2_norm = cu.norm_transformation(tm_translation )

        robot_poses_1 = cu.vector_to_transform_matrix(robot_poses[2*i])
        robot_poses_2 = cu.vector_to_transform_matrix(robot_poses[2*i +1])
        robot_poses_1_inv = np.linalg.inv(robot_poses_1)
        robot_translation = np.dot(robot_poses_1_inv,robot_poses_2)
        robot_norm = cu.norm_transformation(robot_translation ) 
        
        #//calculate scale factor by proportion between TM translation and robot translation
        scale= tm2_norm / robot_norm
        metrics["scale"].append(scale)      
        for state in ['scale','tm2_norm','robot_norm']:
           axis=robot_movement_axes[i]  
           exec('metrics["' + state + '_' + axis + '"].append(' + state + ')') 
            
    return metrics  

def calc_axis_trans_err_ratio(metrics):
    scale_factor=np.mean(np.array(metrics["scale"]))
    arc_factor = 0.25*math.pi*math.sqrt(2)
    #//calculate average translation error of TM axis movements
    Translation_Error_x = abs(scale_factor* np.array(metrics["robot_norm_x"]) - np.array(metrics["tm2_norm_x"]))
    Translation_Error_y = abs(scale_factor* np.array(metrics["robot_norm_y"]) - np.array(metrics["tm2_norm_y"]))
    Translation_Error_z = abs(scale_factor* np.array(metrics["robot_norm_z"]) - np.array(metrics["tm2_norm_z"]))
    #//calculate translation error ratio by proportion between TM avg translation error and robot avg translation error
    metrics["translation_Error_Ratio_x"] = np.mean(np.mean(Translation_Error_x) / (arc_factor * scale_factor * np.mean(np.array(metrics["robot_norm_x"]))))
    metrics["translation_Error_Ratio_y"] = np.mean(np.mean(Translation_Error_y) / (arc_factor * scale_factor * np.mean(np.array(metrics["robot_norm_y"]))))
    metrics["translation_Error_Ratio_z"] = np.mean(np.mean(Translation_Error_z) / (arc_factor * scale_factor * np.mean(np.array(metrics["robot_norm_z"]))))
    return metrics
    
def print_result( metrics):
    print "Scale Factor ", np.mean(np.array(metrics["scale"]))
    print "X Scale Factor ", np.mean(np.array(metrics["scale_x"]))
    print "Y Scale Factor ", np.mean(np.array(metrics["scale_y"]))
    print "Z Scale Factor", np.mean(np.array(metrics["scale_z"]))
    print "Translation Error Ratio X", metrics["translation_Error_Ratio_x"]
    print "Translation Error Ratio Y", metrics["translation_Error_Ratio_y"]
    print "Translation Error Ratio Z", metrics["translation_Error_Ratio_z"]
    print "Average Translation Error Ratio", (metrics["translation_Error_Ratio_x"] +  metrics["translation_Error_Ratio_y"] + metrics["translation_Error_Ratio_z"]) /3
    print "Average Translation Scale Error",1 - np.mean(np.array(metrics["scale"]))


def write_kpi(fp, metrics):
    fp.write("KPI: Average Translation Error Ratio : %f\n" % ((metrics["translation_Error_Ratio_x"] +  metrics["translation_Error_Ratio_y"] + metrics["translation_Error_Ratio_z"]) /3))
    fp.write("KPI: Average Translation Scale Error : %f\n" % (1 - np.mean(np.array(metrics["scale"]))))

def print_kpi(metrics):
    write_kpi(sys.stdout, metrics)

def save_result(results_filename,metrics):
    result_file = open(results_filename, 'w')
    result_file.write(',Scale Factor,X Scale Factor,'+
                      'Y Scale Factor,'+
                      'Z Scale Factor,'+
                      'Translation Error Ratio X,'+
                      'Translation Error Ratio Y,'+
                      'Translation Error Ratio Z,'+
                      'Average Translation Error Ratio,'+
                      'Average Translation Scale Error'+'\n')
    
    result_file.write(','+ str(np.mean(np.array(metrics["scale"]))) +','+
                      str(np.mean(np.array(metrics["scale_x"]))) +','+                       
                      str(np.mean(np.array(metrics["scale_y"]))) +','+
                      str(np.mean(np.array(metrics["scale_z"]))) +','+
                      str(metrics["translation_Error_Ratio_x"]) +','+
                      str(metrics["translation_Error_Ratio_y"]) +','+
                      str(metrics["translation_Error_Ratio_z"]) +','+
                      str((metrics["translation_Error_Ratio_x"] + metrics["translation_Error_Ratio_y"] + metrics["translation_Error_Ratio_z"]) /3) +','+
                      str(abs(1 - np.mean(np.array(metrics["scale"])))))
    result_file.close()
    print "\n Relust saved to ", results_filename

def main(tm2_pose_filename, tm2_interval_filename,robot_movement_axes_filename,robot_pose_filename, results_filename):

    tm2_poses = cu.tum_to_pose_array(tm2_pose_filename)
    tm2_static_intervals = intervals_to_array(tm2_interval_filename)
    robot_poses = cu.robot_pose_to_array(robot_pose_filename) #only pn cornners
    robot_movement_axes = movement_axes_to_list(robot_movement_axes_filename) 
    
    if cu.check_input(tm2_poses,tm2_static_intervals) == 0:
        sys.exit(1)

    metrics = sort_moves_by_axes(tm2_static_intervals,tm2_poses,robot_movement_axes,robot_poses)
    metrics = calc_axis_trans_err_ratio(metrics)
    print_result(metrics)
    save_result(results_filename,metrics)
    print_kpi(metrics)
    kpifp = open(results_filename + '.kpi', 'wt')
    write_kpi(kpifp, metrics)
    kpifp.close()
    

if __name__ == '__main__':
    if len(sys.argv) != 6 and len(sys.argv) != 1:
        print 'usage: %s <poses file> <intervals file> <robot_movement_axes_file> <robot_pose_file> <results filename>' % sys.argv[0]
        sys.exit(1)
    elif len(sys.argv) == 1:
        print 'No input filenames, using test data'
        dirpath = os.getcwd()
        tm2_pose_filename            = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Translation_FF', 'tmPoses', 'HMD_TM_Translation_FF_20180123_151224.txt')
        tm2_interval_filename        = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Translation_FF', 'intervals', 'HMD_Intervals_Translation_FF_20180123_151224.txt') 
        robot_movement_axes_filename = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Translation_FF', 'robotPoses', 'HMD_Robot_Axes_Translation_FF_20180123_151224.txt')
        robot_pose_filename          = os.path.join(dirpath, 'sample_data', 'HMD_20180123_151226', 'Translation_FF', 'robotPoses', 'HMD_Robot_Translation_FF_20180123_151224.txt') 
        results_filename             = os.path.join(dirpath, 'VR_translation_output.csv')       
                
    else:
        tm2_pose_filename = sys.argv[1]
        tm2_interval_filename = sys.argv[2]
        robot_movement_axes_filename = sys.argv[3]
        robot_pose_filename = sys.argv[4]
        results_filename = sys.argv[5]
    main(tm2_pose_filename, tm2_interval_filename,robot_movement_axes_filename,robot_pose_filename, results_filename)
    
