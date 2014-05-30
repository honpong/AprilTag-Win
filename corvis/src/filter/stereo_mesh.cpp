#include "stereo_mesh.h"
#include "filter.h"

bool debug_triangulate_mesh = false;

extern void v4_pp(const char * name, v4 vec);
extern void m4_pp(const char * name, m4 mat);
extern f_t estimate_kr(v4 point, f_t k1, f_t k2, f_t k3);
extern v4 calibrate_im_point(v4 normalized_point, float k1, float k2, float k3);
extern v4 project_point(f_t x, f_t y, f_t center_x, f_t center_y, float focal_length);

struct inddist {
    uint32_t index;
    float distance;
};

bool compare_inddist(struct inddist i1, struct inddist i2)
{
    return i1.distance < i2.distance;
}

bool ray_triangle_intersect(const v4 & p, const v4 & d, const v4 & v0, const v4 & v1, const v4 & v2, v4 & intersection) {
	float t,u,v;
    intersection = v4(0,0,0,0);
    
    v4 e1 = v1 - v0;
    v4 e2 = v2 - v0;
    v4 h = cross(d, e2);
    float a = sum(e1*h);

	if (a > -0.00001 && a < 0.00001)
		return false;

	float f = 1/a;
    v4 s = p - v0;
	u = f * sum(s*h);

	if (u < 0.0 || u > 1.0)
		return false;

    v4 q = cross(s, e1);
	v = f * sum(d*q);

	if (v < 0.0 || u + v > 1.0)
		return false;

	// at this stage we can compute t to find out where
	// the intersection point is on the line
	t = f * sum(e2 * q);

	if (t > 0.00001) { // ray intersection
        intersection = p + t*d;
		return true;
    }

    // this means that there is a line intersection
    // but not a ray intersection
    return false;
}

bool point_mesh_intersect(const stereo_mesh & mesh, const v4 & p0, const v4 & d, v4 & intersection)
{
    v4 points[3];
    for(int i = 0; i < mesh.triangles.size(); i++) {
        stereo_triangle t = mesh.triangles[i];
        points[0] = mesh.vertices[t.vertices[0]];
        points[1] = mesh.vertices[t.vertices[1]];
        points[2] = mesh.vertices[t.vertices[2]];

        // intersect triangle with p0 and d
        if(ray_triangle_intersect(p0, d, points[0], points[1], points[2], intersection)) {
            return true;
        }
    }
    return false;
}

bool stereo_mesh_triangulate(const stereo_mesh & mesh, stereo &g, const stereo_frame & s1, const stereo_frame & s2, int x, int y, v4 & intersection)
{
    vector<struct inddist> distances;

    m4 R1w = to_rotation_matrix(s1.W);
    m4 R2w = to_rotation_matrix(s2.W);
    v4 s1T = s1.T;
    v4 s2T = s2.T;

    if(debug_triangulate_mesh) {
        fprintf(stderr, "x = %d; y = %d;\n", x, y);
        m4_pp("R1w", R1w);
        m4_pp("R2w", R2w);
        v4_pp("s1T", s1T);
        v4_pp("s2T", s2T);

        fprintf(stderr, "s1c = [%f %f];\n", g.center_x, g.center_y);
        fprintf(stderr, "s1f = %f;\n", g.focal_length);
        fprintf(stderr, "s2c = [%f %f];\n", g.center_x, g.center_y);
        fprintf(stderr, "s2f = %f;\n", g.focal_length);
    }

    // Get calibrated camera2 point
    v4 point = project_point(x, y, g.center_x, g.center_y, g.focal_length);
    v4 calibrated_point = calibrate_im_point(point, g.k1, g.k2, g.k3);
    if(debug_triangulate_mesh) {
        v4_pp("point", point);
        v4_pp("calibrated_point", calibrated_point);
    }

    // Rotate the point into the world reference frame and translate
    // back to the origin
    v4 line_direction = R2w*calibrated_point;
    // line_direction is no longer in homogeneous coordinates
    line_direction[3] = 0;
    line_direction = line_direction / norm(line_direction);
    v4 world_point = R2w*calibrated_point + s2T;
    v4 o2 = s2T;
    if(debug_triangulate_mesh) {
        v4_pp("line_direction", line_direction);
        v4_pp("world_point", world_point);
        v4_pp("o2", o2);
    }

    bool success = point_mesh_intersect(mesh, o2, line_direction, intersection);

    return success;
}

