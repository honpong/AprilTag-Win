#include "stereo_mesh.h"
#include "stereo.h"
#include "../filter/fast_detector/fast.h"
#include <iostream>

bool debug_triangulate_mesh = false;
bool debug_mrf = false;
bool enable_match_occupancy = true;
bool enable_top_n = true;
bool enable_mrf = true;

struct inddist {
    uint32_t index;
    float distance;
};

bool compare_inddist(struct inddist i1, struct inddist i2)
{
    return i1.distance < i2.distance;
}

bool ray_triangle_intersect(const v3 & p, const v3 & d, const v3 & v0, const v3 & v1, const v3 & v2, v3 & intersection) {
	float t,u,v;
    intersection = v3(0,0,0);
    
    v3 e1 = v1 - v0;
    v3 e2 = v2 - v0;
    v3 h = d.cross(e2);
    float a = e1.dot(h);

	if (a > -0.00001 && a < 0.00001)
		return false;

	float f = 1/a;
    v3 s = p - v0;
	u = f * s.dot(h);

	if (u < 0.0 || u > 1.0)
		return false;

    v3 q = s.cross(e1);
	v = f * d.dot(q);

	if (v < 0.0 || u + v > 1.0)
		return false;

	// at this stage we can compute t to find out where
	// the intersection point is on the line
	t = f * e2.dot(q);

	if (t > 0.00001) { // ray intersection
        intersection = p + t*d;
		return true;
    }

    // this means that there is a line intersection
    // but not a ray intersection
    return false;
}

