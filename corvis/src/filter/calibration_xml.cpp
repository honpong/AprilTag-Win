#include "calibration_xml.h"

#include "rapidxml.hpp"

#include <iostream>
#include <cstdlib>

namespace rapidxml {
template<typename Ch>
class checked_xml_node {
    xml_node<Ch> *n;
  public:
    checked_xml_node(xml_node<Ch> *n_) : n(n_) {}

    xml_node<Ch> *assert_node(const Ch *name) {
        xml_node<Ch> *f = n->first_node(name);
        if (f) return f;
        throw parse_error("missing node", (void*)name);
    }

    xml_attribute<Ch> *assert_attribute(const Ch *name) {
        xml_attribute<Ch> *f = n->first_attribute(name);
        if (f) return f;
        throw parse_error("missing atrribute", (void*)name);
    }

    template<int Rows, int Cols>
        Eigen::Matrix<f_t,Rows,Cols> assert_m(const Ch *name) {
        Eigen::Matrix<f_t,Rows,Cols> m;
        char *line = assert_node(name)->value();
        size_t end = 0;
        for (int i=0; i<Rows; i++)
            for (int j=0; j<Cols; j++) {
                while (line[end] && std::isspace(line[end])) end++;
                if (line[end++] != (j ? ',' : i ? ';' : '['))
                    throw parse_error(j ? "expected ','" : i ? "expected ';'" : "expected '['", line + end - 1);
                m(i, j) = std::stod(line+=end, &end);
            }
        while (line[end] && std::isspace(line[end])) end++;
        if (line[end] != ']')
            throw parse_error("failed to find closing bracket", (void*)(line + end));
        return m;
    }
};
}

static struct calibration::camera *camera_from_index(calibration &cal, int index) {
    switch (index) {
        case 0: return &cal.fisheye;
        case 1: return &cal.color;
        case 2: return &cal.ir;
        case 3: return &cal.depth;
        default:
            std::cerr << "unknown camera index " << index << "\n";
            return nullptr;
    }
}

static struct calibration::frame *frame_from_index(calibration &cal, int index) {
    switch (index) {
        case 41: return &cal.unity;
        case 42: return &cal.opengl;
        case 43: return &cal.display;
        default:
            std::cerr << "unknown frame index " << index << "\n";
            return nullptr;
    }
}

using namespace rapidxml;
typedef xml_document<char> document;
typedef xml_node<char> node;
typedef xml_attribute<char> atrribute;
typedef checked_xml_node<char> checked_node;

