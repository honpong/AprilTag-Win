///
/// @file stats.h
/// 
/// Calculates a set of running statistics for some incomming data points
/// (e.g. float_stats or double_stats). Storage space is constant.
///

#ifndef STATS_H
#define STATS_H

#ifndef INLINE
#	ifdef _WIN32
#		define INLINE __forceinline
#	else 
#		define INLINE inline
#	endif
#endif

template <typename stat_type> class stats;
typedef class stats<double> double_stats;
typedef class stats<float>  float_stats;

template <typename stat_type> class stats
{
	stat_type sd;
public:
	stat_type mean, total;
	size_t count;

	// Constructor
	stats() 
	{
		reset();
	}

	// Calculate the running stats for all samples since last reset
	INLINE stat_type variance() 
	{ 
		return ( count > 1 ) ?
			sd / ( count - 1 ) : 0.0f; 
	}

	INLINE stat_type std_dev() 
	{ 
		return ( count > 1 ) ?
			sqrt(sd / ( count - 1 )) : 0.0f;  
	}

	INLINE stat_type coff_var() 
	{ 
		return ( count > 1 && mean > 0.0f ) ?
			sqrt(sd / ( count - 1 )) / mean : 0.0f; 
	}

	INLINE void reset() 
	{ 
		count = 0; 
		total = sd = mean = 0.0f;
	}

	INLINE void add_sample( stat_type sample )
	{
		total += sample;
		mean = count++ ? mean : sample;
		const stat_type mean_old = mean;
		mean = mean + ( sample - mean ) / count;
		sd += ( sample - mean_old ) * ( sample - mean );
	}

	INLINE void incorporate( class stats<stat_type>* pStats )
	{
		const size_t COUNT = count + pStats->count;
		mean = COUNT ? ( mean * count + pStats->mean * pStats->count ) / COUNT : mean;
		count += pStats->count;
		total += pStats->total;
	}
};

#endif
