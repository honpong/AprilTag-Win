// (C) Copyright 2015 Jim Radford; All Rights Reserved
#include "gtest/gtest.h"
#include "util.h"
#include "tpose.h"

TEST(TPose, LessThan)
{
    EXPECT_TRUE(tpose{sensor_clock::ns100_to_tp(24000)} < tpose{sensor_clock::ns100_to_tp(25000)});
}

TEST(TPose, Interpolate)
{
    using namespace std::chrono_literals;
    sensor_clock::time_point t;
    EXPECT_TRANSFORMATION_NEAR(transformation(quaternion::Identity(), v3(31,0,0)),
                               tpose(t+3ms,
                                     tpose{t+1ms, transformation(quaternion::Identity(), v3(11,0,0))},
                                     tpose{t+9ms, transformation(quaternion::Identity(), v3(91,0,0))}).G, F_T_EPS);
}

TEST(TPose, Parses)
{
    std::string data = "130777180477627115    0.08218356   -0.99205852   -0.09521491  207.50582886   -0.88409311   -0.11667039    0.45251244   36.77946472   -0.46002755    0.04698976   -0.88666040 -2555.18896484        0.1901\n";

    EXPECT_NO_THROW(tpose_raw(data.c_str())) << "can parse tpose from string";
    tpose_raw tp(data.c_str());
    EXPECT_EQ(tp.t_100ns, 130777180477627115);
    EXPECT_EQ(tp.R(0,1), (f_t)-0.99205852);
    EXPECT_EQ(tp.T_mm[2], (f_t)-2555.18896484);
}

TEST(TPose, NA)
{
    std::string data = "130777180477627115    NA\n";
    EXPECT_THROW(tpose_raw tp(data.c_str()), std::invalid_argument) << "fail to parse NA tpose from string";
}

static const std::string pose_data =
"130777203075717378    0.08205719   -0.99216610   -0.09419688  207.81494141   -0.88427913   -0.11607618    0.45230153   36.59056854   -0.45969227    0.04618175   -0.88687664 -2554.84155273        0.1928\n"
"130777203077322591    0.08207166   -0.99217194   -0.09412272  207.81651306   -0.88430804   -0.11605222    0.45225134   36.58011627   -0.45963424    0.04611646   -0.88691020 -2554.83813477        0.1932\n"
"130777203081497859    0.08208925   -0.99217117   -0.09411521  207.82106018   -0.88424259   -0.11607082    0.45237434   36.59338760   -0.45975682    0.04608561   -0.88684821 -2554.86181641        0.1914\n"
"130777203135886653    0.08208717   -0.99217069   -0.09412238  207.82298279   -0.88422996   -0.11607353    0.45239839   36.59148026   -0.45978153    0.04608972   -0.88683522 -2554.85156250        0.1925\n"
"130777203135916895    0.08208717   -0.99217069   -0.09412238  207.82298279   -0.88422996   -0.11607353    0.45239839   36.59148026   -0.45978153    0.04608972   -0.88683522 -2554.85156250        0.1925\n"
"130777203213101769    0.05975123   -0.98986250   -0.12884854  224.63960266   -0.92206794   -0.10417823    0.37274328  -33.50150681   -0.38238782    0.09653524   -0.91894531 -2555.90551758        0.1612\n"
"130777203213133533 NA                                                                                                                                                                                    \n"
"130777203217647142    0.05463230   -0.98901594   -0.13734165  228.33274841   -0.93060303   -0.10028993    0.35202256  -51.26451111   -0.36192989    0.10857876   -0.92586029 -2554.23950195        0.1518\n"
"130777203249131659    0.02601795   -0.97940892   -0.20020303  246.49345398   -0.97508729   -0.06899694    0.21081822 -165.42213440   -0.22029063    0.18973036   -0.95680434 -2526.90771484        0.1328\n"
"130777203314691092   -0.00814848   -0.91092449   -0.41249266  154.98895264   -0.98036969   -0.07398894    0.18275930 -185.26411438   -0.19699982    0.40588450   -0.89243984 -2580.38574219        0.1064\n"
"130777203366884343    0.02618482   -0.95400882   -0.29863268  164.26480103   -0.96986431   -0.09663314    0.22366320 -154.77915955   -0.24223448    0.28377661   -0.92778945 -2517.87304688        0.1152\n"
"130777203471637182    0.08206524   -0.99212891   -0.09458137  207.85711670   -0.88432837   -0.11625356    0.45215994   36.60706329   -0.45959634    0.04653437   -0.88690805 -2554.85913086        0.1924\n"
"130777203546894407    0.08199111   -0.99215966   -0.09432170  207.83369446   -0.88421148   -0.11608179    0.45243239   36.61133575   -0.45983422    0.04630489   -0.88679665 -2554.86767578        0.2005\n"
"130777203580659037    0.08196622   -0.99215835   -0.09435742  207.83024597   -0.88425696   -0.11607161    0.45234621   36.60868835   -0.45975128    0.04635909   -0.88683689 -2554.85717773        0.2002\n"
    ;

