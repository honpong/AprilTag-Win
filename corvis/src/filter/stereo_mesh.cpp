#include "stereo_mesh.h"
#include "stereo.h"
#include "filter.h"

bool debug_triangulate_mesh = false;
bool enable_match_occupancy = true;
bool enable_top_n = true;
bool enable_mrf = true;

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

bool stereo_mesh_triangulate(const stereo_mesh & mesh, const stereo &g, int x, int y, v4 & intersection)
{
    vector<struct inddist> distances;

    m4 R = to_rotation_matrix(g.reference->W);
    v4 T = g.reference->T;

    // Get calibrated camera2 point
    v4 point = project_point(x, y, g.center_x, g.center_y, g.focal_length);
    v4 calibrated_point = calibrate_im_point(point, g.k1, g.k2, g.k3);
    if(debug_triangulate_mesh) {
        fprintf(stderr, "point, calibrated_point: ");
        point.print();
        calibrated_point.print();
    }

    // Rotate the point into the world reference frame and translate
    // back to the origin
    v4 line_direction = R*calibrated_point;
    // line_direction is no longer in homogeneous coordinates
    line_direction[3] = 0;
    line_direction = line_direction / norm(line_direction);
    v4 world_point = R*calibrated_point + T;
    v4 o2 = T;
    if(debug_triangulate_mesh) {
        fprintf(stderr, "Line direction, world_point, o2: ");
        line_direction.print();
        world_point.print();
        o2.print();
    }

    bool success = point_mesh_intersect(mesh, o2, line_direction, intersection);

    return success;
}

void stereo_mesh_remove_vertex(stereo_mesh & mesh, int ind)
{
    mesh.vertices.erase(mesh.vertices.begin() + ind);
    mesh.vertices_image.erase(mesh.vertices_image.begin() + ind);
    mesh.correspondences_image.erase(mesh.correspondences_image.begin() + ind);
    mesh.match_scores.erase(mesh.match_scores.begin() + ind);
}

int stereo_mesh_add_vertex(stereo_mesh & mesh, f_t x, f_t y, f_t x2, f_t y2, v4 world, float correspondence_score)
{
    image_coordinate imcoord;
    imcoord.x = x;
    imcoord.y = y;
    image_coordinate previous;
    previous.x = x2;
    previous.y = y2;
    mesh.vertices.push_back(world);
    mesh.vertices_image.push_back(imcoord);
    mesh.correspondences_image.push_back(previous);
    mesh.match_scores.push_back(correspondence_score);
    return (int)mesh.match_scores.size() - 1;
}

void stereo_mesh_write_correspondences(const char * filename, const stereo_mesh & mesh)
{
    FILE * correspondences = fopen(filename, "w");
    if(!correspondences) return;

    fprintf(correspondences, "reference_x, reference_y, target_x, target_y, score\n");
    for(int i = 0; i < mesh.vertices_image.size(); i++)
    {
        image_coordinate reference_pt = mesh.vertices_image[i];
        image_coordinate target_pt = mesh.correspondences_image[i];
        float score = mesh.match_scores[i];
        fprintf(correspondences, "%f, %f, %f, %f, %f\n", reference_pt.x, reference_pt.y, target_pt.x, target_pt.y, score);
    }
    fclose(correspondences);
}

void stereo_mesh_write_json(const char * filename, const stereo_mesh & mesh, const char * texturename)
{
    FILE * vertices = fopen(filename, "w");
    if(!vertices) return;

    fprintf(vertices, "{\n");
    fprintf(vertices, "\"texture_name\": \"%s\",\n", texturename);
    fprintf(vertices, "\"vertices\" : [\n");
    for(int i = 0; i < mesh.vertices.size(); i++)
    {
        v4 vertex = mesh.vertices[i];
        image_coordinate imvertex = mesh.vertices_image[i];
        fprintf(vertices, "[%f, %f, %f, %f, %f, %f]", vertex[0], vertex[1], vertex[2], imvertex.x, imvertex.y, mesh.match_scores[i]);
        if(i == mesh.vertices.size()-1)
            fprintf(vertices, "\n");
        else
            fprintf(vertices, ",\n");
    }

    fprintf(vertices, "],\n");

    fprintf(vertices, "\"faces\" : [\n");
    for(int i = 0; i < mesh.triangles.size(); i++)
    {
        fprintf(vertices, "[%d, %d, %d]", mesh.triangles[i].vertices[0], mesh.triangles[i].vertices[1], mesh.triangles[i].vertices[2]);
        if(i == mesh.triangles.size()-1)
            fprintf(vertices, "\n");
        else
            fprintf(vertices, ",\n");
    }

    fprintf(vertices, "]\n");
    fprintf(vertices, "}\n");
    fclose(vertices);
}