bool calibration_deserialize_xml(const std::string &xml, calibration &cal)
{
    cal = calibration {};
    document doc; // needs to be valid in catch
    try {
        doc.parse<
            parse_no_data_nodes|
            //parse_no_element_values|
            parse_no_entity_translation|
            parse_no_utf8|
            parse_comment_nodes|
            parse_trim_whitespace|
            parse_normalize_whitespace|
            parse_validate_closing_tags
        >(doc.allocate_string(xml.c_str()));
        //xml_node<const char> *root = checked_xml_node<const char>(doc).assert_node("rig");
        node *root = doc.first_node("rig");
        if (!root)
            throw parse_error("expected root node <rig>", (void*)xml.c_str());
        strlcpy(cal.device_id, checked_node(root).assert_node("device_id")->value(), sizeof(cal.device_id));
        for (node *camera = root->first_node("camera"); camera; camera = camera->next_sibling("camera"))
            for (node *camera_model_ = camera->first_node("camera_model"); camera_model_; camera_model_ = camera_model_->next_sibling("camera_model")) {
                checked_node camera_model(camera_model_);
                struct calibration::camera *c = camera_from_index(cal, std::atoi(camera_model.assert_attribute("index")->value()));
                if (!c)
                    continue;
                strlcpy(c->name, camera_model.assert_attribute("name")->value(), sizeof(c->name));
                c->intrinsics.width_px = std::atoi(camera_model.assert_node("width")->value());
                c->intrinsics.height_px = std::atoi(camera_model.assert_node("height")->value());
                v3  right   = camera_model.assert_m<3,1>("right"),
                    down    = camera_model.assert_m<3,1>("down"),
                    forward = camera_model.assert_m<3,1>("forward");
                std::string type = camera_model.assert_attribute("type")->value();
                if (type == "calibu_fu_fv_u0_v0_k1_k2_k3") {
                    auto m = camera_model.assert_m<7,1>("params");
                    c->intrinsics.f_x_px = m[0];
                    c->intrinsics.f_y_px = m[1];
                    c->intrinsics.c_x_px = m[2];
                    c->intrinsics.c_y_px = m[3];

                    c->intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
                    c->intrinsics.k1 = m[4];
                    c->intrinsics.k2 = m[5];
                    c->intrinsics.k3 = m[6];
                } else if (type == "calibu_fu_fv_u0_v0_w") {
                    auto m = camera_model.assert_m<5,1>("params");
                    c->intrinsics.f_x_px = m[0];
                    c->intrinsics.f_y_px = m[1];
                    c->intrinsics.c_x_px = m[2];
                    c->intrinsics.c_y_px = m[3];

                    c->intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
                    c->intrinsics.w = m[4];
                } else if (type == "calibu_fu_fv_u0_v0") {
                    auto m = camera_model.assert_m<4,1>("params");
                    c->intrinsics.f_x_px = m[0];
                    c->intrinsics.f_y_px = m[1];
                    c->intrinsics.c_x_px = m[2];
                    c->intrinsics.c_y_px = m[3];

                    c->intrinsics.type = rc_CALIBRATION_TYPE_UNDISTORTED;
                } else {
                    std::cerr << "unknown calibration type " << type << "\n";
                }
            }
        if (node *imu_ = root->first_node("intrinsic_imu_calibration")) {
            checked_node imu(imu_);
            if (std::string("100") == imu.assert_attribute("imu_id")->value()) {
                auto b_w_b_a = imu.assert_m<6,1>("b_w_b_a");
                cal.imu.w_bias_rad__s = b_w_b_a.block<3,1>(0,0);
                cal.imu.a_bias_m__s2  = b_w_b_a.block<3,1>(3,0);

                auto crossterms = imu.assert_m<6,6>("crossterms");
                cal.imu.w_alignment = crossterms.block<3,3>(0,0);
                cal.imu.a_alignment = crossterms.block<3,3>(3,3);

                cal.imu.w_noise_sigma_rad__s = std::atof(imu.assert_node("gyro_noise_sigma")->value());
                cal.imu.a_noise_sigma_m__s2 = std::atof(imu.assert_node("accel_noise_sigma")->value());

                cal.imu.w_bias_sigma_rad__s = std::atof(imu.assert_node("gyro_bias_sigma")->value());
                cal.imu.a_bias_sigma_m__s2 = std::atof(imu.assert_node("accel_bias_sigma")->value());
            }
        }
        for (node *extrinsic_ = root->first_node("extrinsic_calibration"); extrinsic_; extrinsic_ = extrinsic_->next_sibling("extrinsic_calibration")) {
            checked_node extrinsic(extrinsic_);
            auto A_id = std::atoi(extrinsic.assert_attribute("frame_A_id")->value());
            auto B_id = std::atoi(extrinsic.assert_attribute("frame_B_id")->value());
            struct transformation *t = nullptr;
            if (A_id == 100) { // IMU
                if (B_id == 40) // DEVICE
                    t = &cal.device_wrt_imu_m;
                else {
                    struct calibration::camera *c = camera_from_index(cal, B_id);
                    if (c)
                        t = &c->extrinsics_wrt_imu_m;
                }
            } else if (A_id == 40) { // DEVICE
                struct calibration::frame *f = frame_from_index(cal, B_id);
                if (f)
                    t = &f->wrt_device_m;
            } else {
                if (!(B_id == 30 && A_id == 31)) // global_local_level_wrt_device, typo?
                    std::cerr << "unknown transformation : " << B_id << " wrt " << A_id << "\n";
            }
            m4 m = m4::Zero();
            m.block<3,4>(0,0) = extrinsic.assert_m<3,4>("A_T_B");
            if (t)
                *t = transformation(m, m.block<4,1>(0,3));
        }
        if (node *geo_ = root->first_node("geo_location")) {
            checked_node geo(geo_);
            cal.geo_location.latitude_deg  = std::atof(geo.assert_node("latitude")->value());
            cal.geo_location.longitude_deg = std::atof(geo.assert_node("longitude")->value());
            cal.geo_location.altitude_m    = std::atof(geo.assert_node("altitude")->value());
        }
        return true;
    } catch (rapidxml::parse_error e) {
        std::cerr << "error parsing xml: " << xml << "\n" << e.what() << ": " << e.where<const char>() << "\n";
        return false;
    } catch (std::exception e) {
        std::cerr << "error parsing xml: " << xml << "\n" << e.what() << "\n";
        return false;
    }
}