bool point_mesh_intersect(const stereo_mesh & mesh, const v3 & p0, const v3 & d, v3 & intersection)
{
    v3 points[3];
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

bool stereo_mesh_triangulate(const stereo_mesh & mesh, const stereo &g, int x, int y, v3 & intersection)
{
    vector<struct inddist> distances;

    m3 R = to_rotation_matrix(g.reference->W);
    v3 T = g.reference->T;

    // Get calibrated camera2 point
    v3 calibrated_point = g.camera.calibrate_image_point(x, y);
    if(debug_triangulate_mesh) {
        std::cerr << "calibrated_point: " << calibrated_point;
    }

    // Rotate the point into the world reference frame and translate
    // back to the origin
    v3 line_direction = R*calibrated_point;
    // line_direction is no longer in homogeneous coordinates
    line_direction[3] = 0;
    line_direction = line_direction.normalized();
    v3 world_point = R*calibrated_point + T;
    v3 o2 = T;
    if(debug_triangulate_mesh) {
        fprintf(stderr, "Line direction, world_point, o2: ");
        std::cerr << line_direction << world_point << o2;
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

int stereo_mesh_add_vertex(stereo_mesh & mesh, f_t x, f_t y, f_t x2, f_t y2, v3 world, float correspondence_score)
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

void stereo_mesh_write_rotated_json(const char * filename, const stereo_mesh & mesh, const stereo & g, int degrees, const char * texturename)
{
    // Calculate rotation
    m3 R;
    float radians = degrees*M_PI/180;
    R(0, 0) = cos(radians); R(0, 1) = -sin(radians); R(0, 2) = 0;
    R(1, 0) = sin(radians); R(1, 1) = cos(radians); R(1, 2) = 0;
    R(2, 0) = 0; R(2, 1) = 0; R(2, 2) = 1;

    // Transform center point to rotated pixel coordinates
    // rotate around width/2 height/2
    v3 image_midpoint = v3(g.camera.width/2, g.camera.height/2., 0);
    v3 image_midpoint_rotated = image_midpoint;
    bool is_landscape = degrees == 0 || degrees == 180;
    if(!is_landscape) {
        image_midpoint_rotated[0] = image_midpoint[1];
        image_midpoint_rotated[1] = image_midpoint[0];
    }

    v3 image_center = v3(g.camera.center_x, g.camera.center_y, 0);

    image_center = R*(image_center - image_midpoint) + image_midpoint_rotated;

    FILE * vertices = fopen(filename, "w");
    if(!vertices) return;

    fprintf(vertices, "{\n");
    fprintf(vertices, "\"texture_name\": \"%s\",\n", texturename);
    fprintf(vertices, "\"rotation_degrees\": %d,\n", degrees);
    fprintf(vertices, "\"center\": [%g, %g],\n", image_center[0], image_center[1]);
    fprintf(vertices, "\"focal_length\": %g,\n", g.camera.focal_length);
    fprintf(vertices, "\"k1\": %g,\n", g.camera.k1);
    fprintf(vertices, "\"k2\": %g,\n", g.camera.k2);
    fprintf(vertices, "\"k3\": %g,\n", g.camera.k3);

    m3 Rr = g.Rw*R.transpose(); // Rotation to world frame including undoing camera rotation
    fprintf(vertices, "\"world\": { ");
    fprintf(vertices, "\"R\" : [[%g, %g, %g], \n", Rr(0, 0), Rr(0, 1), Rr(0, 2));
    fprintf(vertices, "         [%g, %g, %g], \n", Rr(1, 0), Rr(1, 1), Rr(1, 2));
    fprintf(vertices, "         [%g, %g, %g], \n", Rr(2, 0), Rr(2, 1), Rr(2, 2));
    fprintf(vertices, "\"T\": [%g, %g, %g] }, \n", g.Tw[0], g.Tw[1], g.Tw[2]);

    v3 gravity = Rr.transpose()*v3(0,0,-1);
    fprintf(vertices, "\"gravity\": [%g, %g, %g], \n", gravity[0], gravity[1], gravity[2]);

    fprintf(vertices, "\"vertices\" : [\n");
    for(int i = 0; i < mesh.vertices.size(); i++)
    {
        v3 vertex = R*mesh.vertices[i]; // world coordinates are centered at 0,0
        v3 imvertex = v3(mesh.vertices_image[i].x, mesh.vertices_image[i].y, 0);
        imvertex = R*(imvertex - image_midpoint) + image_midpoint_rotated;
        fprintf(vertices, "[%f, %f, %f, %f, %f, %f]", vertex[0], vertex[1], vertex[2], imvertex[0], imvertex[1], mesh.match_scores[i]);
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
        v3 vertex = mesh.vertices[i];
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
    m3 R = to_rotation_matrix(g.reference->W);
 
    v3 v0 = mesh.vertices[t.vertices[0]];
    v3 v1 = mesh.vertices[t.vertices[1]];
    v3 v2 = mesh.vertices[t.vertices[2]];
    v3 normal = (v1 - v0).cross(v2 - v0);
    normal = normal.normalized();
    
    for(int v = 0; v < 3; v++) {
        float x = mesh.vertices_image[t.vertices[v]].x;
        float y = mesh.vertices_image[t.vertices[v]].y;
        
        // Get calibrated camera2 point
        v3 calibrated_point = g.camera.calibrate_image_point(x, y);
        
        // Rotate the direction into the world reference frame and translate
        // back to the origin
        v3 line_direction = R*calibrated_point;
        // line_direction is no longer in homogeneous coordinates
        line_direction[3] = 0;
        line_direction = line_direction.normalized();
        float dot = fabs(normal.dot(line_direction));
        if(dot < dot_thresh) {
            return false;
        }
    }

    return true;
}

extern "C" {
    #include "libqhull.h"
    #include "mem.h"
    #include "qset.h"
}

vector<stereo_triangle> qhull_mesh_points(vector<image_coordinate> coordinates)
{
    vector<stereo_triangle> triangles;
    if(coordinates.size() < 3)
        return triangles;

    int TOTpoints = (int)coordinates.size();
    int dim= 2;             /* dimension of points */
    int numpoints;            /* number of points */
    // TODO: why is this dim+1
    coordT points[(dim+1)*TOTpoints]; /* array of coordinates for each point */
    coordT *rows[TOTpoints];
    boolT ismalloc= False;    /* True if qhull should free points in qh_freeqhull() or reallocation */
    char flags[250] = "qhull d Qt Qbb Qc Qz";  /* "d" delaunay triangulation "Qt" forces triangle output rest copied from octave */
    /* debug options "s" summary "Tc" check frequently "Tv" verify results */
    /* for full option flags see qh-quick.htm */
    //char flags[250] = "qhull d s d Tcv";
    //must also set outfile=stdout and errfile=stderr to use this
    FILE *outfile= NULL;    /* output from qh_produce_output()
                             use NULL to skip qh_produce_output() */
    FILE *errfile= NULL;    /* error messages from qhull code */
    int exitcode;             /* 0 if no error from qhull */
    facetT *facet;            /* set by FORALLfacets */
    vertexT *vertex, **vertexp; /* set by FOREACHvertex_ */
    int curlong, totlong;     /* memory remaining after qh_memfreeshort */
    int i;

    numpoints = TOTpoints;

    /* Set points */
    for(i = 0; i < numpoints; i++) {
        points[i*dim] = coordinates[i].x;
        points[i*dim+1] = coordinates[i].y;
    }

    /* Set up rows */
    for (i=0; i < numpoints; i++)
        rows[i]= points+dim*i;

    /* Create qhull */
    exitcode = qh_new_qhull (dim, numpoints, points, ismalloc,
                            flags, outfile, errfile);

    if (!exitcode) {                  /* if no error */
        qh_triangulate(); // Octave did this, but the example code did not
        
        FORALLfacets {
            int z = 0;
            stereo_triangle t;
            // a simplical non upper delaunay facet should be a nice triangle
            if (!facet->upperdelaunay && facet->simplicial) {
                FOREACHvertex_(facet->vertices) {
                    int id = qh_pointid (vertex->point);
                    t.vertices[z] = id;
                    z++;
                }
                triangles.push_back(t);
            }
        }
    }
    qh_freeqhull(!qh_ALL);                 /* free long memory */
    qh_memfreeshort (&curlong, &totlong);  /* free short memory and memory allocator */
    if (curlong || totlong)
        fprintf (errfile, "qhull internal warning (user_eg, #2): did not free %d bytes of long memory (%d pieces)\n", totlong, curlong);

    return triangles;
}

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

    vector<image_coordinate> remesh_vertices;
    vector<int> vertex_mapping;
    for(int i = 0; i < mesh.vertices_image.size(); i++) {
        if(occurences[i] > 1) {
            remesh_vertices.push_back(mesh.vertices_image[i]);
            vertex_mapping.push_back(i);
        }
    }

    vector<stereo_triangle> triangles = qhull_mesh_points(remesh_vertices);

    mesh.triangles.clear();
    for(int i = 0; i < triangles.size(); i++) {
        stereo_triangle t;
        t.vertices[0] = vertex_mapping[triangles[i].vertices[0]];
        t.vertices[1] = vertex_mapping[triangles[i].vertices[1]];
        t.vertices[2] = vertex_mapping[triangles[i].vertices[2]];
        mesh.triangles.push_back(t);
    }
}


void stereo_mesh_delaunay(const stereo &g, stereo_mesh & mesh)
{
    vector<stereo_triangle> triangles = qhull_mesh_points(mesh.vertices_image);
    for(int t = 0; t < triangles.size(); t++)
        if(check_triangle(g, mesh, triangles[t]))
            mesh.triangles.push_back(triangles[t]);
}

static const int NLABELS = 9 + 1;
static const int UNKNOWN_LABEL = NLABELS - 1;
static const float PSI_U = 0.07; // unknown labels are .2% of depth difference from mean? was set to 0.002
// slightly less than exp(-1) to try to promote 1 score matches from being labeled unknown
// now expressed in cost units
static const float PHI_U = 0.5; // was 0.04 from paper, but then everything gets set to unknown
static const float PAIRWISE_LAMBDA = 1;
static const float PAIRWISE_BETA = 2.3;

static const int mask_shift = 3;
static const int grid_size = 1 << mask_shift;
vector< vector< struct stereo_match > > stereo_grid_matches;
vector<xy> stereo_grid_locations;

double pairwise_cost(int pix1, int pix2, int i, int j)
{
    // lambda scales the tradeoff between unary and pairwise
    // beta scales where on the exponential these distances live
    if (pix2 < pix1) { // ensure that fnCost(pix1, pix2, i, j) == fnCost(pix2, pix1, j, i)
        int tmp;
        tmp = pix1; pix1 = pix2; pix2 = tmp;
        tmp = i; i = j; j = tmp;
    }
    if(i == UNKNOWN_LABEL && j == UNKNOWN_LABEL)
        return PAIRWISE_LAMBDA*exp(-0);
    else if (i == UNKNOWN_LABEL || j == UNKNOWN_LABEL)
        return PAIRWISE_LAMBDA*exp(-PAIRWISE_BETA * PSI_U);

    float depth1 = stereo_grid_matches[pix1][i].depth;
    float depth2 = stereo_grid_matches[pix2][j].depth;

    // padded match results may have 0 depth and +1 unary cost
    // force unknown label by setting the penalty high
    if(depth1 == 0 || depth2 == 0)
        return PAIRWISE_LAMBDA*exp(-PAIRWISE_BETA * PSI_U*10);

    v3 point1 = stereo_grid_matches[pix1][i].point;
    v3 point2 = stereo_grid_matches[pix2][j].point;
    float dist = (point1 - point2).norm();

    xy p1 = stereo_grid_locations[pix1];
    xy p2 = stereo_grid_locations[pix2];
    float deltapixels = sqrt((p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y  - p2.y))/8;

    float answer = fabs(depth1 - depth2) / ((depth1 + depth2)/2.)/deltapixels;
    answer =  dist / ((depth1 + depth2)/2)/deltapixels;

    return PAIRWISE_LAMBDA*exp(-PAIRWISE_BETA * answer);
}

double unary_cost(int pixel, int label)
{
    //unary_cost is in range 1 (most likely) to 0 (not happening)
    if(label == UNKNOWN_LABEL)
        return (PHI_U + 1)/2;

    return (-stereo_grid_matches[pixel][label].score + 1)/2;
}

vector<int> pixel_neighbors(int pixel) {
    vector<int> neighbors;
    int row_size = 640/grid_size;
    if(pixel+1 < stereo_grid_locations.size()) neighbors.push_back(pixel+1);
    if(pixel+row_size < stereo_grid_locations.size()) neighbors.push_back(pixel+row_size);
    // Only add lower and right neighbors since we do this once for each pixel, avoids duplicate edges
//    if(pixel-1 >= 0) neighbors.push_back(pixel-1);
//    if(pixel-row_size >= 0) neighbors.push_back(pixel-row_size);

    return neighbors;
}

#include <dai/alldai.h>
#include <dai/trwbp.h>

using namespace std;
using namespace dai;

void stereo_mesh_refine_mrf(stereo_mesh & mesh, int width, int height, void (*progress_callback)(float), float start_progress, float end_progress)
{
    int npixels = (int)stereo_grid_locations.size();
    int niterations = 5;

    vector<Var> vars;
    vector<Factor> factors;

    vars.reserve(npixels);
    for(int pixel = 0; pixel < npixels; pixel++)
        vars.push_back( Var(pixel, NLABELS) );

    Real unary[NLABELS];
    for(int pixel = 0; pixel < npixels; pixel++) {
        for(int label = 0; label < NLABELS; label++) {
            unary[label] = unary_cost(pixel, label);
        }
        factors.push_back(Factor(vars[pixel], &unary[0]));
    }

    for(int pixel = 0; pixel < npixels; pixel++) {
        vector<int> neighbors = pixel_neighbors(pixel);
        for(int n = 0; n < neighbors.size(); n++) {
            int neighbor = neighbors[n];
            Real pairwise[NLABELS*NLABELS];
            for(int plabel = 0; plabel < NLABELS; plabel++)
                for(int nlabel = 0; nlabel < NLABELS; nlabel++)
                    pairwise[nlabel*NLABELS + plabel] = pairwise_cost(pixel, neighbor, plabel, nlabel);
            factors.push_back(Factor(VarSet(vars[pixel], vars[neighbor]), &pairwise[0]));
        }
    }

    FactorGraph fg = FactorGraph(factors.begin(), factors.end(), vars.begin(), vars.end(), factors.size(), vars.size());

    TRWBP ia(fg, PropertySet("[updates=0,tol=1e-6,maxiter=10000,logdomain=0,inference=1,damping=0.0,nrtrees=0,verbose=0]"));

    ia.init();

    for(int i = 0; i < niterations; i++) {
        ia.setMaxIter(i + 1);
        double energy = ia.run();
        if(debug_mrf)
            fprintf(stderr, "Iteration %d: %f\n", i, energy);

        if(progress_callback) {
            float progress = start_progress + (end_progress - start_progress)*i/niterations;
            progress_callback(progress);
        }
    }

    State maxState( ia.fg().factor(0).vars() );
    vector<size_t> state;
    int labelhist[NLABELS];
    for(int i=0; i < NLABELS; i++) labelhist[i] = 0;

    for(int pixel = 0; pixel < npixels; pixel++) {
        int label = (int)ia.beliefV(pixel).p().argmax().first;
        labelhist[label]++;
        if(label != UNKNOWN_LABEL) {
            xy pt = stereo_grid_locations[pixel];
            struct stereo_match match = stereo_grid_matches[pixel][label];
            stereo_mesh_add_vertex(mesh, pt.x, pt.y, match.x, match.y, match.point, match.score);
        }
    }

    if(debug_mrf) {
        for(int i = 0; i < NLABELS; i++)
            fprintf(stderr, "%d ", labelhist[i]);
        fprintf(stderr, "\n");
    }
}

void stereo_mesh_write_unary(const char * filename)
{
    FILE * unary_file = fopen(filename, "w");
    if(!unary_file) return;

    for(int pixel = 0; pixel < stereo_grid_matches.size(); pixel++) {
        for(int label = 0; label < NLABELS; label++) {
            float val = unary_cost(pixel, label);
            fprintf(unary_file, "%f", val);
            if(label == NLABELS-1)
                fprintf(unary_file, "\n");
            else
                fprintf(unary_file, ", ");
        }
    }

    fclose(unary_file);
}

void stereo_mesh_write_pairwise(const char * filename)
{
    FILE * pairwise_file = fopen(filename, "w");
    if(!pairwise_file) return;
    int m_width = 640/grid_size;
    int m_height = 480/grid_size;

    for(int y = 0; y < m_height; y++) {
        for(int x = 0; x < m_width; x++) {
            int p1 = y*m_width + x;
            for(int dy = -1; dy <= 1; dy++) {
                for(int dx = -1; dx <= 1; dx++) {
                    if(dx + x < 0 || dx + x >= m_width || dy + y < 0 || dy + y >= m_height || (dx == 0 && dy == 0))
                        continue;
                    int p2 = (y+dy)*m_width + (x+dx);
                    fprintf(pairwise_file, "%d,%d,",p1,p2);
                    for(int l1 = 0; l1 < NLABELS; l1++) {
                        for(int l2 = 0; l2 < NLABELS; l2++) {
                            fprintf(pairwise_file, "%f", pairwise_cost(p1, p2, l1, l2));
                            if(l1 == NLABELS-1 && l2 == NLABELS-1)
                                fprintf(pairwise_file, "\n");
                            else
                                fprintf(pairwise_file, ",");
                        }
                    }
                }
            }
        }
    }

    fclose(pairwise_file);
}

void stereo_mesh_write_topn_correspondences(const char * filename)
{
    FILE * correspondences = fopen(filename, "w");
    if(!correspondences) return;

    for(int i = 0; i < stereo_grid_locations.size(); i++)
    {
        xy reference_pt = stereo_grid_locations[i];
        fprintf(correspondences, "%f, %f, ", reference_pt.x, reference_pt.y);
        for(int j = 0; j < stereo_grid_matches[i].size(); j++)
        {
            stereo_match match = stereo_grid_matches[i][j];
            fprintf(correspondences, "%f, %f, %f, %f", match.x, match.y, match.score, match.depth);
            if(j != stereo_grid_matches[i].size())
                fprintf(correspondences, ", ");
        }
        fprintf(correspondences, "\n");
    }
    fclose(correspondences);
}

void stereo_mesh_add_gradient_grid(stereo_mesh & mesh, const stereo &g, int npoints, void (*progress_callback)(float), float progress_start, float progress_end)
{
    int m_width = g.camera.width / grid_size;
    int m_height = g.camera.height / grid_size;

    float step_range = progress_end - progress_start;
    float mrf_range = 0;
    if(enable_mrf) {
        mrf_range = step_range * 0.25;
        step_range = step_range * 0.75;
    }

    stereo_grid_locations.clear();
    stereo_grid_matches.clear();
    for(int y = 0; y < g.camera.height; y+=grid_size) {
        for(int x = 0; x < g.camera.width; x+=grid_size) {
            int best_x = x;
            int best_y = y;
            float best_gradient = 0;
            for(int dy = 1; dy < grid_size; dy++) {
                for(int dx = 1; dx < grid_size; dx++) {
                    int row = y + dy;
                    int col = x + dx;
                    float gx = ((float)g.reference->image[row*g.camera.width+col] - (float)g.reference->image[row*g.camera.width+ (col-1)])/2.;
                    float gy = ((float)g.reference->image[row*g.camera.width+col] - (float)g.reference->image[(row-1)*g.camera.width + col])/2.;
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
            float progress = progress_start + step_range * (float)y / g.camera.height;
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
        stereo_mesh_refine_mrf(mesh, m_width, m_height, progress_callback, progress_start + step_range, progress_start + step_range + mrf_range);
    }
}

void stereo_mesh_add_gradient(stereo_mesh & mesh, const stereo &g, int npoints, void (*progress_callback)(float), float progress_start, float progress_end)
{
    // TODO: what is the best mask_shift value
    int mask_shift = 2;
    int grid_size = 1 << mask_shift;
    int m_width = g.camera.width / grid_size;
    int m_height = g.camera.height / grid_size;

    bool * match_occupied = (bool *)calloc(m_width*m_height, sizeof(bool));
    float * match_scores = (float *)calloc(m_width*m_height, sizeof(float));
    int * match_ind = (int *)calloc(m_width*m_height, sizeof(int));

    vector<xy> points;
    bool success;
    v3 intersection;

    xy pt;
    for(int row = 1; row < g.camera.height; row++)
        for(int col = 1; col < g.camera.width; col++)
        {
            float dx = ((float)g.reference->image[row*g.camera.width+col] - (float)g.reference->image[row*g.camera.width+ (col-1)])/2.;
            float dy = ((float)g.reference->image[row*g.camera.width+col] - (float)g.reference->image[(row-1)*g.camera.width + col])/2.;
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
            float progress = progress_start + (progress_end - progress_start)*(float)i / nchosen;
            progress_callback(progress);
        }
    }

    free(match_occupied);
    free(match_scores);
    free(match_ind);
}

void stereo_mesh_add_grid(stereo_mesh & mesh, const stereo &g, int step, void (*progress_callback)(float), float progress_start, float progress_end)
{
    bool success;
    v3 intersection;
    struct stereo_match match;

    for(int row = 0; row < g.camera.height; row += step) {
        for(int col=0; col < g.camera.width; col += step) {
            if(progress_callback) {
                float progress = progress_start + (progress_end - progress_start) * (1.*col + row*g.camera.width)/(g.camera.height*g.camera.width);
                progress_callback(progress);
            }
            success = g.triangulate(col, row, intersection, &match);
            if(success) {
                stereo_mesh_add_vertex(mesh, col, row, match.x, match.y, intersection, match.score);
            }
        }
    }
}

void stereo_mesh_add_features(stereo_mesh & mesh, const stereo &g, int maxvertices, void (*progress_callback)(float), float progress_start, float progress_end)
{
    bool success;
    v3 intersection;

    fast_detector_9 fast;
    fast.init(640, 480, 640, 7, 3);

    int bthresh = 30;
    vector<xy> features = fast.detect(g.reference->image, NULL, maxvertices, bthresh, 0, 0, 640, 480);

    for(int i = 0; i < features.size(); i++) {
            if(progress_callback)
                progress_callback(progress_start + (progress_end - progress_start)*i*1./features.size());
            struct stereo_match match;
            success = g.triangulate(features[i].x, features[i].y, intersection, &match);
            if(success) {
                stereo_mesh_add_vertex(mesh, features[i].x, features[i].y, match.x, match.y, intersection, match.score);
            }
    }
}

stereo_mesh stereo_mesh_create(const stereo &g, void(*progress_callback)(float), float progress_start, float progress_end)
{
    stereo_mesh mesh;
    stereo_mesh_add_gradient_grid(mesh, g, 2000, progress_callback, progress_start, progress_end);
    //stereo_mesh_add_gradient(mesh, g, 2000, progress_callback, progress_start, progress_end);
    //stereo_mesh_add_features(mesh, g, 500, progress_callback, progress_start, progress_end);
    //stereo_mesh_add_grid(mesh, g, 10, progress_callback, progress_start, progress_end);
    stereo_mesh_delaunay(g, mesh);
    return mesh;
}