// TODO: Write undistorted coordinates when we are undistorting images in stereo.cpp
void stereo_mesh_write(const char * filename, const stereo_mesh & mesh, const char * texturename)
{
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
    fclose(vertices);
}

// returns false if a line from the origin through each point of the triangle is almost parallel to the triangle
bool check_triangle(const stereo &g, const stereo_mesh & mesh, const stereo_triangle & t)
{
    // triangles that are less than 10 degrees from the viewing angle will be filtered
    const float dot_thresh = cos(M_PI/2 - 10/180. * M_PI);
    m4 R = to_rotation_matrix(g.reference->W);
 
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
        v4 line_direction = R*calibrated_point;
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

    free(in.pointlist);
    free(out.pointlist);
    free(out.trianglelist);
}

void stereo_mesh_delaunay(const stereo &g, stereo_mesh & mesh)
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
        if(check_triangle(g, mesh, t))
            mesh.triangles.push_back(t);
    }

    free(in.pointlist);
    free(out.pointlist);
    free(out.trianglelist);
}

#include "mrf.h"
#include "TRW-S.h"

#define NLABELS (9 + 1)
#define UNKNOWN_LABEL (NLABELS-1)
// TODO: Set these constants and change the type of the function
#define PSI_U 0.07 // unknown labels are .2% of depth difference from mean? was set to 0.002
// slightly less than exp(-1) to try to promote 1 score matches from being labeled unknown
#define PHI_U 0.54 // was 0.04 from paper, but then everything gets set to unknown
static const int mask_shift = 3;
static const int grid_size = 1 << mask_shift;
vector< vector< struct stereo_match > > stereo_grid_matches;
vector<xy> stereo_grid_locations;

MRF::CostVal fnCost(int pix1, int pix2, int i, int j)
{
    if (pix2 < pix1) { // ensure that fnCost(pix1, pix2, i, j) == fnCost(pix2, pix1, j, i)
        int tmp;
        tmp = pix1; pix1 = pix2; pix2 = tmp;
        tmp = i; i = j; j = tmp;
    }
    if(i == UNKNOWN_LABEL && j == UNKNOWN_LABEL)
        return 0;
    else if (i == UNKNOWN_LABEL || j == UNKNOWN_LABEL)
        return PSI_U;

    MRF::CostVal depth1 = stereo_grid_matches[pix1][i].depth;
    MRF::CostVal depth2 = stereo_grid_matches[pix2][j].depth;

    // padded match results may have 0 depth and +1 unary cost
    // force unknown label by setting the penalty high
    if(depth1 == 0 || depth2 == 0) return PHI_U*10;

    xy p1 = stereo_grid_locations[pix1];
    xy p2 = stereo_grid_locations[pix2];
    float deltapixels = sqrt((p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y  - p2.y))/8;

    MRF::CostVal answer = fabs(depth1 - depth2) / ((depth1 + depth2)/2.)/deltapixels;

    return answer;
}