void stereo_mesh_add_vertex(stereo_mesh & mesh, f_t x, f_t y, v4 world, float correspondence_score)
{
    image_coordinate imcoord;
    imcoord.x = x;
    imcoord.y = y;
    mesh.vertices.push_back(world);
    mesh.vertices_image.push_back(imcoord);
    mesh.match_scores.push_back(correspondence_score);
}

void stereo_mesh_write(const char * filename, const stereo_mesh & mesh, const char * texturename)
{
    fprintf(stderr, "writing mesh to %s\n", filename);
    FILE * vertices = fopen(filename, "w");
    if(!vertices) return;
        
    fprintf(vertices, "ply\n");
    fprintf(vertices, "format ascii 1.0\n");
    if(texturename)
    fprintf(vertices, "comment TextureFile %s\n", texturename);
    fprintf(vertices, "element vertex %lu\n", mesh.vertices.size());
    fprintf(vertices, "property float x\n");
    fprintf(vertices, "property float y\n");
    fprintf(vertices, "property float z\n");
    fprintf(vertices, "property float imx\n");
    fprintf(vertices, "property float imy\n");
    fprintf(vertices, "property float match_score\n");
    fprintf(vertices, "element face %lu\n", mesh.triangles.size());
    fprintf(vertices, "property list uchar int vertex_index\n");
    fprintf(vertices, "property list uchar float texcoord\n");
    fprintf(vertices, "end_header\n");

    for(int i = 0; i < mesh.vertices.size(); i++)
    {
        v4 vertex = mesh.vertices[i];
        image_coordinate imvertex = mesh.vertices_image[i];
        fprintf(vertices, "%f %f %f %f %f %f\n", vertex[0], vertex[1], vertex[2], imvertex.x, imvertex.y, mesh.match_scores[i]);
    }

    for(int i = 0; i < mesh.triangles.size(); i++)
    {
        fprintf(vertices, "3 %d %d %d\n", mesh.triangles[i].vertices[0], mesh.triangles[i].vertices[1], mesh.triangles[i].vertices[2]);
        image_coordinate im0 = mesh.vertices_image[mesh.triangles[i].vertices[0]];
        image_coordinate im1 = mesh.vertices_image[mesh.triangles[i].vertices[1]];
        image_coordinate im2 = mesh.vertices_image[mesh.triangles[i].vertices[2]];
        fprintf(vertices, "6 %f %f %f %f %f %f\n", im0.x/640., 1-im0.y/480., im1.x/640., 1-im1.y/480., im2.x/640., 1-im2.y/480);
    }
    fprintf(stderr, "wrote %lu vertices", mesh.vertices.size());
    fclose(vertices);
}

// returns false if a line from the origin through each point of the triangle is almost parallel to the triangle
bool check_triangle(const stereo &g, const stereo_mesh & mesh, const stereo_triangle & t, const stereo_frame & s2)
{
    // triangles that are less than 10 degrees from the viewing angle will be filtered
    const float dot_thresh = cos(M_PI/2 - 10/180. * M_PI);
    m4 R2w = to_rotation_matrix(s2.W);
 
    v4 v0 = mesh.vertices[t.vertices[0]];
    v4 v1 = mesh.vertices[t.vertices[1]];
    v4 v2 = mesh.vertices[t.vertices[2]];
    v4 normal = cross(v1 - v0, v2 - v0);
    normal = normal / norm(normal);
    
    for(int v = 0; v < 3; v++) {
        float x = mesh.vertices_image[t.vertices[v]].x;
        float y = mesh.vertices_image[t.vertices[v]].y;
        
        // Get calibrated camera2 point
        v4 point = project_point(x, y, g.center_x, g.center_y, g.focal_length);
        v4 calibrated_point = calibrate_im_point(point, g.k1, g.k2, g.k3);
        
        // Rotate the direction into the world reference frame and translate
        // back to the origin
        v4 line_direction = R2w*calibrated_point;
        // line_direction is no longer in homogeneous coordinates
        line_direction[3] = 0;
        line_direction = line_direction / norm(line_direction);
        float dot = fabs(sum(normal*line_direction));
        if(dot < dot_thresh) {
            return false;
        }
    }

    return true;
}