#include "rapidxml_print.hpp"

template <typename Derived>
static char *xml_string(document &doc, const Eigen::DenseBase<Derived> &m)
{
    Eigen::IOFormat compact(Eigen::FullPrecision, Eigen::DontAlignCols, ", ", "; ", " ", "", "[", "]");
    std::stringstream s; s << ' ' << m.format(compact) << ' ';
    return doc.allocate_string(s.str().c_str());
}

static char *xml_string(document &doc, double x)
{
    std::stringstream s; s << x;
    return doc.allocate_string(s.str().c_str());
}

bool calibration_serialize_xml(const calibration &cal, std::string &xml)
{
    document doc;
    node *root; doc.append_node(root = doc.allocate_node(node_element, "rig"));
    root->append_node(doc.allocate_node(node_element, "device_id", cal.device_id));

    int index = 0;
    for (const calibration::camera *c : {&cal.fisheye, &cal.color, &cal.ir, &cal.depth}) {
        const char *type =
            c->intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE     ? "calibu_fu_fv_u0_v0_w" :
            c->intrinsics.type == rc_CALIBRATION_TYPE_POLYNOMIAL3 ? "calibu_fu_fv_u0_v0_k1_k2_k3" :
            c->intrinsics.type == rc_CALIBRATION_TYPE_UNDISTORTED ? "calibu_fu_fv_u0_v0" : nullptr;
        if (!type) { index++; continue; }

        node *camera; root->append_node(camera = doc.allocate_node(node_element, "camera"));
        node *camera_model; camera->append_node(camera_model = doc.allocate_node(node_element, "camera_model"));
        camera_model->append_attribute(doc.allocate_attribute("name", c->name));
        camera_model->append_attribute(doc.allocate_attribute("index", xml_string(doc, index++)));
        camera_model->append_attribute(doc.allocate_attribute("type", type));
        camera_model->append_attribute(doc.allocate_attribute("version", "8"));
        camera_model->append_attribute(doc.allocate_attribute("is_cad", "1"));
        camera_model->append_node(doc.allocate_node(node_element, "width", xml_string(doc, c->intrinsics.width_px)));
        camera_model->append_node(doc.allocate_node(node_element, "height", xml_string(doc, c->intrinsics.height_px)));
        m3 m = m3::Identity();
        camera_model->append_node(doc.allocate_node(node_element, "right", xml_string(doc, m.block<3,1>(0,0))));
        camera_model->append_node(doc.allocate_node(node_element, "down", xml_string(doc, m.block<3,1>(0,1))));
        camera_model->append_node(doc.allocate_node(node_element, "forward", xml_string(doc, m.block<3,1>(0,2))));
        std::stringstream s;
        if (c->intrinsics.type == rc_CALIBRATION_TYPE_POLYNOMIAL3) {
            camera_model->append_node(doc.allocate_node(node_element, "params", xml_string(doc, Eigen::Matrix<f_t, 7,1>{
                c->intrinsics.f_x_px, c->intrinsics.f_y_px,
                c->intrinsics.c_x_px, c->intrinsics.c_y_px,
                c->intrinsics.k1, c->intrinsics.k2, c->intrinsics.k3,
            })));
        } else if (c->intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE) {
            camera_model->append_node(doc.allocate_node(node_element, "params", xml_string(doc, Eigen::Matrix<f_t, 5,1>{
                c->intrinsics.f_x_px, c->intrinsics.f_y_px,
                c->intrinsics.c_x_px, c->intrinsics.c_y_px,
                c->intrinsics.w
            })));
        } else if (c->intrinsics.type == rc_CALIBRATION_TYPE_UNDISTORTED) {
            camera_model->append_node(doc.allocate_node(node_element, "params", xml_string(doc, Eigen::Matrix<f_t, 4,1>{
                c->intrinsics.f_x_px, c->intrinsics.f_y_px,
                c->intrinsics.c_x_px, c->intrinsics.c_y_px,
            })));
        }
        camera->append_node(doc.allocate_node(node_element, "pose"));
    }

    {
        node *imu; root->append_node(imu = doc.allocate_node(node_element, "intrinsic_imu_calibration"));
        imu->append_attribute(doc.allocate_attribute("name","Imu"));
        imu->append_attribute(doc.allocate_attribute("imu_id","100"));
        imu->append_attribute(doc.allocate_attribute("serialno","0"));
        imu->append_attribute(doc.allocate_attribute("type",""));
        imu->append_attribute(doc.allocate_attribute("version","1.0"));
        imu->append_attribute(doc.allocate_attribute("is_cad","1"));

        Eigen::Matrix<f_t, 6,1> b_w_b_a = Eigen::Matrix<f_t, 6,1>::Zero();
        b_w_b_a.block<3,1>(0,0) = cal.imu.w_bias_rad__s;
        b_w_b_a.block<3,1>(3,0) = cal.imu.a_bias_m__s2;
        imu->append_node(doc.allocate_node(node_element, "b_w_b_a", xml_string(doc, b_w_b_a)));

        Eigen::Matrix<f_t, 6,6> crossterms = Eigen::Matrix<f_t, 6,6>::Zero();
        crossterms.block<3,3>(0,0) = cal.imu.w_alignment;
        crossterms.block<3,3>(3,3) = cal.imu.a_alignment;
        imu->append_node(doc.allocate_node(node_element, "crossterms", xml_string(doc, crossterms)));

        imu->append_node(doc.allocate_node(node_element, "gyro_noise_sigma", xml_string(doc, cal.imu.w_noise_sigma_rad__s)));
        imu->append_node(doc.allocate_node(node_element, "gyro_bias_sigma",  xml_string(doc, cal.imu.w_bias_sigma_rad__s)));

        imu->append_node(doc.allocate_node(node_element, "accel_noise_sigma", xml_string(doc, cal.imu.a_noise_sigma_m__s2)));
        imu->append_node(doc.allocate_node(node_element, "accel_bias_sigma",  xml_string(doc, cal.imu.a_bias_sigma_m__s2)));
    }

    const struct { const transformation &transformation; const char *A_id, *B_id; } extrinsics[] = {
        { cal.fisheye.extrinsics_wrt_imu_m, "100",  "0" },
        { cal.color.extrinsics_wrt_imu_m,   "100",  "1" },
        { cal.ir.extrinsics_wrt_imu_m,      "100",  "2" },
        { cal.depth.extrinsics_wrt_imu_m,   "100",  "3" },
        { cal.device_wrt_imu_m,             "100", "40" },
        { cal.unity.wrt_device_m,            "40", "41" },
        { cal.opengl.wrt_device_m,           "40", "42" },
        { cal.display.wrt_device_m,          "40", "43" },
    };
    for (auto &e : extrinsics) {
        node *ext_cal; root->append_node(ext_cal = doc.allocate_node(node_element, "extrinsic_calibration"));
        ext_cal->append_attribute(doc.allocate_attribute("name", ""));
        ext_cal->append_attribute(doc.allocate_attribute("index", "0"));
        ext_cal->append_attribute(doc.allocate_attribute("frame_A_id", e.A_id));
        ext_cal->append_attribute(doc.allocate_attribute("frame_B_id", e.B_id));
        ext_cal->append_attribute(doc.allocate_attribute("serialno", "0"));
        ext_cal->append_attribute(doc.allocate_attribute("type", ""));
        ext_cal->append_attribute(doc.allocate_attribute("version", "1.0"));
        ext_cal->append_attribute(doc.allocate_attribute("is_cad", "1"));

        Eigen::Matrix<f_t,3,4> m;
        m.block<3,1>(0,3) = e.transformation.T.block<3,1>(0,0);
        m.block<3,3>(0,0) = to_rotation_matrix(e.transformation.Q).block<3,3>(0,0);
        ext_cal->append_node(doc.allocate_node(node_element, "A_T_B", xml_string(doc, m)));
    }

    {
        if (cal.geo_location.latitude_deg || cal.geo_location.longitude_deg || cal.geo_location.altitude_m) {
            node *geo_location; root->append_node(geo_location = doc.allocate_node(node_element, "geo_location"));
            geo_location->append_node(doc.allocate_node(node_element, "latitude", xml_string(doc, cal.geo_location.latitude_deg)));
            geo_location->append_node(doc.allocate_node(node_element, "longitude", xml_string(doc, cal.geo_location.longitude_deg)));
            geo_location->append_node(doc.allocate_node(node_element, "altitude", xml_string(doc, cal.geo_location.altitude_m)));
        }
    }

    xml.clear();
    print<decltype(std::back_inserter(xml)), char, ' ', 2>(std::back_inserter(xml), doc);
    return true;
}
