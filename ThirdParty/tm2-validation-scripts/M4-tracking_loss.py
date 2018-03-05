#!/usr/bin/env python
'''//Computes number of frames which lost tracking 
//Test scenario:
//robot moves in a circular path
//poses being sent to metric are all during robot move
//assumptions:
//robot speed is in units of mm per sec
//tm pose timestamps are in nano seconds
'''
import sys
import numpy as np
import common_utils as cu
from collections import defaultdict
import math
import os

def print_result(metrics,rounds_intervls):
    print(',Number Of Tracking Loss,' +	'Tracking Loss Ratio	Average,' + 'Translation Error,')
    print 'Tracking_Loss (avg),',
    for state in ['rounds_num_loss','rounds_loss_ratio','rounds_avg_err']:
            print '%.4f,' % np.mean(metrics[state]),
    print '\n'
    for i, tm_ri in enumerate(rounds_intervls):
        print 'Tracking_Loss (phase #%d),' %i,
        for state in ['rounds_num_loss','rounds_loss_ratio','rounds_avg_err']:
            print '%.4f,' % metrics[state][i],
        print '\n'

def save_result(results_filename,metrics):
    result_file = open(results_filename, 'w')
    result_file.write(',Number Of Tracking Loss,' +	'Tracking Loss Ratio	Average,' + 'Translation Error,')
    print 'Tracking_Loss (avg),',
    for state in ['rounds_num_loss','rounds_loss_ratio','rounds_avg_err']:
        result_file.write('%.6f, ' % np.mean(metrics[state]))
    result_file.write('\n')
    for i, tm_ri in enumerate(rounds_intervls):
        print 'Tracking_Loss (phase #%d),' %i,
        for state in ['rounds_num_loss','rounds_loss_ratio','rounds_avg_err']:
            result_file.write('%.6f, ' % metrics[state][i])
        result_file.write('\n')
            
    result_file.close() 
    print "\n Relust saved to ", results_filename

def calc_metrics_tracking_loss(radius, rounds_intervls,tm2_poses,metrics):
    POSE_TESTED_RANGE = 100
    ALLOWED_DIST_ERROR = 100.0 #mm 
    for i, tm_ri in enumerate(rounds_intervls):      
        #//calculate average of errors between current pose and 100 first poses
        round_size =  tm_ri[1] - tm_ri[0] 
        num_loss = 0.0 
        err_sum = 0.0
        rounds_avg_err = 0.0
        rounds_loss_ratio = 0.0
        ## base vector = first 100 poses:
             #base_vector = np.array(tm2_poses[rounds_intervls[0][0] : rounds_intervls[0][0] + POSE_TESTED_RANGE ])[:,1:4]
        ## base vector = first 100 poses mean
            #base_vector = np.ones((POSE_TESTED_RANGE,3)) * np.mean(np.array(tm2_poses[rounds_intervls[0][0] - POSE_TESTED_RANGE : rounds_intervls[0][0] ])[:,1:4], axis = 0)
        ## base vector = duplicate first_point
        base_vector = np.ones((POSE_TESTED_RANGE,3)) * np.array(tm2_poses[rounds_intervls[0][0]])[1:4]

        for j in range (tm_ri[0],tm_ri[1]):
            #//calculate angle of arch between current pose and begin of round
            beta = 360.0  *(j - tm_ri[0] - POSE_TESTED_RANGE + np.array((range(0,POSE_TESTED_RANGE))))  / round_size
            #//calculate length of chord between current pose and begin of round
            chord_dist = 2 * radius * abs(np.sin((beta / 2) * math.pi / 180))
            #//calculate tm distance between current pose and first pose 
            
            tm_translation_vector = np.array(tm2_poses[ j - POSE_TESTED_RANGE  : j  ])[:,1:4] 
            if len(tm_translation_vector) != len(base_vector):
                print "problem with size"
            tm_translation_vector = tm_translation_vector - base_vector
            tm_translation_vector*=1000
            tm_dist = np.linalg.norm(tm_translation_vector, axis =1 )
            #//mean tm translation error
            if len(tm_dist) != len(chord_dist):
                print "problem with size"
                
            cur_pose_err = np.mean(abs(tm_dist - chord_dist))
            
            if cur_pose_err > ALLOWED_DIST_ERROR:
                num_loss+=1
                err_sum += cur_pose_err            
        if (num_loss != 0):
            rounds_avg_err = err_sum / num_loss
        rounds_loss_ratio = num_loss / round_size 
        metrics["rounds_num_loss"].append(num_loss)
        metrics["rounds_loss_ratio"].append(rounds_loss_ratio)
        metrics["rounds_avg_err"].append(rounds_avg_err)
    
def split_to_rounds(radius,robot_speed,tm2_poses):
    #//time to complete a round (nano-seconds) is the circumference of a round (mm) divided by robot speed * 10^9 
    starting_point = 100 
    rounds_intervls = [starting_point]
    time_array =  np.array(tm2_poses)[:,0]
    round_duration = (2 * radius * math.pi) / robot_speed * 1000000000 
    for i in range(0, int((time_array.max()-time_array.min() - starting_point )/round_duration)):
        wish_timestamp = time_array[0] + (i+1) * round_duration
        nearest_idx = np.where(abs(time_array - wish_timestamp )==abs(time_array-wish_timestamp).min())[0] # index of the element of nearest  etimestamp to the the wish timtemp
        rounds_intervls.append(starting_point + nearest_idx[0]-1)
        rounds_intervls.append(starting_point + nearest_idx[0] )
    rounds_intervls.pop(len(rounds_intervls)-1)
    return np.reshape(rounds_intervls,(-1,2))


def main(poses_filename, results_filename):
    radius = 200 
    robot_speed =117.333
    metrics = defaultdict(list)
    #//split poses indices by rounds
    tm2_poses = cu.tum_to_pose_array(poses_filename)
    #//split poses to rounds
    rounds_intervls = split_to_rounds(radius,robot_speed,tm2_poses)
    if cu.check_input(tm2_poses,rounds_intervls) == 0:
        sys.exit(1)
    calc_metrics_tracking_loss(radius, rounds_intervls,tm2_poses,metrics)
    print_result(metrics,rounds_intervls)
    #save_result(results_filename,metrics)

if __name__ == '__main__':
    if len(sys.argv) != 3 and len(sys.argv) != 1:
        print 'usage: %s <poses file> <intervals file> <results filename>' % sys.argv[0]
        sys.exit(1)
    elif len(sys.argv) == 1:
        print 'No input filenames, using test data'
        dirpath = os.getcwd()
        poses_filename = os.path.join(dirpath,'sample_data', 'HMD_20180123_151226', 'Tracking_Loss', 'tmPoses', 'Controller0_TM_Tracking_Loss_20171109_113415.txt')
        results_filename = os.path.join(dirpath,'tracking-loss-output.csv')
    else:
        poses_filename = sys.argv[1]
        results_filename = sys.argv[2]
    main(poses_filename,results_filename)