#include "triangle.h"
void stereo_remesh_delaunay(stereo_mesh & mesh)
{
    // check which vertices have been used by triangles that we kept
    vector<int> occurences(mesh.vertices.size(), 0);

    for(int i = 0; i < mesh.triangles.size(); i++) {
        stereo_triangle t = mesh.triangles[i];
        occurences[t.vertices[0]]++;
        occurences[t.vertices[1]]++;
        occurences[t.vertices[2]]++;
    }
    
    char triswitches[] = "zQB";
    struct triangulateio in, out;
    in.pointlist = (float *)malloc(sizeof(float)*2*mesh.vertices_image.size());
    in.numberofpoints = (int)mesh.vertices_image.size();
    in.pointmarkerlist = NULL;
    in.numberofpointattributes = 0;
    out.pointlist = NULL;
    out.trianglelist = NULL;
    out.edgelist = NULL;
    out.normlist = NULL;
    
    int z = 0;
    vector<int> vertex_mapping;
    for(int i = 0; i < mesh.vertices_image.size(); i++) {
        if(occurences[i] > 1) {
            in.pointlist[z*2] = mesh.vertices_image[i].x;
            in.pointlist[z*2+1] = mesh.vertices_image[i].y;
            z++;
            vertex_mapping.push_back(i);
        }
    }
    in.numberofpoints = z;
    triangulate(triswitches, &in, &out, NULL);
    
    
    mesh.triangles.clear();
    for(int i = 0; i < out.numberoftriangles; i++) {
        stereo_triangle t;
        t.vertices[0] = vertex_mapping[out.trianglelist[i*3]];
        t.vertices[1] = vertex_mapping[out.trianglelist[i*3+1]];
        t.vertices[2] = vertex_mapping[out.trianglelist[i*3+2]];
        mesh.triangles.push_back(t);
    }
    fprintf(stderr, "Formed %lu triangles\n", mesh.triangles.size());

    free(in.pointlist);
    free(out.pointlist);
    free(out.trianglelist);
}

void stereo_mesh_delaunay(stereo &g, stereo_mesh & mesh, const stereo_frame & s2)
{
    char triswitches[] = "zQB";
    struct triangulateio in, out;
    in.pointlist = (float *)malloc(sizeof(float)*2*mesh.vertices_image.size());
    in.numberofpoints = (int)mesh.vertices_image.size();
    in.pointmarkerlist = NULL;
    in.numberofpointattributes = 0;
    out.pointlist = NULL;
    out.trianglelist = NULL;
    out.edgelist = NULL;
    out.normlist = NULL;
    for(int i = 0; i < mesh.vertices_image.size(); i++) {
        in.pointlist[i*2] = mesh.vertices_image[i].x;
        in.pointlist[i*2+1] = mesh.vertices_image[i].y;
    }
    triangulate(triswitches, &in, &out, NULL);
    for(int i = 0; i < out.numberoftriangles; i++) {
        stereo_triangle t;
        t.vertices[0] = out.trianglelist[i*3];
        t.vertices[1] = out.trianglelist[i*3+1];
        t.vertices[2] = out.trianglelist[i*3+2];
        if(check_triangle(g, mesh, t, s2))
            mesh.triangles.push_back(t);
    }
    fprintf(stderr, "Kept %lu of %d triangles\n", mesh.triangles.size(), out.numberoftriangles);
    
    free(in.pointlist);
    free(out.pointlist);
    free(out.trianglelist);
}

