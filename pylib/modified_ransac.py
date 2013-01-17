# Created by Jordan Miller
# Copyright (c) 2013. RealityCap, Inc.
# All Rights Reserved.


import numpy

"""

    This is an implemtation of a variation of the RANSAC algorithm. It mirrors the wikipedia psudocode except in the final
    selction process, where instead of choosing the model w/ the best fit to inliers from amongst those with a qualifying number of 
    inliers, it instead chooses the model with the most inliers. 
    
    it is a simpler implementation with different convergence. it does not take the paramater d (threshold inliers)
    because it alwas returns the max, so can be thresholded externally. 

    from: http://en.wikipedia.org/wiki/RANSAC
    
    input:
        data - a set of observations
        model - a model that can be fitted to data
        n - the minimum number of data required to fit the model
        k - the number of iterations performed by the algorithm
        t - a threshold value for determining when a datum fits a model
    
    output:
        best_model - model parameters which best fit the data (or nil if no good model is found)
        best_consensus_set - data points from which this model has been estimated
        best_error - the error of this model relative to the data
    
    iterations := 0
    best_model := nil
    best_consensus_set := nil
    most_inliers := 0
    
    while iterations < k
        maybe_inliers := n randomly selected values from data
        maybe_model := model parameters fitted to maybe_inliers
        maybe_model_fit := model error accross all data
    
        for every point in data not in maybe_inliers
            if point fits maybe_model with an error smaller than t
                add point to consensus_set
    
        if the number of elements in consensus_set is > most_inliers
            (we have found a model which is better than any of the previous ones,
            keep it until a better one is found)
            best_model := this_model
            best_consensus_set := consensus_set
            most_inliers := this_model_inliers
    
    increment iterations
    
    return best_model, best_consensus_set, best_error

    
"""

def random_partition(n,n_data):
    """return n random rows of data (and also the other len(data)-n rows)"""
    all_idxs = numpy.arange( n_data )
    numpy.random.shuffle(all_idxs)
    idxs1 = all_idxs[:n]
    idxs2 = all_idxs[n:]
    return idxs1, idxs2

def modified_ransac(data,model,n,k,t,debug=False):
    
    iterations = 0
    most_inliers = 0
    best_inlier_idxs = None
    best_fit = None
    
    while iterations < k:
        maybe_inlier_idxs, remainder_idxs = random_partition(n,data.shape[0])
        maybe_inliers = data[maybe_inlier_idxs,:]
        remainder_points = data[remainder_idxs]
        maybe_model = model.fit(maybe_inliers)
        test_err = model.get_error( remainder_points, maybe_model)
        
        # for every point in data not in maybe_inliers, if point fits maybe_model with an error smaller than t, add point to consensus_set
        also_inlier_idxs = remainder_idxs[test_err < t]
        also_inliers = data[also_inlier_idxs,:]

        if debug:
            print 'test_err.min()',test_err.min()
            print 'test_err.max()',test_err.max()
            print 'numpy.mean(test_err)',numpy.mean(test_err)
            print 'iteration %d:len(also_inliers) = %d'%(iterations,len(also_inliers))
        
        if len(also_inliers) > most_inliers:
            most_inliers = len(also_inliers)
            best_fit = maybe_model
            best_inlier_idxs = numpy.concatenate( (maybe_inlier_idxs, also_inlier_idxs) )

        iterations+=1
    
    if best_fit is None:
        raise ValueError("did not meet fit acceptance criteria")
    else:
        return best_fit, {'inliers':best_inlier_idxs}


