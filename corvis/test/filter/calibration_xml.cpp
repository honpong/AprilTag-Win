#include "gtest/gtest.h"
#include "calibration_xml.h"
#include <limits>

namespace rc { namespace testing {

TEST(CalibrationXML, Parse)
{
    const char xml[] = R"(
<rig>
  <device_id>dvt3_cad</device_id>
  <camera>
    <camera_model name="Fisheye" index="0" type="calibu_fu_fv_u0_v0_w" version="8" is_cad="1">
      <width>640</width>
      <height>480</height>
      <!--Use RDF matrix, [right down forward], to decide the coordinate frame convention.-->
      <right>[ 1; 0; 0 ]</right>
      <down>[ 0; 1; 0 ]</down>
      <forward>[ 0; 0; 1 ]</forward>
      <!--Camera parameters ordered as per type name.-->
      <!--This values were obtained by performing intrinsics calibration on a "typical" lens -->
      <params> [255.8;   255.8;   320;   240;   0.922] </params>
    </camera_model>
    <pose />
  </camera>
  <camera>
    <camera_model name="Narrow" index="1" type="calibu_fu_fv_u0_v0_k1_k2_k3" version="8" is_cad="1">
      <width>1280</width>
      <height>720</height>
      <!--Use RDF matrix, [right down forward], to decide the coordinate frame convention.-->
      <right>[ 1; 0; 0 ]</right>
      <down>[ 0; 1; 0 ]</down>
      <forward>[ 0; 0; 1 ]</forward>
      <!--Camera parameters ordered as per type name.-->
      <!--This values were obtained by performing intrinsics calibration on a "typical" lens -->
	  <params> [885.7; 885.7; 640.0; 360.0; -0.073056314; 0.072007; 0.000333766] </params>
    </camera_model>
    <pose />
  </camera>
  <camera>
    <camera_model name="IR" index="2" type="calibu_fu_fv_u0_v0_k1_k2_k3" version="8" is_cad="1">
      <width>640</width>
      <height>480</height>
      <!--Use RDF matrix, [right down forward], to decide the coordinate frame convention.-->
      <right>[ 1; 0; 0 ]</right>
      <down>[ 0; 1; 0 ]</down>
      <forward>[ 0; 0; 1 ]</forward>
      <!--Camera parameters ordered as per type name.-->
      <!--This values were obtained by scaling the Narrow parameters since Yellowstone uses a RGBIR cam. -->
      <params> [566.7; 566.7; 320.0; 240.0; -0.00959507; 0.008005806; 0.000281827] </params>
    </camera_model>
    <pose />
  </camera>
  <camera>
    <camera_model name="Depth" index="3" type="calibu_fu_fv_u0_v0_k1_k2_k3" version="8" is_cad="1">
      <width>640</width>
      <height>480</height>
      <!--Use RDF matrix, [right down forward], to decide the coordinate frame convention.-->
      <right>[ 1; 0; 0 ]</right>
      <down>[ 0; 1; 0 ]</down>
      <forward>[ 0; 0; 1 ]</forward>
      <!--Camera parameters ordered as per type name.-->
      <!--This values were obtained by scaling the IR parameters, resolution is somewhat arbitrary since -->
      <!--Yellowstone depth pipeline already produces a metric point cloud as output. -->
      <params> [566.7; 566.7; 320.1; 240.2; -0.00959507; 0.008005806; 0.000281827] </params>
    </camera_model>

    <pose />
  </camera>
  <intrinsic_imu_calibration name="Imu" imu_id="100" serialno="0" type="" version="1.0" is_cad="1">
    <b_w_b_a> [0.0; 23.0;   0.0;   0.0;   0.0;   4.5] </b_w_b_a>
    <crossterms> [1, 0, 0, 0, 0, 0;   0, 1, 0, 0, 0, 0;   0, 0, 1, 0, 0, 0;   0, 0, 0, 1, 0, 0;   0, 0, 0, 0, 1, 0;   0, 0, 0, 0, 0, 1] </crossterms>
    <!--Values obtained by collecting data from a typical BMX055 imu and performing offline processing. --> 
    <gyro_noise_sigma>5.3088444e-5</gyro_noise_sigma>
    <gyro_bias_sigma>1.4125375e-4</gyro_bias_sigma>
    <accel_noise_sigma>0.001883649</accel_noise_sigma>
    <accel_bias_sigma>1.2589254e-2</accel_bias_sigma>
  </intrinsic_imu_calibration>
  <extrinsic_calibration name="fisheye_wrt_imu" index="0" frame_A_id="100" frame_B_id="0" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1.0,    0.0,  0.0, 0.005911; 0, 1.0, 0.0, 0.001295; 0.0, 0.0, 1.0, 0.00451] </A_T_B>
  </extrinsic_calibration>
  <extrinsic_calibration name="narrow_wrt_imu" index="0" frame_A_id="100" frame_B_id="1" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1.0,    0.0,  0.0, 0.093956; 0, 1.0, 0.0, 0.001295; 0.0, 0.0, 1.0, 0.0023276] </A_T_B>
  </extrinsic_calibration>
  <extrinsic_calibration name="depth_wrt_imu" index="0" frame_A_id="100" frame_B_id="2" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1.0,    0.0,  0.0, 0.035556; 0, 1.0, 0.0, 0.001295; 0.0, 0.0, 1.0, 0.0023276] </A_T_B>
  </extrinsic_calibration>
  <extrinsic_calibration name="ir_wrt_imu" index="0" frame_A_id="100" frame_B_id="3" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1.0,    0.0,  0.0, 0.035556; 0, 1.0, 0.0, 0.001295; 0.0, 0.0, 1.0, 0.0023276] </A_T_B>
  </extrinsic_calibration>
  <!-- The Android to IMU transformation is used in yellowstone to unrotate the IMU data coming through the Android sensor manager.-->
  <!-- Do not touch this next line unless you know what you are doing.-->
  <extrinsic_calibration name="device_wrt_imu" index="0" frame_A_id="100" frame_B_id="40" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1, 0, 0, 0.0;   0, -1, 0.0, 0.0;   0.0, 0.0, -1, 0.0] </A_T_B>
  </extrinsic_calibration>
  <extrinsic_calibration name="unity_wrt_device" index="0" frame_A_id="40" frame_B_id="41" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1, 0, 0, 0;   0, 1, 0, 0;   0, 0, -1, 0] </A_T_B>
  </extrinsic_calibration>
  <extrinsic_calibration name="opengl_wrt_device" index="0" frame_A_id="40" frame_B_id="42" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1, 0, 0, 0;   0, 1, 0, 0;   0, 0, 1, 0] </A_T_B>
  </extrinsic_calibration>
  <extrinsic_calibration name="global_local_level_wrt_device" index="0" frame_A_id="31" frame_B_id="30" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [1, 0, 0, 0;   0, 0, 1, 0;   0, 1, 0, 0] </A_T_B>
  </extrinsic_calibration>
  <extrinsic_calibration name="display_wrt_device" index="0" frame_A_id="40" frame_B_id="43" serialno="0" type="" version="1.0" is_cad="1">
    <A_T_B> [0.0, -1, 0.0, 0.042756;   -1, 0.0, 0.0, 0.035295;   0.0, 0.0, -1.0, -0.0039] </A_T_B>
  </extrinsic_calibration>
  <geo_location>
    <!--Mountain View CA. -->  
    <latitude>37.389444</latitude>
    <longitude>122.0819</longitude>
    <altitude>32</altitude>
  </geo_location>
