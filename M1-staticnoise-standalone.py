#!/usr/bin/env python
'''
//Computes Zero-Drift and Static-Noise error 
//test scenario:
//Robot is static for 30sec, moves in the space for 30sec, and returns to starting point,
//Robot repeats above scenario a few times, 
//calculation ignores first static and motion phases (considers it as initialization phase)
//Metric calculates drift between first and last pose in every movement phase
// and noise in every static phase
//expected arguments to be sent to service:
//- Tracking Module poses during test movement
//- Intervals array, which indicates indices of TM poses in beginning and end of movement phases
'''
import sys
import numpy as np
import common_utils as cu
from collections import defaultdict
import math
import os

def intervals_to_static_noise_array(interval_filename):
    intervals = cu.read_intervals(interval_filename)
    intervals.pop(0)
    intervals.pop(len(intervals)-1)
    return np.reshape(intervals,(-1,2))

def intervals_to_zero_drift_array(interval_filename):
    intervals = cu.read_intervals(interval_filename)
    intervals.pop(0)
    intervals.pop(0)
    return np.reshape(intervals,(-1,2))

def calc_metrics_static_noise( tm2_poses, tm2_static_intervals, metrics):
    for si in tm2_static_intervals:
        c_x = [] 
        c_y = []
        c_z = []
        c_dist = []
        c_rotation = [] 
        static_poses = np.array(tm2_poses[si[0]:si[1]])
        static_poses[:,1:4]*=1000
        translation_mm = static_poses[:,1:4]*1000
        print "Measuring static period from", static_poses[0][0], "to", static_poses[-1][0]
        print "Standard deviation (mm):", np.std(translation_mm, axis=0)
        reference_transformation = cu.average_transformation(static_poses[:, :])
        ref_inv = np.linalg.inv(reference_transformation)
        rows = np.size(static_poses, 0)
        for r in range(rows):
            err = np.dot(ref_inv, cu.pose_to_transform_matrix(static_poses[r,:]))
            T = err[:3,3]
            c_x.append(abs(T[0]))
            c_y.append(abs(T[1]))
            c_z.append(abs(T[2]))
            c_dist.append(np.linalg.norm(T))
            c_rotation.append(abs(cu.rotation_magnitude(err)))
        
        for state in ['mean','max','min','std']:
            for i in ['x','y','z','dist','rotation']:
                exec('metrics["' + i +'_'+ state +'"].append( np.' + state + '(c_' + i +'))') 
             

def calc_metrics_zero_drift( tm2_poses, tm_zero_drift_intervals,metrics):
    POSE_TESTED_RANGE =100
    c_x = [] 
    c_y = []
    c_z = []
    c_dist = []
    c_rotation = [] 
    for i, interval in enumerate(tm_zero_drift_intervals):
        tm_zdi = tm_zero_drift_intervals[i]
        #//calculate average mat of poses in interval range
        tm_poses_1 = np.array(tm2_poses[tm_zdi[0]-POSE_TESTED_RANGE + 1 :tm_zdi[0] + 1])
        tm_poses_2 = np.array(tm2_poses[tm_zdi[1]:tm_zdi[1]+POSE_TESTED_RANGE])
        tm_poses_1[:,1:4]*=1000
        tm_poses_2[:,1:4]*=1000

        tm_poses_avg1 = cu.average_transformation(tm_poses_1)
        tm_poses_avg2 = cu.average_transformation(tm_poses_2)
        
        tm_poses_avg1_inv = np.linalg.inv(tm_poses_avg1)
        err_mat = np.dot(tm_poses_avg1_inv,tm_poses_avg2)
        T = err_mat[:3,3]
        c_x.append(abs(T[0]))
        c_y.append(abs(T[1]))
        c_z.append(abs(T[2]))
        c_dist.append(np.linalg.norm(T)) #check with B.F
        c_rotation.append(abs(cu.rotation_magnitude(err_mat)))
    for state in ['mean','max','min','std']:
        for i in ['x','y','z','dist','rotation']:
            exec('metrics["zd_' + i +'_'+ state +'"].append( np.' + state + '(c_' + i +'))') 
        

def print_result(metrics):
    print(',Translation X Avg,'+ 'Translation X Max,' + 'Translation X Min,' + 'Translation X Std,'+
         'Translation Y Avg,'   + 'Translation Y Max,' + 'Translation Y Min,' + 'Translation Y Std,'+
         'Translation Z Avg,'   + 'Translation Z Max,' + 'Translation Z Min,' + 'Translation Z Std,'+
         'Rotation Avg,' + 'Rotation Max,'+ 'Rotation Min,' + 'Rotation Std,'+
         'Translation Dist Avg,'+'Translation Dist Max,'+'Translation Dist Min,'+'Translation Dist Std')
    
    print 'Zero Drift,',
    for axis in ['zd_x','zd_y','zd_z','zd_rotation','zd_dist']:
        for state in ['mean','max','min','std']:
            print '%.6f, ' % np.mean(metrics[axis + '_' + state]),
    print '\n'
    
    print 'Static Noise (avg of phases),',
    for axis in ['x','y','z','rotation','dist']:
        for state in ['mean','max','min','std']:
            print '%.4f,' % np.mean(metrics[axis + '_' + state]),
    print '\n'
    for i in range(0,5):
        print 'Static Noise (phase #%d),' %i,
        for axis in ['x','y','z','rotation','dist']:
            for state in ['mean','max','min','std']:
                print '%.4f,' % metrics[axis + '_' + state][i],
        print '\n'