void stereo_mesh_refine_mrf(stereo_mesh & mesh, int width, int height)
{
    MRF* mrf;
    MRF::EnergyVal E;
    double lowerBound;
    float t,tot_t;
    int iter;

    /* Create energy function */
    //The cost of pixel p and label l is
    // stored at cost[p*nLabels+l]
    int npixels = (int)stereo_grid_locations.size();
    float lambda = 1;
    float beta = 1;
    fprintf(stderr, "npixels considered %d\n", npixels);
    MRF::CostVal * unary = new MRF::CostVal[npixels*NLABELS];

    for(int pixel = 0; pixel < npixels; pixel++) {
        for(int label = 0; label < NLABELS-1; label++) { // -1 for unknown label
            // TODO: There is a - here because correspondence score is -1 to 1 instead of 1 to -1
            unary[pixel*NLABELS+label] = lambda * exp(-beta * -stereo_grid_matches[pixel][label].score);
        }
        unary[pixel*NLABELS+NLABELS-1] = PHI_U; // unknown label
    }

    DataCost *data         = new DataCost(unary);
    SmoothnessCost *smooth = new SmoothnessCost(fnCost);
    EnergyFunction *energy = new EnergyFunction(data,smooth);

    mrf = new TRWS(width,height,NLABELS,energy);

    // can disable caching of values of general smoothness function:
    //mrf->dontCacheSmoothnessCosts();

    mrf->initialize();
    mrf->clearAnswer();


    E = mrf->totalEnergy();
    printf("Energy at the Start= %g (%g,%g)\n", (float)E,
		   (float)mrf->smoothnessEnergy(), (float)mrf->dataEnergy());

    tot_t = 0;
    for (iter=0; iter<10; iter++) {
		mrf->optimize(10, t);

		E = mrf->totalEnergy();
		lowerBound = mrf->lowerBound();
		tot_t = tot_t + t ;
		printf("energy = %g, lower bound = %f (%f secs)\n", (float)E, lowerBound, tot_t);
    }

    MRF::Label * labels = mrf->getAnswerPtr();
    for(int i = 0; i < width*height; i++) {
        MRF::Label label = labels[i];
        if(label != UNKNOWN_LABEL) {
            xy pt = stereo_grid_locations[i];
            struct stereo_match match = stereo_grid_matches[i][label];
            stereo_mesh_add_vertex(mesh, pt.x, pt.y, match.x, match.y, match.point, match.score);
        }
    }


    delete unary;
    delete smooth;
    delete data;
    delete energy;
    delete mrf;
}

void stereo_mesh_add_gradient_grid(stereo_mesh & mesh, const stereo &g, int npoints, void (*progress_callback)(float))
{
    int m_width = g.width / grid_size;
    int m_height = g.height / grid_size;

    stereo_grid_locations.clear();
    stereo_grid_matches.clear();
    for(int y = 0; y < g.height; y+=grid_size) {
        for(int x = 0; x < g.width; x+=grid_size) {
            int best_x = x;
            int best_y = y;
            float best_gradient = 0;
            for(int dy = 1; dy < grid_size; dy++) {
                for(int dx = 1; dx < grid_size; dx++) {
                    int row = y + dy;
                    int col = x + dx;
                    float gx = ((float)g.reference->image[row*g.width+col] - (float)g.reference->image[row*g.width+ (col-1)])/2.;
                    float gy = ((float)g.reference->image[row*g.width+col] - (float)g.reference->image[(row-1)*g.width + col])/2.;
                    float mag = sqrt(gx*gx + gy*gy);
                    if(mag > best_gradient) {
                        best_gradient = mag;
                        best_x = col;
                        best_y = row;
                    }
                }
            }
            xy pt;
            pt.x = best_x;
            pt.y = best_y;
            stereo_grid_locations.push_back(pt);
            vector<struct stereo_match> matches;
            g.triangulate_top_n(pt.x, pt.y, NLABELS-1, matches);
            stereo_grid_matches.push_back(matches);
        }
        if(progress_callback) {
            float progress = (float)y / g.height;
            progress_callback(progress);
        }
    }

    if(!enable_mrf) {
        for(int i = 0; i < stereo_grid_matches.size(); i++) {
            struct stereo_match match = stereo_grid_matches[i][0];
            xy pt = stereo_grid_locations[i];
            if(match.score < 1) {
                stereo_mesh_add_vertex(mesh, pt.x, pt.y, match.x, match.y, match.point, match.score);
            }
        }
    }
    else {
        stereo_mesh_refine_mrf(mesh, m_width, m_height);
    }
}

