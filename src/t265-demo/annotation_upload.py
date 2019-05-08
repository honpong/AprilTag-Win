import argparse
import json
import os
import re
import six.moves.urllib as urllib
import sys
import tarfile
import zipfile
 
from collections import defaultdict
from io import StringIO
#from matplotlib import pyplot as plt
 
sys.path.append("python-delairstack-1.0.0")
 
from delairstack import DelairStackSDK
 
 
 
 
def load_image_into_numpy_array(image):
  (im_width, im_height) = image.size
  return np.array(image.getdata()).reshape(
      (im_height, im_width, 3)).astype(np.uint8)
 
 
 
sdk = DelairStackSDK('./config.json')
 
ap = argparse.ArgumentParser()
ap.add_argument( "--project", help="Project name")
ap.add_argument( "--working_folder", help="Working directory (default is current directory)")
ap.add_argument( "--list", help="List projects", action="store_true")
ap.add_argument( "--step_download_photos",    help="Download raw photos into working directory", action="store_true")
ap.add_argument( "--step_upload_result",      help="Upload the results to Intel Insight", action="store_true")
ap.add_argument( "--debug_delete_annotations",help="Delete annotations with matching title/description")
args = ap.parse_args()
 
projectsUtil    = sdk.resources('projects')
missionsUtil    = sdk.resources('missions')
photosUtil      = sdk.resources('photos')
annotationsUtil = sdk.resources('annotations')
objectsUtils = sdk.resources(resource_name='dxobjects')
 
# get project list
if args.list:
    print("\nList of projects:")
    projects = projectsUtil.search(name='')
    for p in projects:
        print("  -> Project: "+p.name)
 
working_folder = "./"
if args.working_folder is not None:
    working_folder = args.working_folder + "/"
    print("\nUsing working folder: "+working_folder)
 
# when project is selected
if args.project is not None:
    print("\nSearching project called \""+args.project+"\"")
    projects = projectsUtil.search(name=args.project)
    myProject = projects.pop()
    print("\nProject selected: "+myProject.name)
 
    # download photos and annotations for project
    if args.step_download_photos:
        print("\nList of Missions:")
        # missions for project
        missions = missionsUtil.search(project=myProject)
        for mission in missions:
            print("  -> Mission: "+str(mission.id))
 
            # photos for mission
            print("\n  List of Photos for this Mission:")
            photos = photosUtil.search(mission=mission)
            for photo in photos:
                print("    -> Photo: "+str(photo.id))
                print("       ... checking "+working_folder+"/"+photo.id+".jpg");
                if os.path.exists(working_folder+"/"+photo.id+".jpg"):
                    print("       ... file already downloaded.")
                else:
                    print("       ... downloading");
                    photosUtil.download(photo, destination_directory_path=working_folder)
 
    if args.step_upload_result:
        print("\nStep: Upload results\n")
        GEOJSON_PATHS = []
        dirs = os.listdir( working_folder )
        for file in dirs:
            if file[-5:] == ".json":
                GEOJSON_PATHS.append(working_folder + file)
 
        for geojson_path in GEOJSON_PATHS:
            toks = geojson_path[:-5].split("_")
            toks2 = toks[0].split("/")
            photo_id = toks2[-1]
            print("GeoJson: "+geojson_path)
            print("Photo: "+photo_id)
            with open(geojson_path, 'r') as infile:
                myGeojson = json.load(infile)
                print("Photo: "+photo_id)
                myPhoto = photosUtil.get(id=photo_id)
                print("Photo: "+photo_id)
                myAnnotation = annotationsUtil.create(project=myProject, geojson=myGeojson, photo=myPhoto, target='2d')
                print("Photo: "+photo_id)
                print("Annotation: created")
 
    if args.debug_delete_annotations is not None:
        print("\nStep: Delete annotations from project\n")
        print("Using regex: "+args.debug_delete_annotations)
        annotations = annotationsUtil.search(project=myProject)
        for annotation in annotations:
            if re.search(args.debug_delete_annotations,annotation.feature['properties']['name']):
                print("Annotation "+annotation.id+" found by title: "+annotation.feature['properties']['name']+" and deleted.")
                annotationsUtil.delete(resource=annotation)
            if re.search(args.debug_delete_annotations,annotation.feature['properties']['comment']):
                print("Annotation "+annotation.id+" found by description: "+annotation.feature['properties']['comment']+" and deleted.")
                annotationsUtil.delete(resource=annotation)
 
else:
    print("\nSelect a project with --project \"Project name\"")
 
print("")