def save_result(results_filename,metrics):
    result_file = open(results_filename, 'w')
    result_file.write(',Translation X Avg,'+ 'Translation X Max,' + 'Translation X Min,' + 'Translation X Std,'+
         'Translation Y Avg,'   + 'Translation Y Max,' + 'Translation Y Min,' + 'Translation Y Std,'+
         'Translation Z Avg,'   + 'Translation Z Max,' + 'Translation Z Min,' + 'Translation Z Std,'+
         'Rotation Avg,' + 'Rotation Max,'+ 'Rotation Min,' + 'Rotation Std,'+
         'Translation Dist Avg,'+'Translation Dist Max,'+'Translation Dist Min,'+'Translation Dist Std \n')
    
    result_file.write( 'Zero Drift,')
    for axis in ['zd_x','zd_y','zd_z','zd_rotation','zd_dist']:
        for state in ['mean','max','min','std']:
            result_file.write('%.6f, ' % np.mean(metrics[axis + '_' + state]))
    result_file.write('\n')
    
    result_file.write( 'Static Noise (avg of phases),')
    for axis in ['x','y','z','rotation','dist']:
        for state in ['mean','max','min','std']:
            result_file.write('%.6f, ' % np.mean(metrics[axis + '_' + state]))
    result_file.write('\n')
    
    for i in range(0,5):
        result_file.write( 'Static Noise (phase #%d),' %i)
        for axis in ['x','y','z','rotation','dist']:
            for state in ['mean','max','min','std']:
                result_file.write('%.6f, ' % metrics[axis + '_' + state][i])
        result_file.write('\n')
    result_file.close() 
    print "\n Relust saved to ", results_filename

def write_kpi(fp, metrics):
    for axis in ['rotation', 'dist']:
        for state in ['mean', 'max']:
            #fp.write('KPI: %s: %f\n' % ('static_noise: ' + axis + '-' + state, np.mean(metrics[axis + '_' + state])))
            str1 = 'static_noise: ' + axis + '-' + state
            str2 = axis + '_' + state
            fp.write('KPI: %s: %f\n' % ('static_noise: ' + axis + '-' + state, np.mean(metrics[axis + '_' + state])))
    fp.write('KPI: zero_drift: translation dist error(avg): %f \n' % np.mean(metrics['zd_dist_mean']))
    for axis in ['x', 'y', 'z', 'rotation']: 
        for state in ['mean', 'max']:
            fp.write('KPI: zero_drift %s %s: %f\n' % (axis, state, np.mean(metrics['zd_' + axis + '_' + state])))

def print_kpi(metrics):
    write_kpi(sys.stdout, metrics)

def main(poses_filename, intervals_filename, results_filename):
    metrics = defaultdict(list)
    tm2_poses = cu.tum_to_pose_array(poses_filename)
    tm2_static_intervals = intervals_to_static_noise_array(intervals_filename)
    tm_zero_drift_intervals = intervals_to_zero_drift_array(intervals_filename)
    
    if cu.check_input(tm2_poses,tm2_static_intervals) == 0:
        sys.exit(1)

    calc_metrics_static_noise( tm2_poses , tm2_static_intervals, metrics)
    calc_metrics_zero_drift( tm2_poses, tm_zero_drift_intervals, metrics)
    
    print_kpi(metrics)
    fp = open(results_filename + '.kpi', 'wt')
    write_kpi(fp, metrics)
    fp.close()
    print_result(metrics)
    save_result(results_filename,metrics)

if __name__ == '__main__':
    if len(sys.argv) != 4 and len(sys.argv) != 1:
        print 'usage: %s <poses file> <intervals file> <results filename>' % sys.argv[0]
        sys.exit(1)
    elif len(sys.argv) == 1:
        print 'No input filenames, using test data'
        dirpath = os.getcwd()
        poses_filename = os.path.join(dirpath,'sample_data', 'HMD_20180123_151226', 'ZDrift_SNoise', 'tmPoses', 'HMD_TM_ZDrift_SNoise_20180123_150925.txt')
        intervals_filename = os.path.join(dirpath,'sample_data', 'HMD_20180123_151226', 'ZDrift_SNoise', 'intervals', 'HMD_Intervals_ZDrift_SNoise_20180123_150925.txt')
        results_filename = os.path.join(dirpath,'zero_drift_static_noise_output.csv')
    else:
        poses_filename = sys.argv[1]
        intervals_filename = sys.argv[2]
        results_filename = sys.argv[3]
    main(poses_filename, intervals_filename, results_filename)