void stereo_mesh_add_gradient(stereo_mesh & mesh, const stereo &g, int npoints, void (*progress_callback)(float))
{
    // TODO: what is the best mask_shift value
    int mask_shift = 2;
    int grid_size = 1 << mask_shift;
    int m_width = g.width / grid_size;
    int m_height = g.height / grid_size;

    bool * match_occupied = (bool *)calloc(m_width*m_height, sizeof(bool));
    float * match_scores = (float *)calloc(m_width*m_height, sizeof(float));
    int * match_ind = (int *)calloc(m_width*m_height, sizeof(int));

    vector<xy> points;
    bool success;
    v4 intersection;

    xy pt;
    for(int row = 1; row < g.height; row++)
        for(int col = 1; col < g.width; col++)
        {
            float dx = ((float)g.reference->image[row*g.width+col] - (float)g.reference->image[row*g.width+ (col-1)])/2.;
            float dy = ((float)g.reference->image[row*g.width+col] - (float)g.reference->image[(row-1)*g.width + col])/2.;
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
        struct stereo_match match;

        if(enable_top_n) {
            vector<struct stereo_match> matches;
            success = g.triangulate_top_n(pt.x, pt.y, NLABELS-1, matches);
            if(success) {
                match = matches[0];
                intersection = match.point;
            }
        }
        else
            success = g.triangulate(pt.x, pt.y, intersection, &match);

        if(success) {
            if(!enable_match_occupancy) {
                stereo_mesh_add_vertex(mesh, pt.x, pt.y, match.x, match.y, intersection, match.score);
            }
            else {
                int x2m = match.x / grid_size;
                int y2m = match.y / grid_size;
                int mind = y2m * m_width + x2m;
                if(!match_occupied[mind]) {
                    match_occupied[mind] = true;
                    match_ind[mind] = stereo_mesh_add_vertex(mesh, pt.x, pt.y, match.x, match.y, intersection, match.score);
                    match_scores[mind] = match.score;
                }
                else if(match_scores[mind] > match.score) {
                    stereo_mesh_remove_vertex(mesh, match_ind[mind]);
                    match_ind[mind] = stereo_mesh_add_vertex(mesh, pt.x, pt.y, match.x, match.y, intersection, match.score);
                    match_scores[mind] = match.score;
                }
            }
        }
        if(progress_callback) {
            float progress = (float)i / nchosen;
            progress_callback(progress);
        }
    }

    free(match_occupied);
    free(match_scores);
    free(match_ind);
}

void stereo_mesh_add_grid(stereo_mesh & mesh, const stereo &g, int step, void (*progress_callback)(float))
{
    bool success;
    v4 intersection;
    struct stereo_match match;

    for(int row = 0; row < g.height; row += step) {
        for(int col=0; col < g.width; col += step) {
            if(progress_callback) {
                float progress = (1.*col + row*g.width)/(g.height*g.width);
                progress_callback(progress);
            }
            success = g.triangulate(col, row, intersection, &match);
            if(success) {
                stereo_mesh_add_vertex(mesh, col, row, match.x, match.y, intersection, match.score);
            }
        }
    }
}

#include "tracker.h"
void stereo_mesh_add_features(stereo_mesh & mesh, const stereo &g, int maxvertices, void (*progress_callback)(float))
{
    bool success;
    v4 intersection;

    fast_detector_9 fast;
    fast.init(640, 480, 640);

    int bthresh = 30;
    vector<xy> features = fast.detect(g.reference->image, NULL, maxvertices, bthresh, 0, 0, 640, 480);

    for(int i = 0; i < features.size(); i++) {
            if(progress_callback)
                progress_callback(i*1./features.size());
            struct stereo_match match;
            success = g.triangulate(features[i].x, features[i].y, intersection, &match);
            if(success) {
                stereo_mesh_add_vertex(mesh, features[i].x, features[i].y, match.x, match.y, intersection, match.score);
            }
    }
}

stereo_mesh stereo_mesh_create(const stereo &g, void(*progress_callback)(float))
{
    stereo_mesh mesh;
    stereo_mesh_add_gradient_grid(mesh, g, 2000, progress_callback);
    //stereo_mesh_add_gradient(mesh, g, 2000, progress_callback);
    //stereo_mesh_add_features(mesh, g, 500);
    //stereo_mesh_add_grid(mesh, g, 10, progress_callback);
    stereo_mesh_delaunay(g, mesh);
    return mesh;
}
