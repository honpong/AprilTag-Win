# TM2_Validation_Scripts
include python scripts with quality metrics  

## Building

set intel http proxy server to git: 
```sh
	'git config --global http.proxy http://proxy-chain.intel.com:911'
	'git config --global https.proxy https://proxy-chain.intel.com:912'
```	
also reqaired python 2 


### Scenarios:

	M1- Zero drift Static noise:  
		• Translation error 
			• Avrage  (x, y, z, general)
			• Max (x, y, z, general)
			• Min(x, y, z, general)
			• Std (x, y, z, general)
		• Rotation error magnitude
			• Avrage  (general)
			• Max (general)
			• Min(general)
			• Std (general)

	M2- VR- translation:
		• Scale factor (x, y, z, general)
		• Translation error ratio (x,y,z,avrage )
		• Scale error  (general ) 

	M3- Rotation yow | pitch | rool: 
		• Rotation error
		• Scale factor
		• Scale error  

	
	

### Running 
example data can be found in \\metrics-server\LiveTestsData\tm2-ci-8\HMD_20180123_151226



To run M1-ZDrift_SNoise:

```sh
python M1-ZDrift_SNoise.py ^
sample_data\HMD_20180123_151226\ZDrift_SNoise\tmPoses\HMD_TM_ZDrift_SNoise_20180123_150925.txt ^
sample_data\HMD_20180123_151226\ZDrift_SNoise\intervals\HMD_Intervals_ZDrift_SNoise_20180123_150925.txt ^
zero_drift_static_noise_output.txt
```

To run M2- VR-translation:
	
```sh
python M2-VR_translation.py ^
sample_data\HMD_20180123_151226\Translation_FF\tmPoses\HMD_TM_Translation_FF_20180123_151224.txt ^
sample_data\HMD_20180123_151226\Translation_FF\intervals\HMD_Intervals_Translation_FF_20180123_151224.txt ^
sample_data\HMD_20180123_151226\Translation_FF\robotPoses\HMD_Robot_Axes_Translation_FF_20180123_151224.txt ^
sample_data\HMD_20180123_151226\Translation_FF\robotPoses\HMD_Robot_Translation_FF_20180123_151224.txt ^
VR_translation_output.txt
```

To run M3- rotation_metric:
```sh
python M3-rotation_metric.py ^
sample_data\HMD_20180123_151226\Rotation_PITCH\tmPoses\HMD_TM_Rotation_pitch_20180123_151032.txt ^
sample_data\HMD_20180123_151226\Rotation_PITCH\intervals\HMD_Intervals_Rotation_pitch_20180123_151032.txt ^
sample_data\HMD_20180123_151226\Rotation_PITCH\robotPoses\HMD_Robot_Rotation_pitch_20180123_151032.txt ^
Rotation_PITCH_output.txt
```




