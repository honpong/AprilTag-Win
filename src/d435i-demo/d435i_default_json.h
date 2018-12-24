//
//  d435i_default_json.h
//  d435i-demo
//
//  Created by Hon Pong (Gary) Ho on 11/30/18.
//

#ifndef d435i_default_json_h
#define d435i_default_json_h

const char* default_camera_json = { "\
    {\n\
    \"cameras\": [\n\
                {\n\
                    \"extrinsics\": {\n\
                        \"W_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T\": [\n\
                              0,\n\
                              0,\n\
                              0\n\
                              ],\n\
                        \"W\": [\n\
                              0.0,\n\
                              0.0,\n\
                              0.0\n\
                              ]\n\
                    },\n\
                    \"size_px\": [\n\
                                640,\n\
                                480\n\
                                ],\n\
                    \"distortion\": {\n\
                        \"type\": \"undistorted\"\n\
                    },\n\
                    \"focal_length_px\": [\n\
                                        382.7279357910156,\n\
                                        382.7279357910156\n\
                                        ],\n\
                    \"center_px\": [\n\
                                  320.7322082519531,\n\
                                  242.91510009765625\n\
                                  ]\n\
                },\n\
                {\n\
                    \"extrinsics\": {\n\
                        \"W_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T\": [\n\
                              0.04981447011232376,\n\
                              0,\n\
                              0\n\
                              ],\n\
                        \"W\": [\n\
                              0.0,\n\
                              0.0,\n\
                              0.0\n\
                              ]\n\
                    },\n\
                    \"size_px\": [\n\
                                640,\n\
                                480\n\
                                ],\n\
                    \"distortion\": {\n\
                        \"type\": \"undistorted\"\n\
                    },\n\
                    \"focal_length_px\": [\n\
                                        382.7279357910156,\n\
                                        382.7279357910156\n\
                                        ],\n\
                    \"center_px\": [\n\
                                  320.7322082519531,\n\
                                  242.91510009765625\n\
                                  ]\n\
                },\n\
                {\n\
                    \"extrinsics\": {\n\
                        \"W_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T\": [\n\
                              0.015178494155406952,\n\
                              0.0004153688787482679,\n\
                              0.00034920888720080256\n\
                              ],\n\
                        \"W\": [\n\
                              -0.0009543058618942086,\n\
                              -0.0066496329702882846,\n\
                              -0.003395326197084362\n\
                              ]\n\
                    },\n\
                    \"size_px\": [\n\
                                640,\n\
                                480\n\
                                ],\n\
                    \"distortion\": {\n\
                        \"type\": \"undistorted\"\n\
                    },\n\
                    \"focal_length_px\": [\n\
                                        618.0311279296875,\n\
                                        616.1911010742188\n\
                                        ],\n\
                    \"center_px\": [\n\
                                  328.5360107421875,\n\
                                  242.39776611328125\n\
                                  ]\n\
                }\n\
                ],\n\
    \"depths\": [\n\
               {\n\
                   \"extrinsics\": {\n\
                       \"W_variance\": [\n\
                                      0,\n\
                                      0,\n\
                                      0\n\
                                      ],\n\
                       \"T_variance\": [\n\
                                      0,\n\
                                      0,\n\
                                      0\n\
                                      ],\n\
                       \"T\": [\n\
                             0,\n\
                             0,\n\
                             0\n\
                             ],\n\
                       \"W\": [\n\
                             0.0,\n\
                             0.0,\n\
                             0.0\n\
                             ]\n\
                   },\n\
                   \"size_px\": [\n\
                               640,\n\
                               480\n\
                               ],\n\
                   \"distortion\": {\n\
                       \"type\": \"undistorted\"\n\
                   },\n\
                   \"focal_length_px\": [\n\
                                       382.7279357910156,\n\
                                       382.7279357910156\n\
                                       ],\n\
                   \"center_px\": [\n\
                                 320.7322082519531,\n\
                                 242.91510009765625\n\
                                 ]\n\
               }\n\
               ],\n\
    \"imus\": [\n\
             {\n\
                 \"extrinsics\": {\n\
                     \"W_variance\": [\n\
                                    0,\n\
                                    0,\n\
                                    0\n\
                                    ],\n\
                     \"T_variance\": [\n\
                                    0,\n\
                                    0,\n\
                                    0\n\
                                    ],\n\
                     \"T\": [\n\
                           0,\n\
                           0,\n\
                           0\n\
                           ],\n\
                     \"W\": [\n\
                           0.0,\n\
                           0.0,\n\
                           0.0\n\
                           ]\n\
                 },\n\
                 \"accelerometer\": {\n\
                     \"scale_and_alignment\": [\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1\n\
                                             ],\n\
                     \"bias\": [\n\
                              0,\n\
                              0,\n\
                              0\n\
                              ],\n\
                     \"noise_variance\": 6.7e-05,\n\
                     \"bias_variance\": [\n\
                                       0.0001,\n\
                                       0.0001,\n\
                                       0.0001\n\
                                       ]\n\
                 },\n\
                 \"gyroscope\": {\n\
                     \"scale_and_alignment\": [\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1\n\
                                             ],\n\
                     \"bias\": [\n\
                              0,\n\
                              0,\n\
                              0\n\
                              ],\n\
                     \"noise_variance\": 5.14803e-06,\n\
                     \"bias_variance\": [\n\
                                       5e-07,\n\
                                       5e-07,\n\
                                       5e-07\n\
                                       ]\n\
                 }\n\
             }\n\
             ],\n\
    \"device_type\": \"Intel RealSense D435I\",\n\
    \"calibration_version\": 10,\n\
    \"device_id\": \"\"\n\
}\n\
    "};

