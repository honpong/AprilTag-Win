"""Module dedicated to the annotation resource management

"""
import copy
from datetime import datetime

from ....core.errors import (QueryError)
from ....core.resources.projectmngt.annotations import Annotations
from ....core.resources.resource import Resource


class AnnotationsImpl(Annotations):
    """This class represents an accessor to handle annotation objects and actions

    """
    _template = {}
    _hidden = []
    _immutable = ['created_by', 'created_date', 'modified_date', 'project_id', 'target']

    def __init__(self, project_manager_api, **kwargs):
        super().__init__(provider=project_manager_api)

    def create(self, project: Resource, geojson: dict, photo: Resource = None, target: str = '2d',
               properties: dict = None):
        """Create an Annotation object.

        Args:
            project (Resource): the project used to associate the annotation
            geojson (dict): the geojson defining the features

                Examples:

                json_polygon = {
                  'type': 'Feature',
                  'geometry': {
                    'type': 'Polygon',
                    'coordinates': [
                      [
                        [xmin, ymax],
                        [xmax, ymax],
                        [xmax, ymin],
                        [xmin, ymin],
                        [xmin, ymax]
                      ]
                    ]
                  },
                  'properties': {
                    'name': 'My Polygon',
                    'comment': 'Test a polygon annotation'
                  }
                }

                geojson_point = {
                  "type": "Feature",
                  "geometry": {
                    "type": "Point",
                    "coordinates": [
                      x,
                      y
                    ]
                  },
                  "properties": {
                    "name": "My Point",
                    "comment": "Test a point annotation"
                  }
                }

                geojson_linestring = {
                  "type": "Feature",
                  "geometry": {
                    "type": "LineString",
                    "coordinates": [
                      [
                        x1,
                        y1
                      ],
                      [
                        x2,
                        y2
                      ],
                      [
                        x3,
                        y3
                      ]
                    ]
                  },
                  "properties": {
                    "name": "My Linestring",
                    "comment": "Test a point annotation"
                  }
                }

            photo (Resource): the photo
            target (str): target of the annotation: '2d' | '3d'
            properties (dict): extra feature properties

                {
                    'color' : [R,G,B]
                }

        Returns:
            Resource: the annotation
        """

        geoson_to_send = copy.deepcopy(geojson)

        target_prop = None

        if photo is not None:
            target_prop = {'type': 'photo', 'subId': photo.id}
        else:
            target_prop = {'type': target}

        if 'properties' not in geoson_to_send:
            geoson_to_send['properties'] = {}

        geoson_to_send['properties']['created'] = datetime.utcnow().isoformat()

        if properties is not None:
            geoson_to_send['properties'].update(copy.deepcopy(properties))

        f = {
            'project_id': project.id,
            'target': target_prop,
            'feature': geoson_to_send
            }
        return super().create(**f)

    def update(self, *, resource: Resource):
        """Update the object.

        Args:
            resource (Resource): the annotation to update

        Returns:
            Resource: the annotation

        """
        # check feature changes

        updated_resource = None

        if resource.check_attribute('feature'):
            updated_resource = self.__update_partial(resource, 'feature')

        if resource.check_attribute('feature.geometry'):
            updated_resource = self.__update_partial(resource, 'geometry')

        if resource.check_attribute('feature.properties'):
            updated_resource = self.__update_partial(resource, 'properties')

        if updated_resource is None:
            raise QueryError("Cannot update the resource")

    def __update_partial(self, resource: Resource, property_name: str):
        content = self._provider.put(
                path='{name}/{annotation_id}/{property_name}'.format(name=self._name,
                                                                     annotation_id=resource.id,
                                                                     property_name=property_name),
                data=resource._desc)
        return Resource(
                id=content['_id'],
                desc=content, manager=self)

    def search(self, *, project: Resource, **kwargs):
        """Get all annotations corresponding to a specific project.

        Args:
            project (Resource): the project

        Returns:
             [Resource]: A list of annotations matching with the search criteria

        """
        query = {
            'project_id': project.id
            }
        return super().search(query=query)