</rig>
)";
    calibration cal;
    ASSERT_TRUE(calibration_deserialize_xml(xml, cal));

    EXPECT_EQ(cal.imu.gyro_bias_rad__s[1], 23);
    EXPECT_EQ(cal.imu.accel_bias_m__s2[2], 4.5);
    EXPECT_EQ(cal.imu.gyro_bias_sigma_rad__2, 1.4125375e-4);
    EXPECT_EQ(cal.imu.accel_bias_sigma_m__s2, 1.2589254e-2);
    EXPECT_EQ(cal.imu.gyro_noise_sigma_rad__2, 5.3088444e-5);
    EXPECT_EQ(cal.imu.accel_noise_sigma_m__s2, 0.001883649);

    EXPECT_EQ(cal.fisheye.intrinsics.w, 0.922);
    EXPECT_EQ(cal.fisheye.intrinsics.focal_length_y_px, 255.8);
    EXPECT_EQ(cal.fisheye.extrinsics_wrt_imu_m.T[0], 0.005911);

    EXPECT_EQ(cal.color.intrinsics.focal_length_x_px, 885.7);
    EXPECT_EQ(cal.color.intrinsics.k2, 0.072007);

    EXPECT_EQ(cal.depth.intrinsics.center_x_px, 320.1);
    EXPECT_EQ(cal.depth.intrinsics.center_y_px, 240.2);
    EXPECT_EQ(cal.depth.extrinsics_wrt_imu_m.T[2], 0.0023276);

    EXPECT_EQ(cal.ir.intrinsics.k3, 0.000281827);

    EXPECT_EQ(cal.display.wrt_device_m.T[2], -0.0039);
    EXPECT_NEAR(to_rotation_vector(cal.display.wrt_device_m.Q).raw_vector().x(), M_PI / M_SQRT2, std::numeric_limits<f_t>::epsilon());

    EXPECT_EQ(cal.geo_location.latitude_deg, 37.389444);
    EXPECT_EQ(cal.geo_location.longitude_deg, 122.0819);

    std::string tmp;
    EXPECT_TRUE(calibration_serialize_xml(cal, tmp));
    calibration cal2;
    EXPECT_TRUE(calibration_deserialize_xml(tmp, cal2));
    std::string tmp_roundtrip;
    EXPECT_TRUE(calibration_serialize_xml(cal2, tmp_roundtrip));
    EXPECT_EQ(tmp, tmp_roundtrip);
}

} /*testing*/ } /*rc*/