const char* camera_no_color_json = { "\
    {\n\
    \"cameras\": [\n\
                {\n\
                    \"extrinsics\": {\n\
                        \"W_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T\": [\n\
                              0,\n\
                              0,\n\
                              0\n\
                              ],\n\
                        \"W\": [\n\
                              0.0,\n\
                              0.0,\n\
                              0.0\n\
                              ]\n\
                    },\n\
                    \"size_px\": [\n\
                                640,\n\
                                480\n\
                                ],\n\
                    \"distortion\": {\n\
                        \"type\": \"undistorted\"\n\
                    },\n\
                    \"focal_length_px\": [\n\
                                        382.7279357910156,\n\
                                        382.7279357910156\n\
                                        ],\n\
                    \"center_px\": [\n\
                                  320.7322082519531,\n\
                                  242.91510009765625\n\
                                  ]\n\
                },\n\
                {\n\
                    \"extrinsics\": {\n\
                        \"W_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T_variance\": [\n\
                                       0,\n\
                                       0,\n\
                                       0\n\
                                       ],\n\
                        \"T\": [\n\
                              0.04981447011232376,\n\
                              0,\n\
                              0\n\
                              ],\n\
                        \"W\": [\n\
                              0.0,\n\
                              0.0,\n\
                              0.0\n\
                              ]\n\
                    },\n\
                    \"size_px\": [\n\
                                640,\n\
                                480\n\
                                ],\n\
                    \"distortion\": {\n\
                        \"type\": \"undistorted\"\n\
                    },\n\
                    \"focal_length_px\": [\n\
                                        382.7279357910156,\n\
                                        382.7279357910156\n\
                                        ],\n\
                    \"center_px\": [\n\
                                  320.7322082519531,\n\
                                  242.91510009765625\n\
                                  ]\n\
                }\n\
                ],\n\
    \"depths\": [\n\
               {\n\
                   \"extrinsics\": {\n\
                       \"W_variance\": [\n\
                                      0,\n\
                                      0,\n\
                                      0\n\
                                      ],\n\
                       \"T_variance\": [\n\
                                      0,\n\
                                      0,\n\
                                      0\n\
                                      ],\n\
                       \"T\": [\n\
                             0,\n\
                             0,\n\
                             0\n\
                             ],\n\
                       \"W\": [\n\
                             0.0,\n\
                             0.0,\n\
                             0.0\n\
                             ]\n\
                   },\n\
                   \"size_px\": [\n\
                               640,\n\
                               480\n\
                               ],\n\
                   \"distortion\": {\n\
                       \"type\": \"undistorted\"\n\
                   },\n\
                   \"focal_length_px\": [\n\
                                       382.7279357910156,\n\
                                       382.7279357910156\n\
                                       ],\n\
                   \"center_px\": [\n\
                                 320.7322082519531,\n\
                                 242.91510009765625\n\
                                 ]\n\
               }\n\
               ],\n\
    \"imus\": [\n\
             {\n\
                 \"extrinsics\": {\n\
                     \"W_variance\": [\n\
                                    0,\n\
                                    0,\n\
                                    0\n\
                                    ],\n\
                     \"T_variance\": [\n\
                                    0,\n\
                                    0,\n\
                                    0\n\
                                    ],\n\
                     \"T\": [\n\
                           0,\n\
                           0,\n\
                           0\n\
                           ],\n\
                     \"W\": [\n\
                           0.0,\n\
                           0.0,\n\
                           0.0\n\
                           ]\n\
                 },\n\
                 \"accelerometer\": {\n\
                     \"scale_and_alignment\": [\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1\n\
                                             ],\n\
                     \"bias\": [\n\
                              0,\n\
                              0,\n\
                              0\n\
                              ],\n\
                     \"noise_variance\": 6.7e-05,\n\
                     \"bias_variance\": [\n\
                                       0.0001,\n\
                                       0.0001,\n\
                                       0.0001\n\
                                       ]\n\
                 },\n\
                 \"gyroscope\": {\n\
                     \"scale_and_alignment\": [\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1,\n\
                                             0,\n\
                                             0,\n\
                                             0,\n\
                                             1\n\
                                             ],\n\
                     \"bias\": [\n\
                              0,\n\
                              0,\n\
                              0\n\
                              ],\n\
                     \"noise_variance\": 5.14803e-06,\n\
                     \"bias_variance\": [\n\
                                       5e-07,\n\
                                       5e-07,\n\
                                       5e-07\n\
                                       ]\n\
                 }\n\
             }\n\
             ],\n\
    \"device_type\": \"Intel RealSense D435I\",\n\
    \"calibration_version\": 10,\n\
    \"device_id\": \"\"\n\
}\n\
    "};

#endif /* d435i_default_json_h */