TEST(TPoseSequence, Parses)
{
    std::string data(pose_data);
    std::stringstream f(data);
    tpose_sequence ts;
    f >> ts;
    EXPECT_TRUE(f.eof());
    EXPECT_FALSE(f.fail());
    EXPECT_FALSE(f.bad());
    EXPECT_EQ(ts.tposes.size(), 13);
    EXPECT_EQ(ts.tposes.front().t, sensor_clock::ns100_to_tp(130777203075717378));
    EXPECT_EQ(ts.tposes.back().t, sensor_clock::ns100_to_tp(130777203580659037));

    EXPECT_NEAR(ts.get_length(), 0.00284024241259873 / 100, .00001);
    EXPECT_NEAR(ts.get_path_length(), 58.68254583035501781 / 100, .00001);
}

TEST(TPoseSequence, DISABLED_FailsToParse) // noisy, so disabled
{
    std::string data(pose_data);
    std::string target("0.02601795");
    std::stringstream f(data.replace(data.find(target), target.size(), "ERROR"));
    tpose_sequence ts;
    f >> ts;
    EXPECT_FALSE(f.eof());
    EXPECT_TRUE(f.fail());
    EXPECT_FALSE(f.bad());
    EXPECT_EQ(ts.tposes.size(), 7);
}

TEST(TPoseSequence, Interpolates)
{
    using namespace std::chrono_literals;
    sensor_clock::time_point t;
    tpose_sequence ts;
    ts.tposes.emplace_back(t+100ms,transformation(quaternion::Identity(),v3(1,0,0)));
    ts.tposes.emplace_back(t+200ms,transformation(quaternion::Identity(),v3(2,0,0)));
    ts.tposes.emplace_back(t+300ms,transformation(quaternion::Identity(),v3(3,0,0)));
    ts.tposes.emplace_back(t+400ms,transformation(quaternion::Identity(),v3(4,0,0)));
    tpose tp{t};
    EXPECT_FALSE(ts.get_pose(t-100ms, tp));
    EXPECT_FALSE(ts.get_pose(t+000ms, tp));
    EXPECT_TRUE (ts.get_pose(t+100ms, tp)); EXPECT_TRUE(t+100ms == tp.t); EXPECT_V3_NEAR(v3(1,   0,0), tp.G.T, F_T_EPS);
    EXPECT_TRUE (ts.get_pose(t+125ms, tp)); EXPECT_TRUE(t+125ms == tp.t); EXPECT_V3_NEAR(v3(1.25,0,0), tp.G.T, F_T_EPS);
    EXPECT_TRUE (ts.get_pose(t+225ms, tp)); EXPECT_TRUE(t+225ms == tp.t); EXPECT_V3_NEAR(v3(2.25,0,0), tp.G.T, F_T_EPS);
    EXPECT_TRUE (ts.get_pose(t+300ms, tp)); EXPECT_TRUE(t+300ms == tp.t); EXPECT_V3_NEAR(v3(3,   0,0), tp.G.T, F_T_EPS);
    EXPECT_TRUE (ts.get_pose(t+400ms, tp)); EXPECT_TRUE(t+400ms == tp.t); EXPECT_V3_NEAR(v3(4,   0,0), tp.G.T, F_T_EPS);
    EXPECT_FALSE(ts.get_pose(t+500ms, tp));
}