void stereo_mesh_add_gradient(stereo_mesh & mesh, stereo &g, const stereo_frame & s1, const stereo_frame & s2, m4 F, int npoints, void (*progress_callback)(float))
{
    vector<xy> points;
    bool success;
    v4 intersection;
    float correspondence_score;

    xy pt;
    for(int row = 1; row < g.height; row++)
        for(int col = 1; col < g.width; col++)
        {
            float dx = ((float)s2.image[row*g.width+col] - (float)s2.image[row*g.width+ (col-1)])/2.;
            float dy = ((float)s2.image[row*g.width+col] - (float)s2.image[(row-1)*g.width + col])/2.;
            float mag = sqrt(dx*dx + dy*dy);
            if(mag > 5) {
                pt.x = col;
                pt.y = row;
                points.push_back(pt);
            }
        }

    std::random_shuffle(points.begin(), points.end());
    size_t nchosen = points.size();
    if(npoints < points.size())
        nchosen = npoints;

    for(size_t i = 0; i < nchosen; i++)
    {
        pt = points[i];
        success = g.triangulate(pt.x, pt.y, intersection, &correspondence_score);
        if(success) {
            stereo_mesh_add_vertex(mesh, pt.x, pt.y, intersection, correspondence_score);
        }
        if(progress_callback) {
            float progress = (float)i / nchosen;
            progress_callback(progress);
        }
    }
}

void stereo_mesh_add_grid(stereo_mesh & mesh, stereo &g, const stereo_frame & s1, const stereo_frame & s2, m4 F, int step, void (*progress_callback)(float))
{
    bool success;
    v4 intersection;
    float correspondence_score;

    for(int row = 0; row < g.height; row += step) {
        for(int col=0; col < g.width; col += step) {
            if(progress_callback) {
                float progress = (1.*col + row*g.width)/(g.height*g.width);
                progress_callback(progress);
            }
            success = g.triangulate(col, row, intersection, &correspondence_score);
            if(success) {
                stereo_mesh_add_vertex(mesh, col, row, intersection, correspondence_score);
            }
        }
    }
}

#include "tracker.h"
void stereo_mesh_add_features(stereo_mesh & mesh, stereo &g, const stereo_frame & s1, const stereo_frame & s2, m4 F, int maxvertices, void (*progress_callback)(float))
{
    bool success;
    v4 intersection;
    float correspondence_score;

    fast_detector_9 fast;
    fast.init(640, 480, 640);

    int bthresh = 30;
    vector<xy> features = fast.detect(s2.image, NULL, maxvertices, bthresh, 0, 0, 640, 480);

    fprintf(stderr, "%lu features detected\n", features.size());
    for(int i = 0; i < features.size(); i++) {
            if(progress_callback)
                progress_callback(i*1./features.size());
            success = g.triangulate(features[i].x, features[i].y, intersection, &correspondence_score);
            if(success) {
                stereo_mesh_add_vertex(mesh, features[i].x, features[i].y, intersection, correspondence_score);
            }
    }
}

stereo_mesh stereo_mesh_states(stereo &g, const stereo_frame & s1, const stereo_frame & s2, m4 F, void(*progress_callback)(float))
{
    stereo_mesh mesh;
    //stereo_mesh_add_features(mesh, s1, s2, F, 500);
    //fprintf(stderr, "Valid feature vertices: %lu\n", mesh.vertices.size());
    stereo_mesh_add_gradient(mesh, g, s1, s2, F, 2000, progress_callback);
    //stereo_mesh_add_grid(mesh, g, s1, s2, F, 10, progress_callback);
    fprintf(stderr, "Valid grid vertices: %lu\n", mesh.vertices.size());
    stereo_mesh_delaunay(g, mesh, s2);
    return mesh;
}
